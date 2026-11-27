#include "widgets/containers/scroll_view.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

ScrollView::ScrollView() {
    setFocusable(true);
    style().flexDirection = layout::FlexDirection::Column;
}

void ScrollView::attachChildInternal(std::unique_ptr<Widget> child) {
    if (child) {
        if (axis_ == ScrollAxis::Vertical || axis_ == ScrollAxis::Both) {
            child->style().flexShrink = 0.0f;
        }
    }
    Widget::attachChildInternal(std::move(child));
}

void ScrollView::setScrollbarVisibility(ScrollbarVisibility visibility) noexcept {
    if (scrollbarVisibility_ != visibility) {
        scrollbarVisibility_ = visibility;
        inactivityTimer_ = 0.0f;
        if (visibility == ScrollbarVisibility::Never) {
            scrollbarOpacityAnim_.setCurrentValue(0.0f);
        } else if (visibility == ScrollbarVisibility::AlwaysVisible) {
            scrollbarOpacityAnim_.setCurrentValue(1.0f);
        } else {
            revealScrollbars();
        }
        markPaintDirty();
    }
}

void ScrollView::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiMetrics(scale);
}

void ScrollView::applyDpiMetrics(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        if (!hasCustomThickness_) {
            scrollbarThickness_ = 6.0f * dpiScale_;
        }
        if (!hasCustomPadding_) {
            scrollbarPadding_ = 3.0f * dpiScale_;
        }
        if (!hasCustomHitThickness_) {
            scrollbarHitThickness_ = 14.0f * dpiScale_;
        }
        markPaintDirty();
    }
}

void ScrollView::onContextAttached() {
    if (!hasCustomDpiScale_) {
        float ctxDpi = contextDpiScale();
        if (ctxDpi > 0.0f && ctxDpi != dpiScale_) {
            applyDpiMetrics(ctxDpi);
        }
    }
    markPaintDirty();
}

void ScrollView::onThemeChanged() {
    markPaintDirty();
}

void ScrollView::onEnabledChanged() {
    if (!isEnabled()) {
        if (context_ && context_->mouseGrabWidget() == this) {
            context_->releaseMouseGrab();
        }
        dragTarget_ = DragTarget::None;
        if (isHoveringVertical_ || isHoveringHorizontal_) {
            isHoveringVertical_ = false;
            isHoveringHorizontal_ = false;
            verticalHoverAnim_.stop();
            horizontalHoverAnim_.stop();
            verticalHoverAnim_.setCurrentValue(0.0f);
            horizontalHoverAnim_.setCurrentValue(0.0f);
        }
        markPaintDirty();
    }
}

void ScrollView::revealScrollbars() noexcept {
    inactivityTimer_ = 0.0f;
    if (scrollbarVisibility_ == ScrollbarVisibility::Never) {
        scrollbarOpacityAnim_.setCurrentValue(0.0f);
        return;
    }
    if (scrollbarOpacityAnim_.currentValue() < 0.999f) {
        if (!scrollbarOpacityAnim_.isRunning() || scrollbarOpacityAnim_.targetValue() < 1.0f) {
            scrollbarOpacityAnim_.start(scrollbarOpacityAnim_.currentValue(), 1.0f, kFadeInDuration,
                                        core::animation::EasingCurve::OutCubic);
        }
    } else {
        scrollbarOpacityAnim_.setCurrentValue(1.0f);
    }
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
}

void ScrollView::updateAnimations(float deltaSeconds) {
    bool changed = false;
    if (scrollbarOpacityAnim_.isRunning()) {
        scrollbarOpacityAnim_.update(deltaSeconds);
        changed = true;
    }
    if (verticalHoverAnim_.isRunning()) {
        verticalHoverAnim_.update(deltaSeconds);
        changed = true;
    }
    if (horizontalHoverAnim_.isRunning()) {
        horizontalHoverAnim_.update(deltaSeconds);
        changed = true;
    }

    if (scrollbarVisibility_ == ScrollbarVisibility::Automatic) {
        bool interacting =
            (dragTarget_ != DragTarget::None) || isHoveringVertical_ || isHoveringHorizontal_;
        if (interacting) {
            inactivityTimer_ = 0.0f;
        } else if (scrollbarOpacityAnim_.currentValue() > 0.001f &&
                   (!scrollbarOpacityAnim_.isRunning() ||
                    scrollbarOpacityAnim_.targetValue() > 0.001f)) {
            inactivityTimer_ += deltaSeconds;
            if (inactivityTimer_ >= kInactivityFadeDelay) {
                scrollbarOpacityAnim_.start(scrollbarOpacityAnim_.currentValue(), 0.0f,
                                            kFadeOutDuration,
                                            core::animation::EasingCurve::OutCubic);
                changed = true;
            }
        }
    }

    if (changed) {
        markPaintDirty();
    }
}

