#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace northern::core {

template <typename T>
struct Point2D {
    T x{0};
    T y{0};

    constexpr Point2D() noexcept = default;
    constexpr Point2D(T xVal, T yVal) noexcept : x(xVal), y(yVal) {}

    constexpr Point2D operator+(const Point2D& other) const noexcept {
        return {x + other.x, y + other.y};
    }
    constexpr Point2D operator-(const Point2D& other) const noexcept {
        return {x - other.x, y - other.y};
    }
    constexpr Point2D operator*(T scalar) const noexcept { return {x * scalar, y * scalar}; }
    constexpr Point2D operator/(T scalar) const noexcept { return {x / scalar, y / scalar}; }

    constexpr Point2D& operator+=(const Point2D& other) noexcept {
        x += other.x;
        y += other.y;
        return *this;
    }
    constexpr Point2D& operator-=(const Point2D& other) noexcept {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    constexpr Point2D& operator*=(T scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    constexpr Point2D& operator/=(T scalar) noexcept {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    constexpr bool operator==(const Point2D& other) const noexcept = default;

    [[nodiscard]] constexpr T dot(const Point2D& other) const noexcept {
        return x * other.x + y * other.y;
    }
    [[nodiscard]] float length() const noexcept {
        return std::sqrt(static_cast<float>(x * x + y * y));
    }
    [[nodiscard]] float distanceTo(const Point2D& other) const noexcept {
        return (*this - other).length();
    }
};

template <typename T>
struct Size2D {
    T width{0};
    T height{0};

    constexpr Size2D() noexcept = default;
    constexpr Size2D(T w, T h) noexcept : width(w), height(h) {}

    constexpr Size2D operator+(const Size2D& other) const noexcept {
        return {width + other.width, height + other.height};
    }
    constexpr Size2D operator-(const Size2D& other) const noexcept {
        return {width - other.width, height - other.height};
    }
    constexpr Size2D operator*(T scalar) const noexcept {
        return {width * scalar, height * scalar};
    }
    constexpr Size2D operator/(T scalar) const noexcept {
        return {width / scalar, height / scalar};
    }

    constexpr bool operator==(const Size2D& other) const noexcept = default;

    [[nodiscard]] constexpr bool isEmpty() const noexcept { return width <= 0 || height <= 0; }
    [[nodiscard]] constexpr T area() const noexcept { return width * height; }
    [[nodiscard]] constexpr float aspectRatio() const noexcept {
        return height != 0 ? static_cast<float>(width) / static_cast<float>(height) : 0.0f;
    }
};

template <typename T>
struct Insets2D {
    T top{0};
    T right{0};
    T bottom{0};
    T left{0};

    constexpr Insets2D() noexcept = default;
    constexpr Insets2D(T all) noexcept : top(all), right(all), bottom(all), left(all) {}
    constexpr Insets2D(T vertical, T horizontal) noexcept
        : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {}
    constexpr Insets2D(T t, T r, T b, T l) noexcept : top(t), right(r), bottom(b), left(l) {}

    constexpr bool operator==(const Insets2D& other) const noexcept = default;

    [[nodiscard]] constexpr T horizontal() const noexcept { return left + right; }
    [[nodiscard]] constexpr T vertical() const noexcept { return top + bottom; }
};

template <typename T>
struct Rect2D {
    Point2D<T> origin{};
    Size2D<T> size{};

    constexpr Rect2D() noexcept = default;
    constexpr Rect2D(Point2D<T> o, Size2D<T> s) noexcept : origin(o), size(s) {}
    constexpr Rect2D(T x, T y, T w, T h) noexcept : origin(x, y), size(w, h) {}

    constexpr bool operator==(const Rect2D& other) const noexcept = default;

    [[nodiscard]] constexpr T x() const noexcept { return origin.x; }
    [[nodiscard]] constexpr T y() const noexcept { return origin.y; }
    [[nodiscard]] constexpr T width() const noexcept { return size.width; }
    [[nodiscard]] constexpr T height() const noexcept { return size.height; }

    [[nodiscard]] constexpr T left() const noexcept { return origin.x; }
    [[nodiscard]] constexpr T top() const noexcept { return origin.y; }
    [[nodiscard]] constexpr T right() const noexcept { return origin.x + size.width; }
    [[nodiscard]] constexpr T bottom() const noexcept { return origin.y + size.height; }

    [[nodiscard]] constexpr Point2D<T> center() const noexcept {
        return {origin.x + size.width / 2, origin.y + size.height / 2};
    }

    [[nodiscard]] constexpr bool isEmpty() const noexcept { return size.isEmpty(); }

    [[nodiscard]] constexpr bool contains(const Point2D<T>& p) const noexcept {
        return p.x >= left() && p.x < right() && p.y >= top() && p.y < bottom();
    }

    [[nodiscard]] constexpr bool contains(const Rect2D<T>& r) const noexcept {
        return r.left() >= left() && r.right() <= right() && r.top() >= top() &&
               r.bottom() <= bottom();
    }

    [[nodiscard]] constexpr bool intersects(const Rect2D<T>& r) const noexcept {
        return left() < r.right() && right() > r.left() && top() < r.bottom() && bottom() > r.top();
    }

    [[nodiscard]] constexpr Rect2D<T> intersect(const Rect2D<T>& r) const noexcept {
        T newLeft = std::max(left(), r.left());
        T newTop = std::max(top(), r.top());
        T newRight = std::min(right(), r.right());
        T newBottom = std::min(bottom(), r.bottom());

        if (newRight <= newLeft || newBottom <= newTop) {
            return Rect2D<T>{};
        }
        return Rect2D<T>{Point2D<T>{newLeft, newTop},
                         Size2D<T>{newRight - newLeft, newBottom - newTop}};
    }

    [[nodiscard]] constexpr Rect2D<T> combine(const Rect2D<T>& r) const noexcept {
        if (isEmpty())
            return r;
        if (r.isEmpty())
            return *this;

        T newLeft = std::min(left(), r.left());
        T newTop = std::min(top(), r.top());
        T newRight = std::max(right(), r.right());
        T newBottom = std::max(bottom(), r.bottom());

        return Rect2D<T>{Point2D<T>{newLeft, newTop},
                         Size2D<T>{newRight - newLeft, newBottom - newTop}};
    }

    [[nodiscard]] constexpr Rect2D<T> inset(const Insets2D<T>& insets) const noexcept {
        return Rect2D<T>{
            Point2D<T>{origin.x + insets.left, origin.y + insets.top},
            Size2D<T>{size.width - insets.horizontal(), size.height - insets.vertical()}};
    }

    [[nodiscard]] constexpr Rect2D<T> outset(const Insets2D<T>& insets) const noexcept {
        return Rect2D<T>{
            Point2D<T>{origin.x - insets.left, origin.y - insets.top},
            Size2D<T>{size.width + insets.horizontal(), size.height + insets.vertical()}};
    }
};

using Point = Point2D<float>;
using Size = Size2D<float>;
using Rect = Rect2D<float>;
using Insets = Insets2D<float>;

using PointI = Point2D<int32_t>;
using SizeI = Size2D<int32_t>;
using RectI = Rect2D<int32_t>;
using InsetsI = Insets2D<int32_t>;

} // namespace northern::core
