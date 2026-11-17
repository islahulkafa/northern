#pragma once

#include "core/math/color.hpp"
#include <cstdint>
#include <type_traits>

namespace northern::graphics {

enum class Format : uint32_t {
    Undefined = 0,
    R8_Unorm,
    R8G8B8A8_Unorm,
    B8G8R8A8_Unorm,
    R8G8B8A8_Srgb,
    B8G8R8A8_Srgb,
    R16G16_Sfloat,
    R32_Sfloat,
    R32G32_Sfloat,
    R32G32B32_Sfloat,
    R32G32B32A32_Sfloat,
    D24_Unorm_S8_Uint,
    D32_Sfloat,
};

enum class BufferUsage : uint32_t {
    None = 0,
    Vertex = 1 << 0,
    Index = 1 << 1,
    Uniform = 1 << 2,
    Storage = 1 << 3,
    TransferSrc = 1 << 4,
    TransferDst = 1 << 5,
};

[[nodiscard]] constexpr BufferUsage operator|(BufferUsage lhs, BufferUsage rhs) noexcept {
    return static_cast<BufferUsage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

[[nodiscard]] constexpr BufferUsage operator&(BufferUsage lhs, BufferUsage rhs) noexcept {
    return static_cast<BufferUsage>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

[[nodiscard]] constexpr bool hasFlag(BufferUsage flags, BufferUsage flag) noexcept {
    return (flags & flag) == flag;
}

enum class MemoryUsage : uint32_t {
    GpuOnly,
    CpuToGpu,
    GpuToCpu,
};

enum class TextureUsage : uint32_t {
    None = 0,
    Sampled = 1 << 0,
    ColorAttachment = 1 << 1,
    DepthStencilAttachment = 1 << 2,
    TransferSrc = 1 << 3,
    TransferDst = 1 << 4,
};

[[nodiscard]] constexpr TextureUsage operator|(TextureUsage lhs, TextureUsage rhs) noexcept {
    return static_cast<TextureUsage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

[[nodiscard]] constexpr TextureUsage operator&(TextureUsage lhs, TextureUsage rhs) noexcept {
    return static_cast<TextureUsage>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

[[nodiscard]] constexpr bool hasFlag(TextureUsage flags, TextureUsage flag) noexcept {
    return (flags & flag) == flag;
}

enum class Filter : uint32_t {
    Nearest,
    Linear,
};

enum class AddressMode : uint32_t {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
};

enum class PrimitiveTopology : uint32_t {
    TriangleList,
    TriangleStrip,
    LineList,
    LineStrip,
    PointList,
};

enum class IndexType : uint32_t {
    Uint16,
    Uint32,
};

enum class ShaderStage : uint32_t {
    None = 0,
    Vertex = 1 << 0,
    Fragment = 1 << 1,
    Compute = 1 << 2,
};

[[nodiscard]] constexpr ShaderStage operator|(ShaderStage lhs, ShaderStage rhs) noexcept {
    return static_cast<ShaderStage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

[[nodiscard]] constexpr ShaderStage operator&(ShaderStage lhs, ShaderStage rhs) noexcept {
    return static_cast<ShaderStage>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

[[nodiscard]] constexpr bool hasFlag(ShaderStage flags, ShaderStage flag) noexcept {
    return (flags & flag) == flag;
}

enum class BlendFactor : uint32_t {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
};

enum class BlendOp : uint32_t {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

enum class ColorWriteMask : uint32_t {
    None = 0,
    R = 1 << 0,
    G = 1 << 1,
    B = 1 << 2,
    A = 1 << 3,
    All = R | G | B | A,
};

[[nodiscard]] constexpr ColorWriteMask operator|(ColorWriteMask lhs, ColorWriteMask rhs) noexcept {
    return static_cast<ColorWriteMask>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

struct ClearColor {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{1.0f};

    constexpr ClearColor() noexcept = default;
    constexpr ClearColor(float red, float green, float blue, float alpha = 1.0f) noexcept
        : r(red), g(green), b(blue), a(alpha) {}

    constexpr ClearColor(const core::Color& color) noexcept
        : r(color.r), g(color.g), b(color.b), a(color.a) {}
};

} // namespace northern::graphics
