#include "graphics/mock/mock_device.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::graphics;

TEST(MockDeviceTest, BufferCreationAndMappedWrite) {
    auto device = MockDevice::create();
    ASSERT_NE(device, nullptr);

    const uint32_t initialData[4] = {10, 20, 30, 40};
    BufferDesc desc{
        .size = sizeof(initialData),
        .usage = BufferUsage::Vertex,
        .memoryUsage = MemoryUsage::CpuToGpu,
        .debugName = "MockVertexBuffer",
    };

    auto bufRes = device->createBuffer(desc, initialData);
    ASSERT_TRUE(bufRes.isOk());
    BufferHandle handle = bufRes.value();
    EXPECT_TRUE(handle.isValid());

    void* mapped = device->mapBuffer(handle);
    ASSERT_NE(mapped, nullptr);
    const auto* readBack = static_cast<const uint32_t*>(mapped);
    EXPECT_EQ(readBack[0], 10u);
    EXPECT_EQ(readBack[3], 40u);

    const uint32_t update = 999;
    auto writeRes = device->writeBuffer(handle, sizeof(uint32_t), sizeof(uint32_t), &update);
    EXPECT_TRUE(writeRes.isOk());
    EXPECT_EQ(readBack[1], 999u);

    device->destroyBuffer(handle);
}

TEST(MockDeviceTest, TextureCreationAndPixelWrite) {
    auto device = MockDevice::create();

    TextureDesc texDesc{
        .width = 4,
        .height = 4,
        .format = Format::R8G8B8A8_Unorm,
        .usage = TextureUsage::Sampled,
        .debugName = "MockTexture",
    };

    auto texRes = device->createTexture(texDesc);
    ASSERT_TRUE(texRes.isOk());
    TextureHandle handle = texRes.value();

    uint32_t pixel = 0xFF00FF00; // RGBA
    auto writeRes = device->writeTexture(handle, 1, 1, 1, 1, &pixel);
    EXPECT_TRUE(writeRes.isOk());

    auto* texData = device->getTexture(handle);
    ASSERT_NE(texData, nullptr);
    EXPECT_EQ(texData->storage.size(), 4 * 4 * 4);

    device->destroyTexture(handle);
}

TEST(MockDeviceTest, CommandBufferRecordingAndSubmit) {
    auto device = MockDevice::create();
    auto cmdRes = device->createCommandBuffer();
    ASSERT_TRUE(cmdRes.isOk());
    auto cmd = std::move(cmdRes.value());

    auto* mockCmd = dynamic_cast<MockCommandBuffer*>(cmd.get());
    ASSERT_NE(mockCmd, nullptr);

    cmd->begin();
    EXPECT_FALSE(mockCmd->isInsideRendering());

    RenderingDesc renderingDesc{};
    cmd->beginRendering(renderingDesc);
    EXPECT_TRUE(mockCmd->isInsideRendering());

    cmd->draw(3, 1, 0, 0);
    cmd->drawIndexed(6, 1, 0, 0, 0);
    EXPECT_EQ(mockCmd->drawCallCount(), 2u);
    EXPECT_EQ(mockCmd->totalVerticesDrawn(), 3u);
    EXPECT_EQ(mockCmd->totalIndicesDrawn(), 6u);

    cmd->endRendering();
    EXPECT_FALSE(mockCmd->isInsideRendering());
    cmd->end();

    auto submitRes = device->submit(cmd.get());
    EXPECT_TRUE(submitRes.isOk());
    EXPECT_EQ(device->submitCount(), 1u);

    device->advanceFrame();
    EXPECT_EQ(device->frameCount(), 1u);
}

TEST(MockDeviceTest, SwapchainFrameCycling) {
    auto device = MockDevice::create();
    SwapchainDesc swapDesc{
        .windowHandle = {},
        .size = {640, 480},
        .format = Format::B8G8R8A8_Unorm,
        .vsync = true,
    };

    auto swapRes = device->createSwapchain(swapDesc);
    ASSERT_TRUE(swapRes.isOk());
    auto swapchain = std::move(swapRes.value());

    EXPECT_EQ(swapchain->currentFrameIndex(), 0u);
    TextureHandle t0 = swapchain->currentTexture();
    EXPECT_TRUE(t0.isValid());

    auto acqRes = swapchain->acquireNextImage();
    ASSERT_TRUE(acqRes.isOk());

    auto presRes = swapchain->present(acqRes.value());
    EXPECT_TRUE(presRes.isOk());
    EXPECT_EQ(swapchain->currentFrameIndex(), 1u);

    TextureHandle t1 = swapchain->currentTexture();
    EXPECT_TRUE(t1.isValid());
    EXPECT_NE(t0, t1);

    auto resizeRes = swapchain->resize({800, 600});
    EXPECT_TRUE(resizeRes.isOk());
    EXPECT_EQ(swapchain->size().width, 800);
}
