#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "core/memory/handle_pool.hpp"
#include "graphics/device.hpp"
#include "graphics/vulkan/vk_buffer.hpp"
#include "graphics/vulkan/vk_command_buffer.hpp"
#include "graphics/vulkan/vk_common.hpp"
#include "graphics/vulkan/vk_mem_alloc.hpp"
#include "graphics/vulkan/vk_pipeline.hpp"
#include "graphics/vulkan/vk_staging_pool.hpp"
#include "graphics/vulkan/vk_swapchain.hpp"
#include "graphics/vulkan/vk_texture.hpp"
#include <memory>
#include <vector>

namespace northern::graphics {

class VulkanDevice final : public Device {
public:
    static core::Result<std::unique_ptr<VulkanDevice>, core::Error>
    create(const DeviceDesc& desc = {});

    ~VulkanDevice() override;

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

    [[nodiscard]] core::Result<std::unique_ptr<Swapchain>, core::Error>
    createSwapchain(const SwapchainDesc& desc) override;

    [[nodiscard]] core::Result<std::unique_ptr<CommandBuffer>, core::Error>
    createCommandBuffer() override;

    [[nodiscard]] core::Result<BufferHandle, core::Error>
    createBuffer(const BufferDesc& desc, const void* initialData = nullptr) override;

    [[nodiscard]] core::Result<void, core::Error>
    writeBuffer(BufferHandle handle, uint64_t offset, uint64_t size, const void* data) override;

    [[nodiscard]] void* mapBuffer(BufferHandle handle) override;

    [[nodiscard]] core::Result<TextureHandle, core::Error>
    createTexture(const TextureDesc& desc, const void* initialData = nullptr) override;

    [[nodiscard]] core::Result<void, core::Error> writeTexture(TextureHandle handle, uint32_t x,
                                                               uint32_t y, uint32_t width,
                                                               uint32_t height,
                                                               const void* data) override;

    [[nodiscard]] core::Result<PipelineHandle, core::Error>
    createPipeline(const PipelineDesc& desc) override;

    void destroyBuffer(BufferHandle handle) override;
    void destroyTexture(TextureHandle handle) override;
    void destroyPipeline(PipelineHandle handle) override;

    [[nodiscard]] core::Result<void, core::Error> submit(CommandBuffer* cmdBuffer,
                                                         Swapchain* swapchain = nullptr) override;

    void advanceFrame() override;
    void waitIdle() override;

    // Internal accessors
    [[nodiscard]] VkInstance instance() const noexcept { return instance_; }
    [[nodiscard]] VkPhysicalDevice physicalDevice() const noexcept { return physicalDevice_; }
    [[nodiscard]] VkDevice device() const noexcept { return device_; }
    [[nodiscard]] VkQueue graphicsQueue() const noexcept { return graphicsQueue_; }
    [[nodiscard]] VkQueue presentQueue() const noexcept { return presentQueue_; }
    [[nodiscard]] uint32_t graphicsQueueFamily() const noexcept { return graphicsQueueFamily_; }
    [[nodiscard]] uint32_t presentQueueFamily() const noexcept { return presentQueueFamily_; }
    [[nodiscard]] VulkanMemoryAllocator& allocator() noexcept { return *allocator_; }

    [[nodiscard]] VulkanBuffer* getBuffer(BufferHandle handle);
    [[nodiscard]] VulkanTexture* getTexture(TextureHandle handle);
    [[nodiscard]] VulkanPipeline* getPipeline(PipelineHandle handle);

    [[nodiscard]] TextureHandle registerExternalTexture(std::unique_ptr<VulkanTexture> texture);
    void unregisterExternalTexture(TextureHandle handle);

    [[nodiscard]] uint64_t currentTimeline() const noexcept { return currentTimelineValue_; }
    [[nodiscard]] VkSemaphore timelineSemaphore() const noexcept { return timelineSemaphore_; }
    [[nodiscard]] VkDescriptorSet getOrCreateTextureDescriptorSet(PipelineHandle pipelineHandle,
                                                                  TextureHandle textureHandle);

private:
    explicit VulkanDevice(const DeviceDesc& desc) noexcept;

    core::Result<void, core::Error> initInstance();
    core::Result<void, core::Error> selectPhysicalDevice();
    core::Result<void, core::Error> createLogicalDevice();
    core::Result<void, core::Error> createCommandPool();
    core::Result<void, core::Error> createDescriptorPool();
    core::Result<void, core::Error> createTimelineSemaphore();
    core::Result<void, core::Error> createPipelineCache();

    void retireResources();

    DeviceDesc desc_{};
    VkInstance instance_{VK_NULL_HANDLE};
    VkDebugUtilsMessengerEXT debugMessenger_{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    VkQueue presentQueue_{VK_NULL_HANDLE};
    uint32_t graphicsQueueFamily_{UINT32_MAX};
    uint32_t presentQueueFamily_{UINT32_MAX};
    VkCommandPool commandPool_{VK_NULL_HANDLE};
    VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};
    std::unordered_map<uint64_t, VkDescriptorSet> descriptorSetCache_;
    std::unique_ptr<VulkanMemoryAllocator> allocator_;

    core::HandlePool<std::unique_ptr<VulkanBuffer>, BufferTag> bufferPool_;
    core::HandlePool<std::unique_ptr<VulkanTexture>, TextureTag> texturePool_;
    core::HandlePool<std::unique_ptr<VulkanPipeline>, PipelineTag> pipelinePool_;

    struct RetiredResource {
        uint64_t timelineValue{0};
        VkCommandBuffer cmdBuffer{VK_NULL_HANDLE};
        std::unique_ptr<VulkanBuffer> buffer;
        std::unique_ptr<VulkanTexture> texture;
        std::unique_ptr<VulkanPipeline> pipeline;
    };
    std::vector<RetiredResource> retiredResources_;

    VkSemaphore timelineSemaphore_{VK_NULL_HANDLE};
    uint64_t currentTimelineValue_{0};
    VkPipelineCache pipelineCache_{VK_NULL_HANDLE};
    std::unique_ptr<VulkanStagingPool> stagingPool_{nullptr};
};

} // namespace northern::graphics
