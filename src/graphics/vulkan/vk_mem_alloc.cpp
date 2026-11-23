#include "graphics/vulkan/vk_mem_alloc.hpp"
#include <algorithm>
#include <utility>

namespace northern::graphics {

namespace {
constexpr VkDeviceSize kSuballocationThreshold = 4 * 1024 * 1024; // 4 MB
constexpr VkDeviceSize kDefaultDeviceLocalBlockSize = 64 * 1024 * 1024; // 64 MB
constexpr VkDeviceSize kDefaultHostVisibleBlockSize = 16 * 1024 * 1024; // 16 MB
} // namespace

VulkanMemoryAllocator::VulkanMemoryAllocator(VkDevice device,
                                             VkPhysicalDevice physicalDevice) noexcept
    : device_(device), physicalDevice_(physicalDevice) {
    if (physicalDevice_ != VK_NULL_HANDLE) {
        vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties_);
    }
}

VulkanMemoryAllocator::~VulkanMemoryAllocator() {
    if (device_ != VK_NULL_HANDLE) {
        for (auto& block : blocks_) {
            if (block.mappedBase) {
                vkUnmapMemory(device_, block.memory);
                block.mappedBase = nullptr;
            }
            if (block.memory != VK_NULL_HANDLE) {
                vkFreeMemory(device_, block.memory, nullptr);
                block.memory = VK_NULL_HANDLE;
            }
        }
    }
    blocks_.clear();
}

