#include "widgets/checkbox.hpp"
#include "renderer/paint_context.hpp"
#include "text/text_layout.hpp"
#include "widgets/theme.hpp"

#include <algorithm>

namespace northern::widgets {

Checkbox::Checkbox(std::string text, bool checked, std::shared_ptr<text::Font> font)
    : text_(std::move(text)), font_(std::move(font)), isChecked_(checked) {
    setFocusable(true);
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
    if (isChecked_ != checked) {
        isChecked_ = checked;
        markPaintDirty();
        onToggled.emit(isChecked_);
    }
}

void Checkbox::toggle() {
    setChecked(!isChecked_);
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
    checkedColor_ = color;
    markPaintDirty();
}

void Checkbox::setUncheckedColor(const core::Color& color) noexcept {
    uncheckedColor_ = color;
    markPaintDirty();
}

void Checkbox::setBorderColor(const core::Color& color) noexcept {
    borderColor_ = color;
    markPaintDirty();
}

void Checkbox::setCheckmarkColor(const core::Color& color) noexcept {
    checkmarkColor_ = color;
    markPaintDirty();
}

void Checkbox::setTextColor(const core::Color& color) noexcept {
    textColor_ = color;
    markPaintDirty();
}

void Checkbox::onMouseEnter() {
    isHovered_ = true;
    markPaintDirty();
}

void Checkbox::onMouseLeave() {
    isHovered_ = false;
    isPressed_ = false;
    markPaintDirty();
}

void Checkbox::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        isPressed_ = true;
        setFocus();
        ev.accept();
        markPaintDirty();
    }
}

void Checkbox::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left && isPressed_) {
        isPressed_ = false;
        ev.accept();
        toggle();
    }
}

void Checkbox::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;
    float boxY = targetCenterY - boxSize_ * 0.5f;
    core::Rect boxRect{core::Point{rect.origin.x, boxY}, core::Size{boxSize_, boxSize_}};

    float radii[4] = {boxCornerRadius_, boxCornerRadius_, boxCornerRadius_, boxCornerRadius_};
    core::Color bg = isChecked_ ? checkedColor_ : uncheckedColor_;
    core::Color border = isHovered_ ? borderHoverColor_ : borderColor_;

    ctx.drawBox(boxRect, bg, border, 1.0f, radii);

    if (hasFocus()) {
        theme::paintFocusRing(ctx, boxRect, boxCornerRadius_);
    }

    if (isChecked_) {
        float cx = boxRect.origin.x;
        float cy = boxRect.origin.y;
        float s = boxSize_;
        core::Point p0{cx + s * 0.22f, cy + s * 0.50f};
        core::Point p1{cx + s * 0.42f, cy + s * 0.72f};
        core::Point p2{cx + s * 0.78f, cy + s * 0.28f};
        float thickness = std::max(1.8f, s * 0.12f);
        ctx.drawLine(p0, p1, checkmarkColor_, thickness);
        ctx.drawLine(p1, p2, checkmarkColor_, thickness);
    }

    if (font_ && !text_.empty()) {
        auto layout = text::TextLayout::create(text_, font_);
        float textX = boxRect.origin.x + boxSize_ + gap_;
        float textY = theme::computeCenteredTextY(targetCenterY, *font_);
        ctx.drawTextLayout(core::Point{textX, textY}, layout, textColor_);
    }
}

} // namespace northern::widgets
