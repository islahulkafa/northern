#include "widgets/controls/slider.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace northern::widgets {

namespace {
constexpr float kReadoutGap = 10.0f;
} // namespace

Slider::Slider(float min, float max, float value)
    : minValue_(min), maxValue_(std::max(min, max)),
      value_(std::clamp(value, min, std::max(min, max))) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::PointingHand);
    style().height = layout::Length::points(24.0f);

    layoutNode_.setMeasureFunc([this](const layout::LayoutNode&, float, float) -> core::Size {
        float minW = 120.0f * dpiScale_;
        float minH = std::max(24.0f * dpiScale_, thumbRadius_ * 2.0f * 1.25f);
        if (showValue_) {
            updateReservedLabelWidth();
            minW += kReadoutGap * dpiScale_ + reservedLabelWidth_;
            minH = std::max(minH, reservedLabelHeight_);
        }
        return core::Size{minW, minH};
    });
}

Slider::Slider(SliderSize size, SliderRole role, float min, float max, float value)
    : Slider(min, max, value) {
    role_ = role;
    setSize(size);
}

std::shared_ptr<text::Font> Slider::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        resolvedDefaultFont_ = theme::resolveFont(theme::TypographyStyle::Caption, dpiScale_);
    }
    return resolvedDefaultFont_;
}

std::string Slider::formatValueString(float val) const {
    int prec = valuePrecision_;
    if (prec < 0) {
        if (step_ >= 1.0f || (step_ <= 0.0f && (maxValue_ - minValue_) >= 10.0f)) {
            prec = 0;
        } else {
            prec = 2;
        }
    }
    if (prec == 0) {
        return std::to_string(static_cast<long long>(std::llround(val)));
    }
    char buf[32]{};
    std::snprintf(buf, sizeof(buf), "%.*f", std::clamp(prec, 0, 6), val);
    return std::string(buf);
}

void Slider::updateReservedLabelWidth() const {
    auto f = effectiveFont();
    if (!f) {
        reservedLabelWidth_ = 0.0f;
        reservedLabelHeight_ = 0.0f;
        return;
    }
    if (reservedLabelWidth_ <= 0.0f) {
        auto minL = text::TextLayout::create(formatValueString(minValue_), f);
        auto maxL = text::TextLayout::create(formatValueString(maxValue_), f);
        reservedLabelWidth_ = std::ceil(std::max(minL.size().width, maxL.size().width));
        reservedLabelHeight_ = std::max(minL.size().height, maxL.size().height);
    }
}

void Slider::ensureValueLayout() const {
    if (!showValue_) {
        return;
    }
    std::string text = formatValueString(value_);
    if (cachedValueLayout_ && cachedValueText_ == text) {
        return;
    }
    auto f = effectiveFont();
    if (!f) {
        cachedValueLayout_.reset();
        return;
    }
    cachedValueText_ = std::move(text);
    cachedValueLayout_ = text::TextLayout::create(cachedValueText_, f);
}

float Slider::computeTrackWidth(float totalWidth) const noexcept {
    float effWidth = totalWidth;
    if (showValue_ && reservedLabelWidth_ > 0.0f &&
        totalWidth > reservedLabelWidth_ + kReadoutGap * dpiScale_ + 2.0f * thumbRadius_ + 8.0f) {
        effWidth -= (reservedLabelWidth_ + kReadoutGap * dpiScale_);
    }
    return std::max(1.0f, effWidth - 2.0f * thumbRadius_);
}

void Slider::setValue(float value) {
    float clamped = std::clamp(value, minValue_, maxValue_);
    if (step_ > 0.0f) {
        clamped = minValue_ + std::round((clamped - minValue_) / step_) * step_;
        clamped = std::clamp(clamped, minValue_, maxValue_);
    }
    if (std::abs(value_ - clamped) > 1e-5f) {
        value_ = clamped;
        markPaintDirty();
        onValueChanged.emit(value_);
    }
}

void Slider::setRange(float min, float max) {
    minValue_ = min;
    maxValue_ = std::max(min, max);
    reservedLabelWidth_ = 0.0f;
    cachedValueLayout_.reset();
    setValue(value_);
    if (showValue_) {
        updateReservedLabelWidth();
        markLayoutDirty();
    } else {
        markPaintDirty();
    }
}

