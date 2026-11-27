#include "widgets/controls/radio_button.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

RadioButton::RadioButton(std::string text, bool checked, std::shared_ptr<text::Font> font)
    : text_(std::move(text)), font_(std::move(font)), isChecked_(checked),
      checkAnim_(checked ? 1.0f : 0.0f) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::PointingHand);
    style().flexDirection = layout::FlexDirection::Row;
    style().alignItems = layout::AlignItems::Center;

    layoutNode_.setMeasureFunc([this](const layout::LayoutNode&, float availW,
                                      float) -> core::Size {
        float scaledGap = gap_ * dpiScale_;
        float textW = 0.0f;
        float textH = circleDiameter_;
        if (!text_.empty()) {
            ensureTextLayout();
            if (cachedTextLayout_) {
                textW = cachedTextLayout_->size().width + scaledGap;
                textH = std::max(textH, cachedTextLayout_->size().height);
            }
        }
        float padH = style().padding.left.resolve(availW, 0.0f) +
                     style().padding.right.resolve(availW, 0.0f);
        float padV =
            style().padding.top.resolve(0.0f, 0.0f) + style().padding.bottom.resolve(0.0f, 0.0f);
        return core::Size{circleDiameter_ + textW + padH, textH + padV};
    });
}

std::shared_ptr<text::Font> RadioButton::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        resolvedDefaultFont_ = theme::resolveFont(theme::TypographyStyle::Body, dpiScale_);
    }
    return resolvedDefaultFont_;
}

void RadioButton::ensureTextLayout() const {
    if (cachedTextLayout_ || text_.empty()) {
        return;
    }
    auto f = effectiveFont();
    if (!f) {
        return;
    }
    cachedTextLayout_ = text::TextLayout::create(text_, f);
}

void RadioButton::setChecked(bool checked) {
    if (isChecked_ != checked) {
        isChecked_ = checked;
        float target = isChecked_ ? 1.0f : 0.0f;
        if (context_) {
            checkAnim_.start(checkAnim_.currentValue(), target, 0.16f,
                             core::animation::EasingCurve::OutCubic);
            context_->registerAnimatedWidget(this);
        } else {
            checkAnim_.setCurrentValue(target);
        }
        markPaintDirty();
        onToggled.emit(isChecked_);
        if (isChecked_) {
            onSelected.emit(this);
        }
    }
}

void RadioButton::setText(std::string text) {
    if (text_ != text) {
        text_ = std::move(text);
        cachedTextLayout_.reset();
        markLayoutDirty();
        markPaintDirty();
    }
}

void RadioButton::setValue(std::string value) {
    value_ = std::move(value);
}

void RadioButton::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        cachedTextLayout_.reset();
        markLayoutDirty();
        markPaintDirty();
    }
}

void RadioButton::setSize(RadioSize size) noexcept {
    setCircleSize(toPoints(size) * dpiScale_);
}

void RadioButton::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiScale(scale);
}

void RadioButton::applyDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        float ratio = clamped / dpiScale_;
        dpiScale_ = clamped;
        circleDiameter_ *= ratio;
        dotDiameter_ *= ratio;
        resolvedDefaultFont_.reset();
        if (!font_) {
            cachedTextLayout_.reset();
        }
        markLayoutDirty();
        markPaintDirty();
    }
}

void RadioButton::setCircleSize(float diameter) noexcept {
    float clamped = std::max(8.0f, diameter);
    if (circleDiameter_ != clamped) {
        circleDiameter_ = clamped;
        dotDiameter_ = clamped * 0.5f;
        markLayoutDirty();
        markPaintDirty();
    }
}

void RadioButton::setDotSize(float diameter) noexcept {
    float clamped = std::max(2.0f, diameter);
    if (dotDiameter_ != clamped) {
        dotDiameter_ = clamped;
        markPaintDirty();
    }
}

void RadioButton::setCheckedOuterColor(const core::Color& color) noexcept {
    overrideCheckedOuterColor_ = color;
    markPaintDirty();
}

