#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "graphics/buffer.hpp"
#include "graphics/vulkan/vk_common.hpp"
#include "graphics/vulkan/vk_mem_alloc.hpp"
#include <memory>

namespace northern::graphics {

class VulkanBuffer {
public:
    static core::Result<std::unique_ptr<VulkanBuffer>, core::Error>
    create(VkDevice device, VulkanMemoryAllocator& allocator, const BufferDesc& desc,
           const void* initialData = nullptr);

    ~VulkanBuffer();

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    VulkanBuffer(VulkanBuffer&&) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&&) noexcept;

    [[nodiscard]] VkBuffer handle() const noexcept { return buffer_; }
    [[nodiscard]] const BufferDesc& desc() const noexcept { return desc_; }
    [[nodiscard]] void* mappedData() const noexcept { return allocation_.mappedData; }

    [[nodiscard]] core::Result<void, core::Error> write(uint64_t offset, uint64_t size,
                                                        const void* data);

private:
    VulkanBuffer(VkDevice device, VulkanMemoryAllocator* allocator,
                 const BufferDesc& desc) noexcept;

    VkDevice device_{VK_NULL_HANDLE};
    VulkanMemoryAllocator* allocator_{nullptr};
    BufferDesc desc_{};
    VkBuffer buffer_{VK_NULL_HANDLE};
    VulkanAllocation allocation_{};
};

} // namespace northern::graphics
