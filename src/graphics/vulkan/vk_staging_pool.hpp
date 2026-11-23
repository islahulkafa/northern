#pragma once

#include "graphics/vulkan/vk_buffer.hpp"
#include "graphics/vulkan/vk_common.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace northern::graphics {

class VulkanMemoryAllocator;

struct StagingSlice {
    VkBuffer buffer{VK_NULL_HANDLE};
    void* mappedData{nullptr};
    uint64_t offset{0};
    uint64_t size{0};
    uint32_t bufferIndex{0};
};

class VulkanStagingPool {
public:
    VulkanStagingPool(VkDevice device, VulkanMemoryAllocator& allocator,
                      uint64_t defaultBufferSize = 4 * 1024 * 1024);
    ~VulkanStagingPool();

    VulkanStagingPool(const VulkanStagingPool&) = delete;
    VulkanStagingPool& operator=(const VulkanStagingPool&) = delete;
    VulkanStagingPool(VulkanStagingPool&&) = delete;
    VulkanStagingPool& operator=(VulkanStagingPool&&) = delete;

    [[nodiscard]] StagingSlice allocate(uint64_t size, uint64_t timelineValue,
                                        uint64_t alignment = 16);

    void retire(uint64_t completedTimeline);

private:
    struct StagingBufferItem {
        std::unique_ptr<VulkanBuffer> buffer;
        uint64_t used{0};
        uint64_t timelineValue{0};
    };

    VkDevice device_{VK_NULL_HANDLE};
    VulkanMemoryAllocator& allocator_;
    uint64_t defaultBufferSize_{4 * 1024 * 1024};
    std::vector<StagingBufferItem> buffers_{};
};

} // namespace northern::graphics
