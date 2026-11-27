#include "../../shaders/sdf_shaders.hpp"
#include "graphics/device.hpp"
#include "graphics/mock/mock_device.hpp"
#include "renderer/batch_renderer.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/dynamic_ring_buffer.hpp"
#include "renderer/image/image.hpp"
#include "renderer/paint_context.hpp"
#include "renderer/svg/svg_document.hpp"
#include "renderer/vertex_2d.hpp"
#include "text/font_manager.hpp"
#include <gtest/gtest.h>

TEST(RendererTest, Vertex2DLayoutAndAlignment) {
    using namespace northern::renderer;
    EXPECT_EQ(sizeof(Vertex2D) % 16, 0);
    EXPECT_GE(sizeof(Vertex2D), 72);
}

TEST(RendererTest, DrawCommandVariants) {
    using namespace northern::renderer;
    DrawBoxCommand box{};
    box.rect = {10.0f, 20.0f, 100.0f, 50.0f};
    box.fillColor = northern::core::Color{1.0f, 0.0f, 0.0f, 1.0f};
    box.cornerRadii[0] = 8.0f;
    DrawCommand cmd = box;
    EXPECT_TRUE(std::holds_alternative<DrawBoxCommand>(cmd));

    PushClipCommand clip{};
    clip.clipRect = {0.0f, 0.0f, 50.0f, 50.0f};
    cmd = clip;
    EXPECT_TRUE(std::holds_alternative<PushClipCommand>(cmd));

    PopClipCommand pop{};
    cmd = pop;
    EXPECT_TRUE(std::holds_alternative<PopClipCommand>(cmd));
}

TEST(RendererTest, PaintContextRecordingAndClear) {
    using namespace northern;
    using namespace northern::renderer;

    PaintContext ctx;
    EXPECT_EQ(ctx.commands().size(), 0);

    ctx.drawRect({{10, 10}, {100, 50}}, core::Color::fromRgba8(255, 0, 0));
    ctx.drawRoundedRect({{20, 20}, {60, 60}}, core::Color::fromRgba8(0, 255, 0), 12.0f);
    ctx.pushClip({{0, 0}, {50, 50}});
    ctx.popClip();

    EXPECT_EQ(ctx.commands().size(), 4);

    ctx.clear();
    EXPECT_EQ(ctx.commands().size(), 0);

    PaintContext other;
    other.drawRect({{0, 0}, {10, 10}}, core::Color::white());
    ctx.append(other);
    EXPECT_EQ(ctx.commands().size(), 1);
}

TEST(RendererTest, DynamicRingBufferAllocation) {
    using namespace northern;
    using namespace northern::graphics;
    using namespace northern::renderer;

    auto devRes = Device::create();
    ASSERT_TRUE(devRes.isOk());
    auto device = std::move(devRes.value());

    auto ringRes = DynamicRingBuffer::create(*device, 1024, 2048, 2);
    ASSERT_TRUE(ringRes.isOk());
    auto ring = std::move(ringRes.value());

    ring->resetFrame(0);
    auto alloc1 = ring->allocate(4, 6);
    ASSERT_TRUE(alloc1.isOk());
    EXPECT_EQ(alloc1.value().firstVertex, 0);
    EXPECT_EQ(alloc1.value().firstIndex, 0);
    EXPECT_NE(alloc1.value().vertexData, nullptr);
    EXPECT_NE(alloc1.value().indexData, nullptr);

    auto alloc2 = ring->allocate(4, 6);
    ASSERT_TRUE(alloc2.isOk());
    EXPECT_EQ(alloc2.value().firstVertex, 4);
    EXPECT_EQ(alloc2.value().firstIndex, 6);

    ring->resetFrame(1);
    auto allocFrame1 = ring->allocate(4, 6);
    ASSERT_TRUE(allocFrame1.isOk());
    EXPECT_EQ(allocFrame1.value().firstVertex, 512);
    EXPECT_EQ(allocFrame1.value().firstIndex, 1024);
}

