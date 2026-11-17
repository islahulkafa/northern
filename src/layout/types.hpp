#pragma once

#include <cstdint>

namespace northern::layout {

enum class FlexDirection : uint8_t {
    Row,
    RowReverse,
    Column,
    ColumnReverse,
};

enum class FlexWrap : uint8_t {
    NoWrap,
    Wrap,
    WrapReverse,
};

enum class JustifyContent : uint8_t {
    FlexStart,
    FlexEnd,
    Center,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
};

enum class AlignItems : uint8_t {
    FlexStart,
    FlexEnd,
    Center,
    Baseline,
    Stretch,
};

enum class AlignSelf : uint8_t {
    Auto,
    FlexStart,
    FlexEnd,
    Center,
    Baseline,
    Stretch,
};

[[nodiscard]] constexpr AlignSelf toAlignSelf(AlignItems ai) noexcept {
    switch (ai) {
    case AlignItems::FlexStart:
        return AlignSelf::FlexStart;
    case AlignItems::FlexEnd:
        return AlignSelf::FlexEnd;
    case AlignItems::Center:
        return AlignSelf::Center;
    case AlignItems::Baseline:
        return AlignSelf::Baseline;
    case AlignItems::Stretch:
        return AlignSelf::Stretch;
    }
    return AlignSelf::FlexStart;
}

enum class AlignContent : uint8_t {
    FlexStart,
    FlexEnd,
    Center,
    SpaceBetween,
    SpaceAround,
    Stretch,
};

enum class PositionType : uint8_t {
    Relative,
    Absolute,
};

enum class Display : uint8_t {
    Flex,
    None,
};

} // namespace northern::layout
