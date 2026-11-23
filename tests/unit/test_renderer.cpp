#include "../../shaders/sdf_shaders.hpp"
#include "graphics/device.hpp"
#include "renderer/batch_renderer.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/dynamic_ring_buffer.hpp"
#include "renderer/paint_context.hpp"
#include "renderer/vertex_2d.hpp"
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
