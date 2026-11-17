#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "core/math/geometry.hpp"
#include "core/memory/native_handle.hpp"
#include "graphics/gpu_handle.hpp"
#include "graphics/types.hpp"
#include <cstdint>

namespace northern::graphics {

struct SwapchainDesc {
    core::NativeWindowHandle windowHandle{};
    core::SizeI size{800, 600};
    Format format{Format::B8G8R8A8_Unorm};
    bool vsync{true};
};

class Swapchain {
public:
    virtual ~Swapchain() = default;

    [[nodiscard]] virtual core::Result<uint32_t, core::Error>
    acquireNextImage(uint64_t timeoutNs = UINT64_MAX) = 0;

    [[nodiscard]] virtual core::Result<void, core::Error> present(uint32_t imageIndex) = 0;

    [[nodiscard]] virtual core::Result<void, core::Error> resize(core::SizeI newSize) = 0;

    [[nodiscard]] virtual TextureHandle currentTexture() const noexcept = 0;
    [[nodiscard]] virtual Format format() const noexcept = 0;
    [[nodiscard]] virtual core::SizeI size() const noexcept = 0;
    [[nodiscard]] virtual uint32_t currentFrameIndex() const noexcept = 0;
    [[nodiscard]] virtual uint32_t maxFramesInFlight() const noexcept = 0;
};

} // namespace northern::graphics
