#include "widgets/controls/slider.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

Slider::Slider(float min, float max, float value)
    : minValue_(min), maxValue_(max), value_(std::clamp(value, min, max)) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::PointingHand);
    style().height = layout::Length::points(24.0f);

    layoutNode_.setMeasureFunc([this](const layout::LayoutNode&, float, float) -> core::Size {
        return core::Size{120.0f, std::max(24.0f, thumbRadius_ * 2.0f)};
    });
}

void Slider::setValue(float value) {
    float clamped = std::clamp(value, minValue_, maxValue_);
    if (step_ > 0.0f) {
        clamped = minValue_ + std::round((clamped - minValue_) / step_) * step_;
        clamped = std::clamp(clamped, minValue_, maxValue_);
    }
    if (value_ != clamped) {
        value_ = clamped;
        markPaintDirty();
        onValueChanged.emit(value_);
    }
}

void Slider::setRange(float min, float max) {
    minValue_ = min;
    maxValue_ = std::max(min, max);
    setValue(value_);
}

void Slider::setStep(float step) noexcept {
    step_ = std::max(0.0f, step);
    setValue(value_);
}

void Slider::setTrackHeight(float height) noexcept {
    trackHeight_ = height;
    markPaintDirty();
}

void Slider::setThumbRadius(float radius) noexcept {
    thumbRadius_ = radius;
    markLayoutDirty();
}

void Slider::setInactiveTrackColor(const core::Color& color) noexcept {
    inactiveTrackColor_ = color;
    markPaintDirty();
}

void Slider::setActiveTrackColor(const core::Color& color) noexcept {
    activeTrackColor_ = color;
    markPaintDirty();
}

void Slider::setThumbColor(const core::Color& color) noexcept {
    thumbColor_ = color;
    markPaintDirty();
}

void Slider::setThumbBorderColor(const core::Color& color) noexcept {
    thumbBorderColor_ = color;
    markPaintDirty();
}

void Slider::updateValueFromPosition(float localX) {
    core::Rect rect = layoutRect();
    float trackX = thumbRadius_;
    float trackW = std::max(1.0f, rect.size.width - 2.0f * thumbRadius_);
    float t = std::clamp((localX - trackX) / trackW, 0.0f, 1.0f);
    setValue(minValue_ + t * (maxValue_ - minValue_));
}

void Slider::updateHoverAnimation() {
    float targetScale = (isHovered_ || isDragging_) ? 1.25f : 1.0f;
    thumbScaleAnim_.start(thumbScaleAnim_.currentValue(), targetScale, 0.12f,
                          core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
    markPaintDirty();
}

void Slider::updateAnimations(float deltaSeconds) {
    thumbScaleAnim_.update(deltaSeconds);
    markPaintDirty();
}

bool Slider::hasActiveAnimations() const noexcept {
    return thumbScaleAnim_.isRunning();
}

void Slider::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        isDragging_ = true;
        setFocus();
        updateHoverAnimation();
        updateValueFromPosition(ev.position().x);
        ev.accept();
    }
}

void Slider::onMouseMove(WidgetMouseEvent& ev) {
    if (isDragging_) {
        updateValueFromPosition(ev.position().x);
        ev.accept();
    }
}

void Slider::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left && isDragging_) {
        isDragging_ = false;
        updateHoverAnimation();
        ev.accept();
        markPaintDirty();
    }
}

void Slider::onMouseEnter() {
    isHovered_ = true;
    updateHoverAnimation();
}

void Slider::onMouseLeave() {
    isHovered_ = false;
    updateHoverAnimation();
}

void Slider::onKeyDown(WidgetKeyEvent& ev) {
    float delta = (step_ > 0.0f) ? step_ : (maxValue_ - minValue_) * 0.05f;
    if (ev.key() == platform::Key::Left || ev.key() == platform::Key::Down) {
        setValue(value_ - delta);
        ev.accept();
    } else if (ev.key() == platform::Key::Right || ev.key() == platform::Key::Up) {
        setValue(value_ + delta);
        ev.accept();
    } else if (ev.key() == platform::Key::Home) {
        setValue(minValue_);
        ev.accept();
    } else if (ev.key() == platform::Key::End) {
        setValue(maxValue_);
        ev.accept();
    }
}

void Slider::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    float trackX = rect.origin.x + thumbRadius_;
    float trackW = std::max(1.0f, rect.size.width - 2.0f * thumbRadius_);
    float trackY = rect.origin.y + (rect.size.height - trackHeight_) * 0.5f;

    core::Rect trackRect{core::Point{trackX, trackY}, core::Size{trackW, trackHeight_}};
    ctx.drawRoundedRect(trackRect, inactiveTrackColor_, trackHeight_ * 0.5f);

    float range = maxValue_ - minValue_;
    float t = (range > 0.0f) ? std::clamp((value_ - minValue_) / range, 0.0f, 1.0f) : 0.0f;
    float activeW = trackW * t;

    if (activeW > 0.0f) {
        core::Rect activeRect{core::Point{trackX, trackY}, core::Size{activeW, trackHeight_}};
        ctx.drawRoundedRect(activeRect, activeTrackColor_, trackHeight_ * 0.5f);
    }

    float currentRadius = thumbRadius_ * thumbScaleAnim_.currentValue();
    float thumbCenterX = trackX + activeW;
    float thumbCenterY = trackY + trackHeight_ * 0.5f;
    core::Rect thumbRect{
        core::Point{thumbCenterX - currentRadius, thumbCenterY - currentRadius},
        core::Size{currentRadius * 2.0f, currentRadius * 2.0f},
    };

    float radii[4] = {currentRadius, currentRadius, currentRadius, currentRadius};
    core::Color currentThumbBorder =
        (isHovered_ || isDragging_) ? theme::ColorTokens::AccentDefault : thumbBorderColor_;
    ctx.drawBox(thumbRect, thumbColor_, currentThumbBorder, 1.0f, radii);

    if (hasKeyboardFocus()) {
        theme::paintFocusRing(ctx, thumbRect, currentRadius);
    }
}

} // namespace northern::widgets
