#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "graphics/texture.hpp"
#include "graphics/vulkan/vk_common.hpp"
#include "graphics/vulkan/vk_mem_alloc.hpp"
#include <memory>

namespace northern::graphics {

class VulkanTexture {
public:
    static core::Result<std::unique_ptr<VulkanTexture>, core::Error>
    create(VkDevice device, VulkanMemoryAllocator& allocator, const TextureDesc& desc,
           const SamplerDesc* samplerDesc = nullptr);

    static std::unique_ptr<VulkanTexture> createFromExisting(VkDevice device, VkImage image,
                                                             VkImageView imageView,
                                                             const TextureDesc& desc);

    ~VulkanTexture();

    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;

    VulkanTexture(VulkanTexture&&) noexcept;
    VulkanTexture& operator=(VulkanTexture&&) noexcept;

    [[nodiscard]] VkImage image() const noexcept { return image_; }
    [[nodiscard]] VkImageView imageView() const noexcept { return imageView_; }
    [[nodiscard]] VkSampler sampler() const noexcept { return sampler_; }
    [[nodiscard]] const TextureDesc& desc() const noexcept { return desc_; }
    [[nodiscard]] VkImageLayout layout() const noexcept { return layout_; }
    void setLayout(VkImageLayout layout) noexcept { layout_ = layout; }

private:
    VulkanTexture(VkDevice device, VulkanMemoryAllocator* allocator,
                  const TextureDesc& desc) noexcept;

    VkDevice device_{VK_NULL_HANDLE};
    VulkanMemoryAllocator* allocator_{nullptr};
    TextureDesc desc_{};
    VkImage image_{VK_NULL_HANDLE};
    VkImageView imageView_{VK_NULL_HANDLE};
    VkSampler sampler_{VK_NULL_HANDLE};
    VkImageLayout layout_{VK_IMAGE_LAYOUT_UNDEFINED};
    VulkanAllocation allocation_{};
    bool isExternal_{false};
};

} // namespace northern::graphics
