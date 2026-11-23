#include "renderer/batch_renderer.hpp"
#include "../../shaders/sdf_shaders.hpp"
#include "shaders/text_shaders.hpp"
#include <algorithm>
#include <cmath>

namespace northern::renderer {

BatchRenderer::BatchRenderer(graphics::Device& device,
                             std::unique_ptr<DynamicRingBuffer> ringBuffer,
                             graphics::PipelineHandle sdfPipeline,
                             graphics::PipelineHandle textPipeline,
                             std::unique_ptr<text::GlyphAtlas> glyphAtlas) noexcept
    : device_(&device), ringBuffer_(std::move(ringBuffer)), sdfPipeline_(sdfPipeline),
      textPipeline_(textPipeline), glyphAtlas_(std::move(glyphAtlas)) {}

BatchRenderer::~BatchRenderer() {
    if (device_) {
        if (sdfPipeline_.isValid()) {
            device_->destroyPipeline(sdfPipeline_);
            sdfPipeline_ = graphics::PipelineHandle::invalid();
        }
        if (textPipeline_.isValid()) {
            device_->destroyPipeline(textPipeline_);
            textPipeline_ = graphics::PipelineHandle::invalid();
        }
    }
}

core::Result<std::unique_ptr<BatchRenderer>, core::Error>
BatchRenderer::create(graphics::Device& device, graphics::Format colorFormat, uint32_t maxVertices,
                      uint32_t maxIndices, uint32_t frameSlots) {
    auto ringRes = DynamicRingBuffer::create(device, maxVertices, maxIndices, frameSlots);
    if (ringRes.isErr()) {
        return ringRes.error();
    }

    graphics::PipelineDesc sdfPipeDesc{};
    sdfPipeDesc.debugName = "2D Batch Renderer SDF Pipeline";
    sdfPipeDesc.vertexShader.spirv = shaders::kSdfRectVertSpv;
    sdfPipeDesc.fragmentShader.spirv = shaders::kSdfRectFragSpv;
    sdfPipeDesc.colorAttachmentFormat = colorFormat;
    sdfPipeDesc.pushConstantSize = sizeof(float) * 16; // 4x4 matrix

    sdfPipeDesc.vertexLayout.bindings.push_back({0, sizeof(Vertex2D)});
    sdfPipeDesc.vertexLayout.attributes = {
        {0, 0, graphics::Format::R32G32_Sfloat, offsetof(Vertex2D, position)},
        {1, 0, graphics::Format::R32G32B32A32_Sfloat, offsetof(Vertex2D, color)},
        {2, 0, graphics::Format::R32G32_Sfloat, offsetof(Vertex2D, localPos)},
        {3, 0, graphics::Format::R32G32_Sfloat, offsetof(Vertex2D, boxSize)},
        {4, 0, graphics::Format::R32G32B32A32_Sfloat, offsetof(Vertex2D, cornerRadii)},
        {5, 0, graphics::Format::R32G32B32A32_Sfloat, offsetof(Vertex2D, borderColor)},
        {6, 0, graphics::Format::R32_Sfloat, offsetof(Vertex2D, borderWidth)},
    };

    auto sdfPipeRes = device.createPipeline(sdfPipeDesc);
    if (sdfPipeRes.isErr()) {
        return sdfPipeRes.error();
    }

    graphics::PipelineDesc textPipeDesc{};
    textPipeDesc.debugName = "2D Batch Renderer Text Pipeline";
    textPipeDesc.vertexShader.spirv = shaders::kTextVertSpv;
    textPipeDesc.fragmentShader.spirv = shaders::kTextFragSpv;
    textPipeDesc.hasTexture = true;
    textPipeDesc.colorAttachmentFormat = colorFormat;
    textPipeDesc.pushConstantSize = sizeof(float) * 16;
    textPipeDesc.vertexLayout.bindings.push_back({0, sizeof(Vertex2D)});
    textPipeDesc.vertexLayout.attributes = {
        {0, 0, graphics::Format::R32G32_Sfloat, offsetof(Vertex2D, position)},
        {1, 0, graphics::Format::R32G32B32A32_Sfloat, offsetof(Vertex2D, color)},
        {2, 0, graphics::Format::R32G32_Sfloat, offsetof(Vertex2D, localPos)},
        {3, 0, graphics::Format::R32G32_Sfloat, offsetof(Vertex2D, boxSize)},
        {4, 0, graphics::Format::R32G32B32A32_Sfloat, offsetof(Vertex2D, cornerRadii)},
        {5, 0, graphics::Format::R32G32B32A32_Sfloat, offsetof(Vertex2D, borderColor)},
        {6, 0, graphics::Format::R32_Sfloat, offsetof(Vertex2D, borderWidth)},
    };

    auto textPipeRes = device.createPipeline(textPipeDesc);
    if (textPipeRes.isErr()) {
        device.destroyPipeline(sdfPipeRes.value());
        return textPipeRes.error();
    }

    auto atlasRes = text::GlyphAtlas::create(device, 1024, 1024);
    if (atlasRes.isErr()) {
        device.destroyPipeline(sdfPipeRes.value());
        device.destroyPipeline(textPipeRes.value());
        return atlasRes.error();
    }

    return std::unique_ptr<BatchRenderer>(new BatchRenderer(device, std::move(ringRes.value()),
                                                            sdfPipeRes.value(), textPipeRes.value(),
                                                            std::move(atlasRes.value())));
}

void BatchRenderer::begin(const core::Size& viewportSize, uint32_t frameSlot) {
    viewportSize_ = viewportSize;
    ringBuffer_->resetFrame(frameSlot);
    batches_.clear();
    scissorStack_.clear();

    core::RectI rootScissor{
        {0, 0},
        {static_cast<int32_t>(viewportSize.width), static_cast<int32_t>(viewportSize.height)}};
    scissorStack_.push_back(rootScissor);
}

core::RectI BatchRenderer::currentScissor() const noexcept {
    if (!scissorStack_.empty()) {
        return scissorStack_.back();
    }
    return core::RectI{
        {0, 0},
        {static_cast<int32_t>(viewportSize_.width), static_cast<int32_t>(viewportSize_.height)}};
}

void BatchRenderer::pushScissor(const core::Rect& clipRect) {
    int32_t cx = static_cast<int32_t>(clipRect.origin.x);
    int32_t cy = static_cast<int32_t>(clipRect.origin.y);
    int32_t cw = static_cast<int32_t>(clipRect.size.width);
    int32_t ch = static_cast<int32_t>(clipRect.size.height);

    core::RectI parent = currentScissor();
    int32_t x0 = std::max(parent.origin.x, cx);
    int32_t y0 = std::max(parent.origin.y, cy);
    int32_t x1 = std::min(parent.origin.x + parent.size.width, cx + cw);
    int32_t y1 = std::min(parent.origin.y + parent.size.height, cy + ch);

    int32_t finalW = std::max(0, x1 - x0);
    int32_t finalH = std::max(0, y1 - y0);

    scissorStack_.push_back(core::RectI{{x0, y0}, {finalW, finalH}});
}

void BatchRenderer::popScissor() {
    if (scissorStack_.size() > 1) {
        scissorStack_.pop_back();
    }
}

void BatchRenderer::emitQuad(const DrawBoxCommand& box) {
    if (box.rect.size.width <= 0.0f || box.rect.size.height <= 0.0f) {
        return;
    }

    auto allocRes = ringBuffer_->allocate(4, 6);
    if (allocRes.isErr()) {
        return;
    }
    auto alloc = allocRes.value();

    float pad = 1.5f;
    float x = box.rect.origin.x - pad;
    float y = box.rect.origin.y - pad;
    float w = box.rect.size.width + 2.0f * pad;
    float h = box.rect.size.height + 2.0f * pad;
    float halfW = box.rect.size.width * 0.5f;
    float halfH = box.rect.size.height * 0.5f;

    core::RectI activeScissor = currentScissor();
    if (batches_.empty() || batches_.back().scissorRect != activeScissor ||
        batches_.back().type != BatchType::SdfShape) {
        DrawBatch batch{};
        batch.type = BatchType::SdfShape;
        batch.firstIndex = alloc.firstIndex;
        batch.indexCount = 6;
        batch.scissorRect = activeScissor;
        batches_.push_back(batch);
    } else {
        batches_.back().indexCount += 6;
    }

    Vertex2D* v = alloc.vertexData;
    // Top-Left
    v[0].position = {x, y};
    v[0].color = box.fillColor;
    v[0].localPos = {-halfW - pad, -halfH - pad};
    v[0].boxSize = {box.rect.size.width, box.rect.size.height};
    std::copy_n(box.cornerRadii, 4, v[0].cornerRadii);
    v[0].borderColor = box.borderColor;
    v[0].borderWidth = box.borderWidth;

    // Top-Right
    v[1].position = {x + w, y};
    v[1].color = box.fillColor;
    v[1].localPos = {halfW + pad, -halfH - pad};
    v[1].boxSize = {box.rect.size.width, box.rect.size.height};
    std::copy_n(box.cornerRadii, 4, v[1].cornerRadii);
    v[1].borderColor = box.borderColor;
    v[1].borderWidth = box.borderWidth;

    // Bottom-Right
    v[2].position = {x + w, y + h};
    v[2].color = box.fillColor;
    v[2].localPos = {halfW + pad, halfH + pad};
    v[2].boxSize = {box.rect.size.width, box.rect.size.height};
    std::copy_n(box.cornerRadii, 4, v[2].cornerRadii);
    v[2].borderColor = box.borderColor;
    v[2].borderWidth = box.borderWidth;

    // Bottom-Left
    v[3].position = {x, y + h};
    v[3].color = box.fillColor;
    v[3].localPos = {-halfW - pad, halfH + pad};
    v[3].boxSize = {box.rect.size.width, box.rect.size.height};
    std::copy_n(box.cornerRadii, 4, v[3].cornerRadii);
    v[3].borderColor = box.borderColor;
    v[3].borderWidth = box.borderWidth;

    uint32_t baseV = alloc.firstVertex;
    alloc.indexData[0] = baseV + 0;
    alloc.indexData[1] = baseV + 1;
    alloc.indexData[2] = baseV + 2;
    alloc.indexData[3] = baseV + 2;
    alloc.indexData[4] = baseV + 3;
    alloc.indexData[5] = baseV + 0;
}

void BatchRenderer::emitLine(const DrawLineCommand& cmd) {
    float dx = cmd.p1.x - cmd.p0.x;
    float dy = cmd.p1.y - cmd.p0.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len <= 0.0001f || cmd.thickness <= 0.0f) {
        return;
    }

