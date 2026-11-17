#include "graphics/vulkan/vk_texture.hpp"
#include <utility>

namespace northern::graphics {

VulkanTexture::VulkanTexture(VkDevice device, VulkanMemoryAllocator* allocator,
                             const TextureDesc& desc) noexcept
    : device_(device), allocator_(allocator), desc_(desc) {}

VulkanTexture::~VulkanTexture() {
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device_, sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
    }
    if (!isExternal_) {
        if (imageView_ != VK_NULL_HANDLE) {
            vkDestroyImageView(device_, imageView_, nullptr);
            imageView_ = VK_NULL_HANDLE;
        }
        if (image_ != VK_NULL_HANDLE) {
            vkDestroyImage(device_, image_, nullptr);
            image_ = VK_NULL_HANDLE;
        }
        if (allocator_) {
            allocator_->free(allocation_);
            allocation_ = {};
        }
    }
}

VulkanTexture::VulkanTexture(VulkanTexture&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      allocator_(std::exchange(other.allocator_, nullptr)), desc_(other.desc_),
      image_(std::exchange(other.image_, VK_NULL_HANDLE)),
      imageView_(std::exchange(other.imageView_, VK_NULL_HANDLE)),
      sampler_(std::exchange(other.sampler_, VK_NULL_HANDLE)),
      layout_(std::exchange(other.layout_, VK_IMAGE_LAYOUT_UNDEFINED)),
      allocation_(std::exchange(other.allocation_, {})), isExternal_(other.isExternal_) {}

VulkanTexture& VulkanTexture::operator=(VulkanTexture&& other) noexcept {
    if (this != &other) {
        if (sampler_ != VK_NULL_HANDLE) {
            vkDestroySampler(device_, sampler_, nullptr);
        }
        if (!isExternal_) {
            if (imageView_ != VK_NULL_HANDLE) {
                vkDestroyImageView(device_, imageView_, nullptr);
            }
            if (image_ != VK_NULL_HANDLE) {
                vkDestroyImage(device_, image_, nullptr);
            }
            if (allocator_) {
                allocator_->free(allocation_);
            }
        }
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        allocator_ = std::exchange(other.allocator_, nullptr);
        desc_ = other.desc_;
        image_ = std::exchange(other.image_, VK_NULL_HANDLE);
        imageView_ = std::exchange(other.imageView_, VK_NULL_HANDLE);
        sampler_ = std::exchange(other.sampler_, VK_NULL_HANDLE);
        layout_ = std::exchange(other.layout_, VK_IMAGE_LAYOUT_UNDEFINED);
        allocation_ = std::exchange(other.allocation_, {});
        isExternal_ = other.isExternal_;
    }
    return *this;
}

core::Result<std::unique_ptr<VulkanTexture>, core::Error>
VulkanTexture::create(VkDevice device, VulkanMemoryAllocator& allocator, const TextureDesc& desc,
                      const SamplerDesc* samplerDesc) {
    if (desc.width == 0 || desc.height == 0) {
        return core::Error(core::ErrorCode::InvalidArgument, "Texture dimensions must be non-zero");
    }

    auto texture = std::unique_ptr<VulkanTexture>(new VulkanTexture(device, &allocator, desc));

    VkImageUsageFlags usage = 0;
    if (hasFlag(desc.usage, TextureUsage::Sampled)) {
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (hasFlag(desc.usage, TextureUsage::ColorAttachment)) {
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (hasFlag(desc.usage, TextureUsage::DepthStencilAttachment)) {
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (hasFlag(desc.usage, TextureUsage::TransferSrc)) {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (hasFlag(desc.usage, TextureUsage::TransferDst)) {
        usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = desc.width;
    imageInfo.extent.height = desc.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = desc.mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = toVkFormat(desc.format);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult res = vkCreateImage(device, &imageInfo, nullptr, &texture->image_);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to create VkImage");
    }

    auto allocRes = allocator.allocateImageMemory(texture->image_, MemoryUsage::GpuOnly);
    if (allocRes.isErr()) {
        return allocRes.error();
    }
    texture->allocation_ = allocRes.value();

    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    if (hasFlag(desc.usage, TextureUsage::DepthStencilAttachment)) {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture->image_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageInfo.format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = desc.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    res = vkCreateImageView(device, &viewInfo, nullptr, &texture->imageView_);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to create VkImageView");
    }

    if (samplerDesc || hasFlag(desc.usage, TextureUsage::Sampled)) {
        SamplerDesc defaultSampler{};
        const SamplerDesc& sDesc = samplerDesc ? *samplerDesc : defaultSampler;

        VkSamplerCreateInfo sInfo{};
        sInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sInfo.magFilter = toVkFilter(sDesc.magFilter);
        sInfo.minFilter = toVkFilter(sDesc.minFilter);
        sInfo.addressModeU = toVkAddressMode(sDesc.addressModeU);
        sInfo.addressModeV = toVkAddressMode(sDesc.addressModeV);
        sInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sInfo.unnormalizedCoordinates = VK_FALSE;

        res = vkCreateSampler(device, &sInfo, nullptr, &texture->sampler_);
        if (res != VK_SUCCESS) {
            return makeVkError(res, "Failed to create VkSampler");
        }
    }

    return texture;
}

std::unique_ptr<VulkanTexture> VulkanTexture::createFromExisting(VkDevice device, VkImage image,
                                                                 VkImageView imageView,
                                                                 const TextureDesc& desc) {
    auto texture = std::unique_ptr<VulkanTexture>(new VulkanTexture(device, nullptr, desc));
    texture->image_ = image;
    texture->imageView_ = imageView;
    texture->isExternal_ = true;
    return texture;
}

} // namespace northern::graphics
