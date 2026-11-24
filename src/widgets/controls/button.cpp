#include "widgets/controls/button.hpp"
#include "renderer/paint_context.hpp"
#include "renderer/svg/svg_document.hpp"
#include "text/font_manager.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/icons.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace northern::widgets {

namespace {

constexpr float defaultPointSizeForSize(ButtonSize s) noexcept {
    switch (s) {
    case ButtonSize::Compact:
        return 9.5f;
    case ButtonSize::Standard:
        return 10.5f;
    case ButtonSize::Comfortable:
        return 11.5f;
    }
    return 10.5f;
}

} // namespace

Button::Button(std::string text, std::shared_ptr<text::Font> font, ButtonVariant variant,
               ButtonSize size)
    : text_(std::move(text)), font_(std::move(font)), variant_(variant), size_(size) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::PointingHand);

    applySizeMetrics();
    applyVariantStyle();

    layoutNode_.setMeasureFunc(
        [this](const layout::LayoutNode&, float availW, float) -> core::Size {
            auto f = effectiveFont();
            float textW = 0.0f;
            float textH = 0.0f;
            if (f && !text_.empty()) {
                if (isTextLayoutDirty_ || !cachedTextLayout_) {
                    cachedTextLayout_ = text::TextLayout::create(text_, f);
                    isTextLayoutDirty_ = false;
                }
                textW = cachedTextLayout_->size().width;
                textH = cachedTextLayout_->size().height;
            }

            bool hasIcon = (icon_ && icon_->isValid());
            bool showSpinner = isLoading_;
            bool hasIndicator = showSpinner || hasIcon;
            float contentW = textW;
            if (hasIndicator) {
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
                h = std::max(textH, hasIndicator ? iconSize_ : 0.0f) + padV;
            }

            float w = (text_.empty() && hasIndicator) ? h : (contentW + padH);
            return core::Size{w, h};
        });
}

Button::Button(std::string text, IconId icon, ButtonVariant variant, ButtonSize size)
    : Button(std::move(text), nullptr, variant, size) {
    setIcon(icon);
}

Button::Button(IconId icon, ButtonVariant variant, ButtonSize size)
    : Button("", nullptr, variant, size) {
    setIcon(icon);
}

void Button::setSize(ButtonSize size) noexcept {
    if (size_ != size) {
        size_ = size;
        resolvedDefaultFont_.reset();
        isTextLayoutDirty_ = true;
        applySizeMetrics();
    }
}

void Button::setDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        resolvedDefaultFont_.reset();
        isTextLayoutDirty_ = true;
        applySizeMetrics();
        applyVariantStyle();
        markLayoutDirty();
        markPaintDirty();
    }
}

void Button::setLoading(bool loading) noexcept {
    if (isLoading_ != loading) {
        isLoading_ = loading;
        if (isLoading_) {
            spinnerAnim_.start(0.0f, 1.0f, 0.8f, core::animation::EasingCurve::Linear);
            if (context_) {
                context_->registerAnimatedWidget(this);
            }
        } else {
            spinnerAnim_.stop();
        }
        markLayoutDirty();
        markPaintDirty();
    }
}

std::shared_ptr<text::Font> Button::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
        float pt = defaultPointSizeForSize(size_) * scale;
        resolvedDefaultFont_ = text::FontManager::instance().defaultFont(pt).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void Button::applySizeMetrics() noexcept {
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float h = theme::Metrics::ControlHeightStandard * scale;
    float padH = 12.0f * scale;
    float defIcon = 16.0f * scale;
    float radius = theme::Metrics::CornerRadiusControl * scale;

    switch (size_) {
    case ButtonSize::Compact:
        h = theme::Metrics::ControlHeightCompact * scale;
        padH = 8.0f * scale;
        defIcon = 14.0f * scale;
        radius = 4.0f * scale;
        break;
    case ButtonSize::Standard:
        h = theme::Metrics::ControlHeightStandard * scale;
        padH = 12.0f * scale;
        defIcon = 16.0f * scale;
        radius = theme::Metrics::CornerRadiusControl * scale;
        break;
    case ButtonSize::Comfortable:
        h = theme::Metrics::ControlHeightComfortable * scale;
        padH = 16.0f * scale;
        defIcon = 18.0f * scale;
        radius = theme::Metrics::CornerRadiusControl * scale;
        break;
    }

    style().height = layout::Length::points(h);
    style().padding = layout::RectLength{
        .left = layout::Length::points(padH),
        .top = layout::Length::points(0.0f),
        .right = layout::Length::points(padH),
        .bottom = layout::Length::points(0.0f),
    };
    iconSize_ = defIcon;
    iconGap_ = 6.0f * scale;
    setCornerRadius(radius);
    markLayoutDirty();
}