void RadioButton::setUncheckedOuterColor(const core::Color& color) noexcept {
    overrideUncheckedOuterColor_ = color;
    markPaintDirty();
}

void RadioButton::setBorderColor(const core::Color& color) noexcept {
    overrideBorderColor_ = color;
    markPaintDirty();
}

void RadioButton::setDotColor(const core::Color& color) noexcept {
    overrideDotColor_ = color;
    markPaintDirty();
}

void RadioButton::setTextColor(const core::Color& color) noexcept {
    overrideTextColor_ = color;
    markPaintDirty();
}

void RadioButton::clearColorOverrides() noexcept {
    if (overrideCheckedOuterColor_ || overrideUncheckedOuterColor_ || overrideBorderColor_ ||
        overrideDotColor_ || overrideTextColor_) {
        overrideCheckedOuterColor_.reset();
        overrideUncheckedOuterColor_.reset();
        overrideBorderColor_.reset();
        overrideDotColor_.reset();
        overrideTextColor_.reset();
        markPaintDirty();
    }
}

core::Color RadioButton::effectiveOuterColor() const noexcept {
    const auto& pal = colorPalette();
    core::Color unchecked = overrideUncheckedOuterColor_.value_or(
        pal.mode == theme::ThemeMode::Dark ? pal.surfaceRaised : pal.surfaceDefault);
    core::Color checked = overrideCheckedOuterColor_.value_or(pal.accentDefault);
    if (!isEnabled()) {
        return isChecked_ ? pal.borderDefault : pal.surfaceBase;
    }
    if (isHovered_ && isChecked_ && !overrideCheckedOuterColor_) {
        checked = pal.accentHover;
    }
    float t = checkAnim_.currentValue();
    if (t <= 0.0001f) {
        return unchecked;
    }
    if (t >= 0.9999f) {
        return checked;
    }
    return unchecked.lerp(checked, t);
}

core::Color RadioButton::effectiveBorderColor() const noexcept {
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.borderSubtle;
    }
    if (overrideBorderColor_) {
        return *overrideBorderColor_;
    }
    float t = checkAnim_.currentValue();
    core::Color base = pal.borderDefault.lerp(pal.borderStrong, hoverAnim_.currentValue());
    core::Color active = isHovered_ ? pal.accentHover : pal.accentDefault;
    return base.lerp(active, t);
}

core::Color RadioButton::effectiveDotColor() const noexcept {
    if (overrideDotColor_) {
        return *overrideDotColor_;
    }
    if (!isEnabled()) {
        return colorPalette().textDisabled;
    }
    return core::Color::white();
}

core::Color RadioButton::effectiveTextColor() const noexcept {
    if (overrideTextColor_) {
        return isEnabled() ? *overrideTextColor_
                           : overrideTextColor_->withAlpha(overrideTextColor_->a * 0.40f);
    }
    const auto& pal = colorPalette();
    return isEnabled() ? pal.textPrimary : pal.textDisabled;
}

void RadioButton::cancelActiveInteraction() {
    if (isPressed_ && context_ && context_->mouseGrabWidget() == this) {
        context_->releaseMouseGrab();
    }
    isPressed_ = false;
    isHovered_ = false;
    hoverAnim_.stop();
    hoverAnim_.setCurrentValue(0.0f);
}

void RadioButton::onMouseEnter() {
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

void RadioButton::onMouseLeave() {
    isHovered_ = false;
    if (isEnabled()) {
        hoverAnim_.start(hoverAnim_.currentValue(), 0.0f, 0.12f,
                         core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
    }
    markPaintDirty();
}

void RadioButton::onMouseDown(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.isPrimaryButton()) {
        isPressed_ = true;
        if (context_) {
            context_->setMouseGrabWidget(this);
        }
        setFocus(FocusReason::Pointer);
        ev.accept();
        markPaintDirty();
    }
}

void RadioButton::onMouseUp(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        if (isPressed_ && context_ && context_->mouseGrabWidget() == this) {
            context_->releaseMouseGrab();
        }
        isPressed_ = false;
        return;
    }
    if (ev.isPrimaryButton() && isPressed_) {
        if (context_ && context_->mouseGrabWidget() == this) {
            context_->releaseMouseGrab();
        }
        core::Rect rect = layoutRect();
        bool inside =
            (rect.size.width <= 0.0f || rect.size.height <= 0.0f) || hitTest(ev.position());
        isPressed_ = false;
        isHovered_ = inside;
        ev.accept();
        if (inside && !isChecked_) {
            setChecked(true);
        } else {
            markPaintDirty();
        }
    }
}

