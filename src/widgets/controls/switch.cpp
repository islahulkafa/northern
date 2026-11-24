#include "widgets/controls/switch.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

Switch::Switch(std::string text, bool checked, std::shared_ptr<text::Font> font)
    : text_(std::move(text)), font_(std::move(font)), isChecked_(checked),
      thumbAnim_(checked ? 1.0f : 0.0f) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::PointingHand);
    style().flexDirection = layout::FlexDirection::Row;
    style().alignItems = layout::AlignItems::Center;

    layoutNode_.setMeasureFunc([this](const layout::LayoutNode&, float availW,
                                      float) -> core::Size {
        float textW = 0.0f;
        float textH = trackHeight_;
        if (!text_.empty()) {
            ensureTextLayout();
            if (cachedTextLayout_) {
                textW = cachedTextLayout_->size().width + gap_ * dpiScale_;
                textH = std::max(textH, cachedTextLayout_->size().height);
            }
        }
        float padH = style().padding.left.resolve(availW, 0.0f) +
                     style().padding.right.resolve(availW, 0.0f);
        float padV =
            style().padding.top.resolve(0.0f, 0.0f) + style().padding.bottom.resolve(0.0f, 0.0f);
        return core::Size{trackWidth_ + textW + padH, textH + padV};
    });
}

Switch::Switch(std::string text, SwitchSize size, SwitchRole role, bool checked)
    : Switch(std::move(text), checked, nullptr) {
    role_ = role;
    setSize(size);
}

std::shared_ptr<text::Font> Switch::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        resolvedDefaultFont_ = theme::resolveFont(theme::TypographyStyle::Body, dpiScale_);
    }
    return resolvedDefaultFont_;
}

void Switch::ensureTextLayout() const {
    if (cachedTextLayout_ || text_.empty()) {
        return;
    }
    auto f = effectiveFont();
    if (!f) {
        return;
    }
    cachedTextLayout_ = text::TextLayout::create(text_, f);
}

void Switch::setChecked(bool checked) {
    if (isChecked_ != checked) {
        isChecked_ = checked;
        float target = isChecked_ ? 1.0f : 0.0f;
        if (context_) {
            thumbAnim_.start(thumbAnim_.currentValue(), target, 0.18f,
                             core::animation::EasingCurve::OutCubic);
            context_->registerAnimatedWidget(this);
        } else {
            thumbAnim_.setCurrentValue(target);
        }
        markPaintDirty();
        onToggled.emit(isChecked_);
    }
}

void Switch::updateAnimations(float deltaSeconds) {
    if (thumbAnim_.isRunning()) {
        thumbAnim_.update(deltaSeconds);
    }
    if (hoverAnim_.isRunning()) {
        hoverAnim_.update(deltaSeconds);
    }
    if (pressAnim_.isRunning()) {
        pressAnim_.update(deltaSeconds);
    }
    markPaintDirty();
}

bool Switch::hasActiveAnimations() const noexcept {
    return thumbAnim_.isRunning() || hoverAnim_.isRunning() || pressAnim_.isRunning();
}

void Switch::toggle() {
    setChecked(!isChecked_);
}

void Switch::setText(std::string text) {
    if (text_ != text) {
        text_ = std::move(text);
        cachedTextLayout_.reset();
        markLayoutDirty();
    }
}

void Switch::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        cachedTextLayout_.reset();
        markLayoutDirty();
    }
}

void Switch::setRole(SwitchRole role) noexcept {
    if (role_ != role) {
        role_ = role;
        markPaintDirty();
    }
}

void Switch::setSize(SwitchSize size) noexcept {
    float baseW = 36.0f;
    float baseH = 20.0f;
    switch (size) {
    case SwitchSize::Compact:
        baseW = 28.0f;
        baseH = 16.0f;
        break;
    case SwitchSize::Large:
        baseW = 44.0f;
        baseH = 24.0f;
        break;
    case SwitchSize::Standard:
    default:
        break;
    }
    setTrackSize(baseW * dpiScale_, baseH * dpiScale_);
}