void Slider::setStep(float step) noexcept {
    step_ = std::max(0.0f, step);
    reservedLabelWidth_ = 0.0f;
    cachedValueLayout_.reset();
    setValue(value_);
    markPaintDirty();
}

void Slider::setRole(SliderRole role) noexcept {
    if (role_ != role) {
        role_ = role;
        markPaintDirty();
    }
}

void Slider::setSize(SliderSize size) noexcept {
    float baseTrack = 4.0f;
    float baseThumb = 7.0f;
    float baseHeight = 24.0f;
    switch (size) {
    case SliderSize::Compact:
        baseTrack = 3.0f;
        baseThumb = 6.0f;
        baseHeight = 20.0f;
        break;
    case SliderSize::Thick:
        baseTrack = 6.0f;
        baseThumb = 9.0f;
        baseHeight = 28.0f;
        break;
    case SliderSize::Standard:
    default:
        break;
    }
    trackHeight_ = baseTrack * dpiScale_;
    thumbRadius_ = baseThumb * dpiScale_;
    style().height = layout::Length::points(baseHeight * dpiScale_);
    markLayoutDirty();
}

void Slider::setDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        float ratio = clamped / dpiScale_;
        dpiScale_ = clamped;
        trackHeight_ *= ratio;
        thumbRadius_ *= ratio;
        resolvedDefaultFont_.reset();
        if (!font_) {
            cachedValueLayout_.reset();
            reservedLabelWidth_ = 0.0f;
        }
        if (style().height.isPoints()) {
            style().height = layout::Length::points(style().height.value * ratio);
        }
        markLayoutDirty();
    }
}

void Slider::setTrackHeight(float height) noexcept {
    trackHeight_ = std::max(2.0f, height);
    markPaintDirty();
}

void Slider::setThumbRadius(float radius) noexcept {
    thumbRadius_ = std::max(3.0f, radius);
    markLayoutDirty();
}

void Slider::setShowValue(bool show) {
    if (showValue_ != show) {
        showValue_ = show;
        reservedLabelWidth_ = 0.0f;
        cachedValueLayout_.reset();
        if (showValue_) {
            updateReservedLabelWidth();
        }
        markLayoutDirty();
    }
}

void Slider::setValuePrecision(int decimalPlaces) noexcept {
    if (valuePrecision_ != decimalPlaces) {
        valuePrecision_ = decimalPlaces;
        reservedLabelWidth_ = 0.0f;
        cachedValueLayout_.reset();
        if (showValue_) {
            markLayoutDirty();
        } else {
            markPaintDirty();
        }
    }
}

void Slider::setShowTicks(bool show) noexcept {
    if (showTicks_ != show) {
        showTicks_ = show;
        markPaintDirty();
    }
}

void Slider::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        cachedValueLayout_.reset();
        reservedLabelWidth_ = 0.0f;
        if (showValue_) {
            updateReservedLabelWidth();
            markLayoutDirty();
        }
    }
}

void Slider::setInactiveTrackColor(const core::Color& color) noexcept {
    overrideInactiveTrackColor_ = color;
    markPaintDirty();
}

void Slider::setActiveTrackColor(const core::Color& color) noexcept {
    overrideActiveTrackColor_ = color;
    markPaintDirty();
}

void Slider::setThumbColor(const core::Color& color) noexcept {
    overrideThumbColor_ = color;
    markPaintDirty();
}

void Slider::setThumbBorderColor(const core::Color& color) noexcept {
    overrideThumbBorderColor_ = color;
    markPaintDirty();
}

void Slider::clearColorOverrides() noexcept {
    if (overrideInactiveTrackColor_ || overrideActiveTrackColor_ || overrideThumbColor_ ||
        overrideThumbBorderColor_) {
        overrideInactiveTrackColor_.reset();
        overrideActiveTrackColor_.reset();
        overrideThumbColor_.reset();
        overrideThumbBorderColor_.reset();
        markPaintDirty();
    }
}

core::Color Slider::effectiveInactiveTrackColor() const noexcept {
    if (overrideInactiveTrackColor_) {
        return *overrideInactiveTrackColor_;
    }
    const auto& pal = colorPalette();
    return (pal.mode == theme::ThemeMode::Dark) ? pal.surfaceRaised : pal.borderDefault;
}

