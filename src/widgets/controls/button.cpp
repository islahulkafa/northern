#include "widgets/controls/button.hpp"
#include "renderer/paint_context.hpp"
#include "renderer/svg/svg_document.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

Button::Button(std::string text, std::shared_ptr<text::Font> font, ButtonVariant variant,
               ButtonSize size)
    : text_(std::move(text)), font_(std::move(font)), variant_(variant), size_(size) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::PointingHand);

    applySizeMetrics();
    applyVariantStyle();

    layoutNode_.setMeasureFunc([this](const layout::LayoutNode&, float availW,
                                      float) -> core::Size {
        float textW = 0.0f;
        float textH = 0.0f;
        if (font_ && !text_.empty()) {
            if (isTextLayoutDirty_ || !cachedTextLayout_) {
                cachedTextLayout_ = text::TextLayout::create(text_, font_);
                isTextLayoutDirty_ = false;
            }
            textW = cachedTextLayout_->size().width;
            textH = cachedTextLayout_->size().height;
        }

        bool hasIcon = (icon_ && icon_->isValid());
        float contentW = textW;
        if (hasIcon) {
            if (text_.empty()) {
                contentW = iconSize_;
            } else {
                contentW += iconSize_ + iconGap_;
            }
        }

        float padH = style().padding.left.resolve(availW, 0.0f) +
                     style().padding.right.resolve(availW, 0.0f);
        float h = style().height.resolve(0.0f, 0.0f);
        if (h <= 0.0f) {
            float padV = style().padding.top.resolve(0.0f, 0.0f) +
                         style().padding.bottom.resolve(0.0f, 0.0f);
            h = std::max(textH, hasIcon ? iconSize_ : 0.0f) + padV;
        }

        float w = (text_.empty() && hasIcon) ? h : (contentW + padH);
        return core::Size{w, h};
    });
}

void Button::setSize(ButtonSize size) noexcept {
    if (size_ != size) {
        size_ = size;
        applySizeMetrics();
    }
}

void Button::applySizeMetrics() noexcept {
    float h = theme::Metrics::ControlHeightStandard;
    float padH = 12.0f;
    float defIcon = 16.0f;
    float radius = theme::Metrics::CornerRadiusControl;

    switch (size_) {
    case ButtonSize::Compact:
        h = theme::Metrics::ControlHeightCompact;
        padH = 8.0f;
        defIcon = 14.0f;
        radius = 4.0f;
        break;
    case ButtonSize::Standard:
        h = theme::Metrics::ControlHeightStandard;
        padH = 12.0f;
        defIcon = 16.0f;
        radius = theme::Metrics::CornerRadiusControl;
        break;
    case ButtonSize::Comfortable:
        h = theme::Metrics::ControlHeightComfortable;
        padH = 16.0f;
        defIcon = 18.0f;
        radius = theme::Metrics::CornerRadiusControl;
        break;
    }

    style().height = layout::Length::points(h);
    style().padding = layout::RectLength{
        .left = layout::Length::points(padH),
        .top = layout::Length::points(0.0f),
        .right = layout::Length::points(padH),
        .bottom = layout::Length::points(0.0f),
    };
    if (iconSize_ <= 0.0f || iconSize_ == 14.0f || iconSize_ == 16.0f || iconSize_ == 18.0f) {
        iconSize_ = defIcon;
    }
    setCornerRadius(radius);
    markLayoutDirty();
}

void Button::setIcon(std::shared_ptr<renderer::SvgDocument> icon, IconPlacement placement,
                     float iconSize) noexcept {
    icon_ = std::move(icon);
    iconPlacement_ = placement;
    if (iconSize > 0.0f) {
        iconSize_ = iconSize;
    }
    markLayoutDirty();
    markPaintDirty();
}

void Button::setIconSize(float size) noexcept {
    if (iconSize_ != size) {
        iconSize_ = size;
        markLayoutDirty();
        markPaintDirty();
    }
}

void Button::setIconGap(float gap) noexcept {
    if (iconGap_ != gap) {
        iconGap_ = gap;
        markLayoutDirty();
        markPaintDirty();
    }
}

void Button::setVariant(ButtonVariant variant) noexcept {
    if (variant_ != variant) {
        variant_ = variant;
        customBg_ = false;
        customBorder_ = false;
        customText_ = false;
        applyVariantStyle();
        markPaintDirty();
    }
}