TEST(RendererTest, SdfPipelineCreation) {
    using namespace northern;
    using namespace northern::graphics;
    using namespace northern::renderer;

    auto devRes = Device::create();
    ASSERT_TRUE(devRes.isOk());
    auto device = std::move(devRes.value());

    PipelineDesc pipeDesc{};
    pipeDesc.debugName = "2D SDF Rect Pipeline Test";
    pipeDesc.vertexShader.spirv = shaders::kSdfRectVertSpv;
    pipeDesc.fragmentShader.spirv = shaders::kSdfRectFragSpv;
    pipeDesc.colorAttachmentFormat = Format::B8G8R8A8_Unorm;
    pipeDesc.pushConstantSize = sizeof(float) * 16; // 4x4 mat4 projection

    pipeDesc.vertexLayout.bindings.push_back({0, sizeof(Vertex2D)});
    pipeDesc.vertexLayout.attributes = {
        {0, 0, Format::R32G32_Sfloat, offsetof(Vertex2D, position)},
        {1, 0, Format::R32G32B32A32_Sfloat, offsetof(Vertex2D, color)},
        {2, 0, Format::R32G32_Sfloat, offsetof(Vertex2D, localPos)},
        {3, 0, Format::R32G32_Sfloat, offsetof(Vertex2D, boxSize)},
        {4, 0, Format::R32G32B32A32_Sfloat, offsetof(Vertex2D, cornerRadii)},
        {5, 0, Format::R32G32B32A32_Sfloat, offsetof(Vertex2D, borderColor)},
        {6, 0, Format::R32_Sfloat, offsetof(Vertex2D, borderWidth)},
    };

    auto pipeRes = device->createPipeline(pipeDesc);
    ASSERT_TRUE(pipeRes.isOk());
    PipelineHandle pipe = pipeRes.value();
    EXPECT_TRUE(pipe.isValid());
    device->destroyPipeline(pipe);
}

TEST(RendererTest, BatchCoalescingAndSplitting) {
    using namespace northern;
    using namespace northern::graphics;
    using namespace northern::renderer;

    auto devRes = Device::create();
    ASSERT_TRUE(devRes.isOk());
    auto device = std::move(devRes.value());

    auto rendRes = BatchRenderer::create(*device, Format::B8G8R8A8_Unorm);
    ASSERT_TRUE(rendRes.isOk());
    auto renderer = std::move(rendRes.value());

    PaintContext ctx;
    ctx.drawRect({{10, 10}, {50, 50}}, core::Color::fromRgba8(255, 0, 0));
    ctx.drawRoundedRect({{60, 10}, {50, 50}}, core::Color::fromRgba8(0, 255, 0), 8.0f);
    ctx.drawBorder({{120, 10}, {50, 50}}, core::Color::fromRgba8(0, 0, 255), 2.0f);

    TextureDesc targetDesc{
        .width = 800,
        .height = 600,
        .format = Format::B8G8R8A8_Unorm,
        .usage = TextureUsage::ColorAttachment | TextureUsage::Sampled,
        .debugName = "TestRenderTarget",
    };
    auto targetRes = device->createTexture(targetDesc);
    ASSERT_TRUE(targetRes.isOk());
    TextureHandle targetTex = targetRes.value();

    RenderingDesc renderingDesc{};
    renderingDesc.renderArea = {{0, 0}, {800, 600}};
    renderingDesc.colorAttachment.target = targetTex;
    renderingDesc.colorAttachment.clear = true;
    renderingDesc.colorAttachment.clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

    renderer->begin({800, 600}, 0);
    auto cmdRes = device->createCommandBuffer();
    ASSERT_TRUE(cmdRes.isOk());
    auto cmd = std::move(cmdRes.value());
    cmd->begin();
    cmd->beginRendering(renderingDesc);
    renderer->render(ctx, *cmd);
    EXPECT_EQ(renderer->batchCount(), 1);
    cmd->endRendering();

    ctx.clear();
    ctx.drawRect({{0, 0}, {100, 100}}, core::Color::fromRgba8(255, 255, 255));
    ctx.pushClip({{10, 10}, {50, 50}});
    ctx.drawRect({{0, 0}, {100, 100}}, core::Color::fromRgba8(255, 0, 0));
    ctx.popClip();
    ctx.drawRect({{0, 0}, {100, 100}}, core::Color::fromRgba8(0, 255, 0));

    renderer->begin({800, 600}, 0);
    cmd->beginRendering(renderingDesc);
    renderer->render(ctx, *cmd);
    EXPECT_EQ(renderer->batchCount(), 3);
    cmd->endRendering();
    cmd->end();
    renderer->end();
    device->destroyTexture(targetTex);
}

