#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <numbers>

namespace northern::core {

inline constexpr float kPi = std::numbers::pi_v<float>;
inline constexpr float kHalfPi = kPi * 0.5f;
inline constexpr float kTwoPi = kPi * 2.0f;
inline constexpr float kDegToRad = kPi / 180.0f;
inline constexpr float kRadToDeg = 180.0f / kPi;

template <typename T>
[[nodiscard]] constexpr T clamp(T val, T minVal, T maxVal) noexcept {
    return (val < minVal) ? minVal : ((val > maxVal) ? maxVal : val);
}

template <typename T>
[[nodiscard]] constexpr T lerp(T a, T b, float t) noexcept {
    return static_cast<T>(a + (b - a) * t);
}

[[nodiscard]] inline float smoothstep(float edge0, float edge1, float x) noexcept {
    if (std::abs(edge1 - edge0) < 1e-6f) {
        return (x >= edge1) ? 1.0f : 0.0f;
    }
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

[[nodiscard]] constexpr float radians(float deg) noexcept {
    return deg * kDegToRad;
}

[[nodiscard]] constexpr float degrees(float rad) noexcept {
    return rad * kRadToDeg;
}

[[nodiscard]] inline bool approxEqual(float a, float b, float epsilon = 1e-5f) noexcept {
    return std::abs(a - b) <= epsilon;
}

template <typename T>
[[nodiscard]] constexpr int sign(T val) noexcept {
    return (T(0) < val) - (val < T(0));
}

} // namespace northern::core
