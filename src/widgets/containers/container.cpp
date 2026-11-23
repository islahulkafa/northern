#include "widgets/containers/container.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

void Container::setSurface(SurfaceRole role) noexcept {
    surfaceRole_ = role;
    hasCustomBg_ = false;
    if (isInteractive_) {
        updateInteractiveTargetColors(false);
    }
    markPaintDirty();
}

void Container::setBorderRole(BorderRole role) noexcept {
    borderRole_ = role;
    hasCustomBorder_ = false;
    hasCustomBorderWidth_ = false;
    if (isInteractive_) {
        updateInteractiveTargetColors(false);
    }
    markPaintDirty();
}

void Container::setBackgroundColor(const core::Color& color) noexcept {
    backgroundColor_ = color;
    hasCustomBg_ = true;
    if (isInteractive_) {
        updateInteractiveTargetColors(false);
    }
    markPaintDirty();
}

core::Color Container::effectiveBackgroundColor() const noexcept {
    if (hasCustomBg_) {
        return backgroundColor_;
    }
    const auto& pal = colorPalette();
    switch (surfaceRole_) {
    case SurfaceRole::Canvas:
        return pal.surfaceCanvas;
    case SurfaceRole::Default:
        return pal.surfaceDefault;
    case SurfaceRole::Raised:
        return pal.surfaceRaised;
    case SurfaceRole::Overlay:
        return pal.surfaceOverlay;
    case SurfaceRole::None:
        return core::Color::transparent();
    }
    return core::Color::transparent();
}

void Container::setBorderColor(const core::Color& color) noexcept {
    borderColor_ = color;
    hasCustomBorder_ = true;
    if (isInteractive_) {
        updateInteractiveTargetColors(false);
    }
    markPaintDirty();
}

core::Color Container::effectiveBorderColor() const noexcept {
    if (hasCustomBorder_) {
        return borderColor_;
    }
    const auto& pal = colorPalette();
    switch (borderRole_) {
    case BorderRole::Subtle:
        return pal.borderSubtle;
    case BorderRole::Default:
        return pal.borderDefault;
    case BorderRole::None:
        return core::Color::transparent();
    }
    return core::Color::transparent();
}

void Container::setBorderWidth(float width) noexcept {
    borderWidth_ = width;
    hasCustomBorderWidth_ = true;
    markPaintDirty();
}

float Container::effectiveBorderWidth() const noexcept {
    if (hasCustomBorderWidth_) {
        return borderWidth_;
    }
    if (borderRole_ != BorderRole::None) {
        return theme::Metrics::BorderWidthHairline;
    }
    return 0.0f;
}

void Container::setCornerRadius(float radius) noexcept {
    cornerRadii_[0] = radius;
    cornerRadii_[1] = radius;
    cornerRadii_[2] = radius;
    cornerRadii_[3] = radius;
    markPaintDirty();
}

void Container::setCornerRadii(float r0, float r1, float r2, float r3) noexcept {
    cornerRadii_[0] = r0;
    cornerRadii_[1] = r1;
    cornerRadii_[2] = r2;
    cornerRadii_[3] = r3;
    markPaintDirty();
}

void Container::setClipsChildren(bool clips) noexcept {
    clipsChildren_ = clips;
    markPaintDirty();
}

Container* Container::setFlexDirection(layout::FlexDirection dir) noexcept {
    style().flexDirection = dir;
    markLayoutDirty();
    return this;
}

Container* Container::setJustifyContent(layout::JustifyContent justify) noexcept {
    style().justifyContent = justify;
    markLayoutDirty();
    return this;
}

Container* Container::setAlignItems(layout::AlignItems align) noexcept {
    style().alignItems = align;
    markLayoutDirty();
    return this;
}

Container* Container::setFlexWrap(layout::FlexWrap wrap) noexcept {
    style().flexWrap = wrap;
    markLayoutDirty();
    return this;
}

Container* Container::setPadding(float all) noexcept {
    style().padding = layout::RectLength::all(layout::Length::points(all));
    markLayoutDirty();
    return this;
}

Container* Container::setPadding(float vertical, float horizontal) noexcept {
    style().padding = layout::RectLength::symmetric(layout::Length::points(vertical),
                                                    layout::Length::points(horizontal));
    markLayoutDirty();
    return this;
}

