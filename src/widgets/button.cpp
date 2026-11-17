#include "widgets/button.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/theme.hpp"

#include <algorithm>

namespace northern::widgets {

Button::Button(std::string text, std::shared_ptr<text::Font> font, ButtonVariant variant)
    : text_(std::move(text)), font_(std::move(font)), variant_(variant) {
    setFocusable(true);
    style().padding = layout::RectLength{
        .left = layout::Length::points(12.0f),
        .top = layout::Length::points(6.0f),
        .right = layout::Length::points(12.0f),
        .bottom = layout::Length::points(6.0f),
    };

    applyVariantStyle();

    layoutNode_.setMeasureFunc([this](const layout::LayoutNode&, float availW,
                                      float) -> core::Size {
        float textW = 0.0f;
        float textH = 0.0f;
        if (font_ && !text_.empty()) {
            auto tempLayout = text::TextLayout::create(text_, font_);
            textW = tempLayout.size().width;
            textH = tempLayout.size().height;
        }
        float padH = style().padding.left.resolve(availW, 0.0f) +
                     style().padding.right.resolve(availW, 0.0f);
        float padV =
            style().padding.top.resolve(0.0f, 0.0f) + style().padding.bottom.resolve(0.0f, 0.0f);
        return core::Size{textW + padH, textH + padV};
    });
}

void Button::setVariant(ButtonVariant variant) noexcept {
    if (variant_ != variant) {
        variant_ = variant;
        applyVariantStyle();
        markPaintDirty();
    }
}

void Button::applyVariantStyle() noexcept {
    switch (variant_) {
    case ButtonVariant::Primary:
        idleBgColor_ = theme::ColorTokens::AccentDefault;
        hoverBgColor_ = theme::ColorTokens::AccentHover;
        pressedBgColor_ = theme::ColorTokens::AccentActive;
        idleBorderColor_ = core::Color::transparent();
        hoverBorderColor_ = core::Color::transparent();
        pressedBorderColor_ = core::Color::transparent();
        idleTextColor_ = core::Color::white();
        hoverTextColor_ = core::Color::white();
        pressedTextColor_ = core::Color::white();
        borderWidth_ = 0.0f;
        break;
    case ButtonVariant::Secondary:
        idleBgColor_ = theme::ColorTokens::SurfaceRaised;
        hoverBgColor_ = core::Color{0.188f, 0.188f, 0.204f, 1.0f};
        pressedBgColor_ = theme::ColorTokens::SurfaceDefault;
        idleBorderColor_ = theme::ColorTokens::BorderDefault;
        hoverBorderColor_ = theme::ColorTokens::BorderStrong;
        pressedBorderColor_ = theme::ColorTokens::BorderDefault;
        idleTextColor_ = theme::ColorTokens::TextPrimary;
        hoverTextColor_ = theme::ColorTokens::TextPrimary;
        pressedTextColor_ = theme::ColorTokens::TextSecondary;
        borderWidth_ = theme::Metrics::BorderWidthHairline;
        break;
    case ButtonVariant::Ghost:
        idleBgColor_ = core::Color::transparent();
        hoverBgColor_ = core::Color{1.0f, 1.0f, 1.0f, 0.06f};
        pressedBgColor_ = core::Color{1.0f, 1.0f, 1.0f, 0.10f};
        idleBorderColor_ = core::Color::transparent();
        hoverBorderColor_ = core::Color::transparent();
        pressedBorderColor_ = core::Color::transparent();
        idleTextColor_ = theme::ColorTokens::TextPrimary;
        hoverTextColor_ = theme::ColorTokens::TextPrimary;
        pressedTextColor_ = theme::ColorTokens::TextSecondary;
        borderWidth_ = 0.0f;
        break;
    case ButtonVariant::Destructive:
        idleBgColor_ = theme::ColorTokens::Danger;
        hoverBgColor_ = core::Color::fromRgba8(220, 38, 38, 255);
        pressedBgColor_ = core::Color::fromRgba8(185, 28, 28, 255);
        idleBorderColor_ = core::Color::transparent();
        hoverBorderColor_ = core::Color::transparent();
        pressedBorderColor_ = core::Color::transparent();
        idleTextColor_ = core::Color::white();
        hoverTextColor_ = core::Color::white();
        pressedTextColor_ = core::Color::white();
        borderWidth_ = 0.0f;
        break;
    }
}

void Button::setText(std::string text) {
    if (text_ != text) {
        text_ = std::move(text);
        markLayoutDirty();
    }
}

void Button::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        markLayoutDirty();
    }
}

