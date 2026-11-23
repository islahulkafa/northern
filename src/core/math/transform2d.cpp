#include "core/math/transform2d.hpp"
#include <algorithm>

namespace northern::core {

std::optional<Transform2D> Transform2D::inverted(float epsilon) const noexcept {
    float det = determinant();
    if (std::abs(det) < epsilon) {
        return std::nullopt;
    }

    float invDet = 1.0f / det;
    return Transform2D{
        d * invDet,
        -b * invDet,
        -c * invDet,
        a * invDet,
        (c * ty - d * tx) * invDet,
        (b * tx - a * ty) * invDet
    };
}

Rect Transform2D::transformRect(const Rect& rect) const noexcept {
    if (rect.isEmpty()) {
        return rect;
    }

    Point p0 = transformPoint(rect.origin);
    Point p1 = transformPoint(Point{rect.right(), rect.top()});
    Point p2 = transformPoint(Point{rect.right(), rect.bottom()});
    Point p3 = transformPoint(Point{rect.left(), rect.bottom()});

    float minX = std::min({p0.x, p1.x, p2.x, p3.x});
    float maxX = std::max({p0.x, p1.x, p2.x, p3.x});
    float minY = std::min({p0.y, p1.y, p2.y, p3.y});
    float maxY = std::max({p0.y, p1.y, p2.y, p3.y});

    return Rect{Point{minX, minY}, Size{maxX - minX, maxY - minY}};
}

} // namespace northern::core