void Button::setIcon(std::shared_ptr<renderer::SvgDocument> icon, IconPlacement placement,
                     float iconSize) noexcept {
    icon_ = std::move(icon);
    iconPlacement_ = placement;
    if (iconSize > 0.0f) {
        iconSize_ = iconSize * (dpiScale_ > 0.0f ? dpiScale_ : 1.0f);
    }
    markLayoutDirty();
    markPaintDirty();
}

void Button::setIcon(IconId icon, IconPlacement placement, float iconSize) noexcept {
    setIcon(getSvgDocument(icon), placement, iconSize);
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
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    borderWidth_ = (variant_ == ButtonVariant::Secondary || variant_ == ButtonVariant::Outline)
                       ? std::max(1.0f, std::round(theme::Metrics::BorderWidthHairline * scale))
                       : 0.0f;
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
    case ButtonVariant::Outline:
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
        return pal.surfaceOverlay;
    case ButtonVariant::Outline:
        return pal.mode == theme::ThemeMode::Dark ? core::Color{1.0f, 1.0f, 1.0f, 0.06f}
                                                  : core::Color{0.0f, 0.0f, 0.0f, 0.04f};
    case ButtonVariant::Ghost:
        return pal.mode == theme::ThemeMode::Dark ? core::Color{1.0f, 1.0f, 1.0f, 0.08f}
                                                  : core::Color{0.0f, 0.0f, 0.0f, 0.06f};
    case ButtonVariant::Destructive:
        return pal.danger.lerp(core::Color::black(), 0.15f);
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
    case ButtonVariant::Outline:
        return pal.mode == theme::ThemeMode::Dark ? core::Color{1.0f, 1.0f, 1.0f, 0.12f}
                                                  : core::Color{0.0f, 0.0f, 0.0f, 0.08f};
    case ButtonVariant::Ghost:
        return pal.mode == theme::ThemeMode::Dark ? core::Color{1.0f, 1.0f, 1.0f, 0.14f}
                                                  : core::Color{0.0f, 0.0f, 0.0f, 0.10f};
    case ButtonVariant::Destructive:
        return pal.danger.lerp(core::Color::black(), 0.25f);
    }
    return pal.surfaceDefault;
}

core::Color Button::effectiveIdleBorderColor() const noexcept {
    if (customBorder_) {
        return idleBorderColor_;
    }
    const auto& pal = colorPalette();
    if (variant_ == ButtonVariant::Secondary) {
        return pal.borderSubtle;
    }
    if (variant_ == ButtonVariant::Outline) {
        return pal.borderDefault;
    }
    return core::Color::transparent();
}

core::Color Button::effectiveHoverBorderColor() const noexcept {
    if (customBorder_) {
        return hoverBorderColor_;
    }
    const auto& pal = colorPalette();
    if (variant_ == ButtonVariant::Secondary || variant_ == ButtonVariant::Outline) {
        return pal.borderStrong;
    }
    return core::Color::transparent();
}

core::Color Button::effectivePressedBorderColor() const noexcept {
    if (customBorder_) {
        return pressedBorderColor_;
    }
    const auto& pal = colorPalette();
    if (variant_ == ButtonVariant::Secondary || variant_ == ButtonVariant::Outline) {
        return pal.borderDefault;
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
    case ButtonVariant::Outline:
        return pal.textPrimary;
    case ButtonVariant::Ghost:
        return pal.textSecondary;
    }
    return pal.textPrimary;
}

core::Color Button::effectiveHoverTextColor() const noexcept {
    if (customText_) {
        return hoverTextColor_;
    }
    const auto& pal = colorPalette();
    if (variant_ == ButtonVariant::Ghost) {
        return pal.textPrimary;
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
    case ButtonVariant::Outline:
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
        markPaintDirty();
    }
}

void Button::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedDefaultFont_.reset();
        isTextLayoutDirty_ = true;
        markLayoutDirty();
        markPaintDirty();
    }
}