    auto allocRes = ringBuffer_->allocate(4, 6);
    if (allocRes.isErr()) {
        return;
    }
    auto alloc = allocRes.value();

    float pad = 1.5f;
    float halfL = len * 0.5f;
    float halfW = cmd.thickness * 0.5f;

    core::Point center{(cmd.p0.x + cmd.p1.x) * 0.5f, (cmd.p0.y + cmd.p1.y) * 0.5f};
    core::Point T{dx / len, dy / len};
    core::Point N{-T.y, T.x};

    float extL = halfL + pad;
    float extW = halfW + pad;

    core::Point pt0 = center - T * extL - N * extW;
    core::Point pt1 = center + T * extL - N * extW;
    core::Point pt2 = center + T * extL + N * extW;
    core::Point pt3 = center - T * extL + N * extW;

    core::RectI activeScissor = currentScissor();
    if (batches_.empty() || batches_.back().scissorRect != activeScissor ||
        batches_.back().type != BatchType::SdfShape) {
        DrawBatch batch{};
        batch.type = BatchType::SdfShape;
        batch.firstIndex = alloc.firstIndex;
        batch.indexCount = 6;
        batch.scissorRect = activeScissor;
        batches_.push_back(batch);
    } else {
        batches_.back().indexCount += 6;
    }

