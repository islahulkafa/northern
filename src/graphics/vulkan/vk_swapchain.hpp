#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/vulkan/vk_common.hpp"
#include "graphics/vulkan/vk_texture.hpp"
#include <memory>
#include <vector>

namespace northern::graphics {

class VulkanDevice;

class VulkanSwapchain final : public Swapchain {
public:
    static core::Result<std::unique_ptr<VulkanSwapchain>, core::Error>
    create(VulkanDevice* device, const SwapchainDesc& desc);

    ~VulkanSwapchain() override;

    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    [[nodiscard]] core::Result<uint32_t, core::Error>
    acquireNextImage(uint64_t timeoutNs = UINT64_MAX) override;

    [[nodiscard]] core::Result<void, core::Error> present(uint32_t imageIndex) override;

    [[nodiscard]] core::Result<void, core::Error> resize(core::SizeI newSize) override;

    [[nodiscard]] TextureHandle currentTexture() const noexcept override;
    [[nodiscard]] Format format() const noexcept override { return format_; }
    [[nodiscard]] core::SizeI size() const noexcept override { return size_; }
    [[nodiscard]] uint32_t currentFrameIndex() const noexcept override { return currentFrame_; }
    [[nodiscard]] uint32_t maxFramesInFlight() const noexcept override {
        return kMaxFramesInFlight;
    }

    [[nodiscard]] VkSurfaceKHR surface() const noexcept { return surface_; }
    [[nodiscard]] VkSwapchainKHR handle() const noexcept { return swapchain_; }
    [[nodiscard]] VkSemaphore currentImageAvailableSemaphore() const noexcept {
        return imageAvailableSemaphores_[currentFrame_];
    }
    [[nodiscard]] VkSemaphore currentRenderFinishedSemaphore() const noexcept {
        return renderFinishedSemaphores_[currentFrame_];
    }
    [[nodiscard]] VkFence currentInFlightFence() const noexcept {
        return inFlightFences_[currentFrame_];
    }

private:
    explicit VulkanSwapchain(VulkanDevice* device, const SwapchainDesc& desc) noexcept;

    core::Result<void, core::Error> recreate();
    void cleanup();

    VulkanDevice* device_{nullptr};
    SwapchainDesc desc_{};
    core::SizeI size_{0, 0};
    Format format_{Format::B8G8R8A8_Unorm};
    VkFormat vkFormat_{VK_FORMAT_B8G8R8A8_UNORM};
    VkColorSpaceKHR colorSpace_{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    VkSurfaceKHR surface_{VK_NULL_HANDLE};
    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};

    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
    std::vector<TextureHandle> textureHandles_;

    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;

    uint32_t currentImageIndex_{0};
    uint32_t currentFrame_{0};
    static constexpr uint32_t kMaxFramesInFlight = 2;
};

} // namespace northern::graphics