ButtonState Button::state() const noexcept {
    if (!isEnabled()) {
        return ButtonState::Disabled;
    }
    if (isLoading_) {
        return ButtonState::Idle;
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

    if (!isEnabled()) {
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
    if (isLoading_) {
        spinnerAnim_.update(deltaSeconds);
        if (!spinnerAnim_.isRunning()) {
            spinnerAnim_.start(0.0f, 1.0f, 0.8f, core::animation::EasingCurve::Linear);
        }
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
        markPaintDirty();
    }
}

bool Button::hasActiveAnimations() const noexcept {
    return bgAnim_.isRunning() || borderAnim_.isRunning() ||
           (isLoading_ && spinnerAnim_.isRunning());
}

void Button::onMouseEnter() {
    if (!isEnabled() || isLoading_) {
        return;
    }
    isHovered_ = true;
    updateTargetColors(true);
}

void Button::onMouseLeave() {
    if (!isEnabled() || isLoading_) {
        return;
    }
    isHovered_ = false;
    updateTargetColors(true);
}

void Button::onMouseMove(WidgetMouseEvent& ev) {
    if (!isEnabled() || isLoading_) {
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
    if (!isEnabled() || isLoading_) {
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
    if (!isEnabled() || isLoading_) {
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
    if (!isEnabled() || isLoading_) {
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

    if (!isEnabled()) {
        bg = effectiveIdleBackgroundColor().withAlpha(0.35f);
        border = effectiveIdleBorderColor().withAlpha(0.20f);
        textColor = colorPalette().textDisabled;
    } else {
        bg = bgAnim_.isRunning()
                 ? bgAnim_.currentValue()
                 : ((st == ButtonState::Pressed)   ? effectivePressedBackgroundColor()
                    : (st == ButtonState::Hovered) ? effectiveHoverBackgroundColor()
                                                   : effectiveIdleBackgroundColor());
        border = borderAnim_.isRunning()
                     ? borderAnim_.currentValue()
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

    auto f = effectiveFont();
    bool hasIcon = (icon_ && icon_->isValid());
    bool showSpinner = isLoading_;
    bool hasIndicator = showSpinner || hasIcon;
    bool hasText = (f && !text_.empty() && textColor.a > 0.0f);

    auto drawSpinner = [&](const core::Rect& spinRect, const core::Color& col) {
        float startAngle = spinnerAnim_.currentValue() * 2.0f * std::numbers::pi_v<float>;
        float arcSpan = 1.5f * std::numbers::pi_v<float>;
        constexpr int kSegments = 10;
        float radius = spinRect.size.width * 0.38f;
        core::Point center{spinRect.origin.x + spinRect.size.width * 0.5f,
                           spinRect.origin.y + spinRect.size.height * 0.5f};
        float strokeW = std::max(1.5f, 1.75f * (dpiScale_ > 0.0f ? dpiScale_ : 1.0f));

        for (int seg = 0; seg < kSegments; ++seg) {
            float t0 = static_cast<float>(seg) / static_cast<float>(kSegments);
            float t1 = static_cast<float>(seg + 1) / static_cast<float>(kSegments);
            float a0 = startAngle + t0 * arcSpan;
            float a1 = startAngle + t1 * arcSpan;
            core::Point p0{center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius};
            core::Point p1{center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius};
            float segAlpha = 0.2f + 0.8f * t1;
            ctx.drawLine(p0, p1, col.withAlpha(col.a * segAlpha), strokeW);
        }
    };

    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;
    float indicatorY = std::round(targetCenterY - iconSize_ * 0.5f);
    float textY = f ? theme::computeCenteredTextY(targetCenterY, *f) : targetCenterY;

    if (hasIndicator && !hasText) {
        // Indicator-only centered
        float s = std::min(iconSize_, std::min(rect.size.width, rect.size.height));
        float ox = rect.origin.x + (rect.size.width - s) * 0.5f;
        float oy = std::round(targetCenterY - s * 0.5f);
        core::Rect indRect{{ox, oy}, {s, s}};
        if (showSpinner) {
            drawSpinner(indRect, textColor);
        } else if (hasIcon) {
            ctx.drawSvg(*icon_, indRect, textColor);
        }
    } else if (hasIndicator && hasText) {
        // Indicator + Text layout
        if (isTextLayoutDirty_ || !cachedTextLayout_) {
            cachedTextLayout_ = text::TextLayout::create(text_, f);
            isTextLayoutDirty_ = false;
        }
        float textW = cachedTextLayout_->size().width;
        float totalW = iconSize_ + iconGap_ + textW;
        float startX = rect.origin.x + std::max(0.0f, (rect.size.width - totalW) * 0.5f);

        if (showSpinner || iconPlacement_ == IconPlacement::Leading) {
            core::Rect indRect{{startX, indicatorY}, {iconSize_, iconSize_}};
            if (showSpinner) {
                drawSpinner(indRect, textColor);
            } else {
                ctx.drawSvg(*icon_, indRect, textColor);
            }
            ctx.drawTextLayout(core::Point{startX + iconSize_ + iconGap_, textY},
                               *cachedTextLayout_, textColor);
        } else {
            ctx.drawTextLayout(core::Point{startX, textY}, *cachedTextLayout_, textColor);
            ctx.drawSvg(*icon_,
                        core::Rect{{startX + textW + iconGap_, indicatorY}, {iconSize_, iconSize_}},
                        textColor);
        }
    } else if (hasText) {
        // Text-only centered
        if (isTextLayoutDirty_ || !cachedTextLayout_) {
            cachedTextLayout_ = text::TextLayout::create(text_, f);
            isTextLayoutDirty_ = false;
        }
        float textW = cachedTextLayout_->size().width;
        float textX = rect.origin.x + std::max(0.0f, (rect.size.width - textW) * 0.5f);
        ctx.drawTextLayout(core::Point{textX, textY}, *cachedTextLayout_, textColor);
    }
}

} // namespace northern::widgets