    Vertex2D* v = alloc.vertexData;
    float radius = halfW;
    float radii[4] = {radius, radius, radius, radius};

    v[0].position = pt0;
    v[0].color = cmd.color;
    v[0].localPos = {-extL, -extW};
    v[0].boxSize = {len, cmd.thickness};
    std::copy_n(radii, 4, v[0].cornerRadii);
    v[0].borderColor = core::Color::transparent();
    v[0].borderWidth = 0.0f;

    v[1].position = pt1;
    v[1].color = cmd.color;
    v[1].localPos = {extL, -extW};
    v[1].boxSize = {len, cmd.thickness};
    std::copy_n(radii, 4, v[1].cornerRadii);
    v[1].borderColor = core::Color::transparent();
    v[1].borderWidth = 0.0f;

    v[2].position = pt2;
    v[2].color = cmd.color;
    v[2].localPos = {extL, extW};
    v[2].boxSize = {len, cmd.thickness};
    std::copy_n(radii, 4, v[2].cornerRadii);
    v[2].borderColor = core::Color::transparent();
    v[2].borderWidth = 0.0f;

    v[3].position = pt3;
    v[3].color = cmd.color;
    v[3].localPos = {-extL, extW};
    v[3].boxSize = {len, cmd.thickness};
    std::copy_n(radii, 4, v[3].cornerRadii);
    v[3].borderColor = core::Color::transparent();
    v[3].borderWidth = 0.0f;

