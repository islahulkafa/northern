#include "widgets/controls/checkbox.hpp"
#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

Checkbox::Checkbox(std::string text, bool checked, std::shared_ptr<text::Font> font)
    : Checkbox(std::move(text), checked ? CheckState::Checked : CheckState::Unchecked,
               std::move(font)) {}

Checkbox::Checkbox(std::string text, CheckState state, std::shared_ptr<text::Font> font)
    : text_(std::move(text)), font_(std::move(font)), checkState_(state),
      displayedGlyphState_(state == CheckState::Indeterminate ? CheckState::Indeterminate
                                                              : CheckState::Checked),
      checkAnim_(state == CheckState::Unchecked ? 0.0f : 1.0f) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::PointingHand);
    style().flexDirection = layout::FlexDirection::Row;
    style().alignItems = layout::AlignItems::Center;

    layoutNode_.setMeasureFunc([this](const layout::LayoutNode&, float availW,
                                      float) -> core::Size {
        auto f = effectiveFont();
        float textW = 0.0f;
        float textH = boxSize_;
        if (f && !text_.empty()) {
            if (isTextLayoutDirty_ || !cachedTextLayout_) {
                cachedTextLayout_ = text::TextLayout::create(text_, f);
                isTextLayoutDirty_ = false;
            }
            textW = cachedTextLayout_->size().width + gap_;
            textH = std::max(textH, cachedTextLayout_->size().height);
        }
        float padH = style().padding.left.resolve(availW, 0.0f) +
                     style().padding.right.resolve(availW, 0.0f);
        float padV =
            style().padding.top.resolve(0.0f, 0.0f) + style().padding.bottom.resolve(0.0f, 0.0f);
        return core::Size{boxSize_ + textW + padH, textH + padV};
    });
}

void Checkbox::setChecked(bool checked) {
    setCheckState(checked ? CheckState::Checked : CheckState::Unchecked);
}

void Checkbox::setCheckState(CheckState state) {
    if (checkState_ != state) {
        CheckState prevState = checkState_;
        checkState_ = state;
        if (state == CheckState::Checked || state == CheckState::Indeterminate) {
            displayedGlyphState_ = state;
        }

        float target = (state == CheckState::Unchecked) ? 0.0f : 1.0f;
        float startVal = checkAnim_.currentValue();
        if (prevState != CheckState::Unchecked && state != CheckState::Unchecked) {
            startVal = 0.0f;
        }

        checkAnim_.start(startVal, target, 0.15f, core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
        markPaintDirty();
        onToggled.emit(checkState_);
        onCheckedChanged.emit(isChecked());
    }
}

void Checkbox::updateAnimations(float deltaSeconds) {
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

bool Checkbox::hasActiveAnimations() const noexcept {
    return checkAnim_.isRunning() || hoverAnim_.isRunning();
}

void Checkbox::toggle() {
    if (!isEnabled()) {
        return;
    }
    if (tristate_) {
        switch (checkState_) {
        case CheckState::Unchecked:
            setCheckState(CheckState::Checked);
            break;
        case CheckState::Checked:
            setCheckState(CheckState::Indeterminate);
            break;
        case CheckState::Indeterminate:
            setCheckState(CheckState::Unchecked);
            break;
        }
    } else {
        setCheckState(isChecked() ? CheckState::Unchecked : CheckState::Checked);
    }
}

void Checkbox::setText(std::string text) {
    if (text_ != text) {
        text_ = std::move(text);
        isTextLayoutDirty_ = true;
        markLayoutDirty();
        markPaintDirty();
    }
}

void Checkbox::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedDefaultFont_.reset();
        isTextLayoutDirty_ = true;
        markLayoutDirty();
        markPaintDirty();
    }
}

std::shared_ptr<text::Font> Checkbox::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
        resolvedDefaultFont_ =
            text::FontManager::instance().defaultFont(10.5f * scale).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void Checkbox::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiScale(scale);
}

void Checkbox::applyDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        if (!hasCustomBoxSize_) {
            boxSize_ = 16.0f * dpiScale_;
        }
        if (!hasCustomCornerRadius_) {
            boxCornerRadius_ = 4.0f * dpiScale_;
        }
        gap_ = 8.0f * dpiScale_;
        resolvedDefaultFont_.reset();
        isTextLayoutDirty_ = true;
        markLayoutDirty();
        markPaintDirty();
    }
}

void Checkbox::setBoxSize(float size) noexcept {
    hasCustomBoxSize_ = true;
    boxSize_ = size;
    markLayoutDirty();
    markPaintDirty();
}

void Checkbox::setBoxCornerRadius(float radius) noexcept {
    hasCustomCornerRadius_ = true;
    boxCornerRadius_ = radius;
    markPaintDirty();
}

void Checkbox::setCheckedColor(const core::Color& color) noexcept {
    overrideCheckedColor_ = color;
    markPaintDirty();
}

