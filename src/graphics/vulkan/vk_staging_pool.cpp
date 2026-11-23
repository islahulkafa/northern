#include "graphics/vulkan/vk_staging_pool.hpp"
#include "graphics/vulkan/vk_mem_alloc.hpp"
#include <algorithm>

namespace northern::graphics {

VulkanStagingPool::VulkanStagingPool(VkDevice device, VulkanMemoryAllocator& allocator,
                                     uint64_t defaultBufferSize)
    : device_(device), allocator_(allocator),
      defaultBufferSize_(std::max<uint64_t>(defaultBufferSize, 64 * 1024)) {}

VulkanStagingPool::~VulkanStagingPool() = default;

StagingSlice VulkanStagingPool::allocate(uint64_t size, uint64_t timelineValue, uint64_t alignment) {
    if (size == 0) {
        return StagingSlice{};
    }

    // Look for an existing buffer that has enough space
    for (size_t i = 0; i < buffers_.size(); ++i) {
        auto& item = buffers_[i];
        uint64_t alignedOffset = (item.used + (alignment - 1)) & ~(alignment - 1);
        if (alignedOffset + size <= item.buffer->desc().size) {
            item.used = alignedOffset + size;
            item.timelineValue = std::max(item.timelineValue, timelineValue);

            auto* basePtr = static_cast<uint8_t*>(item.buffer->mappedData());
            return StagingSlice{
                item.buffer->handle(),
                basePtr ? (basePtr + alignedOffset) : nullptr,
                alignedOffset,
                size,
                static_cast<uint32_t>(i)
            };
        }
    }

    // Need a new buffer
    uint64_t bufSize = std::max(defaultBufferSize_, size + alignment);
    BufferDesc desc{
        .size = bufSize,
        .usage = BufferUsage::TransferSrc,
        .memoryUsage = MemoryUsage::CpuToGpu,
        .debugName = "VulkanStagingPoolBuffer",
    };

    auto bufRes = VulkanBuffer::create(device_, allocator_, desc);
    if (bufRes.isErr()) {
        return StagingSlice{};
    }

    size_t newIndex = buffers_.size();
    buffers_.push_back(StagingBufferItem{
        std::move(bufRes.value()),
        size,
        timelineValue
    });

    auto* basePtr = static_cast<uint8_t*>(buffers_.back().buffer->mappedData());
    return StagingSlice{
        buffers_.back().buffer->handle(),
        basePtr,
        0,
        size,
        static_cast<uint32_t>(newIndex)
    };
}

void VulkanStagingPool::retire(uint64_t completedTimeline) {
    for (auto& item : buffers_) {
        if (item.timelineValue <= completedTimeline) {
            item.used = 0;
        }
    }
}

} // namespace northern::graphics
