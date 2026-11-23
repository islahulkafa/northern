#pragma once

#include "core/math/geometry.hpp"
#include <cmath>
#include <optional>

namespace northern::core {

struct Transform2D {
    float a{1.0f};  // m11 (scale x)
    float b{0.0f};  // m12 (shear y)
    float c{0.0f};  // m21 (shear x)
    float d{1.0f};  // m22 (scale y)
    float tx{0.0f}; // dx (translate x)
    float ty{0.0f}; // dy (translate y)

    constexpr Transform2D() noexcept = default;
    constexpr Transform2D(float a_, float b_, float c_, float d_, float tx_, float ty_) noexcept
        : a(a_), b(b_), c(c_), d(d_), tx(tx_), ty(ty_) {}

    [[nodiscard]] static constexpr Transform2D identity() noexcept {
        return Transform2D{1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    }

    [[nodiscard]] static constexpr Transform2D translation(float x, float y) noexcept {
        return Transform2D{1.0f, 0.0f, 0.0f, 1.0f, x, y};
    }

    [[nodiscard]] static constexpr Transform2D translation(const Point& p) noexcept {
        return translation(p.x, p.y);
    }

    [[nodiscard]] static constexpr Transform2D scale(float sx, float sy) noexcept {
        return Transform2D{sx, 0.0f, 0.0f, sy, 0.0f, 0.0f};
    }

    [[nodiscard]] static constexpr Transform2D uniformScale(float s) noexcept {
        return scale(s, s);
    }

    [[nodiscard]] static Transform2D rotation(float angleRadians) noexcept {
        float cosA = std::cos(angleRadians);
        float sinA = std::sin(angleRadians);
        return Transform2D{cosA, sinA, -sinA, cosA, 0.0f, 0.0f};
    }

    [[nodiscard]] static Transform2D rotationAround(float angleRadians, const Point& center) noexcept {
        return translation(center.x, center.y) * rotation(angleRadians) * translation(-center.x, -center.y);
    }

    [[nodiscard]] constexpr float determinant() const noexcept {
        return a * d - b * c;
    }

    [[nodiscard]] constexpr bool isInvertible(float epsilon = 1e-6f) const noexcept {
        float det = determinant();
        return (det > epsilon) || (det < -epsilon);
    }

    [[nodiscard]] std::optional<Transform2D> inverted(float epsilon = 1e-6f) const noexcept;

    [[nodiscard]] constexpr Point transformPoint(const Point& p) const noexcept {
        return Point{
            a * p.x + c * p.y + tx,
            b * p.x + d * p.y + ty
        };
    }

    [[nodiscard]] Rect transformRect(const Rect& rect) const noexcept;

    [[nodiscard]] constexpr Transform2D operator*(const Transform2D& rhs) const noexcept {
        return Transform2D{
            a * rhs.a + c * rhs.b,
            b * rhs.a + d * rhs.b,
            a * rhs.c + c * rhs.d,
            b * rhs.c + d * rhs.d,
            a * rhs.tx + c * rhs.ty + tx,
            b * rhs.tx + d * rhs.ty + ty
        };
    }

    constexpr Transform2D& operator*=(const Transform2D& rhs) noexcept {
        *this = *this * rhs;
        return *this;
    }

    constexpr bool operator==(const Transform2D& other) const noexcept = default;
};

} // namespace northern::core
