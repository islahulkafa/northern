#include "graphics/vulkan/vk_mem_alloc.hpp"
#include <utility>

namespace northern::graphics {

VulkanMemoryAllocator::VulkanMemoryAllocator(VkDevice device,
                                             VkPhysicalDevice physicalDevice) noexcept
    : device_(device), physicalDevice_(physicalDevice) {
    if (physicalDevice_ != VK_NULL_HANDLE) {
        vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties_);
    }
}

VulkanMemoryAllocator::~VulkanMemoryAllocator() = default;

VulkanMemoryAllocator::VulkanMemoryAllocator(VulkanMemoryAllocator&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      physicalDevice_(std::exchange(other.physicalDevice_, VK_NULL_HANDLE)),
      memProperties_(other.memProperties_) {}

VulkanMemoryAllocator& VulkanMemoryAllocator::operator=(VulkanMemoryAllocator&& other) noexcept {
    if (this != &other) {
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        physicalDevice_ = std::exchange(other.physicalDevice_, VK_NULL_HANDLE);
        memProperties_ = other.memProperties_;
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

    uint32_t memoryType = findMemoryType(memReqs.memoryTypeBits, properties);
    if (memoryType == UINT32_MAX) {
        return core::Error(core::ErrorCode::GraphicsError,
                           "Failed to find suitable Vulkan memory type for buffer");
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = memoryType;

    VulkanAllocation alloc{};
    alloc.size = memReqs.size;

    VkResult res = vkAllocateMemory(device_, &allocInfo, nullptr, &alloc.memory);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to allocate Vulkan buffer memory");
    }

    res = vkBindBufferMemory(device_, buffer, alloc.memory, 0);
    if (res != VK_SUCCESS) {
        vkFreeMemory(device_, alloc.memory, nullptr);
        return makeVkError(res, "Failed to bind Vulkan buffer memory");
    }

    if (usage == MemoryUsage::CpuToGpu || usage == MemoryUsage::GpuToCpu) {
        res = vkMapMemory(device_, alloc.memory, 0, memReqs.size, 0, &alloc.mappedData);
        if (res != VK_SUCCESS) {
            vkFreeMemory(device_, alloc.memory, nullptr);
            return makeVkError(res, "Failed to map Vulkan buffer memory");
        }
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

    uint32_t memoryType = findMemoryType(memReqs.memoryTypeBits, properties);
    if (memoryType == UINT32_MAX) {
        return core::Error(core::ErrorCode::GraphicsError,
                           "Failed to find suitable Vulkan memory type for image");
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = memoryType;

    VulkanAllocation alloc{};
    alloc.size = memReqs.size;

    VkResult res = vkAllocateMemory(device_, &allocInfo, nullptr, &alloc.memory);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to allocate Vulkan image memory");
    }

    res = vkBindImageMemory(device_, image, alloc.memory, 0);
    if (res != VK_SUCCESS) {
        vkFreeMemory(device_, alloc.memory, nullptr);
        return makeVkError(res, "Failed to bind Vulkan image memory");
    }

    return alloc;
}

void VulkanMemoryAllocator::free(const VulkanAllocation& alloc) {
    if (alloc.memory == VK_NULL_HANDLE) {
        return;
    }
    if (alloc.mappedData != nullptr) {
        vkUnmapMemory(device_, alloc.memory);
    }
    vkFreeMemory(device_, alloc.memory, nullptr);
}

} // namespace northern::graphics