bool ScrollView::hasActiveAnimations() const noexcept {
    if (scrollbarOpacityAnim_.isRunning() || verticalHoverAnim_.isRunning() ||
        horizontalHoverAnim_.isRunning()) {
        return true;
    }
    if (scrollbarVisibility_ == ScrollbarVisibility::Automatic && dragTarget_ == DragTarget::None &&
        !isHoveringVertical_ && !isHoveringHorizontal_ &&
        scrollbarOpacityAnim_.currentValue() > 0.001f &&
        (hasVerticalScrollbar() || hasHorizontalScrollbar())) {
        return true;
    }
    return false;
}

void ScrollView::clampScrollOffset() {
    core::Point maxOffset = maxScrollOffset();
    core::Point clamped{
        std::clamp(scrollOffset_.x, 0.0f, maxOffset.x),
        std::clamp(scrollOffset_.y, 0.0f, maxOffset.y),
    };
    if (clamped.x != scrollOffset_.x || clamped.y != scrollOffset_.y) {
        scrollOffset_ = clamped;
        markScreenRectDirty();
        markPaintDirty();
        onScrolled.emit(scrollOffset_);
    }
}

void ScrollView::scrollTo(const core::Point& offset) {
    core::Point maxOffset = maxScrollOffset();
    core::Point clamped{
        std::clamp(offset.x, 0.0f, maxOffset.x),
        std::clamp(offset.y, 0.0f, maxOffset.y),
    };
    if (clamped.x != scrollOffset_.x || clamped.y != scrollOffset_.y) {
        scrollOffset_ = clamped;
        revealScrollbars();
        markScreenRectDirty();
        markPaintDirty();
        onScrolled.emit(scrollOffset_);
    }
}

void ScrollView::scrollBy(const core::Point& delta) {
    scrollTo(core::Point{scrollOffset_.x + delta.x, scrollOffset_.y + delta.y});
}

core::Size ScrollView::contentSize() const noexcept {
    float maxW = 0.0f;
    float maxH = 0.0f;
    for (const auto& child : children()) {
        if (!child || !child->isVisible()) {
            continue;
        }
        core::Rect cr = child->layoutRect();
        maxW = std::max(maxW, cr.origin.x + cr.size.width);
        maxH = std::max(maxH, cr.origin.y + cr.size.height);
    }
    return core::Size{maxW, maxH};
}

core::Point ScrollView::maxScrollOffset() const noexcept {
    core::Size cs = contentSize();
    core::Size vs = layoutRect().size;
    float maxX = (axis_ == ScrollAxis::Horizontal || axis_ == ScrollAxis::Both)
                     ? std::max(0.0f, cs.width - vs.width)
                     : 0.0f;
    float maxY = (axis_ == ScrollAxis::Vertical || axis_ == ScrollAxis::Both)
                     ? std::max(0.0f, cs.height - vs.height)
                     : 0.0f;
    return core::Point{maxX, maxY};
}

void ScrollView::setScrollbarThickness(float thickness) noexcept {
    scrollbarThickness_ = thickness;
    hasCustomThickness_ = true;
    markPaintDirty();
}

void ScrollView::setScrollbarPadding(float padding) noexcept {
    scrollbarPadding_ = padding;
    hasCustomPadding_ = true;
    markPaintDirty();
}

void ScrollView::setScrollbarHitThickness(float hitThickness) noexcept {
    scrollbarHitThickness_ = hitThickness;
    hasCustomHitThickness_ = true;
    markPaintDirty();
}

void ScrollView::setScrollbarColor(const core::Color& color) noexcept {
    customScrollbarColor_ = color;
    markPaintDirty();
}

void ScrollView::setScrollbarHoverColor(const core::Color& color) noexcept {
    customScrollbarHoverColor_ = color;
    markPaintDirty();
}

