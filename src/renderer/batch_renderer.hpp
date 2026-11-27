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
#include <unordered_map>
#include <vector>

namespace northern::renderer {

enum class BatchType {
    SdfShape,
    Text,
    Mask,
    Image,
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

    void setDpiScale(float scale) noexcept { dpiScale_ = scale > 0.0f ? scale : 1.0f; }
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }
    [[nodiscard]] bool isDualSourceTextEnabled() const noexcept { return dualSrcEnabled_; }

    void setImageCacheBudgetBytes(size_t bytes) noexcept { imageCacheBudgetBytes_ = bytes; }
    [[nodiscard]] size_t imageCacheBudgetBytes() const noexcept { return imageCacheBudgetBytes_; }
    [[nodiscard]] size_t cachedImageCount() const noexcept { return imageCache_.size(); }
    [[nodiscard]] size_t cachedImageBytes() const noexcept { return cachedImageBytes_; }

    [[nodiscard]] size_t batchCount() const noexcept { return batches_.size(); }
    [[nodiscard]] const std::vector<DrawBatch>& batches() const noexcept { return batches_; }
    [[nodiscard]] text::GlyphAtlas* glyphAtlas() const noexcept { return glyphAtlas_.get(); }
    [[nodiscard]] text::GlyphAtlas* maskAtlas() const noexcept { return maskAtlas_.get(); }

private:
    struct CachedImageEntry {
        std::weak_ptr<const Image::Impl> owner;
        graphics::TextureHandle texture{graphics::TextureHandle::invalid()};
        size_t byteSize{0};
        uint64_t lastUsedFrame{0};
    };

    BatchRenderer(graphics::Device& device, std::unique_ptr<DynamicRingBuffer> ringBuffer,
                  graphics::PipelineHandle sdfPipeline, graphics::PipelineHandle textPipeline,
                  graphics::PipelineHandle maskPipeline, graphics::PipelineHandle imagePipeline,
                  std::unique_ptr<text::GlyphAtlas> glyphAtlas,
                  std::unique_ptr<text::GlyphAtlas> maskAtlas, bool dualSrcEnabled) noexcept;

    core::RectI currentScissor() const noexcept;
    void pushScissor(const core::Rect& clipRect);
    void popScissor();
    void pruneImageCache();
    void emitQuad(const DrawBoxCommand& box);
    void emitLine(const DrawLineCommand& cmd);
    void emitGlyphs(const DrawGlyphsCommand& cmd);
    void emitSvg(const DrawSvgCommand& cmd);
    void emitImage(const DrawImageCommand& cmd);

    graphics::Device* device_{nullptr};
    std::unique_ptr<DynamicRingBuffer> ringBuffer_;
    graphics::PipelineHandle sdfPipeline_{graphics::PipelineHandle::invalid()};
    graphics::PipelineHandle textPipeline_{graphics::PipelineHandle::invalid()};
    graphics::PipelineHandle maskPipeline_{graphics::PipelineHandle::invalid()};
    graphics::PipelineHandle imagePipeline_{graphics::PipelineHandle::invalid()};
    std::unique_ptr<text::GlyphAtlas> glyphAtlas_;
    std::unique_ptr<text::GlyphAtlas> maskAtlas_;
    bool dualSrcEnabled_{false};
    float dpiScale_{1.0f};

    std::unordered_map<uint64_t, CachedImageEntry> imageCache_;
    size_t cachedImageBytes_{0};
    size_t imageCacheBudgetBytes_{128ull * 1024ull * 1024ull};
    uint64_t frameCounter_{0};

    core::Size viewportSize_{0.0f, 0.0f};
    std::vector<core::RectI> scissorStack_;
    std::vector<DrawBatch> batches_;
};

} // namespace northern::renderer
