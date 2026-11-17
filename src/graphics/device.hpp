#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "graphics/buffer.hpp"
#include "graphics/command_buffer.hpp"
#include "graphics/gpu_handle.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/texture.hpp"
#include <memory>
#include <string_view>

namespace northern::graphics {

struct DeviceDesc {
#ifdef NDEBUG
    bool enableValidation{false};
#else
    bool enableValidation{true};
#endif
    std::string_view appName{"Northern Application"};
};

class Device {
public:
    virtual ~Device() = default;

    [[nodiscard]] static core::Result<std::unique_ptr<Device>, core::Error>
    create(const DeviceDesc& desc = {});

    [[nodiscard]] virtual core::Result<std::unique_ptr<Swapchain>, core::Error>
    createSwapchain(const SwapchainDesc& desc) = 0;

    [[nodiscard]] virtual core::Result<std::unique_ptr<CommandBuffer>, core::Error>
    createCommandBuffer() = 0;

    [[nodiscard]] virtual core::Result<BufferHandle, core::Error>
    createBuffer(const BufferDesc& desc, const void* initialData = nullptr) = 0;

    [[nodiscard]] virtual core::Result<void, core::Error>
    writeBuffer(BufferHandle handle, uint64_t offset, uint64_t size, const void* data) = 0;

    [[nodiscard]] virtual void* mapBuffer(BufferHandle handle) = 0;

    [[nodiscard]] virtual core::Result<TextureHandle, core::Error>
    createTexture(const TextureDesc& desc, const void* initialData = nullptr) = 0;

    [[nodiscard]] virtual core::Result<void, core::Error>
    writeTexture(TextureHandle handle, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                 const void* data) = 0;

    [[nodiscard]] virtual core::Result<PipelineHandle, core::Error>
    createPipeline(const PipelineDesc& desc) = 0;

    virtual void destroyBuffer(BufferHandle handle) = 0;
    virtual void destroyTexture(TextureHandle handle) = 0;
    virtual void destroyPipeline(PipelineHandle handle) = 0;

    [[nodiscard]] virtual core::Result<void, core::Error>
    submit(CommandBuffer* cmdBuffer, Swapchain* swapchain = nullptr) = 0;

    virtual void advanceFrame() = 0;
    virtual void waitIdle() = 0;
};

} // namespace northern::graphics