core::Color Slider::effectiveActiveTrackColor() const noexcept {
    if (overrideActiveTrackColor_) {
        return isEnabled()
                   ? *overrideActiveTrackColor_
                   : overrideActiveTrackColor_->withAlpha(overrideActiveTrackColor_->a * 0.40f);
    }
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.textDisabled;
    }
    switch (role_) {
    case SliderRole::Accent:
        return pal.accentDefault;
    case SliderRole::Success:
        return pal.success;
    case SliderRole::Warning:
        return pal.warning;
    case SliderRole::Danger:
        return pal.danger;
    case SliderRole::Info:
        return pal.info;
    }
    return pal.accentDefault;
}

core::Color Slider::effectiveThumbColor() const noexcept {
    if (overrideThumbColor_) {
        return *overrideThumbColor_;
    }
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.surfaceBase;
    }
    return (pal.mode == theme::ThemeMode::Dark) ? pal.textPrimary : core::Color::white();
}

core::Color Slider::effectiveThumbBorderColor() const noexcept {
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.borderSubtle;
    }
    if (overrideThumbBorderColor_) {
        return *overrideThumbBorderColor_;
    }
    return (isHovered_ || isDragging_) ? effectiveActiveTrackColor() : pal.borderStrong;
}

void Slider::updateValueFromPosition(float localX) {
    core::Rect rect = layoutRect();
    float trackX = thumbRadius_;
    float trackW = computeTrackWidth(rect.size.width);
    float t = std::clamp((localX - trackX) / trackW, 0.0f, 1.0f);
    setValue(minValue_ + t * (maxValue_ - minValue_));
}

void Slider::updateHoverAnimation() {
    float targetScale = (isEnabled() && (isHovered_ || isDragging_)) ? 1.25f : 1.0f;
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
    if (!isEnabled()) {
        return;
    }
    if (ev.button() == platform::MouseButton::Left) {
        isDragging_ = true;
        setFocus(FocusReason::Pointer);
        updateHoverAnimation();
        updateValueFromPosition(ev.position().x);
        ev.accept();
    }
}

void Slider::onMouseMove(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (isDragging_) {
        updateValueFromPosition(ev.position().x);
        ev.accept();
    }
}

void Slider::onMouseUp(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        isDragging_ = false;
        return;
    }
    if (ev.button() == platform::MouseButton::Left && isDragging_) {
        isDragging_ = false;
        updateHoverAnimation();
        ev.accept();
        markPaintDirty();
        onValueCommitted.emit(value_);
    }
}

void Slider::onMouseEnter() {
    if (!isEnabled()) {
        return;
    }
    isHovered_ = true;
    updateHoverAnimation();
}

void Slider::onMouseLeave() {
    isHovered_ = false;
    updateHoverAnimation();
}

void Slider::onMouseWheel(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    float rawDelta = (std::abs(ev.wheelDelta().y) > 0.001f) ? ev.wheelDelta().y : ev.wheelDelta().x;
    if (std::abs(rawDelta) <= 0.001f) {
        return;
    }
    float stepDelta = (step_ > 0.0f) ? step_ : (maxValue_ - minValue_) * 0.05f;
    float dir = rawDelta > 0.0f ? 1.0f : -1.0f;
    float prev = value_;
    setValue(value_ + dir * stepDelta);
    ev.accept();
    if (std::abs(value_ - prev) > 1e-5f) {
        onValueCommitted.emit(value_);
    }
}

void Slider::onKeyDown(WidgetKeyEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    float stepDelta = (step_ > 0.0f) ? step_ : (maxValue_ - minValue_) * 0.05f;
    float pageDelta = (step_ > 0.0f) ? (step_ * 5.0f) : (maxValue_ - minValue_) * 0.10f;
    float prev = value_;
    bool handled = false;

    if (ev.key() == platform::Key::Left || ev.key() == platform::Key::Down) {
        setValue(value_ - stepDelta);
        handled = true;
    } else if (ev.key() == platform::Key::Right || ev.key() == platform::Key::Up) {
        setValue(value_ + stepDelta);
        handled = true;
    } else if (ev.key() == platform::Key::PageDown) {
        setValue(value_ - pageDelta);
        handled = true;
    } else if (ev.key() == platform::Key::PageUp) {
        setValue(value_ + pageDelta);
        handled = true;
    } else if (ev.key() == platform::Key::Home) {
        setValue(minValue_);
        handled = true;
    } else if (ev.key() == platform::Key::End) {
        setValue(maxValue_);
        handled = true;
    }

    if (handled) {
        ev.accept();
        if (std::abs(value_ - prev) > 1e-5f) {
            onValueCommitted.emit(value_);
        }
    }
}

