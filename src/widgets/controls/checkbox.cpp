#include "widgets/controls/checkbox.hpp"
#include "renderer/paint_context.hpp"
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
        float textW = 0.0f;
        float textH = boxSize_;
        if (font_ && !text_.empty()) {
            auto l = text::TextLayout::create(text_, font_);
            textW = l.size().width + gap_;
            textH = std::max(textH, l.size().height);
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
    checkAnim_.update(deltaSeconds);
    hoverAnim_.update(deltaSeconds);
    markPaintDirty();
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
        markLayoutDirty();
    }
}

void Checkbox::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        markLayoutDirty();
    }
}

void Checkbox::setBoxSize(float size) noexcept {
    boxSize_ = size;
    markLayoutDirty();
}

void Checkbox::setBoxCornerRadius(float radius) noexcept {
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
    isPressed_ = false;
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
    if (ev.button() == platform::MouseButton::Left) {
        isPressed_ = true;
        setFocus(FocusReason::Pointer);
        ev.accept();
        markPaintDirty();
    }
}

void Checkbox::onMouseUp(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.button() == platform::MouseButton::Left && isPressed_) {
        isPressed_ = false;
        ev.accept();
        toggle();
    }
}

void Checkbox::onKeyDown(WidgetKeyEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        ev.accept();
        toggle();
    }
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

    float radii[4] = {boxCornerRadius_, boxCornerRadius_, boxCornerRadius_, boxCornerRadius_};
    core::Color baseBg = effectiveUncheckedColor().lerp(effectiveCheckedColor(), progress);
    core::Color idleBorder = effectiveBorderColor(false);
    core::Color hoverBorder = effectiveBorderColor(true);
    core::Color baseBorder = idleBorder.lerp(hoverBorder, hoverAlpha);
    core::Color border = baseBorder.lerp(effectiveCheckedColor(), progress);

    if (!isEnabled()) {
        baseBg = baseBg.withAlpha(baseBg.a * 0.35f);
        border = border.withAlpha(border.a * 0.25f);
    }

    ctx.drawBox(boxRect, baseBg, border, 1.0f, radii);

    if (hasKeyboardFocus()) {
        theme::paintFocusRing(ctx, boxRect, boxCornerRadius_);
    }

    if (progress > 0.001f) {
        float innerPadding = std::max(2.5f, boxSize_ * 0.22f);
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

    if (font_ && !text_.empty()) {
        auto layout = text::TextLayout::create(text_, font_);
        float textX = boxRect.origin.x + boxSize_ + gap_;
        float textY = theme::computeCenteredTextY(targetCenterY, *font_);
        ctx.drawTextLayout(core::Point{textX, textY}, layout, effectiveTextColor());
    }
}

} // namespace northern::widgets
