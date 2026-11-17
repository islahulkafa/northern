#pragma once

#include "graphics/command_buffer.hpp"
#include "graphics/vulkan/vk_common.hpp"

namespace northern::graphics {

class VulkanDevice;

class VulkanCommandBuffer final : public CommandBuffer {
public:
    VulkanCommandBuffer(VkDevice device, VkCommandPool pool, VulkanDevice* deviceImpl) noexcept;
    ~VulkanCommandBuffer() override;

    VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
    VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;

    VulkanCommandBuffer(VulkanCommandBuffer&&) noexcept;
    VulkanCommandBuffer& operator=(VulkanCommandBuffer&&) noexcept;

    void begin() override;
    void end() override;

    void beginRendering(const RenderingDesc& desc) override;
    void endRendering() override;

    void setViewport(const core::Rect& rect) override;
    void setScissor(const core::RectI& rect) override;

    void bindPipeline(PipelineHandle pipeline) override;
    void bindTexture(uint32_t set, uint32_t binding, TextureHandle texture) override;
    void bindVertexBuffer(uint32_t binding, BufferHandle buffer, uint64_t offset = 0) override;
    void bindIndexBuffer(BufferHandle buffer, uint64_t offset, IndexType indexType) override;

    void pushConstants(PipelineHandle pipeline, ShaderStage stage, uint32_t offset, uint32_t size,
                       const void* data) override;

    void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0,
              uint32_t firstInstance = 0) override;

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0,
                     int32_t vertexOffset = 0, uint32_t firstInstance = 0) override;

    [[nodiscard]] VkCommandBuffer handle() const noexcept { return cmd_; }

private:
    VkDevice device_{VK_NULL_HANDLE};
    VkCommandPool pool_{VK_NULL_HANDLE};
    VulkanDevice* deviceImpl_{nullptr};
    VkCommandBuffer cmd_{VK_NULL_HANDLE};
    TextureHandle currentRenderTarget_{TextureHandle::invalid()};
    PipelineHandle currentPipeline_{PipelineHandle::invalid()};
    VkPipelineLayout currentPipelineLayout_{VK_NULL_HANDLE};
};

} // namespace northern::graphics