void Switch::setDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        float ratio = clamped / dpiScale_;
        dpiScale_ = clamped;
        trackWidth_ *= ratio;
        trackHeight_ *= ratio;
        thumbDiameter_ *= ratio;
        resolvedDefaultFont_.reset();
        if (!font_) {
            cachedTextLayout_.reset();
        }
        markLayoutDirty();
    }
}

void Switch::setLabelPlacement(SwitchLabelPlacement placement) noexcept {
    if (labelPlacement_ != placement) {
        labelPlacement_ = placement;
        markPaintDirty();
    }
}

void Switch::setTrackSize(float width, float height) noexcept {
    trackWidth_ = std::max(12.0f, width);
    trackHeight_ = std::max(8.0f, height);
    thumbDiameter_ = std::max(4.0f, trackHeight_ - 4.0f * dpiScale_);
    markLayoutDirty();
}

void Switch::setCheckedTrackColor(const core::Color& color) noexcept {
    overrideCheckedTrackColor_ = color;
    markPaintDirty();
}

void Switch::setUncheckedTrackColor(const core::Color& color) noexcept {
    overrideUncheckedTrackColor_ = color;
    markPaintDirty();
}

void Switch::setBorderColor(const core::Color& color) noexcept {
    overrideBorderColor_ = color;
    markPaintDirty();
}

void Switch::setCheckedThumbColor(const core::Color& color) noexcept {
    overrideCheckedThumbColor_ = color;
    markPaintDirty();
}

void Switch::setUncheckedThumbColor(const core::Color& color) noexcept {
    overrideUncheckedThumbColor_ = color;
    markPaintDirty();
}

void Switch::setTextColor(const core::Color& color) noexcept {
    overrideTextColor_ = color;
    markPaintDirty();
}

void Switch::clearColorOverrides() noexcept {
    if (overrideCheckedTrackColor_ || overrideUncheckedTrackColor_ || overrideBorderColor_ ||
        overrideCheckedThumbColor_ || overrideUncheckedThumbColor_ || overrideTextColor_) {
        overrideCheckedTrackColor_.reset();
        overrideUncheckedTrackColor_.reset();
        overrideBorderColor_.reset();
        overrideCheckedThumbColor_.reset();
        overrideUncheckedThumbColor_.reset();
        overrideTextColor_.reset();
        markPaintDirty();
    }
}

core::Color Switch::effectiveTrackColor() const noexcept {
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return isChecked_ ? pal.borderDefault : pal.surfaceBase;
    }

    core::Color unchecked = overrideUncheckedTrackColor_.value_or(
        pal.mode == theme::ThemeMode::Dark ? pal.surfaceRaised : pal.surfaceDefault);
    core::Color roleColor = pal.accentDefault;
    switch (role_) {
    case SwitchRole::Accent:
        roleColor =
            (isHovered_ && !overrideCheckedTrackColor_) ? pal.accentHover : pal.accentDefault;
        break;
    case SwitchRole::Success:
        roleColor = pal.success;
        break;
    case SwitchRole::Warning:
        roleColor = pal.warning;
        break;
    case SwitchRole::Danger:
        roleColor = pal.danger;
        break;
    }
    core::Color checked = overrideCheckedTrackColor_.value_or(roleColor);

    float t = thumbAnim_.currentValue();
    if (t <= 0.0001f) {
        return unchecked;
    }
    if (t >= 0.9999f) {
        return checked;
    }
    return unchecked.lerp(checked, t);
}

core::Color Switch::effectiveBorderColor() const noexcept {
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.borderSubtle;
    }
    if (overrideBorderColor_) {
        return *overrideBorderColor_;
    }
    float t = thumbAnim_.currentValue();
    core::Color base = pal.borderDefault.lerp(pal.borderStrong, hoverAnim_.currentValue());
    core::Color active = effectiveTrackColor();
    return base.lerp(active, t);
}

core::Color Switch::effectiveThumbColor() const noexcept {
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.textDisabled;
    }
    core::Color unchecked = overrideUncheckedThumbColor_.value_or(pal.textSecondary);
    core::Color checked = overrideCheckedThumbColor_.value_or(core::Color::white());
    float t = thumbAnim_.currentValue();
    if (t <= 0.0001f) {
        return unchecked;
    }
    if (t >= 0.9999f) {
        return checked;
    }
    return unchecked.lerp(checked, t);
}

