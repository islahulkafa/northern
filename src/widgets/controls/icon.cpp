#include "widgets/controls/icon.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

Icon::Icon(IconId iconId, float size, std::optional<core::Color> color)
    : iconId_(iconId), doc_(getSvgDocument(iconId)), overrideColor_(color), iconSize_(size) {
    style().width = layout::Length::points(size);
    style().height = layout::Length::points(size);
}

Icon::Icon(IconId iconId, IconSize size, IconRole role)
    : Icon(iconId, toPoints(size), std::nullopt) {
    role_ = role;
}

Icon::Icon(std::shared_ptr<renderer::SvgDocument> doc, float size, std::optional<core::Color> color)
    : doc_(std::move(doc)), overrideColor_(color), iconSize_(size) {
    style().width = layout::Length::points(size);
    style().height = layout::Length::points(size);
}

Icon::Icon(std::shared_ptr<renderer::SvgDocument> doc, IconSize size, IconRole role)
    : Icon(std::move(doc), toPoints(size), std::nullopt) {
    role_ = role;
}

Icon::Icon(std::string_view svgContent, float size, std::optional<core::Color> color)
    : overrideColor_(color), iconSize_(size) {
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
    if (iconSize_ != size) {
        iconSize_ = size;
        style().width = layout::Length::points(size);
        style().height = layout::Length::points(size);
        markLayoutDirty();
    }
}

void Icon::setInteractive(bool interactive) noexcept {
    if (isInteractive_ != interactive) {
        isInteractive_ = interactive;
        setFocusable(interactive);
        setCursorShape(interactive ? platform::CursorShape::PointingHand
                                   : platform::CursorShape::Arrow);
        if (!interactive) {
            isHovered_ = false;
            isPressed_ = false;
            hoverAnim_.start(hoverAnim_.currentValue(), 0.0f, 0.12f,
                             core::animation::EasingCurve::OutCubic);
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

void Icon::onMouseDown(WidgetMouseEvent& ev) {
    if (!isInteractive_ || !isEnabled()) {
        return;
    }
    if (ev.button() == platform::MouseButton::Left) {
        isPressed_ = true;
        setFocus(FocusReason::Pointer);
        ev.accept();
        markPaintDirty();
    }
}

void Icon::onMouseUp(WidgetMouseEvent& ev) {
    if (!isInteractive_ || !isEnabled()) {
        return;
    }
    if (ev.button() == platform::MouseButton::Left && isPressed_) {
        isPressed_ = false;
        ev.accept();
        markPaintDirty();
        onClicked.emit();
    }
}

void Icon::onKeyDown(WidgetKeyEvent& ev) {
    if (!isInteractive_ || !isEnabled()) {
        return;
    }
    if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        ev.accept();
        onClicked.emit();
    }
}

void Icon::updateAnimations(float deltaSeconds) {
    hoverAnim_.update(deltaSeconds);
    markPaintDirty();
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

    float hoverVal = hoverAnim_.currentValue();

    if (isInteractive_ && hoverVal > 0.001f && isEnabled()) {
        const auto& pal = colorPalette();
        core::Color backdrop = pal.borderSubtle.withAlpha(pal.borderSubtle.a * hoverVal * 1.5f);
        ctx.drawRoundedRect(sRect, backdrop, 4.0f);
    }

    if (isInteractive_ && hasKeyboardFocus()) {
        theme::paintFocusRing(ctx, sRect, 4.0f);
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
