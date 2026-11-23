#include "graphics/mock/mock_device.hpp"
#include <algorithm>
#include <cstring>

namespace northern::graphics {

// MockCommandBuffer
void MockCommandBuffer::begin() {
    drawCallCount_ = 0;
    totalVerticesDrawn_ = 0;
    totalIndicesDrawn_ = 0;
    insideRendering_ = false;
    boundPipeline_ = PipelineHandle::invalid();
}

void MockCommandBuffer::end() {
    insideRendering_ = false;
}

void MockCommandBuffer::beginRendering(const RenderingDesc& /*desc*/) {
    insideRendering_ = true;
}

void MockCommandBuffer::endRendering() {
    insideRendering_ = false;
}

void MockCommandBuffer::setViewport(const core::Rect& rect) {
    currentViewport_ = rect;
}

void MockCommandBuffer::setScissor(const core::RectI& rect) {
    currentScissor_ = rect;
}

void MockCommandBuffer::bindPipeline(PipelineHandle pipeline) {
    boundPipeline_ = pipeline;
}

void MockCommandBuffer::bindTexture(uint32_t /*set*/, uint32_t /*binding*/,
                                    TextureHandle /*texture*/) {}

void MockCommandBuffer::bindVertexBuffer(uint32_t /*binding*/, BufferHandle /*buffer*/,
                                         uint64_t /*offset*/) {}

void MockCommandBuffer::bindIndexBuffer(BufferHandle /*buffer*/, uint64_t /*offset*/,
                                        IndexType /*indexType*/) {}

void MockCommandBuffer::pushConstants(PipelineHandle /*pipeline*/, ShaderStage /*stage*/,
                                      uint32_t /*offset*/, uint32_t /*size*/,
                                      const void* /*data*/) {}

void MockCommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t /*firstVertex*/,
                             uint32_t /*firstInstance*/) {
    ++drawCallCount_;
    totalVerticesDrawn_ += vertexCount * instanceCount;
}

void MockCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                    uint32_t /*firstIndex*/, int32_t /*vertexOffset*/,
                                    uint32_t /*firstInstance*/) {
    ++drawCallCount_;
    totalIndicesDrawn_ += indexCount * instanceCount;
}

// MockSwapchain

MockSwapchain::MockSwapchain(MockDevice* device, const SwapchainDesc& desc)
    : device_(device), desc_(desc) {
    for (uint32_t i = 0; i < 2; ++i) {
        TextureDesc texDesc{
            .width = static_cast<uint32_t>(desc.size.width),
            .height = static_cast<uint32_t>(desc.size.height),
            .format = desc.format,
            .usage = TextureUsage::ColorAttachment | TextureUsage::Sampled,
            .debugName = "MockSwapchainTarget",
        };
        auto res = device_->createTexture(texDesc);
        if (res.isOk()) {
            renderTargets_.push_back(res.value());
        }
    }
}

MockSwapchain::~MockSwapchain() {
    for (auto handle : renderTargets_) {
        device_->destroyTexture(handle);
    }
    renderTargets_.clear();
}

core::Result<uint32_t, core::Error> MockSwapchain::acquireNextImage(uint64_t /*timeoutNs*/) {
    return currentFrame_;
}

core::Result<void, core::Error> MockSwapchain::present(uint32_t /*imageIndex*/) {
    currentFrame_ = (currentFrame_ + 1) % 2;
    return {};
}

core::Result<void, core::Error> MockSwapchain::resize(core::SizeI newSize) {
    desc_.size = newSize;
    for (auto handle : renderTargets_) {
        device_->destroyTexture(handle);
    }
    renderTargets_.clear();

    for (uint32_t i = 0; i < 2; ++i) {
        TextureDesc texDesc{
            .width = static_cast<uint32_t>(desc_.size.width),
            .height = static_cast<uint32_t>(desc_.size.height),
            .format = desc_.format,
            .usage = TextureUsage::ColorAttachment | TextureUsage::Sampled,
            .debugName = "MockSwapchainTargetResized",
        };
        auto res = device_->createTexture(texDesc);
        if (res.isOk()) {
            renderTargets_.push_back(res.value());
        }
    }
    return {};
}

TextureHandle MockSwapchain::currentTexture() const noexcept {
    if (renderTargets_.empty()) {
        return TextureHandle::invalid();
    }
    return renderTargets_[currentFrame_ % renderTargets_.size()];
}

