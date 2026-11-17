#pragma once

#include "core/math/geometry.hpp"
#include <cstdint>

namespace northern::text {

struct GlyphMetrics {
    float width{0.0f};
    float height{0.0f};
    float bearingX{0.0f};
    float bearingY{0.0f};
    float advanceX{0.0f};
    float advanceY{0.0f};
};

struct ShapedGlyph {
    uint32_t glyphId{0};
    uint32_t cluster{0};
    core::Point position{0.0f, 0.0f};
    core::Point advance{0.0f, 0.0f};
    core::Rect uvRect{0.0f, 0.0f, 0.0f, 0.0f};
};

} // namespace northern::text