core::Color Switch::effectiveTextColor() const noexcept {
    if (overrideTextColor_) {
        return isEnabled() ? *overrideTextColor_
                           : overrideTextColor_->withAlpha(overrideTextColor_->a * 0.40f);
    }
    const auto& pal = colorPalette();
    return isEnabled() ? pal.textPrimary : pal.textDisabled;
}

void Switch::onMouseEnter() {
    if (!isEnabled()) {
        return;
    }
    isHovered_ = true;
    hoverAnim_.start(hoverAnim_.currentValue(), 1.0f, 0.12f,
                     core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
    markPaintDirty();
}

void Switch::onMouseLeave() {
    isHovered_ = false;
    isPressed_ = false;
    if (isEnabled()) {
        hoverAnim_.start(hoverAnim_.currentValue(), 0.0f, 0.12f,
                         core::animation::EasingCurve::OutCubic);
        pressAnim_.start(pressAnim_.currentValue(), 0.0f, 0.08f,
                         core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
    }
    markPaintDirty();
}

void Switch::onMouseDown(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.button() == platform::MouseButton::Left) {
        isPressed_ = true;
        setFocus(FocusReason::Pointer);
        pressAnim_.start(pressAnim_.currentValue(), 1.0f, 0.08f,
                         core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
        ev.accept();
        markPaintDirty();
    }
}

void Switch::onMouseUp(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        isPressed_ = false;
        return;
    }
    if (ev.button() == platform::MouseButton::Left && isPressed_) {
        isPressed_ = false;
        pressAnim_.start(pressAnim_.currentValue(), 0.0f, 0.08f,
                         core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
        ev.accept();
        if (layoutRect().size.width <= 0.0f || hitTest(ev.position())) {
            toggle();
        } else {
            markPaintDirty();
        }
    }
}

void Switch::onKeyDown(WidgetKeyEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        ev.accept();
        toggle();
    }
}

void Switch::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;
    float trackY = targetCenterY - trackHeight_ * 0.5f;
    float trackX = rect.origin.x;

    bool hasText = !text_.empty();
    if (hasText) {
        ensureTextLayout();
    }

    if (hasText && labelPlacement_ == SwitchLabelPlacement::Leading) {
        trackX = rect.origin.x + std::max(0.0f, rect.size.width - trackWidth_);
    }

    core::Rect trackRect{core::Point{trackX, trackY}, core::Size{trackWidth_, trackHeight_}};

    float trackRadius = trackHeight_ * 0.5f;
    float radii[4] = {trackRadius, trackRadius, trackRadius, trackRadius};
    float progress = thumbAnim_.currentValue();

    ctx.drawBox(trackRect, effectiveTrackColor(), effectiveBorderColor(), 1.0f, radii);

    if (hasKeyboardFocus() && isEnabled()) {
        theme::paintFocusRing(ctx, trackRect, trackRadius);
    }

    float pad = (trackHeight_ - thumbDiameter_) * 0.5f;
    float travel = std::max(0.0f, trackWidth_ - pad * 2.0f - thumbDiameter_);
    float stretch = std::min(travel * 0.35f, 3.5f * dpiScale_) * pressAnim_.currentValue();
    float thumbW = thumbDiameter_ + stretch;
    float thumbX = trackRect.origin.x + pad + (travel - stretch) * progress;
    float thumbY = trackRect.origin.y + pad;

    core::Rect thumbRect{core::Point{thumbX, thumbY}, core::Size{thumbW, thumbDiameter_}};
    ctx.drawRoundedRect(thumbRect, effectiveThumbColor(), thumbDiameter_ * 0.5f);

    if (hasText) {
        auto f = effectiveFont();
        if (cachedTextLayout_ && f) {
            float textX = (labelPlacement_ == SwitchLabelPlacement::Leading)
                              ? rect.origin.x
                              : (trackRect.origin.x + trackWidth_ + gap_ * dpiScale_);
            float textY = theme::computeCenteredTextY(targetCenterY, *f);
            ctx.drawTextLayout(core::Point{textX, textY}, *cachedTextLayout_, effectiveTextColor());
        }
    }
}

} // namespace northern::widgets
