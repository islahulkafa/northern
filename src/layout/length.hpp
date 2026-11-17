#pragma once

#include <cstdint>

namespace northern::layout {

enum class LengthType : uint8_t {
    Auto,
    Points,
    Percent,
};

struct Length {
    LengthType type{LengthType::Auto};
    float value{0.0f};

    static constexpr Length autoLength() noexcept { return Length{LengthType::Auto, 0.0f}; }

    static constexpr Length points(float pts) noexcept { return Length{LengthType::Points, pts}; }

    static constexpr Length percent(float pct) noexcept { return Length{LengthType::Percent, pct}; }

    [[nodiscard]] constexpr bool isAuto() const noexcept { return type == LengthType::Auto; }

    [[nodiscard]] constexpr bool isPoints() const noexcept { return type == LengthType::Points; }

    [[nodiscard]] constexpr bool isPercent() const noexcept { return type == LengthType::Percent; }

    [[nodiscard]] constexpr float resolve(float parentDimension,
                                          float autoValue = 0.0f) const noexcept {
        switch (type) {
        case LengthType::Points:
            return value;
        case LengthType::Percent:
            return parentDimension * (value / 100.0f);
        case LengthType::Auto:
        default:
            return autoValue;
        }
    }

    constexpr bool operator==(const Length& other) const noexcept {
        return type == other.type && value == other.value;
    }

    constexpr bool operator!=(const Length& other) const noexcept { return !(*this == other); }
};

} // namespace northern::layout