ButtonState Button::state() const noexcept {
    if (isPressed_) {
        return ButtonState::Pressed;
    }
    if (isHovered_) {
        return ButtonState::Hovered;
    }
    return ButtonState::Idle;
}

void Button::setIdleBackgroundColor(const core::Color& color) noexcept {
    idleBgColor_ = color;
    markPaintDirty();
}

void Button::setHoverBackgroundColor(const core::Color& color) noexcept {
    hoverBgColor_ = color;
    markPaintDirty();
}

void Button::setPressedBackgroundColor(const core::Color& color) noexcept {
    pressedBgColor_ = color;
    markPaintDirty();
}

void Button::setIdleBorderColor(const core::Color& color) noexcept {
    idleBorderColor_ = color;
    markPaintDirty();
}

void Button::setHoverBorderColor(const core::Color& color) noexcept {
    hoverBorderColor_ = color;
    markPaintDirty();
}

void Button::setPressedBorderColor(const core::Color& color) noexcept {
    pressedBorderColor_ = color;
    markPaintDirty();
}

void Button::setIdleTextColor(const core::Color& color) noexcept {
    idleTextColor_ = color;
    markPaintDirty();
}

void Button::setHoverTextColor(const core::Color& color) noexcept {
    hoverTextColor_ = color;
    markPaintDirty();
}

void Button::setPressedTextColor(const core::Color& color) noexcept {
    pressedTextColor_ = color;
    markPaintDirty();
}

void Button::setBorderWidth(float width) noexcept {
    borderWidth_ = width;
    markPaintDirty();
}

void Button::setCornerRadius(float radius) noexcept {
    cornerRadii_[0] = radius;
    cornerRadii_[1] = radius;
    cornerRadii_[2] = radius;
    cornerRadii_[3] = radius;
    markPaintDirty();
}

void Button::setCornerRadii(float r0, float r1, float r2, float r3) noexcept {
    cornerRadii_[0] = r0;
    cornerRadii_[1] = r1;
    cornerRadii_[2] = r2;
    cornerRadii_[3] = r3;
    markPaintDirty();
}

void Button::onMouseEnter() {
    isHovered_ = true;
    markPaintDirty();
}

void Button::onMouseLeave() {
    isHovered_ = false;
    isPressed_ = false;
    markPaintDirty();
}

void Button::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        isPressed_ = true;
        ev.accept();
        markPaintDirty();
    }
}

void Button::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left && isPressed_) {
        isPressed_ = false;
        ev.accept();
        markPaintDirty();
        onClicked.emit();
    }
}

void Button::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    ButtonState st = state();
    const core::Color& bg = (st == ButtonState::Pressed)   ? pressedBgColor_
                            : (st == ButtonState::Hovered) ? hoverBgColor_
                                                           : idleBgColor_;
    const core::Color& border = (st == ButtonState::Pressed)   ? pressedBorderColor_
                                : (st == ButtonState::Hovered) ? hoverBorderColor_
                                                               : idleBorderColor_;
    const core::Color& textColor = (st == ButtonState::Pressed)   ? pressedTextColor_
                                   : (st == ButtonState::Hovered) ? hoverTextColor_
                                                                  : idleTextColor_;

    ctx.drawBox(rect, bg, border, borderWidth_, cornerRadii_);

    if (hasFocus()) {
        theme::paintFocusRing(ctx, rect, cornerRadii_[0]);
    }

    if (font_ && !text_.empty() && textColor.a > 0.0f) {
        auto textLayout =
            text::TextLayout::create(text_, font_, rect.size.width, text::TextAlignment::Center);
        float textY = theme::computeCenteredTextY(rect.origin.y + rect.size.height * 0.5f, *font_);
        ctx.drawTextLayout(core::Point{rect.origin.x, textY}, textLayout, textColor);
    }
}

} // namespace northern::widgets
