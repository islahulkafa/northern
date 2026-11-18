#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

namespace northern::core::animation {

enum class EasingCurve {
    Linear,
    InQuad,
    OutQuad,
    InOutQuad,
    InCubic,
    OutCubic,
    InOutCubic,
    OutExpo,
    InOutExpo,
    OutElastic,
};

inline constexpr float linear(float t) noexcept {
    return t;
}

inline constexpr float easeInQuad(float t) noexcept {
    return t * t;
}

inline constexpr float easeOutQuad(float t) noexcept {
    return t * (2.0f - t);
}

inline constexpr float easeInOutQuad(float t) noexcept {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

inline constexpr float easeInCubic(float t) noexcept {
    return t * t * t;
}

inline constexpr float easeOutCubic(float t) noexcept {
    float f = t - 1.0f;
    return f * f * f + 1.0f;
}

inline constexpr float easeInOutCubic(float t) noexcept {
    return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}

inline float easeOutExpo(float t) noexcept {
    return (t >= 1.0f) ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}

inline float easeInOutExpo(float t) noexcept {
    if (t <= 0.0f) {
        return 0.0f;
    }
    if (t >= 1.0f) {
        return 1.0f;
    }
    if (t < 0.5f) {
        return 0.5f * std::pow(2.0f, (20.0f * t) - 10.0f);
    }
    return 1.0f - 0.5f * std::pow(2.0f, (-20.0f * t) + 10.0f);
}

inline float easeOutElastic(float t) noexcept {
    if (t <= 0.0f) {
        return 0.0f;
    }
    if (t >= 1.0f) {
        return 1.0f;
    }
    constexpr float c4 = (2.0f * static_cast<float>(std::numbers::pi)) / 3.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

inline float cubicBezier(float p1x, float p1y, float p2x, float p2y, float t) noexcept {
    t = std::clamp(t, 0.0f, 1.0f);
    if (t <= 0.0f) {
        return 0.0f;
    }
    if (t >= 1.0f) {
        return 1.0f;
    }

    // Solve for s where Bx(s) = t using Newton-Raphson
    float s = t;
    for (int i = 0; i < 8; ++i) {
        float oneMinusS = 1.0f - s;
        float x =
            3.0f * oneMinusS * oneMinusS * s * p1x + 3.0f * oneMinusS * s * s * p2x + s * s * s;
        float dx = 3.0f * oneMinusS * oneMinusS * p1x + 6.0f * oneMinusS * s * (p2x - p1x) +
                   3.0f * s * s * (1.0f - p2x);
        if (std::abs(dx) < 1e-6f) {
            break;
        }
        float err = x - t;
        s -= err / dx;
        s = std::clamp(s, 0.0f, 1.0f);
        if (std::abs(err) < 1e-5f) {
            break;
        }
    }

    float oneMinusS = 1.0f - s;
    return 3.0f * oneMinusS * oneMinusS * s * p1y + 3.0f * oneMinusS * s * s * p2y + s * s * s;
}

inline float evaluateEasing(EasingCurve curve, float t) noexcept {
    t = std::clamp(t, 0.0f, 1.0f);
    switch (curve) {
    case EasingCurve::Linear:
        return linear(t);
    case EasingCurve::InQuad:
        return easeInQuad(t);
    case EasingCurve::OutQuad:
        return easeOutQuad(t);
    case EasingCurve::InOutQuad:
        return easeInOutQuad(t);
    case EasingCurve::InCubic:
        return easeInCubic(t);
    case EasingCurve::OutCubic:
        return easeOutCubic(t);
    case EasingCurve::InOutCubic:
        return easeInOutCubic(t);
    case EasingCurve::OutExpo:
        return easeOutExpo(t);
    case EasingCurve::InOutExpo:
        return easeInOutExpo(t);
    case EasingCurve::OutElastic:
        return easeOutElastic(t);
    }
    return t;
}

} // namespace northern::core::animation