TEST(RendererTest, LineRenderingCapsuleGeometryAndShortSegments) {
    using namespace northern;
    using namespace northern::graphics;
    using namespace northern::renderer;

    auto devRes = Device::create();
    ASSERT_TRUE(devRes.isOk());
    auto device = std::move(devRes.value());

    auto rendRes = BatchRenderer::create(*device, Format::B8G8R8A8_Unorm);
    ASSERT_TRUE(rendRes.isOk());
    auto renderer = std::move(rendRes.value());

    PaintContext ctx;
    // Standard line
    ctx.drawLine({10, 10}, {20, 10}, core::Color::fromRgba8(255, 255, 255), 2.0f);
    // Short line (shorter than thickness)
    ctx.drawLine({30, 10}, {30.5f, 10}, core::Color::fromRgba8(255, 255, 255), 2.0f);
    // Dot (zero-length line)
    ctx.drawLine({50, 10}, {50, 10}, core::Color::fromRgba8(255, 255, 255), 2.0f);

    TextureDesc targetDesc{
        .width = 100,
        .height = 100,
        .format = Format::B8G8R8A8_Unorm,
        .usage = TextureUsage::ColorAttachment,
        .debugName = "LineTestRenderTarget",
    };
    auto targetRes = device->createTexture(targetDesc);
    ASSERT_TRUE(targetRes.isOk());
    TextureHandle targetTex = targetRes.value();

    RenderingDesc renderingDesc{};
    renderingDesc.renderArea = {{0, 0}, {100, 100}};
    renderingDesc.colorAttachment.target = targetTex;
    renderingDesc.colorAttachment.clear = true;
    renderingDesc.colorAttachment.clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

    renderer->begin({100, 100}, 0);
    auto cmdRes = device->createCommandBuffer();
    ASSERT_TRUE(cmdRes.isOk());
    auto cmd = std::move(cmdRes.value());
    cmd->begin();
    cmd->beginRendering(renderingDesc);
    renderer->render(ctx, *cmd);
    EXPECT_EQ(renderer->batchCount(), 1);
    cmd->endRendering();
    cmd->end();
    renderer->end();
    device->destroyTexture(targetTex);
}

TEST(RendererTest, SvgAnalyticalAtlasCachingAndBatchCoalescing) {
    using namespace northern;
    using namespace northern::graphics;
    using namespace northern::renderer;

    auto devRes = Device::create();
    ASSERT_TRUE(devRes.isOk());
    auto device = std::move(devRes.value());

    auto rendRes = BatchRenderer::create(*device, Format::B8G8R8A8_Unorm);
    ASSERT_TRUE(rendRes.isOk());
    auto renderer = std::move(rendRes.value());

    auto svgRes = SvgDocument::fromString(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" "
        "stroke=\"currentColor\" stroke-width=\"2\"><circle cx=\"11\" cy=\"11\" r=\"8\"/>"
        "<line x1=\"21\" y1=\"21\" x2=\"16.65\" y2=\"16.65\"/></svg>");
    ASSERT_TRUE(svgRes.isOk());

    PaintContext ctx;
    ctx.drawSvg(*svgRes.value(), {{12.0f, 20.0f}, {20.0f, 20.0f}}, core::Color::white());
    ctx.drawSvg(*svgRes.value(), {{48.0f, 20.0f}, {20.0f, 20.0f}},
                core::Color::fromRgba8(37, 99, 235));

    TextureDesc targetDesc{
        .width = 128,
        .height = 64,
        .format = Format::B8G8R8A8_Unorm,
        .usage = TextureUsage::ColorAttachment,
        .debugName = "SvgAtlasTestTarget",
    };
    auto targetRes = device->createTexture(targetDesc);
    ASSERT_TRUE(targetRes.isOk());
    TextureHandle targetTex = targetRes.value();

    RenderingDesc renderingDesc{};
    renderingDesc.renderArea = {{0, 0}, {128, 64}};
    renderingDesc.colorAttachment.target = targetTex;
    renderingDesc.colorAttachment.clear = true;

    renderer->begin({128, 64}, 0);
    auto cmdRes = device->createCommandBuffer();
    ASSERT_TRUE(cmdRes.isOk());
    auto cmd = std::move(cmdRes.value());
    cmd->begin();
    cmd->beginRendering(renderingDesc);
    renderer->render(ctx, *cmd);
    EXPECT_EQ(renderer->batchCount(), 1);
    cmd->endRendering();
    cmd->end();
    renderer->end();
    device->destroyTexture(targetTex);
}

