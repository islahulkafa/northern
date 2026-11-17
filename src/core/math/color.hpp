#pragma once

#include "core/error/result.hpp"
#include <cstdint>
#include <string_view>

namespace northern::core {

struct Color {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{1.0f};

    constexpr Color() noexcept : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {}
    constexpr Color(float red, float green, float blue, float alpha = 1.0f) noexcept
        : r(red), g(green), b(blue), a(alpha) {}

    constexpr bool operator==(const Color& other) const noexcept = default;

    [[nodiscard]] static constexpr Color fromRgba8(uint8_t red, uint8_t green, uint8_t blue,
                                                   uint8_t alpha = 255) noexcept {
        return Color{static_cast<float>(red) / 255.0f, static_cast<float>(green) / 255.0f,
                     static_cast<float>(blue) / 255.0f, static_cast<float>(alpha) / 255.0f};
    }

    [[nodiscard]] static Result<Color, Error> fromHex(std::string_view hex);

    [[nodiscard]] uint32_t toRgba8() const noexcept;
    [[nodiscard]] uint32_t toArgb8() const noexcept;

    [[nodiscard]] Color withAlpha(float newAlpha) const noexcept;
    [[nodiscard]] Color lerp(const Color& other, float t) const noexcept;

    static constexpr Color transparent() noexcept { return Color{0.0f, 0.0f, 0.0f, 0.0f}; }
    static constexpr Color black() noexcept { return Color{0.0f, 0.0f, 0.0f, 1.0f}; }
    static constexpr Color white() noexcept { return Color{1.0f, 1.0f, 1.0f, 1.0f}; }
    static constexpr Color red() noexcept { return Color{1.0f, 0.0f, 0.0f, 1.0f}; }
    static constexpr Color green() noexcept { return Color{0.0f, 1.0f, 0.0f, 1.0f}; }
    static constexpr Color blue() noexcept { return Color{0.0f, 0.0f, 1.0f, 1.0f}; }
    static constexpr Color yellow() noexcept { return Color{1.0f, 1.0f, 0.0f, 1.0f}; }
    static constexpr Color cyan() noexcept { return Color{0.0f, 1.0f, 1.0f, 1.0f}; }
    static constexpr Color magenta() noexcept { return Color{1.0f, 0.0f, 1.0f, 1.0f}; }
    static constexpr Color gray() noexcept { return Color{0.5f, 0.5f, 0.5f, 1.0f}; }
};

} // namespace northern::core
