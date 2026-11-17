#include "widgets/slider.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/theme.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

Slider::Slider(float min, float max, float value)
    : minValue_(min), maxValue_(max), value_(std::clamp(value, min, max)) {
    setFocusable(true);
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

void Slider::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        isDragging_ = true;
        setFocus();
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
        ev.accept();
        markPaintDirty();
    }
}

void Slider::onMouseEnter() {
    isHovered_ = true;
    markPaintDirty();
}

void Slider::onMouseLeave() {
    isHovered_ = false;
    markPaintDirty();
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

    float thumbCenterX = trackX + activeW;
    float thumbCenterY = trackY + trackHeight_ * 0.5f;
    core::Rect thumbRect{
        core::Point{thumbCenterX - thumbRadius_, thumbCenterY - thumbRadius_},
        core::Size{thumbRadius_ * 2.0f, thumbRadius_ * 2.0f},
    };

    float radii[4] = {thumbRadius_, thumbRadius_, thumbRadius_, thumbRadius_};
    core::Color currentThumbBorder =
        (isHovered_ || isDragging_) ? theme::ColorTokens::AccentDefault : thumbBorderColor_;
    ctx.drawBox(thumbRect, thumbColor_, currentThumbBorder, 1.0f, radii);

    if (hasFocus()) {
        theme::paintFocusRing(ctx, thumbRect, thumbRadius_);
    }
}

} // namespace northern::widgets
