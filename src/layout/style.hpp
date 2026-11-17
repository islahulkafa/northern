#pragma once

#include "layout/length.hpp"
#include "layout/types.hpp"

namespace northern::layout {

struct RectLength {
    Length left{Length::autoLength()};
    Length top{Length::autoLength()};
    Length right{Length::autoLength()};
    Length bottom{Length::autoLength()};

    static constexpr RectLength all(Length len) noexcept { return RectLength{len, len, len, len}; }

    static constexpr RectLength symmetric(Length vertical, Length horizontal) noexcept {
        return RectLength{horizontal, vertical, horizontal, vertical};
    }

    static constexpr RectLength zero() noexcept { return all(Length::points(0.0f)); }
};

struct Style {
    Display display{Display::Flex};
    FlexDirection flexDirection{FlexDirection::Row};
    FlexWrap flexWrap{FlexWrap::NoWrap};
    JustifyContent justifyContent{JustifyContent::FlexStart};
    AlignItems alignItems{AlignItems::Stretch};
    AlignSelf alignSelf{AlignSelf::Auto};
    AlignContent alignContent{AlignContent::FlexStart};
    PositionType positionType{PositionType::Relative};

    Length width{Length::autoLength()};
    Length height{Length::autoLength()};
    Length minWidth{Length::autoLength()};
    Length minHeight{Length::autoLength()};
    Length maxWidth{Length::autoLength()};
    Length maxHeight{Length::autoLength()};

    float flexGrow{0.0f};
    float flexShrink{1.0f};
    Length flexBasis{Length::autoLength()};

    RectLength margin{RectLength::zero()};
    RectLength padding{RectLength::zero()};
    RectLength position{RectLength::all(Length::autoLength())};

    float rowGap{0.0f};
    float columnGap{0.0f};
};

} // namespace northern::layout
