#pragma once

#include "core/math/geometry.hpp"
#include "graphics/gpu_handle.hpp"
#include "graphics/types.hpp"
#include <cstdint>

namespace northern::graphics {

struct RenderingAttachmentDesc {
    TextureHandle target{TextureHandle::invalid()};
    bool clear{true};
    ClearColor clearColor{0.0f, 0.0f, 0.0f, 1.0f};
};

struct RenderingDesc {
    core::RectI renderArea{0, 0, 0, 0};
    RenderingAttachmentDesc colorAttachment{};
};

class CommandBuffer {
public:
    virtual ~CommandBuffer() = default;

    virtual void begin() = 0;
    virtual void end() = 0;

    virtual void beginRendering(const RenderingDesc& desc) = 0;
    virtual void endRendering() = 0;

    virtual void setViewport(const core::Rect& rect) = 0;
    virtual void setScissor(const core::RectI& rect) = 0;

    virtual void bindPipeline(PipelineHandle pipeline) = 0;
    virtual void bindTexture(uint32_t set, uint32_t binding, TextureHandle texture) = 0;
    virtual void bindVertexBuffer(uint32_t binding, BufferHandle buffer, uint64_t offset = 0) = 0;
    virtual void bindIndexBuffer(BufferHandle buffer, uint64_t offset, IndexType indexType) = 0;

    virtual void pushConstants(PipelineHandle pipeline, ShaderStage stage, uint32_t offset,
                               uint32_t size, const void* data) = 0;

    virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0,
                      uint32_t firstInstance = 0) = 0;

    virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
                             uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                             uint32_t firstInstance = 0) = 0;
};

} // namespace northern::graphics
