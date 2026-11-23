#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "graphics/types.hpp"
#include "graphics/vulkan/vk_common.hpp"
#include <cstdint>
#include <vector>

namespace northern::graphics {

struct VulkanAllocation {
    VkDeviceMemory memory{VK_NULL_HANDLE};
    VkDeviceSize offset{0};
    VkDeviceSize size{0};
    void* mappedData{nullptr};
    int32_t blockIndex{-1}; // -1 if dedicated allocation
};

class VulkanMemoryAllocator {
public:
    VulkanMemoryAllocator(VkDevice device, VkPhysicalDevice physicalDevice) noexcept;
    ~VulkanMemoryAllocator();

    VulkanMemoryAllocator(const VulkanMemoryAllocator&) = delete;
    VulkanMemoryAllocator& operator=(const VulkanMemoryAllocator&) = delete;
    VulkanMemoryAllocator(VulkanMemoryAllocator&&) noexcept;
    VulkanMemoryAllocator& operator=(VulkanMemoryAllocator&&) noexcept;

    [[nodiscard]] core::Result<VulkanAllocation, core::Error>
    allocateBufferMemory(VkBuffer buffer, MemoryUsage usage);

    [[nodiscard]] core::Result<VulkanAllocation, core::Error>
    allocateImageMemory(VkImage image, MemoryUsage usage);

    void free(const VulkanAllocation& alloc);

    [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter,
                                          VkMemoryPropertyFlags properties) const noexcept;

private:
    struct FreeRange {
        VkDeviceSize offset{0};
        VkDeviceSize size{0};
    };

    struct MemoryBlock {
        VkDeviceMemory memory{VK_NULL_HANDLE};
        VkDeviceSize totalSize{0};
        uint32_t memoryTypeIndex{0};
        bool isHostVisible{false};
        void* mappedBase{nullptr};
        std::vector<FreeRange> freeRanges{};
    };

    core::Result<VulkanAllocation, core::Error>
    allocateSuballocated(const VkMemoryRequirements& memReqs, VkMemoryPropertyFlags properties);

    void coalesceFreeRanges(std::vector<FreeRange>& ranges);

    VkDevice device_{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
    VkPhysicalDeviceMemoryProperties memProperties_{};
    std::vector<MemoryBlock> blocks_{};
};

} // namespace northern::graphics
