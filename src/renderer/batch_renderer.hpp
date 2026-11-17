#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "core/math/geometry.hpp"
#include "graphics/command_buffer.hpp"
#include "graphics/device.hpp"
#include "renderer/dynamic_ring_buffer.hpp"
#include "renderer/paint_context.hpp"
#include "text/glyph_atlas.hpp"
#include <memory>
#include <vector>

namespace northern::renderer {

enum class BatchType {
    SdfShape,
    Text,
};

struct DrawBatch {
    BatchType type{BatchType::SdfShape};
    uint32_t firstIndex{0};
    uint32_t indexCount{0};
    core::RectI scissorRect{};
    graphics::TextureHandle texture{graphics::TextureHandle::invalid()};
};

class BatchRenderer {
public:
    static core::Result<std::unique_ptr<BatchRenderer>, core::Error>
    create(graphics::Device& device, graphics::Format colorFormat, uint32_t maxVertices = 65536,
           uint32_t maxIndices = 196608, uint32_t frameSlots = 3);

    ~BatchRenderer();

    BatchRenderer(const BatchRenderer&) = delete;
    BatchRenderer& operator=(const BatchRenderer&) = delete;
    BatchRenderer(BatchRenderer&&) noexcept = default;
    BatchRenderer& operator=(BatchRenderer&&) noexcept = default;

    void begin(const core::Size& viewportSize, uint32_t frameSlot);
    void render(const PaintContext& context, graphics::CommandBuffer& cmdBuffer);
    void end();

    [[nodiscard]] size_t batchCount() const noexcept { return batches_.size(); }
    [[nodiscard]] const std::vector<DrawBatch>& batches() const noexcept { return batches_; }
    [[nodiscard]] text::GlyphAtlas* glyphAtlas() const noexcept { return glyphAtlas_.get(); }

private:
    BatchRenderer(graphics::Device& device, std::unique_ptr<DynamicRingBuffer> ringBuffer,
                  graphics::PipelineHandle sdfPipeline, graphics::PipelineHandle textPipeline,
                  std::unique_ptr<text::GlyphAtlas> glyphAtlas) noexcept;

    core::RectI currentScissor() const noexcept;
    void pushScissor(const core::Rect& clipRect);
    void popScissor();
    void emitQuad(const DrawBoxCommand& box);
    void emitLine(const DrawLineCommand& cmd);
    void emitGlyphs(const DrawGlyphsCommand& cmd);

    graphics::Device* device_{nullptr};
    std::unique_ptr<DynamicRingBuffer> ringBuffer_;
    graphics::PipelineHandle sdfPipeline_{graphics::PipelineHandle::invalid()};
    graphics::PipelineHandle textPipeline_{graphics::PipelineHandle::invalid()};
    std::unique_ptr<text::GlyphAtlas> glyphAtlas_;

    core::Size viewportSize_{0.0f, 0.0f};
    std::vector<core::RectI> scissorStack_;
    std::vector<DrawBatch> batches_;
};

} // namespace northern::renderer
