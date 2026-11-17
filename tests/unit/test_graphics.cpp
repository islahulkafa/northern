#include "graphics/buffer.hpp"
#include "graphics/device.hpp"
#include "graphics/gpu_handle.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/texture.hpp"
#include "graphics/types.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::graphics;

TEST(GraphicsTest, BufferUsageBitwiseOperations) {
    BufferUsage usage = BufferUsage::Vertex | BufferUsage::TransferDst;
    EXPECT_TRUE(hasFlag(usage, BufferUsage::Vertex));
    EXPECT_TRUE(hasFlag(usage, BufferUsage::TransferDst));
    EXPECT_FALSE(hasFlag(usage, BufferUsage::Index));
    EXPECT_FALSE(hasFlag(usage, BufferUsage::Uniform));

    BufferUsage combined = usage | BufferUsage::Index;
    EXPECT_TRUE(hasFlag(combined, BufferUsage::Index));

    BufferUsage masked = combined & BufferUsage::Vertex;
    EXPECT_EQ(masked, BufferUsage::Vertex);
}

TEST(GraphicsTest, TextureUsageBitwiseOperations) {
    TextureUsage usage = TextureUsage::Sampled | TextureUsage::ColorAttachment;
    EXPECT_TRUE(hasFlag(usage, TextureUsage::Sampled));
    EXPECT_TRUE(hasFlag(usage, TextureUsage::ColorAttachment));
    EXPECT_FALSE(hasFlag(usage, TextureUsage::DepthStencilAttachment));
}

TEST(GraphicsTest, ShaderStageBitwiseOperations) {
    ShaderStage stages = ShaderStage::Vertex | ShaderStage::Fragment;
    EXPECT_TRUE(hasFlag(stages, ShaderStage::Vertex));
    EXPECT_TRUE(hasFlag(stages, ShaderStage::Fragment));
    EXPECT_FALSE(hasFlag(stages, ShaderStage::Compute));
}

TEST(GraphicsTest, ClearColorConversion) {
    core::Color col{0.2f, 0.4f, 0.6f, 0.8f};
    ClearColor clear(col);
    EXPECT_FLOAT_EQ(clear.r, 0.2f);
    EXPECT_FLOAT_EQ(clear.g, 0.4f);
    EXPECT_FLOAT_EQ(clear.b, 0.6f);
    EXPECT_FLOAT_EQ(clear.a, 0.8f);
}

TEST(GraphicsTest, GpuHandleLifecycle) {
    BufferHandle invalidHandle{};
    EXPECT_FALSE(invalidHandle.isValid());
    EXPECT_FALSE(static_cast<bool>(invalidHandle));

    BufferHandle h1(1, 1);
    EXPECT_TRUE(h1.isValid());
    EXPECT_EQ(h1.index, 1U);
    EXPECT_EQ(h1.generation, 1U);

    BufferHandle h2(1, 1);
    EXPECT_EQ(h1, h2);

    BufferHandle h3(1, 2);
    EXPECT_NE(h1, h3);
}

TEST(GraphicsTest, VulkanDeviceAndBufferLifecycle) {
    DeviceDesc desc;
    desc.enableValidation = false;
    desc.appName = "Northern Unit Tests";

    auto devRes = Device::create(desc);
    if (devRes.isErr()) {
        GTEST_SKIP() << "Vulkan device unavailable in current environment: "
                     << devRes.error().message;
    }

    auto device = std::move(devRes.value());
    ASSERT_NE(device, nullptr);

    // Test buffer creation with initial data
    const float vertices[] = {0.0f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f};
    BufferDesc bufDesc;
    bufDesc.size = sizeof(vertices);
    bufDesc.usage = BufferUsage::Vertex | BufferUsage::TransferDst;
    bufDesc.memoryUsage = MemoryUsage::CpuToGpu;
    bufDesc.debugName = "TestVertexBuffer";

    auto bufRes = device->createBuffer(bufDesc, vertices);
    ASSERT_TRUE(bufRes.isOk()) << bufRes.error().message;

    BufferHandle handle = bufRes.value();
    EXPECT_TRUE(handle.isValid());

    // Test buffer write
    const float updatedVertices[] = {1.0f, 1.0f, 2.0f, 2.0f, 3.0f, 3.0f};
    auto writeRes = device->writeBuffer(handle, 0, sizeof(updatedVertices), updatedVertices);
    EXPECT_TRUE(writeRes.isOk());

    // Test command buffer allocation
    auto cmdRes = device->createCommandBuffer();
    EXPECT_TRUE(cmdRes.isOk());

    // Test deferred buffer destruction
    device->destroyBuffer(handle);
    device->advanceFrame();
    device->waitIdle();
}