void Checkbox::setUncheckedColor(const core::Color& color) noexcept {
    overrideUncheckedColor_ = color;
    markPaintDirty();
}

void Checkbox::setBorderColor(const core::Color& color) noexcept {
    overrideBorderColor_ = color;
    markPaintDirty();
}

void Checkbox::setBorderHoverColor(const core::Color& color) noexcept {
    overrideBorderHoverColor_ = color;
    markPaintDirty();
}

void Checkbox::setCheckmarkColor(const core::Color& color) noexcept {
    overrideCheckmarkColor_ = color;
    markPaintDirty();
}

void Checkbox::setTextColor(const core::Color& color) noexcept {
    overrideTextColor_ = color;
    markPaintDirty();
}

core::Color Checkbox::effectiveCheckedColor() const noexcept {
    if (overrideCheckedColor_) {
        return *overrideCheckedColor_;
    }
    return colorPalette().accentDefault;
}

core::Color Checkbox::effectiveUncheckedColor() const noexcept {
    if (overrideUncheckedColor_) {
        return *overrideUncheckedColor_;
    }
    return core::Color::transparent();
}

core::Color Checkbox::effectiveBorderColor(bool hovered) const noexcept {
    if (hovered) {
        if (overrideBorderHoverColor_) {
            return *overrideBorderHoverColor_;
        }
        if (overrideBorderColor_) {
            return *overrideBorderColor_;
        }
        return colorPalette().borderStrong;
    }
    if (overrideBorderColor_) {
        return *overrideBorderColor_;
    }
    return colorPalette().borderDefault;
}

core::Color Checkbox::effectiveCheckmarkColor() const noexcept {
    if (overrideCheckmarkColor_) {
        return *overrideCheckmarkColor_;
    }
    return core::Color::white();
}

core::Color Checkbox::effectiveTextColor() const noexcept {
    if (overrideTextColor_) {
        return *overrideTextColor_;
    }
    return isEnabled() ? colorPalette().textPrimary : colorPalette().textDisabled;
}

void Checkbox::cancelActiveInteraction() {
    if (isPressed_ && context_ && context_->mouseGrabWidget() == this) {
        context_->releaseMouseGrab();
    }
    isPressed_ = false;
    isHovered_ = false;
    hoverAnim_.stop();
    hoverAnim_.setCurrentValue(0.0f);
}