TEST(RendererTest, BatchRendererSeparateMaskAndDualSrcTextBatches) {
    using namespace northern;
    using namespace northern::graphics;
    using namespace northern::renderer;
    using namespace northern::text;

    auto dev = MockDevice::create();
    dev->setDualSourceBlendingSupported(true);

    auto rendRes = BatchRenderer::create(*dev, Format::B8G8R8A8_Unorm);
    ASSERT_TRUE(rendRes.isOk());
    auto renderer = std::move(rendRes.value());
    EXPECT_TRUE(renderer->isDualSourceTextEnabled());
    ASSERT_NE(renderer->glyphAtlas(), nullptr);
    ASSERT_NE(renderer->maskAtlas(), nullptr);
    EXPECT_EQ(renderer->glyphAtlas()->renderMode(), GlyphRenderMode::LcdRgb);
    EXPECT_EQ(renderer->maskAtlas()->renderMode(), GlyphRenderMode::Grayscale);

    auto fontRes = FontManager::instance().defaultFont(13.0f);
    ASSERT_TRUE(fontRes.isOk());
    auto svgRes = SvgDocument::fromString(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" "
        "stroke=\"currentColor\" stroke-width=\"2\"><circle cx=\"12\" cy=\"12\" r=\"9\"/></svg>");
    ASSERT_TRUE(svgRes.isOk());

    PaintContext ctx;
    ctx.drawText({10.0f, 20.0f}, "Hi", fontRes.value(), core::Color::white());
    ctx.drawSvg(*svgRes.value(), {{40.0f, 10.0f}, {16.0f, 16.0f}}, core::Color::white());

    auto cmdRes = dev->createCommandBuffer();
    ASSERT_TRUE(cmdRes.isOk());
    auto cmd = std::move(cmdRes.value());

    renderer->begin({200.0f, 100.0f}, 0);
    cmd->begin();
    renderer->render(ctx, *cmd);
    cmd->end();
    renderer->end();

    ASSERT_EQ(renderer->batchCount(), 2u);
    EXPECT_EQ(renderer->batches()[0].type, BatchType::Text);
    EXPECT_EQ(renderer->batches()[1].type, BatchType::Mask);
    EXPECT_NE(renderer->batches()[0].texture, renderer->batches()[1].texture);
}

TEST(RendererTest, BatchRendererImageUploadBatchingAndWeakPtrEviction) {
    using namespace northern;
    using namespace northern::graphics;
    using namespace northern::renderer;

    auto dev = MockDevice::create();
    auto rendRes = BatchRenderer::create(*dev, Format::B8G8R8A8_Unorm);
    ASSERT_TRUE(rendRes.isOk());
    auto renderer = std::move(rendRes.value());

    std::vector<uint8_t> px1(16 * 16 * 4, 255); // 1024 bytes
    std::vector<uint8_t> px2(16 * 16 * 4, 128); // 1024 bytes
    auto img1 = Image::fromPixels(16, 16, px1).value();
    auto img2 = Image::fromPixels(16, 16, px2).value();

    // Limit cache budget to 1500 bytes so LRU evicts img1 when img2 is uploaded on next frame
    renderer->setImageCacheBudgetBytes(1500);

    auto cmdRes = dev->createCommandBuffer();
    ASSERT_TRUE(cmdRes.isOk());
    auto cmd = std::move(cmdRes.value());

    PaintContext ctx1;
    ctx1.drawImage(*img1, {{0.0f, 0.0f}, {32.0f, 32.0f}}, 6.0f);
    ctx1.drawImage(*img1, {{40.0f, 0.0f}, {32.0f, 32.0f}}, 6.0f);

    renderer->begin({200.0f, 100.0f}, 0);
    cmd->begin();
    renderer->render(ctx1, *cmd);
    cmd->end();
    renderer->end();

    ASSERT_EQ(renderer->batchCount(), 1u);
    EXPECT_EQ(renderer->batches()[0].type, BatchType::Image);
    EXPECT_EQ(renderer->cachedImageCount(), 1u);
    EXPECT_EQ(renderer->cachedImageBytes(), 1024u);

    // Frame 2: draw img2, pushing cache over 1500 bytes, then frame 3 prunes LRU img1
    PaintContext ctx2;
    ctx2.drawImage(*img2, {{0.0f, 0.0f}, {32.0f, 32.0f}});
    renderer->begin({200.0f, 100.0f}, 1);
    cmd->begin();
    renderer->render(ctx2, *cmd);
    cmd->end();
    renderer->end();
    EXPECT_EQ(renderer->cachedImageCount(), 2u);

    renderer->begin({200.0f, 100.0f}, 2);
    EXPECT_EQ(renderer->cachedImageCount(), 1u);
    EXPECT_EQ(renderer->cachedImageBytes(), 1024u);
    renderer->end();

    // Releasing img2 (and clearing ctx2) causes automatic weak_ptr eviction on next begin()
    ctx2.clear();
    img2.reset();
    renderer->begin({200.0f, 100.0f}, 0);
    EXPECT_EQ(renderer->cachedImageCount(), 0u);
    EXPECT_EQ(renderer->cachedImageBytes(), 0u);
    renderer->end();
}