Container* Container::setPadding(const layout::RectLength& padding) noexcept {
    style().padding = padding;
    markLayoutDirty();
    return this;
}

Container* Container::setGap(float gap) noexcept {
    style().rowGap = gap;
    style().columnGap = gap;
    markLayoutDirty();
    return this;
}

Container* Container::setRowGap(float gap) noexcept {
    style().rowGap = gap;
    markLayoutDirty();
    return this;
}

Container* Container::setColumnGap(float gap) noexcept {
    style().columnGap = gap;
    markLayoutDirty();
    return this;
}

void Container::setInteractive(bool interactive) noexcept {
    if (isInteractive_ == interactive) {
        return;
    }
    isInteractive_ = interactive;
    setFocusable(interactive);
    setCursorShape(interactive ? platform::CursorShape::PointingHand
                               : platform::CursorShape::Arrow);
    if (!interactive) {
        isHovered_ = false;
        isPressed_ = false;
        bgAnim_.stop();
        borderAnim_.stop();
    } else {
        updateInteractiveTargetColors(false);
    }
    markPaintDirty();
}

void Container::updateAnimations(float deltaSeconds) {
    if (isInteractive_) {
        bgAnim_.update(deltaSeconds);
        borderAnim_.update(deltaSeconds);
        markPaintDirty();
    }
}

bool Container::hasActiveAnimations() const noexcept {
    return isInteractive_ && (bgAnim_.isRunning() || borderAnim_.isRunning());
}

core::Color Container::targetInteractiveBg() const noexcept {
    const auto& pal = colorPalette();
    core::Color baseBg = effectiveBackgroundColor();

    if (!isEnabled()) {
        return baseBg.withAlpha(0.35f);
    }

    if (isPressed_) {
        if (hasCustomBg_) {
            return baseBg.lerp(
                pal.mode == theme::ThemeMode::Dark ? core::Color::white() : core::Color::black(),
                0.12f);
        }
        switch (surfaceRole_) {
        case SurfaceRole::Canvas:
            return pal.surfaceRaised;
        case SurfaceRole::Default:
            return pal.surfaceOverlay;
        case SurfaceRole::Raised:
            return pal.mode == theme::ThemeMode::Dark ? core::Color{0.24f, 0.24f, 0.27f, 1.0f}
                                                      : core::Color{0.88f, 0.88f, 0.90f, 1.0f};
        case SurfaceRole::Overlay:
            return pal.mode == theme::ThemeMode::Dark ? core::Color{0.27f, 0.27f, 0.30f, 1.0f}
                                                      : core::Color{0.91f, 0.91f, 0.93f, 1.0f};
        case SurfaceRole::None:
            return pal.surfaceRaised;
        }
    } else if (isHovered_) {
        if (hasCustomBg_) {
            return baseBg.lerp(
                pal.mode == theme::ThemeMode::Dark ? core::Color::white() : core::Color::black(),
                0.06f);
        }
        switch (surfaceRole_) {
        case SurfaceRole::Canvas:
            return pal.surfaceDefault;
        case SurfaceRole::Default:
            return pal.surfaceRaised;
        case SurfaceRole::Raised:
            return pal.surfaceOverlay;
        case SurfaceRole::Overlay:
            return pal.mode == theme::ThemeMode::Dark ? core::Color{0.24f, 0.24f, 0.27f, 1.0f}
                                                      : core::Color{0.94f, 0.94f, 0.95f, 1.0f};
        case SurfaceRole::None:
            return pal.surfaceDefault;
        }
    }

    return baseBg;
}

core::Color Container::targetInteractiveBorder() const noexcept {
    const auto& pal = colorPalette();
    core::Color baseBorder = effectiveBorderColor();

    if (!isEnabled()) {
        return baseBorder.withAlpha(0.20f);
    }

    if (isPressed_) {
        if (hasCustomBorder_) {
            return baseBorder.lerp(
                pal.mode == theme::ThemeMode::Dark ? core::Color::white() : core::Color::black(),
                0.15f);
        }
        switch (borderRole_) {
        case BorderRole::Subtle:
        case BorderRole::Default:
            return pal.borderStrong;
        case BorderRole::None:
            return core::Color::transparent();
        }
    } else if (isHovered_) {
        if (hasCustomBorder_) {
            return baseBorder.lerp(
                pal.mode == theme::ThemeMode::Dark ? core::Color::white() : core::Color::black(),
                0.10f);
        }
        switch (borderRole_) {
        case BorderRole::Subtle:
            return pal.borderDefault;
        case BorderRole::Default:
            return pal.borderStrong;
        case BorderRole::None:
            return core::Color::transparent();
        }
    }

    return baseBorder;
}