void Button::applyVariantStyle() noexcept {
    borderWidth_ =
        (variant_ == ButtonVariant::Secondary) ? theme::Metrics::BorderWidthHairline : 0.0f;
    updateTargetColors(false);
}

core::Color Button::effectiveIdleBackgroundColor() const noexcept {
    if (customBg_) {
        return idleBgColor_;
    }
    const auto& pal = colorPalette();
    switch (variant_) {
    case ButtonVariant::Primary:
        return pal.accentDefault;
    case ButtonVariant::Secondary:
        return pal.surfaceRaised;
    case ButtonVariant::Ghost:
        return core::Color::transparent();
    case ButtonVariant::Destructive:
        return pal.danger;
    }
    return pal.surfaceRaised;
}

core::Color Button::effectiveHoverBackgroundColor() const noexcept {
    if (customBg_) {
        return hoverBgColor_;
    }
    const auto& pal = colorPalette();
    switch (variant_) {
    case ButtonVariant::Primary:
        return pal.accentHover;
    case ButtonVariant::Secondary:
        return pal.mode == theme::ThemeMode::Dark ? core::Color{0.188f, 0.188f, 0.204f, 1.0f}
                                                  : core::Color{0.90f, 0.90f, 0.92f, 1.0f};
    case ButtonVariant::Ghost:
        return pal.mode == theme::ThemeMode::Dark ? core::Color{1.0f, 1.0f, 1.0f, 0.06f}
                                                  : core::Color{0.0f, 0.0f, 0.0f, 0.06f};
    case ButtonVariant::Destructive:
        return pal.mode == theme::ThemeMode::Dark ? core::Color::fromRgba8(220, 38, 38, 255)
                                                  : core::Color::fromRgba8(239, 68, 68, 255);
    }
    return pal.surfaceRaised;
}

core::Color Button::effectivePressedBackgroundColor() const noexcept {
    if (customBg_) {
        return pressedBgColor_;
    }
    const auto& pal = colorPalette();
    switch (variant_) {
    case ButtonVariant::Primary:
        return pal.accentActive;
    case ButtonVariant::Secondary:
        return pal.surfaceDefault;
    case ButtonVariant::Ghost:
        return pal.mode == theme::ThemeMode::Dark ? core::Color{1.0f, 1.0f, 1.0f, 0.10f}
                                                  : core::Color{0.0f, 0.0f, 0.0f, 0.10f};
    case ButtonVariant::Destructive:
        return core::Color::fromRgba8(185, 28, 28, 255);
    }
    return pal.surfaceDefault;
}

core::Color Button::effectiveIdleBorderColor() const noexcept {
    if (customBorder_) {
        return idleBorderColor_;
    }
    if (variant_ == ButtonVariant::Secondary) {
        return colorPalette().borderDefault;
    }
    return core::Color::transparent();
}

core::Color Button::effectiveHoverBorderColor() const noexcept {
    if (customBorder_) {
        return hoverBorderColor_;
    }
    if (variant_ == ButtonVariant::Secondary) {
        return colorPalette().borderStrong;
    }
    return core::Color::transparent();
}

core::Color Button::effectivePressedBorderColor() const noexcept {
    if (customBorder_) {
        return pressedBorderColor_;
    }
    if (variant_ == ButtonVariant::Secondary) {
        return colorPalette().borderDefault;
    }
    return core::Color::transparent();
}

core::Color Button::effectiveIdleTextColor() const noexcept {
    if (customText_) {
        return idleTextColor_;
    }
    const auto& pal = colorPalette();
    switch (variant_) {
    case ButtonVariant::Primary:
    case ButtonVariant::Destructive:
        return core::Color::white();
    case ButtonVariant::Secondary:
    case ButtonVariant::Ghost:
        return pal.textPrimary;
    }
    return pal.textPrimary;
}

core::Color Button::effectiveHoverTextColor() const noexcept {
    if (customText_) {
        return hoverTextColor_;
    }
    return effectiveIdleTextColor();
}

core::Color Button::effectivePressedTextColor() const noexcept {
    if (customText_) {
        return pressedTextColor_;
    }
    const auto& pal = colorPalette();
    switch (variant_) {
    case ButtonVariant::Primary:
    case ButtonVariant::Destructive:
        return core::Color::white();
    case ButtonVariant::Secondary:
    case ButtonVariant::Ghost:
        return pal.textSecondary;
    }
    return pal.textSecondary;
}