void RadioButton::onKeyDown(WidgetKeyEvent& ev) {
    if (!isEnabled() || !ev.isDown()) {
        return;
    }
    if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        ev.accept();
        if (!isChecked_) {
            setChecked(true);
        }
    }
}

void RadioButton::onFocusGained() {
    markPaintDirty();
}

void RadioButton::onFocusLost() {
    cancelActiveInteraction();
    markPaintDirty();
}

void RadioButton::onContextAttached() {
    if (!hasCustomDpiScale_) {
        applyDpiScale(contextDpiScale());
    }
}

void RadioButton::onThemeChanged() {
    cachedTextLayout_.reset();
    markLayoutDirty();
    markPaintDirty();
}

void RadioButton::onEnabledChanged() {
    if (!isEnabled()) {
        cancelActiveInteraction();
        setCursorShape(platform::CursorShape::Arrow);
    } else {
        setCursorShape(platform::CursorShape::PointingHand);
    }
    markPaintDirty();
}

void RadioButton::updateAnimations(float deltaSeconds) {
    bool changed = false;
    if (checkAnim_.isRunning()) {
        checkAnim_.update(deltaSeconds);
        changed = true;
    }
    if (hoverAnim_.isRunning()) {
        hoverAnim_.update(deltaSeconds);
        changed = true;
    }
    if (changed) {
        markPaintDirty();
    }
}

bool RadioButton::hasActiveAnimations() const noexcept {
    return checkAnim_.isRunning() || hoverAnim_.isRunning();
}

void RadioButton::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;
    float circleY = targetCenterY - circleDiameter_ * 0.5f;
    core::Rect circleRect{core::Point{rect.origin.x, circleY},
                          core::Size{circleDiameter_, circleDiameter_}};

    float radius = circleDiameter_ * 0.5f;
    float radii[4] = {radius, radius, radius, radius};
    float t = checkAnim_.currentValue();

    core::Color bg = effectiveOuterColor();
    core::Color border = effectiveBorderColor();
    if (isPressed_ && isEnabled()) {
        bg = bg.lerp(colorPalette().surfaceOverlay, 0.25f);
    }

    float borderWidth = std::max(1.0f, std::round(theme::Metrics::BorderWidthHairline * scale));
    ctx.drawBox(circleRect, bg, border, borderWidth, radii);

    if (hasKeyboardFocus() && isEnabled()) {
        paintFocusRing(ctx, circleRect, radius);
    }

    if (t > 0.001f) {
        float currentDotDiameter = dotDiameter_ * t;
        float dotOffset = (circleDiameter_ - currentDotDiameter) * 0.5f;
        core::Rect dotRect{
            core::Point{circleRect.origin.x + dotOffset, circleRect.origin.y + dotOffset},
            core::Size{currentDotDiameter, currentDotDiameter}};
        core::Color currentDotColor = effectiveDotColor();
        currentDotColor.a *= t;
        ctx.drawRoundedRect(dotRect, currentDotColor, currentDotDiameter * 0.5f);
    }

    if (!text_.empty()) {
        ensureTextLayout();
        auto f = effectiveFont();
        if (cachedTextLayout_ && f) {
            float textX = circleRect.origin.x + circleDiameter_ + gap_ * dpiScale_;
            float textY = theme::computeCenteredTextY(targetCenterY, *f);
            ctx.drawTextLayout(core::Point{textX, textY}, *cachedTextLayout_, effectiveTextColor());
        }
    }
}

} // namespace northern::widgets