void Container::updateInteractiveTargetColors(bool animated) {
    if (!isInteractive_) {
        return;
    }

    core::Color targetBg = targetInteractiveBg();
    core::Color targetBorder = targetInteractiveBorder();

    if (animated) {
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

void Container::onMouseEnter() {
    if (!isInteractive_ || !isEnabled()) {
        return;
    }
    isHovered_ = true;
    updateInteractiveTargetColors(true);
}

void Container::onMouseLeave() {
    if (!isInteractive_ || !isEnabled()) {
        return;
    }
    isHovered_ = false;
    updateInteractiveTargetColors(true);
}

void Container::onMouseMove(WidgetMouseEvent& ev) {
    if (!isInteractive_ || !isEnabled()) {
        return;
    }
    if (isPressed_) {
        core::Rect rect = layoutRect();
        bool inside = ev.position().x >= 0.0f && ev.position().x <= rect.size.width &&
                      ev.position().y >= 0.0f && ev.position().y <= rect.size.height;
        if (inside != isHovered_) {
            isHovered_ = inside;
            updateInteractiveTargetColors(true);
        }
        ev.accept();
    }
}

void Container::onMouseDown(WidgetMouseEvent& ev) {
    if (!isInteractive_ || !isEnabled()) {
        return;
    }
    if (ev.button() == platform::MouseButton::Left) {
        isPressed_ = true;
        isHovered_ = true;
        ev.accept();
        updateInteractiveTargetColors(true);
    }
}

void Container::onMouseUp(WidgetMouseEvent& ev) {
    if (!isInteractive_ || !isEnabled()) {
        return;
    }
    if (ev.button() == platform::MouseButton::Left && isPressed_) {
        core::Rect rect = layoutRect();
        bool inside = ev.position().x >= 0.0f && ev.position().x <= rect.size.width &&
                      ev.position().y >= 0.0f && ev.position().y <= rect.size.height;
        isPressed_ = false;
        ev.accept();
        updateInteractiveTargetColors(true);
        if (inside) {
            onClicked.emit();
        }
    }
}

void Container::onKeyDown(WidgetKeyEvent& ev) {
    if (!isInteractive_ || !isEnabled()) {
        return;
    }
    if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        ev.accept();
        onClicked.emit();
    }
}

void Container::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    core::Color bg;
    core::Color border;
    float bWidth = effectiveBorderWidth();

    if (isInteractive_) {
        bg = bgAnim_.isRunning() ? bgAnim_.currentValue() : targetInteractiveBg();
        border = borderAnim_.isRunning() ? borderAnim_.currentValue() : targetInteractiveBorder();
    } else {
        bg = effectiveBackgroundColor();
        border = effectiveBorderColor();
    }

    bool hasBg = bg.a > 0.0f;
    bool hasBorder = bWidth > 0.0f && border.a > 0.0f;

    if (hasBg && hasBorder) {
        ctx.drawBox(rect, bg, border, bWidth, cornerRadii_);
    } else if (hasBg) {
        bool hasRadius = cornerRadii_[0] > 0.0f || cornerRadii_[1] > 0.0f ||
                         cornerRadii_[2] > 0.0f || cornerRadii_[3] > 0.0f;
        if (hasRadius) {
            ctx.drawRoundedRect(rect, bg, cornerRadii_);
        } else {
            ctx.drawRect(rect, bg);
        }
    } else if (hasBorder) {
        ctx.drawBorder(rect, border, bWidth, cornerRadii_[0]);
    }

    if (isInteractive_ && hasKeyboardFocus() && isEnabled()) {
        theme::paintFocusRing(ctx, rect, cornerRadii_[0]);
    }
}

} // namespace northern::widgets
