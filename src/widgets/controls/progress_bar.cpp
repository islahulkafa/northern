#include "widgets/controls/progress_bar.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

ProgressBar::ProgressBar(float progress, bool isIndeterminate)
    : progress_(std::clamp(progress, 0.0f, 1.0f)), isIndeterminate_(isIndeterminate),
      progressAnim_(progress_) {
    style().width = layout::Length::percent(100.0f);
    style().height = layout::Length::points(barHeight_);

    layoutNode_.setMeasureFunc(
        [this](const layout::LayoutNode&, float availW, float) -> core::Size {
            float w = std::max(60.0f, availW);
            return core::Size{w, barHeight_};
        });
}

void ProgressBar::setProgress(float progress) {
    float clamped = std::clamp(progress, 0.0f, 1.0f);
    if (std::abs(progress_ - clamped) > 0.0001f) {
        progress_ = clamped;
        progressAnim_.start(progressAnim_.currentValue(), progress_, 0.20f,
                            core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
        markPaintDirty();
    }
}

void ProgressBar::setIndeterminate(bool isIndeterminate) {
    if (isIndeterminate_ != isIndeterminate) {
        isIndeterminate_ = isIndeterminate;
        indeterminateOffset_ = 0.0f;
        if (isIndeterminate_ && context_) {
            context_->registerAnimatedWidget(this);
        }
        markPaintDirty();
    }
}

void ProgressBar::updateAnimations(float deltaSeconds) {
    if (progressAnim_.isRunning()) {
        progressAnim_.update(deltaSeconds);
    }
    if (isIndeterminate_) {
        advanceAnimation(deltaSeconds);
    }
    markPaintDirty();
}

bool ProgressBar::hasActiveAnimations() const noexcept {
    return isIndeterminate_ || progressAnim_.isRunning();
}

void ProgressBar::setBarHeight(float height) noexcept {
    barHeight_ = std::max(2.0f, height);
    style().height = layout::Length::points(barHeight_);
    markLayoutDirty();
}

void ProgressBar::setTrackColor(const core::Color& color) noexcept {
    trackColor_ = color;
    markPaintDirty();
}

void ProgressBar::setFillColor(const core::Color& color) noexcept {
    fillColor_ = color;
    markPaintDirty();
}

void ProgressBar::setTrackBorderColor(const core::Color& color) noexcept {
    trackBorderColor_ = color;
    markPaintDirty();
}

void ProgressBar::advanceAnimation(float deltaSeconds) noexcept {
    if (isIndeterminate_) {
        constexpr float speed = 0.8f;
        indeterminateOffset_ += deltaSeconds * speed;
        if (indeterminateOffset_ > 1.0f) {
            indeterminateOffset_ -= std::floor(indeterminateOffset_);
        }
        markPaintDirty();
    }
}

void ProgressBar::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;
    float trackY = targetCenterY - barHeight_ * 0.5f;
    core::Rect trackRect{core::Point{rect.origin.x, trackY},
                         core::Size{rect.size.width, barHeight_}};

    float radius = barHeight_ * 0.5f;
    float radii[4] = {radius, radius, radius, radius};

    ctx.drawBox(trackRect, trackColor_, trackBorderColor_, 1.0f, radii);

    if (isIndeterminate_) {
        float pulseW = trackRect.size.width * 0.30f;
        float totalTravel = trackRect.size.width + pulseW;
        float pulseX = trackRect.origin.x - pulseW + totalTravel * indeterminateOffset_;

        core::Rect pulseRect{core::Point{pulseX, trackY}, core::Size{pulseW, barHeight_}};
        ctx.pushClip(trackRect);
        ctx.drawRoundedRect(pulseRect, fillColor_, radius);
        ctx.popClip();
    } else {
        float cur = progressAnim_.currentValue();
        if (cur > 0.001f) {
            float fillW = std::clamp(trackRect.size.width * cur, barHeight_, trackRect.size.width);
            core::Rect fillRect{core::Point{trackRect.origin.x, trackY},
                                core::Size{fillW, barHeight_}};
            ctx.drawRoundedRect(fillRect, fillColor_, radius);
        }
    }
}

} // namespace northern::widgets
