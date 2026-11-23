#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget.hpp"

namespace northern::widgets {

class ProgressBar : public Widget {
public:
    explicit ProgressBar(float progress = 0.0f, bool isIndeterminate = false);
    ~ProgressBar() override = default;

    void setProgress(float progress);
    [[nodiscard]] float progress() const noexcept { return progress_; }

    void setIndeterminate(bool isIndeterminate);
    [[nodiscard]] bool isIndeterminate() const noexcept { return isIndeterminate_; }

    void setBarHeight(float height) noexcept;
    [[nodiscard]] float barHeight() const noexcept { return barHeight_; }

    void setTrackColor(const core::Color& color) noexcept;
    void setFillColor(const core::Color& color) noexcept;
    void setTrackBorderColor(const core::Color& color) noexcept;

    void advanceAnimation(float deltaSeconds) noexcept;
    [[nodiscard]] float indeterminateOffset() const noexcept { return indeterminateOffset_; }

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    float progress_{0.0f};
    bool isIndeterminate_{false};
    float barHeight_{4.0f};
    float indeterminateOffset_{0.0f};

    core::animation::PropertyAnimation<float> progressAnim_{0.0f};

    core::Color trackColor_{theme::ColorTokens::SurfaceRaised};
    core::Color fillColor_{theme::ColorTokens::AccentDefault};
    core::Color trackBorderColor_{theme::ColorTokens::BorderSubtle};
};

} // namespace northern::widgets