void Button::setText(std::string text) {
    if (text_ != text) {
        text_ = std::move(text);
        isTextLayoutDirty_ = true;
        markLayoutDirty();
    }
}

void Button::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        isTextLayoutDirty_ = true;
        markLayoutDirty();
    }
}

ButtonState Button::state() const noexcept {
    if (!isEnabled()) {
        return ButtonState::Disabled;
    }
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
    customBg_ = true;
    markPaintDirty();
}

void Button::setHoverBackgroundColor(const core::Color& color) noexcept {
    hoverBgColor_ = color;
    customBg_ = true;
    markPaintDirty();
}

void Button::setPressedBackgroundColor(const core::Color& color) noexcept {
    pressedBgColor_ = color;
    customBg_ = true;
    markPaintDirty();
}

void Button::setIdleBorderColor(const core::Color& color) noexcept {
    idleBorderColor_ = color;
    customBorder_ = true;
    markPaintDirty();
}

void Button::setHoverBorderColor(const core::Color& color) noexcept {
    hoverBorderColor_ = color;
    customBorder_ = true;
    markPaintDirty();
}

void Button::setPressedBorderColor(const core::Color& color) noexcept {
    pressedBorderColor_ = color;
    customBorder_ = true;
    markPaintDirty();
}

void Button::setIdleTextColor(const core::Color& color) noexcept {
    idleTextColor_ = color;
    customText_ = true;
    markPaintDirty();
}

void Button::setHoverTextColor(const core::Color& color) noexcept {
    hoverTextColor_ = color;
    customText_ = true;
    markPaintDirty();
}

void Button::setPressedTextColor(const core::Color& color) noexcept {
    pressedTextColor_ = color;
    customText_ = true;
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

void Button::updateTargetColors(bool animated) {
    ButtonState st = state();
    core::Color targetBg;
    core::Color targetBorder;

    if (st == ButtonState::Disabled) {
        targetBg = effectiveIdleBackgroundColor().withAlpha(0.35f);
        targetBorder = effectiveIdleBorderColor().withAlpha(0.20f);
    } else {
        targetBg = (st == ButtonState::Pressed)   ? effectivePressedBackgroundColor()
                   : (st == ButtonState::Hovered) ? effectiveHoverBackgroundColor()
                                                  : effectiveIdleBackgroundColor();
        targetBorder = (st == ButtonState::Pressed)   ? effectivePressedBorderColor()
                       : (st == ButtonState::Hovered) ? effectiveHoverBorderColor()
                                                      : effectiveIdleBorderColor();
    }

    if (animated && isEnabled()) {
        bgAnim_.start(bgAnim_.currentValue(), targetBg, 0.12f,
                      core::animation::EasingCurve::OutCubic);
        borderAnim_.start(borderAnim_.currentValue(), targetBorder, 0.12f,
                          core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
    } else {
        bgAnim_.setCurrentValue(targetBg);
        borderAnim_.setCurrentValue(targetBorder);
    }
    markPaintDirty();
}

void Button::updateAnimations(float deltaSeconds) {
    bgAnim_.update(deltaSeconds);
    borderAnim_.update(deltaSeconds);
    markPaintDirty();
}

bool Button::hasActiveAnimations() const noexcept {
    return bgAnim_.isRunning() || borderAnim_.isRunning();
}

void Button::onMouseEnter() {
    if (!isEnabled()) {
        return;
    }
    isHovered_ = true;
    updateTargetColors(true);
}

void Button::onMouseLeave() {
    if (!isEnabled()) {
        return;
    }
    isHovered_ = false;
    updateTargetColors(true);
}

void Button::onMouseMove(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (isPressed_) {
        core::Rect rect = layoutRect();
        bool inside = ev.position().x >= 0.0f && ev.position().x <= rect.size.width &&
                      ev.position().y >= 0.0f && ev.position().y <= rect.size.height;
        if (inside != isHovered_) {
            isHovered_ = inside;
            updateTargetColors(true);
        }
        ev.accept();
    }
}

void Button::onMouseDown(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.button() == platform::MouseButton::Left) {
        isPressed_ = true;
        isHovered_ = true;
        ev.accept();
        updateTargetColors(true);
    }
}

void Button::onMouseUp(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.button() == platform::MouseButton::Left && isPressed_) {
        core::Rect rect = layoutRect();
        bool inside = ev.position().x >= 0.0f && ev.position().x <= rect.size.width &&
                      ev.position().y >= 0.0f && ev.position().y <= rect.size.height;
        isPressed_ = false;
        ev.accept();
        updateTargetColors(true);
        if (inside) {
            onClicked.emit();
        }
    }
}

