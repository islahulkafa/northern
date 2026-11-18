#pragma once

#include <algorithm>
#include <cmath>

namespace northern::core::animation {

struct SpringConfig {
    float stiffness{220.0f}; // Tension / spring constant k
    float damping{22.0f};    // Friction / damping coefficient c
    float mass{1.0f};        // Mass m

    static constexpr SpringConfig Default() noexcept { return SpringConfig{220.0f, 22.0f, 1.0f}; }
    static constexpr SpringConfig Gentle() noexcept { return SpringConfig{120.0f, 18.0f, 1.0f}; }
    static constexpr SpringConfig Snappy() noexcept { return SpringConfig{320.0f, 26.0f, 1.0f}; }
    static constexpr SpringConfig Bouncy() noexcept { return SpringConfig{200.0f, 12.0f, 1.0f}; }
};

class Spring {
public:
    explicit Spring(float initialPosition = 0.0f,
                    SpringConfig config = SpringConfig::Default()) noexcept
        : position_(initialPosition), target_(initialPosition), config_(config) {}

    void setConfig(SpringConfig config) noexcept { config_ = config; }
    [[nodiscard]] const SpringConfig& config() const noexcept { return config_; }

    void setTarget(float target) noexcept { target_ = target; }
    [[nodiscard]] float target() const noexcept { return target_; }

    void setPosition(float pos) noexcept {
        position_ = pos;
        velocity_ = 0.0f;
    }
    [[nodiscard]] float position() const noexcept { return position_; }
    [[nodiscard]] float velocity() const noexcept { return velocity_; }

    void snapToTarget() noexcept {
        position_ = target_;
        velocity_ = 0.0f;
    }

    [[nodiscard]] bool isAtRest(float posTolerance = 0.001f,
                                float velTolerance = 0.001f) const noexcept {
        return std::abs(position_ - target_) <= posTolerance && std::abs(velocity_) <= velTolerance;
    }

    void update(float dt) noexcept {
        if (dt <= 0.0f) {
            return;
        }
        if (isAtRest()) {
            position_ = target_;
            velocity_ = 0.0f;
            return;
        }

        float m = std::max(0.0001f, config_.mass);
        float k = std::max(0.0f, config_.stiffness);
        float c = std::max(0.0f, config_.damping);

        float omega0 = std::sqrt(k / m);
        float zeta = c / (2.0f * std::sqrt(k * m));

        float p0 = position_ - target_;
        float v0 = velocity_;

        if (zeta < 0.999f) {
            // Under-damped (oscillatory decay)
            float omegaD = omega0 * std::sqrt(1.0f - zeta * zeta);
            float gamma = zeta * omega0;
            float c1 = p0;
            float c2 = (v0 + gamma * p0) / omegaD;

            float expTerm = std::exp(-gamma * dt);
            float cosTerm = std::cos(omegaD * dt);
            float sinTerm = std::sin(omegaD * dt);

            float p = expTerm * (c1 * cosTerm + c2 * sinTerm);
            float v = -gamma * p + expTerm * (-c1 * omegaD * sinTerm + c2 * omegaD * cosTerm);

            position_ = target_ + p;
            velocity_ = v;
        } else if (zeta <= 1.001f) {
            // Critically damped
            float c1 = p0;
            float c2 = v0 + omega0 * p0;
            float expTerm = std::exp(-omega0 * dt);

            float p = expTerm * (c1 + c2 * dt);
            float v = expTerm * (c2 - omega0 * (c1 + c2 * dt));

            position_ = target_ + p;
            velocity_ = v;
        } else {
            // Over-damped
            float omegaD = omega0 * std::sqrt(zeta * zeta - 1.0f);
            float gamma = zeta * omega0;
            float r1 = -gamma + omegaD;
            float r2 = -gamma - omegaD;

            float c2 = (v0 - r1 * p0) / (r2 - r1);
            float c1 = p0 - c2;

            float exp1 = std::exp(r1 * dt);
            float exp2 = std::exp(r2 * dt);

            float p = c1 * exp1 + c2 * exp2;
            float v = c1 * r1 * exp1 + c2 * r2 * exp2;

            position_ = target_ + p;
            velocity_ = v;
        }

        if (isAtRest()) {
            position_ = target_;
            velocity_ = 0.0f;
        }
    }

private:
    float position_{0.0f};
    float velocity_{0.0f};
    float target_{0.0f};
    SpringConfig config_{};
};

} // namespace northern::core::animation
