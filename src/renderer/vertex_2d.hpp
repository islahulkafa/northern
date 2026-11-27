#pragma once

#include "core/math/color.hpp"
#include "core/math/geometry.hpp"

namespace northern::renderer {

struct alignas(16) Vertex2D {
    core::Point position;    // Screen position (x, y)
    core::Color color;       // Fill / tint color (r, g, b, a)
    core::Point localPos;    // Local box coordinate [-halfW, halfW], [-halfH, halfH]
    core::Size boxSize;      // Full element width and height
    float cornerRadii[4];    // Corner radii: [TL, TR, BR, BL]
    core::Color borderColor; // Border color (r, g, b, a)
    float borderWidth{0.0f}; // Border thickness in pixels
    core::Point uv{0.0f, 0.0f};
    float padding[3]{0.0f, 0.0f, 0.0f};
};

} // namespace northern::renderer
