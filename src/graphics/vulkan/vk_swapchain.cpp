#include "graphics/vulkan/vk_swapchain.hpp"
#include "graphics/vulkan/vk_device.hpp"
#include <algorithm>
#include <utility>

namespace northern::graphics {

VulkanSwapchain::VulkanSwapchain(VulkanDevice* device, const SwapchainDesc& desc) noexcept
    : device_(device), desc_(desc), size_(desc.size), format_(desc.format) {}

VulkanSwapchain::~VulkanSwapchain() {
    device_->waitIdle();
    cleanup();

    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_->device(), swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }

    for (VkSemaphore sem : imageAvailableSemaphores_) {
        if (sem != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_->device(), sem, nullptr);
        }
    }
    for (VkSemaphore sem : renderFinishedSemaphores_) {
        if (sem != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_->device(), sem, nullptr);
        }
    }
    for (VkFence fence : inFlightFences_) {
        if (fence != VK_NULL_HANDLE) {
            vkDestroyFence(device_->device(), fence, nullptr);
        }
    }

    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(device_->instance(), surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
}

core::Result<std::unique_ptr<VulkanSwapchain>, core::Error>
VulkanSwapchain::create(VulkanDevice* device, const SwapchainDesc& desc) {
    if (!device) {
        return core::Error(core::ErrorCode::InvalidArgument, "Device cannot be null");
    }
    if (!desc.windowHandle.isValid()) {
        return core::Error(core::ErrorCode::InvalidArgument, "Invalid window handle for swapchain");
    }

    auto swapchain = std::unique_ptr<VulkanSwapchain>(new VulkanSwapchain(device, desc));

#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR surfaceInfo{};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd = reinterpret_cast<HWND>(desc.windowHandle.handle);
    surfaceInfo.hinstance = GetModuleHandleW(nullptr);

    VkResult res =
        vkCreateWin32SurfaceKHR(device->instance(), &surfaceInfo, nullptr, &swapchain->surface_);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to create Win32 Vulkan surface");
    }
#else
    return core::Error(core::ErrorCode::NotImplemented,
                       "Vulkan surface not implemented on this platform");
#endif

    VkBool32 presentSupported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(device->physicalDevice(), device->presentQueueFamily(),
                                         swapchain->surface_, &presentSupported);
    if (!presentSupported) {
        return core::Error(core::ErrorCode::GraphicsError,
                           "Presentation not supported on selected queue family");
    }

    swapchain->imageAvailableSemaphores_.resize(kMaxFramesInFlight);
    swapchain->renderFinishedSemaphores_.resize(kMaxFramesInFlight);
    swapchain->inFlightFences_.resize(kMaxFramesInFlight);

    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        res = vkCreateSemaphore(device->device(), &semInfo, nullptr,
                                &swapchain->imageAvailableSemaphores_[i]);
        if (res != VK_SUCCESS) {
            return makeVkError(res, "Failed to create image available semaphore");
        }
        res = vkCreateSemaphore(device->device(), &semInfo, nullptr,
                                &swapchain->renderFinishedSemaphores_[i]);
        if (res != VK_SUCCESS) {
            return makeVkError(res, "Failed to create render finished semaphore");
        }
        res = vkCreateFence(device->device(), &fenceInfo, nullptr, &swapchain->inFlightFences_[i]);
        if (res != VK_SUCCESS) {
            return makeVkError(res, "Failed to create in-flight fence");
        }
    }

    auto recRes = swapchain->recreate();
    if (recRes.isErr()) {
        return recRes.error();
    }

    return swapchain;
}

