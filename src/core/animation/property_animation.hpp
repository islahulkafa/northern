#pragma once

#include "core/animation/easing.hpp"
#include "core/animation/spring.hpp"
#include "core/math/color.hpp"
#include "core/math/geometry.hpp"
#include "core/signal/signal.hpp"

#include <algorithm>
#include <type_traits>

namespace northern::core::animation {

namespace detail {

template <typename T>
inline T interpolate(const T& a, const T& b, float t) noexcept {
    if constexpr (std::is_same_v<T, Color>) {
        return a.lerp(b, t);
    } else {
        return a + (b - a) * t;
    }
}

} // namespace detail

template <typename T>
class PropertyAnimation {
public:
    enum class Mode {
        Idle,
        Easing,
        SpringDriven,
    };

    Signal<const T&> onUpdate;
    Signal<> onFinished;

    explicit PropertyAnimation(T initialValue = T{}) noexcept
        : startValue_(initialValue), endValue_(initialValue), currentValue_(initialValue) {}

    void start(T from, T to, float duration, EasingCurve curve = EasingCurve::OutCubic) {
        startValue_ = from;
        endValue_ = to;
        currentValue_ = from;
        duration_ = std::max(0.0001f, duration);
        elapsed_ = 0.0f;
        curve_ = curve;
        mode_ = Mode::Easing;
    }

    void startSpring(float from, float to, SpringConfig config = SpringConfig::Default())
        requires std::is_same_v<T, float>
    {
        startValue_ = from;
        endValue_ = to;
        currentValue_ = from;
        spring_ = Spring(from, config);
        spring_.setTarget(to);
        mode_ = Mode::SpringDriven;
    }

    void stop() noexcept { mode_ = Mode::Idle; }

    [[nodiscard]] Mode mode() const noexcept { return mode_; }
    [[nodiscard]] bool isRunning() const noexcept { return mode_ != Mode::Idle; }
    [[nodiscard]] bool isFinished() const noexcept { return mode_ == Mode::Idle; }

    [[nodiscard]] const T& currentValue() const noexcept { return currentValue_; }
    [[nodiscard]] const T& targetValue() const noexcept { return endValue_; }

    void setCurrentValue(const T& val) noexcept {
        currentValue_ = val;
        startValue_ = val;
        endValue_ = val;
        mode_ = Mode::Idle;
    }

    bool update(float dt) {
        if (mode_ == Mode::Idle) {
            return false;
        }

        if (mode_ == Mode::Easing) {
            elapsed_ += dt;
            float progress = std::clamp(elapsed_ / duration_, 0.0f, 1.0f);
            float eased = evaluateEasing(curve_, progress);
            currentValue_ = detail::interpolate(startValue_, endValue_, eased);
            onUpdate.emit(currentValue_);

            if (progress >= 1.0f) {
                currentValue_ = endValue_;
                mode_ = Mode::Idle;
                onFinished.emit();
                return false;
            }
            return true;
        }

        if constexpr (std::is_same_v<T, float>) {
            if (mode_ == Mode::SpringDriven) {
                spring_.update(dt);
                currentValue_ = spring_.position();
                onUpdate.emit(currentValue_);

                if (spring_.isAtRest()) {
                    currentValue_ = endValue_;
                    mode_ = Mode::Idle;
                    onFinished.emit();
                    return false;
                }
                return true;
            }
        }

        return false;
    }

private:
    T startValue_{};
    T endValue_{};
    T currentValue_{};
    float duration_{0.2f};
    float elapsed_{0.0f};
    EasingCurve curve_{EasingCurve::OutCubic};
    Mode mode_{Mode::Idle};
    Spring spring_{0.0f};
};

} // namespace northern::core::animation
