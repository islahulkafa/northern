#pragma once

#include "core/memory/handle_pool.hpp"
#include "graphics/device.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace northern::graphics {

struct MockBufferData {
    BufferDesc desc{};
    std::vector<uint8_t> storage{};
};

struct MockTextureData {
    TextureDesc desc{};
    std::vector<uint8_t> storage{};
};

class MockDevice;

class MockCommandBuffer final : public CommandBuffer {
public:
    explicit MockCommandBuffer(MockDevice* device) noexcept : device_(device) {}
    ~MockCommandBuffer() override = default;

    void begin() override;
    void end() override;
    void beginRendering(const RenderingDesc& desc) override;
    void endRendering() override;
    void setViewport(const core::Rect& rect) override;
    void setScissor(const core::RectI& rect) override;
    void bindPipeline(PipelineHandle pipeline) override;
    void bindTexture(uint32_t set, uint32_t binding, TextureHandle texture) override;
    void bindVertexBuffer(uint32_t binding, BufferHandle buffer, uint64_t offset) override;
    void bindIndexBuffer(BufferHandle buffer, uint64_t offset, IndexType indexType) override;
    void pushConstants(PipelineHandle pipeline, ShaderStage stage, uint32_t offset, uint32_t size,
                       const void* data) override;
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
              uint32_t firstInstance) override;
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                     int32_t vertexOffset, uint32_t firstInstance) override;

    [[nodiscard]] uint32_t drawCallCount() const noexcept { return drawCallCount_; }
    [[nodiscard]] uint32_t totalVerticesDrawn() const noexcept { return totalVerticesDrawn_; }
    [[nodiscard]] uint32_t totalIndicesDrawn() const noexcept { return totalIndicesDrawn_; }
    [[nodiscard]] bool isInsideRendering() const noexcept { return insideRendering_; }
    [[nodiscard]] MockDevice* device() const noexcept { return device_; }

private:
    MockDevice* device_{nullptr};
    bool insideRendering_{false};
    uint32_t drawCallCount_{0};
    uint32_t totalVerticesDrawn_{0};
    uint32_t totalIndicesDrawn_{0};
    core::Rect currentViewport_{};
    core::RectI currentScissor_{};
    PipelineHandle boundPipeline_{PipelineHandle::invalid()};
};

class MockSwapchain final : public Swapchain {
public:
    MockSwapchain(MockDevice* device, const SwapchainDesc& desc);
    ~MockSwapchain() override;

    [[nodiscard]] core::Result<uint32_t, core::Error>
    acquireNextImage(uint64_t timeoutNs = UINT64_MAX) override;

    [[nodiscard]] core::Result<void, core::Error> present(uint32_t imageIndex) override;
    [[nodiscard]] core::Result<void, core::Error> resize(core::SizeI newSize) override;

    [[nodiscard]] TextureHandle currentTexture() const noexcept override;
    [[nodiscard]] Format format() const noexcept override { return desc_.format; }
    [[nodiscard]] core::SizeI size() const noexcept override { return desc_.size; }
    [[nodiscard]] uint32_t currentFrameIndex() const noexcept override { return currentFrame_; }
    [[nodiscard]] uint32_t maxFramesInFlight() const noexcept override { return 2; }

private:
    MockDevice* device_{nullptr};
    SwapchainDesc desc_{};
    uint32_t currentFrame_{0};
    std::vector<TextureHandle> renderTargets_{};
};

class MockDevice final : public Device {
public:
    static std::unique_ptr<MockDevice> create();

    MockDevice() = default;
    ~MockDevice() override;

    MockDevice(const MockDevice&) = delete;
    MockDevice& operator=(const MockDevice&) = delete;

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

    [[nodiscard]] bool supportsDualSourceBlending() const noexcept override {
        return dualSrcBlendSupported_;
    }
    void setDualSourceBlendingSupported(bool supported) noexcept {
        dualSrcBlendSupported_ = supported;
    }

    [[nodiscard]] MockBufferData* getBuffer(BufferHandle handle);
    [[nodiscard]] MockTextureData* getTexture(TextureHandle handle);
    [[nodiscard]] uint64_t frameCount() const noexcept { return frameCount_; }
    [[nodiscard]] uint64_t submitCount() const noexcept { return submitCount_; }

private:
    core::HandlePool<MockBufferData, BufferTag> bufferPool_{};
    core::HandlePool<MockTextureData, TextureTag> texturePool_{};
    core::HandlePool<PipelineDesc, PipelineTag> pipelinePool_{};
    uint64_t frameCount_{0};
    uint64_t submitCount_{0};
    bool dualSrcBlendSupported_{false};
};

} // namespace northern::graphics