void ScrollView::setSurface(SurfaceRole role) noexcept {
    if (surfaceRole_ != role || hasCustomBg_) {
        surfaceRole_ = role;
        hasCustomBg_ = false;
        markPaintDirty();
    }
}

void ScrollView::setBackgroundColor(const core::Color& color) noexcept {
    bgColor_ = color;
    hasCustomBg_ = true;
    markPaintDirty();
}

core::Color ScrollView::effectiveBackgroundColor() const noexcept {
    if (hasCustomBg_) {
        return bgColor_;
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

bool ScrollView::hasVerticalScrollbar() const noexcept {
    if (axis_ != ScrollAxis::Vertical && axis_ != ScrollAxis::Both) {
        return false;
    }
    return maxScrollOffset().y > 0.0f && layoutRect().size.height > 0.0f;
}

bool ScrollView::hasHorizontalScrollbar() const noexcept {
    if (axis_ != ScrollAxis::Horizontal && axis_ != ScrollAxis::Both) {
        return false;
    }
    return maxScrollOffset().x > 0.0f && layoutRect().size.width > 0.0f;
}

core::Rect ScrollView::verticalHitRect() const noexcept {
    if (!hasVerticalScrollbar()) {
        return core::Rect{};
    }
    float viewW = layoutRect().size.width;
    float viewH = layoutRect().size.height;
    float cornerHit = hasHorizontalScrollbar() ? scrollbarHitThickness_ : 0.0f;
    return core::Rect{
        core::Point{std::max(0.0f, viewW - scrollbarHitThickness_), 0.0f},
        core::Size{scrollbarHitThickness_, std::max(0.0f, viewH - cornerHit)},
    };
}

core::Rect ScrollView::verticalTrackRect() const noexcept {
    if (!hasVerticalScrollbar()) {
        return core::Rect{};
    }
    float viewW = layoutRect().size.width;
    float viewH = layoutRect().size.height;
    float cornerReservation =
        hasHorizontalScrollbar() ? (scrollbarThickness_ + scrollbarPadding_) : 0.0f;
    float trackH = std::max(0.0f, viewH - 2.0f * scrollbarPadding_ - cornerReservation);
    return core::Rect{
        core::Point{std::max(0.0f, viewW - scrollbarThickness_ - scrollbarPadding_),
                    scrollbarPadding_},
        core::Size{scrollbarThickness_, trackH},
    };
}

core::Rect ScrollView::verticalThumbRect() const noexcept {
    core::Rect track = verticalTrackRect();
    if (track.size.height <= 0.0f) {
        return core::Rect{};
    }
    float viewH = layoutRect().size.height;
    float contentH = contentSize().height;
    if (contentH <= 0.0f) {
        return core::Rect{};
    }

    float thumbH =
        std::clamp(track.size.height * (viewH / contentH), 24.0f * dpiScale_, track.size.height);
    float travel = track.size.height - thumbH;
    float maxOffY = maxScrollOffset().y;
    float thumbY = track.origin.y +
                   (travel > 0.0f && maxOffY > 0.0f ? travel * (scrollOffset_.y / maxOffY) : 0.0f);

    return core::Rect{
        core::Point{track.origin.x, thumbY},
        core::Size{scrollbarThickness_, thumbH},
    };
}

core::Rect ScrollView::horizontalHitRect() const noexcept {
    if (!hasHorizontalScrollbar()) {
        return core::Rect{};
    }
    float viewW = layoutRect().size.width;
    float viewH = layoutRect().size.height;
    float cornerHit = hasVerticalScrollbar() ? scrollbarHitThickness_ : 0.0f;
    return core::Rect{
        core::Point{0.0f, std::max(0.0f, viewH - scrollbarHitThickness_)},
        core::Size{std::max(0.0f, viewW - cornerHit), scrollbarHitThickness_},
    };
}

core::Rect ScrollView::horizontalTrackRect() const noexcept {
    if (!hasHorizontalScrollbar()) {
        return core::Rect{};
    }
    float viewW = layoutRect().size.width;
    float viewH = layoutRect().size.height;
    float cornerReservation =
        hasVerticalScrollbar() ? (scrollbarThickness_ + scrollbarPadding_) : 0.0f;
    float trackW = std::max(0.0f, viewW - 2.0f * scrollbarPadding_ - cornerReservation);
    return core::Rect{
        core::Point{scrollbarPadding_,
                    std::max(0.0f, viewH - scrollbarThickness_ - scrollbarPadding_)},
        core::Size{trackW, scrollbarThickness_},
    };
}

core::Rect ScrollView::horizontalThumbRect() const noexcept {
    core::Rect track = horizontalTrackRect();
    if (track.size.width <= 0.0f) {
        return core::Rect{};
    }
    float viewW = layoutRect().size.width;
    float contentW = contentSize().width;
    if (contentW <= 0.0f) {
        return core::Rect{};
    }

    float thumbW =
        std::clamp(track.size.width * (viewW / contentW), 24.0f * dpiScale_, track.size.width);
    float travel = track.size.width - thumbW;
    float maxOffX = maxScrollOffset().x;
    float thumbX = track.origin.x +
                   (travel > 0.0f && maxOffX > 0.0f ? travel * (scrollOffset_.x / maxOffX) : 0.0f);

    return core::Rect{
        core::Point{thumbX, track.origin.y},
        core::Size{thumbW, scrollbarThickness_},
    };
}

void ScrollView::onMouseWheel(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    clampScrollOffset();
    float step = 32.0f * dpiScale_;
    float dy = -ev.wheelDelta().y * step;
    float dx = -ev.wheelDelta().x * step;

    if (platform::hasModifier(ev.modifiers(), platform::KeyModifier::Shift) && dy != 0.0f &&
        dx == 0.0f) {
        dx = dy;
        dy = 0.0f;
    }

    core::Point old = scrollOffset_;
    scrollBy(core::Point{dx, dy});
    if (old.x != scrollOffset_.x || old.y != scrollOffset_.y) {
        ev.accept();
    }
}

void ScrollView::onMouseDown(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.isPrimaryButton()) {
        clampScrollOffset();
        core::Point pos = ev.position();

        // Check vertical scrollbar interaction
        if (hasVerticalScrollbar() && verticalHitRect().contains(pos)) {
            core::Rect vThumb = verticalThumbRect();
            if (vThumb.contains(pos)) {
                dragTarget_ = DragTarget::VerticalThumb;
                dragStartMousePos_ = pos.y;
                dragStartScrollOffset_ = scrollOffset_.y;
            } else {
                // Track click-to-jump
                core::Rect vTrack = verticalTrackRect();
                float travel = vTrack.size.height - vThumb.size.height;
                if (travel > 0.0f) {
                    float clickFraction = std::clamp(
                        (pos.y - vTrack.origin.y - vThumb.size.height * 0.5f) / travel, 0.0f, 1.0f);
                    scrollTo(core::Point{scrollOffset_.x, clickFraction * maxScrollOffset().y});
                    dragTarget_ = DragTarget::VerticalThumb;
                    dragStartMousePos_ = pos.y;
                    dragStartScrollOffset_ = scrollOffset_.y;
                }
            }
            if (context_) {
                context_->setMouseGrabWidget(this);
            }
            revealScrollbars();
            markPaintDirty();
            ev.accept();
            return;
        }

        // Check horizontal scrollbar interaction
        if (hasHorizontalScrollbar() && horizontalHitRect().contains(pos)) {
            core::Rect hThumb = horizontalThumbRect();
            if (hThumb.contains(pos)) {
                dragTarget_ = DragTarget::HorizontalThumb;
                dragStartMousePos_ = pos.x;
                dragStartScrollOffset_ = scrollOffset_.x;
            } else {
                // Track click-to-jump
                core::Rect hTrack = horizontalTrackRect();
                float travel = hTrack.size.width - hThumb.size.width;
                if (travel > 0.0f) {
                    float clickFraction = std::clamp(
                        (pos.x - hTrack.origin.x - hThumb.size.width * 0.5f) / travel, 0.0f, 1.0f);
                    scrollTo(core::Point{clickFraction * maxScrollOffset().x, scrollOffset_.y});
                    dragTarget_ = DragTarget::HorizontalThumb;
                    dragStartMousePos_ = pos.x;
                    dragStartScrollOffset_ = scrollOffset_.x;
                }
            }
            if (context_) {
                context_->setMouseGrabWidget(this);
            }
            revealScrollbars();
            markPaintDirty();
            ev.accept();
            return;
        }

        setFocus(FocusReason::Pointer);
    }
}

void ScrollView::onMouseMove(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        onMouseLeave();
        return;
    }
    core::Point pos = ev.position();

    if (dragTarget_ == DragTarget::VerticalThumb) {
        float delta = pos.y - dragStartMousePos_;
        core::Rect vTrack = verticalTrackRect();
        core::Rect vThumb = verticalThumbRect();
        float travel = vTrack.size.height - vThumb.size.height;
        if (travel > 0.0f) {
            float scrollDelta = (delta / travel) * maxScrollOffset().y;
            scrollTo(core::Point{scrollOffset_.x, dragStartScrollOffset_ + scrollDelta});
        }
        ev.accept();
        return;
    }

    if (dragTarget_ == DragTarget::HorizontalThumb) {
        float delta = pos.x - dragStartMousePos_;
        core::Rect hTrack = horizontalTrackRect();
        core::Rect hThumb = horizontalThumbRect();
        float travel = hTrack.size.width - hThumb.size.width;
        if (travel > 0.0f) {
            float scrollDelta = (delta / travel) * maxScrollOffset().x;
            scrollTo(core::Point{dragStartScrollOffset_ + scrollDelta, scrollOffset_.y});
        }
        ev.accept();
        return;
    }

    bool hoverV = hasVerticalScrollbar() && verticalHitRect().contains(pos);
    bool hoverH = hasHorizontalScrollbar() && horizontalHitRect().contains(pos);
    if (hoverV != isHoveringVertical_ || hoverH != isHoveringHorizontal_) {
        if (hoverV != isHoveringVertical_) {
            isHoveringVertical_ = hoverV;
            verticalHoverAnim_.start(verticalHoverAnim_.currentValue(), hoverV ? 1.0f : 0.0f,
                                     kHoverExpandDuration, core::animation::EasingCurve::OutCubic);
        }
        if (hoverH != isHoveringHorizontal_) {
            isHoveringHorizontal_ = hoverH;
            horizontalHoverAnim_.start(horizontalHoverAnim_.currentValue(), hoverH ? 1.0f : 0.0f,
                                       kHoverExpandDuration,
                                       core::animation::EasingCurve::OutCubic);
        }
        if (hoverV || hoverH) {
            revealScrollbars();
        } else if (context_) {
            context_->registerAnimatedWidget(this);
        }
        markPaintDirty();
    }
}

