#include "core/math/color.hpp"
#include <algorithm>

namespace northern::core {

namespace {

int parseHexDigit(char c) noexcept {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

} // namespace

Result<Color, Error> Color::fromHex(std::string_view hex) {
    if (hex.starts_with('#')) {
        hex.remove_prefix(1);
    }

    auto parseByte = [](char high, char low) -> int {
        int h = parseHexDigit(high);
        int l = parseHexDigit(low);
        if (h < 0 || l < 0)
            return -1;
        return (h << 4) | l;
    };

    if (hex.length() == 3) { // RGB
        int rVal = parseHexDigit(hex[0]);
        int gVal = parseHexDigit(hex[1]);
        int bVal = parseHexDigit(hex[2]);
        if (rVal < 0 || gVal < 0 || bVal < 0) {
            return NORTHERN_MAKE_ERROR(ErrorCode::ParseError,
                                       "Invalid hex character in 3-digit hex color");
        }
        return Color::fromRgba8(static_cast<uint8_t>((rVal << 4) | rVal),
                                static_cast<uint8_t>((gVal << 4) | gVal),
                                static_cast<uint8_t>((bVal << 4) | bVal));
    }

    if (hex.length() == 4) { // RGBA
        int rVal = parseHexDigit(hex[0]);
        int gVal = parseHexDigit(hex[1]);
        int bVal = parseHexDigit(hex[2]);
        int aVal = parseHexDigit(hex[3]);
        if (rVal < 0 || gVal < 0 || bVal < 0 || aVal < 0) {
            return NORTHERN_MAKE_ERROR(ErrorCode::ParseError,
                                       "Invalid hex character in 4-digit hex color");
        }
        return Color::fromRgba8(
            static_cast<uint8_t>((rVal << 4) | rVal), static_cast<uint8_t>((gVal << 4) | gVal),
            static_cast<uint8_t>((bVal << 4) | bVal), static_cast<uint8_t>((aVal << 4) | aVal));
    }

    if (hex.length() == 6) { // RRGGBB
        int rVal = parseByte(hex[0], hex[1]);
        int gVal = parseByte(hex[2], hex[3]);
        int bVal = parseByte(hex[4], hex[5]);
        if (rVal < 0 || gVal < 0 || bVal < 0) {
            return NORTHERN_MAKE_ERROR(ErrorCode::ParseError,
                                       "Invalid hex character in 6-digit hex color");
        }
        return Color::fromRgba8(static_cast<uint8_t>(rVal), static_cast<uint8_t>(gVal),
                                static_cast<uint8_t>(bVal));
    }

    if (hex.length() == 8) { // RRGGBBAA
        int rVal = parseByte(hex[0], hex[1]);
        int gVal = parseByte(hex[2], hex[3]);
        int bVal = parseByte(hex[4], hex[5]);
        int aVal = parseByte(hex[6], hex[7]);
        if (rVal < 0 || gVal < 0 || bVal < 0 || aVal < 0) {
            return NORTHERN_MAKE_ERROR(ErrorCode::ParseError,
                                       "Invalid hex character in 8-digit hex color");
        }
        return Color::fromRgba8(static_cast<uint8_t>(rVal), static_cast<uint8_t>(gVal),
                                static_cast<uint8_t>(bVal), static_cast<uint8_t>(aVal));
    }

    return NORTHERN_MAKE_ERROR(ErrorCode::InvalidArgument,
                               "Hex color string must be 3, 4, 6, or 8 characters");
}

uint32_t Color::toRgba8() const noexcept {
    const auto rByte = static_cast<uint32_t>(std::clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
    const auto gByte = static_cast<uint32_t>(std::clamp(g, 0.0f, 1.0f) * 255.0f + 0.5f);
    const auto bByte = static_cast<uint32_t>(std::clamp(b, 0.0f, 1.0f) * 255.0f + 0.5f);
    const auto aByte = static_cast<uint32_t>(std::clamp(a, 0.0f, 1.0f) * 255.0f + 0.5f);
    return (rByte << 24) | (gByte << 16) | (bByte << 8) | aByte;
}

uint32_t Color::toArgb8() const noexcept {
    const auto rByte = static_cast<uint32_t>(std::clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
    const auto gByte = static_cast<uint32_t>(std::clamp(g, 0.0f, 1.0f) * 255.0f + 0.5f);
    const auto bByte = static_cast<uint32_t>(std::clamp(b, 0.0f, 1.0f) * 255.0f + 0.5f);
    const auto aByte = static_cast<uint32_t>(std::clamp(a, 0.0f, 1.0f) * 255.0f + 0.5f);
    return (aByte << 24) | (rByte << 16) | (gByte << 8) | bByte;
}

Color Color::withAlpha(float newAlpha) const noexcept {
    return Color{r, g, b, std::clamp(newAlpha, 0.0f, 1.0f)};
}

Color Color::lerp(const Color& other, float t) const noexcept {
    t = std::clamp(t, 0.0f, 1.0f);
    return Color{r + (other.r - r) * t, g + (other.g - g) * t, b + (other.b - b) * t,
                 a + (other.a - a) * t};
}

} // namespace northern::core