VulkanMemoryAllocator::VulkanMemoryAllocator(VulkanMemoryAllocator&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      physicalDevice_(std::exchange(other.physicalDevice_, VK_NULL_HANDLE)),
      memProperties_(other.memProperties_),
      blocks_(std::move(other.blocks_)) {}

VulkanMemoryAllocator& VulkanMemoryAllocator::operator=(VulkanMemoryAllocator&& other) noexcept {
    if (this != &other) {
        if (device_ != VK_NULL_HANDLE) {
            for (auto& block : blocks_) {
                if (block.mappedBase) {
                    vkUnmapMemory(device_, block.memory);
                }
                if (block.memory != VK_NULL_HANDLE) {
                    vkFreeMemory(device_, block.memory, nullptr);
                }
            }
        }
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        physicalDevice_ = std::exchange(other.physicalDevice_, VK_NULL_HANDLE);
        memProperties_ = other.memProperties_;
        blocks_ = std::move(other.blocks_);
    }
    return *this;
}

uint32_t VulkanMemoryAllocator::findMemoryType(uint32_t typeFilter,
                                               VkMemoryPropertyFlags properties) const noexcept {
    for (uint32_t i = 0; i < memProperties_.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProperties_.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return UINT32_MAX;
}

void VulkanMemoryAllocator::coalesceFreeRanges(std::vector<FreeRange>& ranges) {
    if (ranges.size() <= 1) {
        return;
    }

    std::sort(ranges.begin(), ranges.end(), [](const FreeRange& a, const FreeRange& b) {
        return a.offset < b.offset;
    });

    std::vector<FreeRange> merged;
    merged.reserve(ranges.size());
    merged.push_back(ranges[0]);

    for (size_t i = 1; i < ranges.size(); ++i) {
        auto& top = merged.back();
        if (top.offset + top.size == ranges[i].offset) {
            top.size += ranges[i].size;
        } else {
            merged.push_back(ranges[i]);
        }
    }
    ranges = std::move(merged);
}

core::Result<VulkanAllocation, core::Error>
VulkanMemoryAllocator::allocateSuballocated(const VkMemoryRequirements& memReqs,
                                            VkMemoryPropertyFlags properties) {
    uint32_t memoryType = findMemoryType(memReqs.memoryTypeBits, properties);
    if (memoryType == UINT32_MAX) {
        return core::Error(core::ErrorCode::GraphicsError,
                           "Failed to find suitable Vulkan memory type");
    }

    bool isHostVisible = (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;

    // For very large allocations, fallback to dedicated allocation
    if (memReqs.size > kSuballocationThreshold) {
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = memoryType;

        VulkanAllocation alloc{};
        alloc.size = memReqs.size;
        alloc.offset = 0;
        alloc.blockIndex = -1;

        VkResult res = vkAllocateMemory(device_, &allocInfo, nullptr, &alloc.memory);
        if (res != VK_SUCCESS) {
            return makeVkError(res, "Failed to allocate dedicated Vulkan memory");
        }

        if (isHostVisible) {
            res = vkMapMemory(device_, alloc.memory, 0, memReqs.size, 0, &alloc.mappedData);
            if (res != VK_SUCCESS) {
                vkFreeMemory(device_, alloc.memory, nullptr);
                return makeVkError(res, "Failed to map dedicated Vulkan memory");
            }
        }
        return alloc;
    }

    // Try finding space in an existing block
    for (size_t bIdx = 0; bIdx < blocks_.size(); ++bIdx) {
        auto& block = blocks_[bIdx];
        if (block.memoryTypeIndex != memoryType) {
            continue;
        }

        for (size_t rIdx = 0; rIdx < block.freeRanges.size(); ++rIdx) {
            FreeRange range = block.freeRanges[rIdx];
            VkDeviceSize alignedOffset = (range.offset + (memReqs.alignment - 1)) & ~(memReqs.alignment - 1);
            VkDeviceSize padding = alignedOffset - range.offset;

            if (padding + memReqs.size <= range.size) {
                // Remove the old range and insert leftover fragments
                block.freeRanges.erase(block.freeRanges.begin() + rIdx);

                if (padding > 0) {
                    block.freeRanges.push_back(FreeRange{range.offset, padding});
                }
                VkDeviceSize remaining = range.size - (padding + memReqs.size);
                if (remaining > 0) {
                    block.freeRanges.push_back(FreeRange{alignedOffset + memReqs.size, remaining});
                }
                coalesceFreeRanges(block.freeRanges);

                VulkanAllocation alloc{};
                alloc.memory = block.memory;
                alloc.offset = alignedOffset;
                alloc.size = memReqs.size;
                alloc.blockIndex = static_cast<int32_t>(bIdx);
                if (block.isHostVisible && block.mappedBase) {
                    alloc.mappedData = static_cast<uint8_t*>(block.mappedBase) + alignedOffset;
                }
                return alloc;
            }
        }
    }

    // Allocate a new block
    VkDeviceSize defaultBlockSize = isHostVisible ? kDefaultHostVisibleBlockSize : kDefaultDeviceLocalBlockSize;
    VkDeviceSize blockSize = std::max(defaultBlockSize, memReqs.size + memReqs.alignment);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = blockSize;
    allocInfo.memoryTypeIndex = memoryType;

    VkDeviceMemory blockMemory = VK_NULL_HANDLE;
    VkResult res = vkAllocateMemory(device_, &allocInfo, nullptr, &blockMemory);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to allocate new Vulkan memory block");
    }

    void* mappedBase = nullptr;
    if (isHostVisible) {
        res = vkMapMemory(device_, blockMemory, 0, blockSize, 0, &mappedBase);
        if (res != VK_SUCCESS) {
            vkFreeMemory(device_, blockMemory, nullptr);
            return makeVkError(res, "Failed to map new Vulkan memory block");
        }
    }

    int32_t newBlockIdx = static_cast<int32_t>(blocks_.size());
    MemoryBlock newBlock{};
    newBlock.memory = blockMemory;
    newBlock.totalSize = blockSize;
    newBlock.memoryTypeIndex = memoryType;
    newBlock.isHostVisible = isHostVisible;
    newBlock.mappedBase = mappedBase;

    VkDeviceSize remaining = blockSize - memReqs.size;
    if (remaining > 0) {
        newBlock.freeRanges.push_back(FreeRange{memReqs.size, remaining});
    }
    blocks_.push_back(std::move(newBlock));

    VulkanAllocation alloc{};
    alloc.memory = blockMemory;
    alloc.offset = 0;
    alloc.size = memReqs.size;
    alloc.blockIndex = newBlockIdx;
    alloc.mappedData = mappedBase;

    return alloc;
}

core::Result<VulkanAllocation, core::Error>
VulkanMemoryAllocator::allocateBufferMemory(VkBuffer buffer, MemoryUsage usage) {
    VkMemoryRequirements memReqs{};
    vkGetBufferMemoryRequirements(device_, buffer, &memReqs);

    VkMemoryPropertyFlags properties = 0;
    switch (usage) {
    case MemoryUsage::GpuOnly:
        properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    case MemoryUsage::CpuToGpu:
    case MemoryUsage::GpuToCpu:
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    }

    auto allocRes = allocateSuballocated(memReqs, properties);
    if (allocRes.isErr()) {
        return allocRes.error();
    }

    VulkanAllocation alloc = allocRes.value();
    VkResult res = vkBindBufferMemory(device_, buffer, alloc.memory, alloc.offset);
    if (res != VK_SUCCESS) {
        free(alloc);
        return makeVkError(res, "Failed to bind Vulkan buffer memory");
    }

    return alloc;
}

core::Result<VulkanAllocation, core::Error>
VulkanMemoryAllocator::allocateImageMemory(VkImage image, MemoryUsage usage) {
    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(device_, image, &memReqs);

    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (usage != MemoryUsage::GpuOnly) {
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    auto allocRes = allocateSuballocated(memReqs, properties);
    if (allocRes.isErr()) {
        return allocRes.error();
    }

    VulkanAllocation alloc = allocRes.value();
    VkResult res = vkBindImageMemory(device_, image, alloc.memory, alloc.offset);
    if (res != VK_SUCCESS) {
        free(alloc);
        return makeVkError(res, "Failed to bind Vulkan image memory");
    }

    return alloc;
}

void VulkanMemoryAllocator::free(const VulkanAllocation& alloc) {
    if (alloc.memory == VK_NULL_HANDLE) {
        return;
    }

    if (alloc.blockIndex == -1) {
        // Dedicated allocation
        if (alloc.mappedData) {
            vkUnmapMemory(device_, alloc.memory);
        }
        vkFreeMemory(device_, alloc.memory, nullptr);
        return;
    }

    if (alloc.blockIndex >= 0 && alloc.blockIndex < static_cast<int32_t>(blocks_.size())) {
        auto& block = blocks_[alloc.blockIndex];
        block.freeRanges.push_back(FreeRange{alloc.offset, alloc.size});
        coalesceFreeRanges(block.freeRanges);
    }
}

} // namespace northern::graphics