core::Result<void, core::Error> VulkanSwapchain::recreate() {
    device_->waitIdle();

    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_->physicalDevice(), surface_, &capabilities);

    VkExtent2D extent{};
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        extent.width =
            std::clamp(static_cast<uint32_t>(desc_.size.width), capabilities.minImageExtent.width,
                       capabilities.maxImageExtent.width);
        extent.height =
            std::clamp(static_cast<uint32_t>(desc_.size.height), capabilities.minImageExtent.height,
                       capabilities.maxImageExtent.height);
    }

    if (extent.width == 0 || extent.height == 0) {
        return {};
    }

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device_->physicalDevice(), surface_, &formatCount,
                                         nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device_->physicalDevice(), surface_, &formatCount,
                                         formats.data());

    VkSurfaceFormatKHR chosenFormat = formats[0];
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenFormat = f;
            break;
        }
    }
    vkFormat_ = chosenFormat.format;
    format_ = fromVkFormat(vkFormat_);
    colorSpace_ = chosenFormat.colorSpace;

    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device_->physicalDevice(), surface_, &modeCount,
                                              nullptr);
    std::vector<VkPresentModeKHR> modes(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device_->physicalDevice(), surface_, &modeCount,
                                              modes.data());

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!desc_.vsync) {
        for (const auto& m : modes) {
            if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
    }

    VkSwapchainKHR oldSwapchain = swapchain_;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = vkFormat_;
    createInfo.imageColorSpace = colorSpace_;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapchain;

    VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
    VkResult res = vkCreateSwapchainKHR(device_->device(), &createInfo, nullptr, &newSwapchain);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to create Vulkan swapchain");
    }

    cleanup();

    if (oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_->device(), oldSwapchain, nullptr);
    }
    swapchain_ = newSwapchain;

    uint32_t actualImageCount = 0;
    vkGetSwapchainImagesKHR(device_->device(), swapchain_, &actualImageCount, nullptr);
    images_.resize(actualImageCount);
    vkGetSwapchainImagesKHR(device_->device(), swapchain_, &actualImageCount, images_.data());

    imageViews_.resize(actualImageCount);
    textureHandles_.resize(actualImageCount);

    for (uint32_t i = 0; i < actualImageCount; ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images_[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = vkFormat_;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        res = vkCreateImageView(device_->device(), &viewInfo, nullptr, &imageViews_[i]);
        if (res != VK_SUCCESS) {
            return makeVkError(res, "Failed to create swapchain image view");
        }

        TextureDesc texDesc{};
        texDesc.width = extent.width;
        texDesc.height = extent.height;
        texDesc.format = format_;
        texDesc.usage = TextureUsage::ColorAttachment;

        auto tex = VulkanTexture::createFromExisting(device_->device(), images_[i], imageViews_[i],
                                                     texDesc);
        textureHandles_[i] = device_->registerExternalTexture(std::move(tex));
    }

    size_ = {static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height)};
    currentImageIndex_ = 0;

    return {};
}

void VulkanSwapchain::cleanup() {
    for (TextureHandle handle : textureHandles_) {
        if (handle.isValid()) {
            device_->unregisterExternalTexture(handle);
        }
    }
    textureHandles_.clear();

    for (VkImageView view : imageViews_) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(device_->device(), view, nullptr);
        }
    }
    imageViews_.clear();
    images_.clear();
}

core::Result<uint32_t, core::Error> VulkanSwapchain::acquireNextImage(uint64_t timeoutNs) {
    vkWaitForFences(device_->device(), 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    VkResult res = vkAcquireNextImageKHR(device_->device(), swapchain_, timeoutNs,
                                         imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE,
                                         &currentImageIndex_);

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        auto recRes = recreate();
        if (recRes.isErr()) {
            return recRes.error();
        }
        return acquireNextImage(timeoutNs);
    }
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        return makeVkError(res, "Failed to acquire next swapchain image");
    }

    return currentImageIndex_;
}

core::Result<void, core::Error> VulkanSwapchain::present(uint32_t imageIndex) {
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores_[currentFrame_];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &imageIndex;

    VkResult res = vkQueuePresentKHR(device_->presentQueue(), &presentInfo);
    currentFrame_ = (currentFrame_ + 1) % kMaxFramesInFlight;

    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        return recreate();
    }
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to present swapchain image");
    }

    return {};
}

core::Result<void, core::Error> VulkanSwapchain::resize(core::SizeI newSize) {
    if (newSize.width <= 0 || newSize.height <= 0) {
        return {};
    }
    desc_.size = newSize;
    return recreate();
}

TextureHandle VulkanSwapchain::currentTexture() const noexcept {
    if (currentImageIndex_ < textureHandles_.size()) {
        return textureHandles_[currentImageIndex_];
    }
    return TextureHandle::invalid();
}

} // namespace northern::graphics
