#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "core/math/geometry.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace northern::text {

enum class StrokeCap : uint8_t {
    Butt = 0,
    Round = 1,
    Square = 2,
};

enum class StrokeJoin : uint8_t {
    Miter = 0,
    Round = 1,
    Bevel = 2,
};

enum class FillRule : uint8_t {
    NonZero = 0,
    EvenOdd = 1,
};

struct CubicSubpath {
    // Control points in pixel coordinates: [p0, c1, c2, p1, c1, c2, p2, ...] (size == 1 + 3*N)
    std::vector<core::Point> points;
    bool closed{false};
};

struct VectorShapeDesc {
    std::vector<CubicSubpath> subpaths;
    bool hasFill{false};
    FillRule fillRule{FillRule::NonZero};
    float fillOpacity{1.0f};
    bool hasStroke{false};
    float strokeWidth{1.0f};
    StrokeCap strokeCap{StrokeCap::Butt};
    StrokeJoin strokeJoin{StrokeJoin::Miter};
    float miterLimit{4.0f};
    float strokeOpacity{1.0f};
    float opacity{1.0f};
};

core::Result<void, core::Error> rasterizeVectorShapes(std::span<const VectorShapeDesc> shapes,
                                                      uint32_t width, uint32_t height,
                                                      std::span<uint8_t> outAlphaMask);

} // namespace northern::text