// MockDevice

std::unique_ptr<MockDevice> MockDevice::create() {
    return std::make_unique<MockDevice>();
}

MockDevice::~MockDevice() = default;

core::Result<std::unique_ptr<Swapchain>, core::Error>
MockDevice::createSwapchain(const SwapchainDesc& desc) {
    return std::make_unique<MockSwapchain>(this, desc);
}

core::Result<std::unique_ptr<CommandBuffer>, core::Error> MockDevice::createCommandBuffer() {
    return std::make_unique<MockCommandBuffer>(this);
}

core::Result<BufferHandle, core::Error> MockDevice::createBuffer(const BufferDesc& desc,
                                                                 const void* initialData) {
    MockBufferData bufData;
    bufData.desc = desc;
    bufData.storage.resize(desc.size);
    if (initialData) {
        std::memcpy(bufData.storage.data(), initialData, desc.size);
    }
    return bufferPool_.allocate(std::move(bufData));
}

core::Result<void, core::Error> MockDevice::writeBuffer(BufferHandle handle, uint64_t offset,
                                                        uint64_t size, const void* data) {
    auto* buf = bufferPool_.get(handle);
    if (!buf) {
        return core::Error(core::ErrorCode::InvalidArgument, "Invalid buffer handle");
    }
    if (offset + size > buf->storage.size()) {
        return core::Error(core::ErrorCode::OutOfBounds, "Buffer write out of bounds");
    }
    std::memcpy(buf->storage.data() + offset, data, size);
    return {};
}

void* MockDevice::mapBuffer(BufferHandle handle) {
    auto* buf = bufferPool_.get(handle);
    return buf ? buf->storage.data() : nullptr;
}

core::Result<TextureHandle, core::Error> MockDevice::createTexture(const TextureDesc& desc,
                                                                   const void* initialData) {
    uint32_t bpp = 4;
    if (desc.format == Format::R8_Unorm) {
        bpp = 1;
    }
    size_t byteSize = static_cast<size_t>(desc.width) * desc.height * bpp;

    MockTextureData texData;
    texData.desc = desc;
    texData.storage.resize(byteSize);
    if (initialData) {
        std::memcpy(texData.storage.data(), initialData, byteSize);
    }
    return texturePool_.allocate(std::move(texData));
}

core::Result<void, core::Error> MockDevice::writeTexture(TextureHandle handle, uint32_t x,
                                                         uint32_t y, uint32_t width,
                                                         uint32_t height, const void* data) {
    auto* tex = texturePool_.get(handle);
    if (!tex) {
        return core::Error(core::ErrorCode::InvalidArgument, "Invalid texture handle");
    }
    uint32_t bpp = (tex->desc.format == Format::R8_Unorm) ? 1 : 4;
    size_t rowBytes = static_cast<size_t>(width) * bpp;
    size_t dstStride = static_cast<size_t>(tex->desc.width) * bpp;

    const auto* src = static_cast<const uint8_t*>(data);
    for (uint32_t row = 0; row < height; ++row) {
        size_t dstOffset =
            (static_cast<size_t>(y + row) * dstStride) + (static_cast<size_t>(x) * bpp);
        if (dstOffset + rowBytes <= tex->storage.size()) {
            std::memcpy(tex->storage.data() + dstOffset, src + (row * rowBytes), rowBytes);
        }
    }
    return {};
}

core::Result<PipelineHandle, core::Error> MockDevice::createPipeline(const PipelineDesc& desc) {
    return pipelinePool_.allocate(desc);
}

void MockDevice::destroyBuffer(BufferHandle handle) {
    bufferPool_.release(handle);
}

void MockDevice::destroyTexture(TextureHandle handle) {
    texturePool_.release(handle);
}

void MockDevice::destroyPipeline(PipelineHandle handle) {
    pipelinePool_.release(handle);
}

core::Result<void, core::Error> MockDevice::submit(CommandBuffer* /*cmdBuffer*/,
                                                   Swapchain* /*swapchain*/) {
    ++submitCount_;
    return {};
}

void MockDevice::advanceFrame() {
    ++frameCount_;
}

void MockDevice::waitIdle() {}

MockBufferData* MockDevice::getBuffer(BufferHandle handle) {
    return bufferPool_.get(handle);
}

MockTextureData* MockDevice::getTexture(TextureHandle handle) {
    return texturePool_.get(handle);
}

} // namespace northern::graphics