void Slider::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    if (showValue_) {
        updateReservedLabelWidth();
    }

    float trackX = rect.origin.x + thumbRadius_;
    float trackW = computeTrackWidth(rect.size.width);
    float trackY = rect.origin.y + (rect.size.height - trackHeight_) * 0.5f;

    core::Rect trackRect{core::Point{trackX, trackY}, core::Size{trackW, trackHeight_}};
    float trackRadius = trackHeight_ * 0.5f;
    ctx.drawRoundedRect(trackRect, effectiveInactiveTrackColor(), trackRadius);

    float range = maxValue_ - minValue_;
    float t = (range > 0.0f) ? std::clamp((value_ - minValue_) / range, 0.0f, 1.0f) : 0.0f;
    float activeW = trackW * t;
    core::Color activeCol = effectiveActiveTrackColor();

    if (activeW >= 0.5f) {
        float fillRadius = std::min(trackRadius, activeW * 0.5f);
        core::Rect activeRect{core::Point{trackX, trackY}, core::Size{activeW, trackHeight_}};
        ctx.drawRoundedRect(activeRect, activeCol, fillRadius);
    }

    if (showTicks_ && step_ > 0.0f && range > 0.0f) {
        int tickCount = static_cast<int>(std::floor(range / step_ + 0.001f)) + 1;
        if (tickCount >= 2 && tickCount <= 25) {
            float tickDia = std::clamp(trackHeight_ * 0.75f, 2.5f, 4.5f * dpiScale_);
            float tickY = trackY + (trackHeight_ - tickDia) * 0.5f;
            const auto& pal = colorPalette();
            core::Color tickColor = pal.textTertiary.withAlpha(0.55f);
            for (int i = 0; i < tickCount; ++i) {
                float tickVal = std::min(maxValue_, minValue_ + static_cast<float>(i) * step_);
                float frac = (tickVal - minValue_) / range;
                float tx = trackX + trackW * frac - tickDia * 0.5f;
                ctx.drawRoundedRect(
                    core::Rect{core::Point{tx, tickY}, core::Size{tickDia, tickDia}}, tickColor,
                    tickDia * 0.5f);
            }
        }
    }

    float currentRadius = thumbRadius_ * thumbScaleAnim_.currentValue();
    float thumbCenterX = trackX + activeW;
    float thumbCenterY = trackY + trackHeight_ * 0.5f;
    core::Rect thumbRect{
        core::Point{thumbCenterX - currentRadius, thumbCenterY - currentRadius},
        core::Size{currentRadius * 2.0f, currentRadius * 2.0f},
    };

    float radii[4] = {currentRadius, currentRadius, currentRadius, currentRadius};
    ctx.drawBox(thumbRect, effectiveThumbColor(), effectiveThumbBorderColor(), 1.5f * dpiScale_,
                radii);

    if (hasKeyboardFocus() && isEnabled()) {
        theme::paintFocusRing(ctx, thumbRect, currentRadius);
    }

    if (showValue_ && reservedLabelWidth_ > 0.0f &&
        rect.size.width > trackW + 2.0f * thumbRadius_ + kReadoutGap * dpiScale_) {
        ensureValueLayout();
        if (cachedValueLayout_) {
            auto f = effectiveFont();
            float labelColStart = trackX + trackW + thumbRadius_ + kReadoutGap * dpiScale_;
            float textX = labelColStart +
                          std::max(0.0f, reservedLabelWidth_ - cachedValueLayout_->size().width);
            float textY = f ? theme::computeCenteredTextY(thumbCenterY, *f) : rect.origin.y;
            const auto& pal = colorPalette();
            core::Color textCol = isEnabled() ? pal.textSecondary : pal.textDisabled;
            ctx.drawTextLayout(core::Point{textX, textY}, *cachedValueLayout_, textCol);
        }
    }
}

} // namespace northern::widgets