void Checkbox::onMouseEnter() {
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

void Checkbox::onMouseLeave() {
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

void Checkbox::onMouseDown(WidgetMouseEvent& ev) {
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

void Checkbox::onMouseUp(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.isPrimaryButton() && isPressed_) {
        if (context_ && context_->mouseGrabWidget() == this) {
            context_->releaseMouseGrab();
        }
        core::Rect rect = layoutRect();
        bool inside = (rect.size.width <= 0.0f || rect.size.height <= 0.0f) ||
                      (ev.position().x >= 0.0f && ev.position().x <= rect.size.width &&
                       ev.position().y >= 0.0f && ev.position().y <= rect.size.height);
        isPressed_ = false;
        isHovered_ = inside;
        ev.accept();
        markPaintDirty();
        if (inside) {
            toggle();
        }
    }
}

void Checkbox::onKeyDown(WidgetKeyEvent& ev) {
    if (!isEnabled() || !ev.isDown()) {
        return;
    }
    if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        ev.accept();
        toggle();
    }
}

void Checkbox::onFocusGained() {
    markPaintDirty();
}

void Checkbox::onFocusLost() {
    cancelActiveInteraction();
    markPaintDirty();
}

void Checkbox::onContextAttached() {
    if (!hasCustomDpiScale_) {
        applyDpiScale(contextDpiScale());
    }
}

void Checkbox::onThemeChanged() {
    isTextLayoutDirty_ = true;
    markLayoutDirty();
    markPaintDirty();
}

void Checkbox::onEnabledChanged() {
    if (!isEnabled()) {
        cancelActiveInteraction();
        setCursorShape(platform::CursorShape::Arrow);
    } else {
        setCursorShape(platform::CursorShape::PointingHand);
    }
    markPaintDirty();
}

void Checkbox::drawCheckmark(renderer::PaintContext& ctx, const core::Rect& inner, float progress,
                             const core::Color& color) const {
    if (progress <= 0.001f || inner.size.width <= 0.0f || inner.size.height <= 0.0f) {
        return;
    }

    core::Point p0{inner.origin.x + inner.size.width * 0.08f,
                   inner.origin.y + inner.size.height * 0.50f};
    core::Point p1{inner.origin.x + inner.size.width * 0.38f,
                   inner.origin.y + inner.size.height * 0.86f};
    core::Point p2{inner.origin.x + inner.size.width * 0.92f,
                   inner.origin.y + inner.size.height * 0.14f};

    float dx1 = p1.x - p0.x;
    float dy1 = p1.y - p0.y;
    float len1 = std::sqrt(dx1 * dx1 + dy1 * dy1);

    float dx2 = p2.x - p1.x;
    float dy2 = p2.y - p1.y;
    float len2 = std::sqrt(dx2 * dx2 + dy2 * dy2);

    float totalLen = len1 + len2;
    if (totalLen <= 0.001f) {
        return;
    }

    float thickness = std::clamp(boxSize_ * 0.11f, 1.5f, 2.5f);
    float drawnLen = progress * totalLen;

    if (drawnLen > 0.0f) {
        float t1 = std::min(drawnLen / len1, 1.0f);
        core::Point curr1{p0.x + dx1 * t1, p0.y + dy1 * t1};
        ctx.drawLine(p0, curr1, color, thickness);
    }

    if (drawnLen > len1) {
        float t2 = std::min((drawnLen - len1) / len2, 1.0f);
        core::Point curr2{p1.x + dx2 * t2, p1.y + dy2 * t2};
        ctx.drawLine(p1, curr2, color, thickness);
    }
}

void Checkbox::drawIndeterminate(renderer::PaintContext& ctx, const core::Rect& inner,
                                 float progress, const core::Color& color) const {
    if (progress <= 0.001f || inner.size.width <= 0.0f || inner.size.height <= 0.0f) {
        return;
    }

    float cy = inner.origin.y + inner.size.height * 0.5f;
    float x0 = inner.origin.x + inner.size.width * 0.10f;
    float totalDashLen = inner.size.width * 0.80f;
    float x1 = x0 + totalDashLen * progress;

    float thickness = std::clamp(boxSize_ * 0.12f, 1.8f, 2.8f);
    ctx.drawLine(core::Point{x0, cy}, core::Point{x1, cy}, color, thickness);
}

void Checkbox::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    float progress = checkAnim_.currentValue();
    float hoverAlpha = hoverAnim_.currentValue();

    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;
    float padLeft = style().padding.left.resolve(rect.size.width, 0.0f);
    float boxX = rect.origin.x + padLeft;
    float boxY = targetCenterY - boxSize_ * 0.5f;
    core::Rect boxRect{core::Point{boxX, boxY}, core::Size{boxSize_, boxSize_}};

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float radii[4] = {boxCornerRadius_, boxCornerRadius_, boxCornerRadius_, boxCornerRadius_};
    core::Color baseBg = effectiveUncheckedColor().lerp(effectiveCheckedColor(), progress);
    if (isEnabled() && isPressed_ && checkState_ == CheckState::Unchecked) {
        baseBg = baseBg.lerp(colorPalette().textPrimary, 0.06f);
    }
    core::Color idleBorder = effectiveBorderColor(false);
    core::Color hoverBorder = effectiveBorderColor(true);
    core::Color baseBorder = idleBorder.lerp(hoverBorder, hoverAlpha);
    core::Color border = baseBorder.lerp(effectiveCheckedColor(), progress);

    if (!isEnabled()) {
        baseBg = baseBg.withAlpha(baseBg.a * 0.35f);
        border = border.withAlpha(border.a * 0.25f);
    }

    float borderWidth = std::max(1.0f, std::round(theme::Metrics::BorderWidthHairline * scale));
    ctx.drawBox(boxRect, baseBg, border, borderWidth, radii);

    if (isEnabled() && hasKeyboardFocus()) {
        paintFocusRing(ctx, boxRect, boxCornerRadius_);
    }

    if (progress > 0.001f) {
        float innerPadding = std::max(2.5f * scale, boxSize_ * 0.22f);
        core::Rect innerRect{
            core::Point{boxRect.origin.x + innerPadding, boxRect.origin.y + innerPadding},
            core::Size{boxSize_ - 2.0f * innerPadding, boxSize_ - 2.0f * innerPadding}};

        core::Color markCol = effectiveCheckmarkColor();
        float alphaMult = isEnabled() ? 1.0f : 0.40f;
        core::Color resolvedMarkColor =
            markCol.withAlpha(markCol.a * std::min(progress * 1.5f, 1.0f) * alphaMult);

        if (displayedGlyphState_ == CheckState::Indeterminate) {
            drawIndeterminate(ctx, innerRect, progress, resolvedMarkColor);
        } else {
            drawCheckmark(ctx, innerRect, progress, resolvedMarkColor);
        }
    }

    auto f = effectiveFont();
    if (f && !text_.empty()) {
        if (isTextLayoutDirty_ || !cachedTextLayout_) {
            cachedTextLayout_ = text::TextLayout::create(text_, f);
            isTextLayoutDirty_ = false;
        }
        float textX = boxRect.origin.x + boxSize_ + gap_;
        float textY = theme::computeCenteredTextY(targetCenterY, *f);
        ctx.drawTextLayout(core::Point{textX, textY}, *cachedTextLayout_, effectiveTextColor());
    }
}

} // namespace northern::widgets
