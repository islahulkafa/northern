#include "widgets/radio_button.hpp"
#include "renderer/paint_context.hpp"
#include "text/text_layout.hpp"
#include "widgets/widget_context.hpp"

#include <algorithm>

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
        float textW = 0.0f;
        float textH = circleDiameter_;
        if (font_ && !text_.empty()) {
            auto l = text::TextLayout::create(text_, font_);
            textW = l.size().width + gap_;
            textH = std::max(textH, l.size().height);
        }
        float padH = style().padding.left.resolve(availW, 0.0f) +
                     style().padding.right.resolve(availW, 0.0f);
        float padV =
            style().padding.top.resolve(0.0f, 0.0f) + style().padding.bottom.resolve(0.0f, 0.0f);
        return core::Size{circleDiameter_ + textW + padH, textH + padV};
    });
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
        markLayoutDirty();
    }
}

void RadioButton::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        markLayoutDirty();
    }
}

void RadioButton::setCircleSize(float diameter) noexcept {
    circleDiameter_ = diameter;
    dotDiameter_ = diameter * 0.5f;
    markLayoutDirty();
}

void RadioButton::setDotSize(float diameter) noexcept {
    dotDiameter_ = diameter;
    markPaintDirty();
}

void RadioButton::setCheckedOuterColor(const core::Color& color) noexcept {
    checkedOuterColor_ = color;
    markPaintDirty();
}

void RadioButton::setUncheckedOuterColor(const core::Color& color) noexcept {
    uncheckedOuterColor_ = color;
    markPaintDirty();
}

void RadioButton::setBorderColor(const core::Color& color) noexcept {
    borderColor_ = color;
    markPaintDirty();
}

void RadioButton::setDotColor(const core::Color& color) noexcept {
    dotColor_ = color;
    markPaintDirty();
}

void RadioButton::setTextColor(const core::Color& color) noexcept {
    textColor_ = color;
    markPaintDirty();
}

void RadioButton::onMouseEnter() {
    isHovered_ = true;
    markPaintDirty();
}

void RadioButton::onMouseLeave() {
    isHovered_ = false;
    isPressed_ = false;
    markPaintDirty();
}

void RadioButton::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        isPressed_ = true;
        setFocus();
        ev.accept();
        markPaintDirty();
    }
}

void RadioButton::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left && isPressed_) {
        isPressed_ = false;
        ev.accept();
        if (!isChecked_) {
            setChecked(true);
        }
    }
}

void RadioButton::onKeyDown(WidgetKeyEvent& ev) {
    if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        ev.accept();
        if (!isChecked_) {
            setChecked(true);
        }
    }
}

void RadioButton::updateAnimations(float deltaSeconds) {
    checkAnim_.update(deltaSeconds);
    markPaintDirty();
}

bool RadioButton::hasActiveAnimations() const noexcept {
    return checkAnim_.isRunning();
}

void RadioButton::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;
    float circleY = targetCenterY - circleDiameter_ * 0.5f;
    core::Rect circleRect{core::Point{rect.origin.x, circleY},
                          core::Size{circleDiameter_, circleDiameter_}};

    float radius = circleDiameter_ * 0.5f;
    float radii[4] = {radius, radius, radius, radius};
    float t = checkAnim_.currentValue();
    core::Color bg = uncheckedOuterColor_.lerp(checkedOuterColor_, t);
    core::Color border = isHovered_ ? borderHoverColor_ : borderColor_;

    ctx.drawBox(circleRect, bg, border, 1.0f, radii);

    if (hasKeyboardFocus()) {
        theme::paintFocusRing(ctx, circleRect, radius);
    }

    if (t > 0.001f) {
        float currentDotDiameter = dotDiameter_ * t;
        float dotOffset = (circleDiameter_ - currentDotDiameter) * 0.5f;
        core::Rect dotRect{
            core::Point{circleRect.origin.x + dotOffset, circleRect.origin.y + dotOffset},
            core::Size{currentDotDiameter, currentDotDiameter}};
        core::Color currentDotColor = dotColor_;
        currentDotColor.a *= t;
        ctx.drawRoundedRect(dotRect, currentDotColor, currentDotDiameter * 0.5f);
    }

    if (font_ && !text_.empty()) {
        auto layout = text::TextLayout::create(text_, font_);
        float textX = circleRect.origin.x + circleDiameter_ + gap_;
        float textY = theme::computeCenteredTextY(targetCenterY, *font_);
        ctx.drawTextLayout(core::Point{textX, textY}, layout, textColor_);
    }
}

} // namespace northern::widgets