    uint32_t baseV = alloc.firstVertex;
    alloc.indexData[0] = baseV + 0;
    alloc.indexData[1] = baseV + 1;
    alloc.indexData[2] = baseV + 2;
    alloc.indexData[3] = baseV + 2;
    alloc.indexData[4] = baseV + 3;
    alloc.indexData[5] = baseV + 0;
}

void BatchRenderer::emitGlyphs(const DrawGlyphsCommand& cmd) {
    if (!cmd.font || cmd.glyphs.empty() || !glyphAtlas_) {
        return;
    }

    core::RectI activeScissor = currentScissor();
    graphics::TextureHandle atlasTex = glyphAtlas_->texture();

    struct PendingGlyph {
        text::CachedGlyph cached;
        float basePixelX;
        float targetY;
    };

    std::vector<PendingGlyph> pending;
    pending.reserve(cmd.glyphs.size());

    for (const auto& g : cmd.glyphs) {
        float targetX = cmd.origin.x + g.position.x;
        float targetY = cmd.origin.y + g.position.y;
        float basePixelX = std::floor(targetX);
        float fractX = targetX - basePixelX;
        uint8_t subpixelBin = static_cast<uint8_t>(std::clamp(static_cast<int>(fractX * 4.0f), 0, 3));

        auto gRes = glyphAtlas_->getOrInsert(*cmd.font, g.glyphId, subpixelBin);
        if (gRes.isErr()) {
            continue;
        }
        const auto& cached = gRes.value();
        if (cached.size.width <= 0.0f || cached.size.height <= 0.0f) {
            continue;
        }
        pending.push_back({cached, basePixelX, targetY});
    }

    if (pending.empty()) {
        return;
    }

    uint32_t count = static_cast<uint32_t>(pending.size());
    auto allocRes = ringBuffer_->allocate(count * 4, count * 6);
    if (allocRes.isErr()) {
        return;
    }
    auto alloc = allocRes.value();

    if (batches_.empty() || batches_.back().scissorRect != activeScissor ||
        batches_.back().type != BatchType::Text || batches_.back().texture != atlasTex) {
        DrawBatch batch{};
        batch.type = BatchType::Text;
        batch.firstIndex = alloc.firstIndex;
        batch.indexCount = count * 6;
        batch.scissorRect = activeScissor;
        batch.texture = atlasTex;
        batches_.push_back(batch);
    } else {
        batches_.back().indexCount += count * 6;
    }

    Vertex2D* v = alloc.vertexData;
    uint32_t* idx = alloc.indexData;
    uint32_t baseV = alloc.firstVertex;

    for (uint32_t i = 0; i < count; ++i) {
        const auto& pg = pending[i];
        float x = pg.basePixelX + pg.cached.offset.x;
        float y = std::round(pg.targetY) - pg.cached.offset.y;
        float w = pg.cached.size.width;
        float h = pg.cached.size.height;

        float u0 = pg.cached.uvRect.origin.x;
        float v0 = pg.cached.uvRect.origin.y;
        float u1 = u0 + pg.cached.uvRect.size.width;
        float v1 = v0 + pg.cached.uvRect.size.height;

        v[0].position = {x, y};
        v[0].color = cmd.color;
        v[0].localPos = {u0, v0};

        v[1].position = {x + w, y};
        v[1].color = cmd.color;
        v[1].localPos = {u1, v0};

        v[2].position = {x + w, y + h};
        v[2].color = cmd.color;
        v[2].localPos = {u1, v1};

        v[3].position = {x, y + h};
        v[3].color = cmd.color;
        v[3].localPos = {u0, v1};

        uint32_t curBase = baseV + i * 4;
        idx[0] = curBase + 0;
        idx[1] = curBase + 1;
        idx[2] = curBase + 2;
        idx[3] = curBase + 2;
        idx[4] = curBase + 3;
        idx[5] = curBase + 0;

        v += 4;
        idx += 6;
    }
}