void Button::onKeyDown(WidgetKeyEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        ev.accept();
        onClicked.emit();
    }
}

void Button::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    ButtonState st = state();
    core::Color bg;
    core::Color border;
    core::Color textColor;

    if (st == ButtonState::Disabled) {
        bg = effectiveIdleBackgroundColor().withAlpha(0.35f);
        border = effectiveIdleBorderColor().withAlpha(0.20f);
        textColor = colorPalette().textDisabled;
    } else {
        bg = bgAnim_.isRunning() ? bgAnim_.currentValue()
             : ((st == ButtonState::Pressed)   ? effectivePressedBackgroundColor()
                : (st == ButtonState::Hovered) ? effectiveHoverBackgroundColor()
                                               : effectiveIdleBackgroundColor());
        border = borderAnim_.isRunning() ? borderAnim_.currentValue()
                 : ((st == ButtonState::Pressed)   ? effectivePressedBorderColor()
                    : (st == ButtonState::Hovered) ? effectiveHoverBorderColor()
                                                   : effectiveIdleBorderColor());
        textColor = (st == ButtonState::Pressed)   ? effectivePressedTextColor()
                    : (st == ButtonState::Hovered) ? effectiveHoverTextColor()
                                                   : effectiveIdleTextColor();
    }

    ctx.drawBox(rect, bg, border, borderWidth_, cornerRadii_);

    if (hasKeyboardFocus() && isEnabled()) {
        theme::paintFocusRing(ctx, rect, cornerRadii_[0]);
    }

    bool hasIcon = (icon_ && icon_->isValid());
    bool hasText = (font_ && !text_.empty() && textColor.a > 0.0f);

    if (hasIcon && !hasText) {
        // Icon-only centered
        float s = std::min(iconSize_, std::min(rect.size.width, rect.size.height));
        float ox = rect.origin.x + (rect.size.width - s) * 0.5f;
        float oy = rect.origin.y + (rect.size.height - s) * 0.5f;
        ctx.drawSvg(*icon_, core::Rect{{ox, oy}, {s, s}}, textColor);
    } else if (hasIcon && hasText) {
        // Icon + Text layout
        if (isTextLayoutDirty_ || !cachedTextLayout_) {
            cachedTextLayout_ = text::TextLayout::create(text_, font_);
            isTextLayoutDirty_ = false;
        }
        float textW = cachedTextLayout_->size().width;
        float totalW = iconSize_ + iconGap_ + textW;
        float startX = rect.origin.x + std::max(0.0f, (rect.size.width - totalW) * 0.5f);
        float iconY = rect.origin.y + (rect.size.height - iconSize_) * 0.5f;
        float textY = theme::computeCenteredTextY(rect.origin.y + rect.size.height * 0.5f, *font_);

        if (iconPlacement_ == IconPlacement::Leading) {
            ctx.drawSvg(*icon_, core::Rect{{startX, iconY}, {iconSize_, iconSize_}}, textColor);
            ctx.drawTextLayout(core::Point{startX + iconSize_ + iconGap_, textY},
                               *cachedTextLayout_, textColor);
        } else {
            ctx.drawTextLayout(core::Point{startX, textY}, *cachedTextLayout_, textColor);
            ctx.drawSvg(*icon_,
                        core::Rect{{startX + textW + iconGap_, iconY}, {iconSize_, iconSize_}},
                        textColor);
        }
    } else if (hasText) {
        // Text-only centered
        if (isTextLayoutDirty_ || !cachedTextLayout_) {
            cachedTextLayout_ = text::TextLayout::create(text_, font_);
            isTextLayoutDirty_ = false;
        }
        float textW = cachedTextLayout_->size().width;
        float textX = rect.origin.x + std::max(0.0f, (rect.size.width - textW) * 0.5f);
        float textY = theme::computeCenteredTextY(rect.origin.y + rect.size.height * 0.5f, *font_);
        ctx.drawTextLayout(core::Point{textX, textY}, *cachedTextLayout_, textColor);
    }
}

} // namespace northern::widgets