void ScrollView::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.isPrimaryButton() && dragTarget_ != DragTarget::None) {
        dragTarget_ = DragTarget::None;
        if (context_) {
            context_->releaseMouseGrab();
            context_->registerAnimatedWidget(this);
        }
        revealScrollbars();
        markPaintDirty();
        ev.accept();
    }
}

void ScrollView::onMouseLeave() {
    if ((dragTarget_ == DragTarget::None || !isEnabled()) &&
        (isHoveringVertical_ || isHoveringHorizontal_)) {
        isHoveringVertical_ = false;
        isHoveringHorizontal_ = false;
        verticalHoverAnim_.start(verticalHoverAnim_.currentValue(), 0.0f, kHoverExpandDuration,
                                 core::animation::EasingCurve::OutCubic);
        horizontalHoverAnim_.start(horizontalHoverAnim_.currentValue(), 0.0f, kHoverExpandDuration,
                                   core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
        markPaintDirty();
    }
}

void ScrollView::onKeyDown(WidgetKeyEvent& ev) {
    if (!ev.isDown() || !isEnabled()) {
        return;
    }

    float viewH = layoutRect().size.height;
    float keyStep = 40.0f * dpiScale_;
    switch (ev.key()) {
    case platform::Key::Down:
        scrollBy(core::Point{0.0f, keyStep});
        ev.accept();
        break;
    case platform::Key::Up:
        scrollBy(core::Point{0.0f, -keyStep});
        ev.accept();
        break;
    case platform::Key::PageDown:
        scrollBy(core::Point{0.0f, viewH * 0.85f});
        ev.accept();
        break;
    case platform::Key::PageUp:
        scrollBy(core::Point{0.0f, -viewH * 0.85f});
        ev.accept();
        break;
    case platform::Key::Home:
        scrollTo(core::Point{scrollOffset_.x, 0.0f});
        ev.accept();
        break;
    case platform::Key::End:
        scrollTo(core::Point{scrollOffset_.x, maxScrollOffset().y});
        ev.accept();
        break;
    case platform::Key::Right:
        scrollBy(core::Point{keyStep, 0.0f});
        ev.accept();
        break;
    case platform::Key::Left:
        scrollBy(core::Point{-keyStep, 0.0f});
        ev.accept();
        break;
    default:
        break;
    }
}

void ScrollView::paint(renderer::PaintContext& ctx) {
    clampScrollOffset();
    core::Color bg = effectiveBackgroundColor();
    if (bg.a > 0.0f) {
        ctx.drawRect(screenRect(), bg);
    }
    if (isEnabled() && hasKeyboardFocus()) {
        paintFocusRing(ctx, screenRect(), 0.0f);
    }
}

void ScrollView::paintAxisScrollbar(renderer::PaintContext& ctx, const core::Rect& sRect,
                                    const core::Rect& tRect, const core::Rect& thRect,
                                    bool isVertical, bool isHovered, bool isDragging, float hoverT,
                                    float opacity) const {
    if (thRect.size.width <= 0.0f || thRect.size.height <= 0.0f) {
        return;
    }

    const auto& pal = colorPalette();
    core::Rect screenTrack{
        core::Point{sRect.origin.x + tRect.origin.x, sRect.origin.y + tRect.origin.y}, tRect.size};
    float expandT = isDragging ? 1.0f : std::max(hoverT, isHovered ? 1.0f : 0.0f);
    float visThickness = scrollbarThickness_ * (0.67f + 0.33f * std::clamp(expandT, 0.0f, 1.0f));
    float inset = scrollbarThickness_ - visThickness;

    core::Rect screenThumb = isVertical
                                 ? core::Rect{core::Point{sRect.origin.x + thRect.origin.x + inset,
                                                          sRect.origin.y + thRect.origin.y},
                                              core::Size{visThickness, thRect.size.height}}
                                 : core::Rect{core::Point{sRect.origin.x + thRect.origin.x,
                                                          sRect.origin.y + thRect.origin.y + inset},
                                              core::Size{thRect.size.width, visThickness}};

    if (isHovered || isDragging) {
        float trackR = scrollbarThickness_ * 0.5f;
        float trackRadii[4]{trackR, trackR, trackR, trackR};
        ctx.drawRoundedRect(screenTrack, pal.surfaceRaised.withAlpha(0.25f * opacity), trackRadii);
    }

    core::Color thumbCol;
    if (customScrollbarHoverColor_ && (isDragging || isHovered)) {
        thumbCol = *customScrollbarHoverColor_;
    } else if (customScrollbarColor_) {
        thumbCol = *customScrollbarColor_;
    } else if (isDragging) {
        thumbCol = pal.textSecondary;
    } else if (isHovered) {
        thumbCol = pal.textSecondary.withAlpha(0.8f);
    } else {
        thumbCol = pal.textTertiary.withAlpha(0.45f);
    }
    thumbCol = thumbCol.withAlpha(thumbCol.a * opacity);
    float thumbR = visThickness * 0.5f;
    float thumbRadii[4]{thumbR, thumbR, thumbR, thumbR};
    ctx.drawRoundedRect(screenThumb, thumbCol, thumbRadii);
}

void ScrollView::paintOverlay(renderer::PaintContext& ctx) {
    core::Rect sRect = screenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f ||
        scrollbarVisibility_ == ScrollbarVisibility::Never) {
        return;
    }

    float opacity = (scrollbarVisibility_ == ScrollbarVisibility::AlwaysVisible)
                        ? 1.0f
                        : scrollbarOpacityAnim_.currentValue();
    if (opacity <= 0.001f) {
        return;
    }

    if (context_ && scrollbarVisibility_ == ScrollbarVisibility::Automatic &&
        (hasVerticalScrollbar() || hasHorizontalScrollbar()) && hasActiveAnimations()) {
        context_->registerAnimatedWidget(this);
    }

    if (hasVerticalScrollbar()) {
        paintAxisScrollbar(ctx, sRect, verticalTrackRect(), verticalThumbRect(), true,
                           isHoveringVertical_, dragTarget_ == DragTarget::VerticalThumb,
                           verticalHoverAnim_.currentValue(), opacity);
    }

    if (hasHorizontalScrollbar()) {
        paintAxisScrollbar(ctx, sRect, horizontalTrackRect(), horizontalThumbRect(), false,
                           isHoveringHorizontal_, dragTarget_ == DragTarget::HorizontalThumb,
                           horizontalHoverAnim_.currentValue(), opacity);
    }
}

} // namespace northern::widgets
