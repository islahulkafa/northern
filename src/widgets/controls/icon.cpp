#include "widgets/controls/icon.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

Icon::Icon(IconId iconId, float size, std::optional<core::Color> color)
    : iconId_(iconId), doc_(getSvgDocument(iconId)), overrideColor_(color), baseIconSize_(size),
      iconSize_(size) {
    style().width = layout::Length::points(size);
    style().height = layout::Length::points(size);
}

Icon::Icon(IconId iconId, IconSize size, IconRole role)
    : Icon(iconId, toPoints(size), std::nullopt) {
    role_ = role;
}

Icon::Icon(std::shared_ptr<renderer::SvgDocument> doc, float size, std::optional<core::Color> color)
    : doc_(std::move(doc)), overrideColor_(color), baseIconSize_(size), iconSize_(size) {
    style().width = layout::Length::points(size);
    style().height = layout::Length::points(size);
}

Icon::Icon(std::shared_ptr<renderer::SvgDocument> doc, IconSize size, IconRole role)
    : Icon(std::move(doc), toPoints(size), std::nullopt) {
    role_ = role;
}

Icon::Icon(std::string_view svgContent, float size, std::optional<core::Color> color)
    : overrideColor_(color), baseIconSize_(size), iconSize_(size) {
    auto res = renderer::SvgDocument::fromString(svgContent);
    if (res.isOk()) {
        doc_ = res.value();
    }
    style().width = layout::Length::points(size);
    style().height = layout::Length::points(size);
}

Icon::Icon(std::string_view svgContent, IconSize size, IconRole role)
    : Icon(svgContent, toPoints(size), std::nullopt) {
    role_ = role;
}

void Icon::setIcon(IconId iconId) {
    if (iconId_ != iconId) {
        iconId_ = iconId;
        doc_ = getSvgDocument(iconId);
        markPaintDirty();
    }
}

void Icon::setSvgDocument(std::shared_ptr<renderer::SvgDocument> doc) {
    iconId_.reset();
    doc_ = std::move(doc);
    markPaintDirty();
}

void Icon::setSvgContent(std::string_view svgContent) {
    iconId_.reset();
    auto res = renderer::SvgDocument::fromString(svgContent);
    doc_ = res.isOk() ? res.value() : nullptr;
    markPaintDirty();
}

void Icon::setRole(IconRole role) noexcept {
    if (role_ != role) {
        role_ = role;
        markPaintDirty();
    }
}

void Icon::setColor(core::Color color) {
    overrideColor_ = color;
    markPaintDirty();
}

core::Color Icon::effectiveColor() const noexcept {
    if (overrideColor_) {
        return *overrideColor_;
    }
    const auto& pal = colorPalette();
    switch (role_) {
    case IconRole::Primary:
        return pal.textPrimary;
    case IconRole::Secondary:
        return pal.textSecondary;
    case IconRole::Tertiary:
        return pal.textTertiary;
    case IconRole::Disabled:
        return pal.textDisabled;
    case IconRole::Accent:
        return pal.accentDefault;
    case IconRole::Success:
        return pal.success;
    case IconRole::Warning:
        return pal.warning;
    case IconRole::Danger:
        return pal.danger;
    case IconRole::Info:
        return pal.info;
    }
    return pal.textPrimary;
}

void Icon::setIconSize(float size) {
    hasCustomSize_ = true;
    if (iconSize_ != size) {
        iconSize_ = size;
        style().width = layout::Length::points(size);
        style().height = layout::Length::points(size);
        markLayoutDirty();
        markPaintDirty();
    }
}

void Icon::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiScale(scale);
}

void Icon::applyDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        if (!hasCustomSize_) {
            iconSize_ = baseIconSize_ * dpiScale_;
            style().width = layout::Length::points(iconSize_);
            style().height = layout::Length::points(iconSize_);
            markLayoutDirty();
        }
        markPaintDirty();
    }
}

void Icon::cancelActiveInteraction() {
    if (isPressed_ && context_ && context_->mouseGrabWidget() == this) {
        context_->releaseMouseGrab();
    }
    isPressed_ = false;
    isHovered_ = false;
    hoverAnim_.stop();
    hoverAnim_.setCurrentValue(0.0f);
}

