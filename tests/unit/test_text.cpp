#include "graphics/device.hpp"
#include "renderer/batch_renderer.hpp"
#include "renderer/paint_context.hpp"
#include "renderer/vertex_2d.hpp"
#include "shaders/text_shaders.hpp"
#include "text/font.hpp"
#include "text/font_manager.hpp"
#include "text/glyph_atlas.hpp"
#include "text/text_layout.hpp"
#include "text/text_shaper.hpp"
#include <gtest/gtest.h>

TEST(TextTest, FontLoadingAndMetrics) {
    using namespace northern::text;
    auto fontRes = Font::fromFile("C:/Windows/Fonts/segoeui.ttf", 16.0f);
    ASSERT_TRUE(fontRes.isOk());
    auto font = fontRes.value();
    EXPECT_GT(font->id(), 0u);
    EXPECT_GT(font->ascender(), 0.0f);
    EXPECT_LT(font->descender(), 0.0f);
    EXPECT_GT(font->lineHeight(), 0.0f);
}

TEST(TextTest, UnicodeTextShaping) {
    using namespace northern::text;
    auto fontRes = Font::fromFile("C:/Windows/Fonts/segoeui.ttf", 16.0f);
    ASSERT_TRUE(fontRes.isOk());
    auto font = fontRes.value();

    auto glyphs = TextShaper::shape(*font, "Hello World");
    EXPECT_EQ(glyphs.size(), 11);
    EXPECT_GT(glyphs[0].advance.x, 0.0f);
}

TEST(TextTest, FontManagerDefaultFont) {
    using namespace northern::text;
    auto& mgr = FontManager::instance();
    auto fontRes = mgr.defaultFont(14.0f);
    ASSERT_TRUE(fontRes.isOk());
    auto font = fontRes.value();
    EXPECT_EQ(font->pointSize(), 14.0f);
}

TEST(TextTest, TextLayoutMeasurementAndWrapping) {
    using namespace northern;
    using namespace northern::text;

    auto fontRes = FontManager::instance().defaultFont(14.0f);
    ASSERT_TRUE(fontRes.isOk());
    auto font = fontRes.value();

    auto single = TextLayout::create("Hello World", font);
    EXPECT_EQ(single.lines().size(), 1);
    EXPECT_GT(single.size().width, 0.0f);
    EXPECT_GT(single.size().height, 0.0f);

    auto wrapped = TextLayout::create("The quick brown fox jumps over the lazy dog", font, 100.0f);
    EXPECT_GT(wrapped.lines().size(), 1);
    EXPECT_LE(wrapped.size().width, 105.0f);
}

TEST(TextTest, GlyphAtlasRasterizationAndPacking) {
    using namespace northern;
    using namespace northern::graphics;
    using namespace northern::text;

    auto devRes = Device::create();
    ASSERT_TRUE(devRes.isOk());
    auto device = std::move(devRes.value());

    auto fontRes = FontManager::instance().defaultFont(16.0f);
    ASSERT_TRUE(fontRes.isOk());
    auto font = fontRes.value();

    auto atlasRes = GlyphAtlas::create(*device, 512, 512);
    ASSERT_TRUE(atlasRes.isOk());
    auto atlas = std::move(atlasRes.value());

    auto shaped = TextShaper::shape(*font, "A");
    ASSERT_FALSE(shaped.empty());
    auto gRes = atlas->getOrInsert(*font, shaped[0].glyphId);
    ASSERT_TRUE(gRes.isOk());
    EXPECT_GT(gRes.value().size.width, 0.0f);
    EXPECT_GT(gRes.value().size.height, 0.0f);
    EXPECT_GT(gRes.value().uvRect.size.width, 0.0f);

    EXPECT_TRUE(atlas->isDirty());
    auto flushRes = atlas->flush();
    EXPECT_TRUE(flushRes.isOk());
    EXPECT_FALSE(atlas->isDirty());
}

TEST(TextTest, SubpixelGlyphBinning) {
    using namespace northern;
    using namespace northern::graphics;
    using namespace northern::text;

    auto devRes = Device::create();
    ASSERT_TRUE(devRes.isOk());
    auto device = std::move(devRes.value());

    auto fontRes = FontManager::instance().defaultFont(16.0f);
    ASSERT_TRUE(fontRes.isOk());
    auto font = fontRes.value();

    auto atlasRes = GlyphAtlas::create(*device, 512, 512);
    ASSERT_TRUE(atlasRes.isOk());
    auto atlas = std::move(atlasRes.value());

    auto shaped = TextShaper::shape(*font, "e");
    ASSERT_FALSE(shaped.empty());
    uint32_t glyphId = shaped[0].glyphId;

    auto bin0 = atlas->getOrInsert(*font, glyphId, 0);
    auto bin1 = atlas->getOrInsert(*font, glyphId, 1);
    auto bin2 = atlas->getOrInsert(*font, glyphId, 2);
    auto bin3 = atlas->getOrInsert(*font, glyphId, 3);

    ASSERT_TRUE(bin0.isOk());
    ASSERT_TRUE(bin1.isOk());
    ASSERT_TRUE(bin2.isOk());
    ASSERT_TRUE(bin3.isOk());

    EXPECT_EQ(bin0.value().subpixelBin, 0);
    EXPECT_EQ(bin1.value().subpixelBin, 1);
    EXPECT_EQ(bin2.value().subpixelBin, 2);
    EXPECT_EQ(bin3.value().subpixelBin, 3);

    // Bins must have distinct texture locations
    EXPECT_NE(bin0.value().uvRect.origin.x, bin1.value().uvRect.origin.x);
    EXPECT_NE(bin1.value().uvRect.origin.x, bin2.value().uvRect.origin.x);
    EXPECT_NE(bin2.value().uvRect.origin.x, bin3.value().uvRect.origin.x);

    EXPECT_TRUE(atlas->isDirty());
    auto flushRes = atlas->flush();
    EXPECT_TRUE(flushRes.isOk());
    EXPECT_FALSE(atlas->isDirty());
}