void BatchRenderer::render(const PaintContext& context, graphics::CommandBuffer& cmdBuffer) {
    for (const auto& cmd : context.commands()) {
        std::visit(
            [this](const auto& c) {
                using T = std::decay_t<decltype(c)>;
                if constexpr (std::is_same_v<T, DrawBoxCommand>) {
                    emitQuad(c);
                } else if constexpr (std::is_same_v<T, DrawLineCommand>) {
                    emitLine(c);
                } else if constexpr (std::is_same_v<T, DrawGlyphsCommand>) {
                    emitGlyphs(c);
                } else if constexpr (std::is_same_v<T, PushClipCommand>) {
                    pushScissor(c.clipRect);
                } else if constexpr (std::is_same_v<T, PopClipCommand>) {
                    popScissor();
                }
            },
            cmd);
    }

    if (glyphAtlas_) {
        (void)glyphAtlas_->flush();
    }

    if (batches_.empty()) {
        return;
    }

    float w = viewportSize_.width > 0.0f ? viewportSize_.width : 1.0f;
    float h = viewportSize_.height > 0.0f ? viewportSize_.height : 1.0f;
    float proj[16] = {2.0f / w, 0.0f, 0.0f, 0.0f, 0.0f,  2.0f / h, 0.0f, 0.0f,
                      0.0f,     0.0f, 1.0f, 0.0f, -1.0f, -1.0f,    0.0f, 1.0f};

    cmdBuffer.setViewport({{0.0f, 0.0f}, viewportSize_});
    cmdBuffer.bindVertexBuffer(0, ringBuffer_->vertexBuffer(), 0);
    cmdBuffer.bindIndexBuffer(ringBuffer_->indexBuffer(), 0, graphics::IndexType::Uint32);

    graphics::PipelineHandle currentlyBoundPipeline = graphics::PipelineHandle::invalid();

    for (const auto& batch : batches_) {
        if (batch.indexCount > 0) {
            graphics::PipelineHandle targetPipeline =
                (batch.type == BatchType::Text) ? textPipeline_ : sdfPipeline_;

            if (currentlyBoundPipeline != targetPipeline) {
                cmdBuffer.bindPipeline(targetPipeline);
                cmdBuffer.pushConstants(
                    targetPipeline, graphics::ShaderStage::Vertex | graphics::ShaderStage::Fragment,
                    0, sizeof(proj), proj);
                currentlyBoundPipeline = targetPipeline;
            }

            if (batch.type == BatchType::Text && batch.texture.isValid()) {
                cmdBuffer.bindTexture(0, 0, batch.texture);
            }

            cmdBuffer.setScissor(batch.scissorRect);
            cmdBuffer.drawIndexed(batch.indexCount, 1, batch.firstIndex, 0, 0);
        }
    }
}

void BatchRenderer::end() {
    scissorStack_.clear();
}

} // namespace northern::renderer