void Icon::setInteractive(bool interactive) noexcept {
    if (isInteractive_ != interactive) {
        isInteractive_ = interactive;
        setFocusable(interactive);
        setCursorShape((interactive && isEnabled()) ? platform::CursorShape::PointingHand
                                                    : platform::CursorShape::Arrow);
        if (!interactive) {
            cancelActiveInteraction();
        }
        markPaintDirty();
    }
}

void Icon::onMouseEnter() {
    if (!isInteractive_ || !isEnabled()) {
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

void Icon::onMouseLeave() {
    if (!isInteractive_) {
        return;
    }
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

void Icon::onMouseDown(WidgetMouseEvent& ev) {
    if (!isInteractive_ || !isEnabled()) {
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

void Icon::onMouseUp(WidgetMouseEvent& ev) {
    if (!isInteractive_ || !isEnabled()) {
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
            onClicked.emit();
        }
    }
}

void Icon::onKeyDown(WidgetKeyEvent& ev) {
    if (!isInteractive_ || !isEnabled() || !ev.isDown()) {
        return;
    }
    if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        ev.accept();
        onClicked.emit();
    }
}

void Icon::onFocusGained() {
    markPaintDirty();
}

void Icon::onFocusLost() {
    cancelActiveInteraction();
    markPaintDirty();
}

void Icon::onContextAttached() {
    if (!hasCustomDpiScale_) {
        applyDpiScale(contextDpiScale());
    }
}

void Icon::onThemeChanged() {
    markPaintDirty();
}

void Icon::onEnabledChanged() {
    if (!isEnabled()) {
        cancelActiveInteraction();
        setCursorShape(platform::CursorShape::Arrow);
    } else {
        setCursorShape(isInteractive_ ? platform::CursorShape::PointingHand
                                      : platform::CursorShape::Arrow);
    }
    markPaintDirty();
}

void Icon::updateAnimations(float deltaSeconds) {
    if (hoverAnim_.isRunning()) {
        hoverAnim_.update(deltaSeconds);
        markPaintDirty();
    }
}

bool Icon::hasActiveAnimations() const noexcept {
    return hoverAnim_.isRunning();
}

void Icon::paint(renderer::PaintContext& ctx) {
    if (!doc_ || !doc_->isValid()) {
        return;
    }

    core::Rect sRect = screenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return;
    }

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float hoverVal = hoverAnim_.currentValue();
    float cornerRadius = theme::Metrics::CornerRadiusControl * scale;

    if (isInteractive_ && isEnabled() && (hoverVal > 0.001f || isPressed_)) {
        const auto& pal = colorPalette();
        float alpha = isPressed_ ? 0.85f : (0.55f * hoverVal);
        ctx.drawRoundedRect(sRect, pal.surfaceRaised.withAlpha(alpha), cornerRadius);
    }

    if (isInteractive_ && isEnabled() && hasKeyboardFocus()) {
        paintFocusRing(ctx, sRect, cornerRadius);
    }

    float s = std::min(iconSize_, std::min(sRect.size.width, sRect.size.height));
    if (s <= 0.0f) {
        return;
    }

    float ox = sRect.origin.x + (sRect.size.width - s) * 0.5f;
    float oy = sRect.origin.y + (sRect.size.height - s) * 0.5f;
    core::Rect targetRect{core::Point{ox, oy}, core::Size{s, s}};

    core::Color iconCol = effectiveColor();
    if (isInteractive_) {
        float alphaMult = isEnabled() ? (0.80f + 0.20f * hoverVal) : 0.40f;
        if (isPressed_) {
            alphaMult = 0.65f;
        }
        iconCol = iconCol.withAlpha(iconCol.a * alphaMult);
    } else if (!isEnabled()) {
        iconCol = iconCol.withAlpha(iconCol.a * 0.40f);
    }

    ctx.drawSvg(*doc_, targetRect, iconCol);
}

} // namespace northern::widgets