TEST(TextTest, TextPipelineAndDescriptorBinding) {
    using namespace northern;
    using namespace northern::graphics;
    using namespace northern::renderer;

    auto devRes = Device::create();
    ASSERT_TRUE(devRes.isOk());
    auto device = std::move(devRes.value());

    PipelineDesc pipeDesc{};
    pipeDesc.debugName = "Text Pipeline Test";
    pipeDesc.vertexShader.spirv = shaders::kTextVertSpv;
    pipeDesc.fragmentShader.spirv = shaders::kTextFragSpv;
    pipeDesc.hasTexture = true;
    pipeDesc.pushConstantSize = sizeof(float) * 16;
    pipeDesc.colorAttachmentFormat = Format::B8G8R8A8_Unorm;
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
    EXPECT_TRUE(pipeRes.value().isValid());

    TextureDesc texDesc{
        .width = 128,
        .height = 128,
        .format = Format::R8_Unorm,
        .usage = TextureUsage::Sampled | TextureUsage::TransferDst,
    };
    auto texRes = device->createTexture(texDesc);
    ASSERT_TRUE(texRes.isOk());

    auto cmdRes = device->createCommandBuffer();
    ASSERT_TRUE(cmdRes.isOk());
    auto cmd = std::move(cmdRes.value());

    cmd->begin();
    cmd->bindPipeline(pipeRes.value());
    cmd->bindTexture(0, 0, texRes.value());
    cmd->end();

    device->destroyTexture(texRes.value());
    device->destroyPipeline(pipeRes.value());
}

TEST(TextTest, PaintContextDrawTextAndBatching) {
    using namespace northern;
    using namespace northern::graphics;
    using namespace northern::renderer;
    using namespace northern::text;

    auto devRes = Device::create();
    ASSERT_TRUE(devRes.isOk());
    auto device = std::move(devRes.value());

    auto batchRes = BatchRenderer::create(*device, Format::B8G8R8A8_Unorm);
    ASSERT_TRUE(batchRes.isOk());
    auto batchRenderer = std::move(batchRes.value());

    auto fontRes = FontManager::instance().defaultFont(16.0f);
    ASSERT_TRUE(fontRes.isOk());
    auto font = fontRes.value();

    PaintContext ctx;
    ctx.drawRoundedRect({{10.0f, 10.0f}, {200.0f, 100.0f}}, core::Color{0.2f, 0.2f, 0.2f, 1.0f},
                        8.0f);
    ctx.drawText({20.0f, 40.0f}, "Hello Northern", font, core::Color::white());

    auto cmdRes = device->createCommandBuffer();
    ASSERT_TRUE(cmdRes.isOk());
    auto cmd = std::move(cmdRes.value());

    batchRenderer->begin({800.0f, 600.0f}, 0);
    cmd->begin();
    batchRenderer->render(ctx, *cmd);
    cmd->end();
    batchRenderer->end();

    EXPECT_EQ(batchRenderer->batchCount(), 2);
    EXPECT_EQ(batchRenderer->batches()[0].type, BatchType::SdfShape);
    EXPECT_EQ(batchRenderer->batches()[1].type, BatchType::Text);
}

TEST(TextTest, GlyphPixelSnappingAndQuadPadding) {
    using namespace northern;
    using namespace northern::graphics;
    using namespace northern::renderer;
    using namespace northern::text;

    auto devRes = Device::create();
    ASSERT_TRUE(devRes.isOk());
    auto device = std::move(devRes.value());

    auto batchRes = BatchRenderer::create(*device, Format::B8G8R8A8_Unorm);
    ASSERT_TRUE(batchRes.isOk());
    auto batchRenderer = std::move(batchRes.value());

    auto fontRes = FontManager::instance().defaultFont(16.0f);
    ASSERT_TRUE(fontRes.isOk());
    auto font = fontRes.value();

    PaintContext ctx;
    ctx.drawRoundedRect({{10.0f, 10.0f}, {100.0f, 50.0f}}, core::Color{0.2f, 0.2f, 0.2f, 1.0f},
                        8.0f);
    // Draw text with fractional origin to verify sub-pixel snapping
    ctx.drawText({20.35f, 40.65f}, "T", font, core::Color::white());

    auto cmdRes = device->createCommandBuffer();
    ASSERT_TRUE(cmdRes.isOk());
    auto cmd = std::move(cmdRes.value());

    batchRenderer->begin({800.0f, 600.0f}, 0);
    cmd->begin();
    batchRenderer->render(ctx, *cmd);
    cmd->end();
    batchRenderer->end();

    EXPECT_EQ(batchRenderer->batchCount(), 2);
}
