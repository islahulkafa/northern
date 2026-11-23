#include "widgets/containers/scroll_view.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"

#include <algorithm>

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

void ScrollView::scrollTo(const core::Point& offset) {
    core::Point maxOffset = maxScrollOffset();
    core::Point clamped{
        std::clamp(offset.x, 0.0f, maxOffset.x),
        std::clamp(offset.y, 0.0f, maxOffset.y),
    };
    if (clamped.x != scrollOffset_.x || clamped.y != scrollOffset_.y) {
        scrollOffset_ = clamped;
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
    markPaintDirty();
}

void ScrollView::setScrollbarPadding(float padding) noexcept {
    scrollbarPadding_ = padding;
    markPaintDirty();
}

void ScrollView::setScrollbarHitThickness(float hitThickness) noexcept {
    scrollbarHitThickness_ = hitThickness;
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

void ScrollView::setBackgroundColor(const core::Color& color) noexcept {
    bgColor_ = color;
    markPaintDirty();
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

    float thumbH = std::clamp(track.size.height * (viewH / contentH), 24.0f, track.size.height);
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

    float thumbW = std::clamp(track.size.width * (viewW / contentW), 24.0f, track.size.width);
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
    float step = 32.0f;
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
    if (ev.button() == platform::MouseButton::Left) {
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
            markPaintDirty();
            ev.accept();
            return;
        }

        setFocus();
    }
}

void ScrollView::onMouseMove(WidgetMouseEvent& ev) {
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
        isHoveringVertical_ = hoverV;
        isHoveringHorizontal_ = hoverH;
        markPaintDirty();
    }
}

void ScrollView::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left && dragTarget_ != DragTarget::None) {
        dragTarget_ = DragTarget::None;
        markPaintDirty();
        ev.accept();
    }
}

void ScrollView::onMouseLeave() {
    if (dragTarget_ == DragTarget::None && (isHoveringVertical_ || isHoveringHorizontal_)) {
        isHoveringVertical_ = false;
        isHoveringHorizontal_ = false;
        markPaintDirty();
    }
}

void ScrollView::onKeyDown(WidgetKeyEvent& ev) {
    if (!ev.isDown()) {
        return;
    }

    float viewH = layoutRect().size.height;
    switch (ev.key()) {
    case platform::Key::Down:
        scrollBy(core::Point{0.0f, 40.0f});
        ev.accept();
        break;
    case platform::Key::Up:
        scrollBy(core::Point{0.0f, -40.0f});
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
        scrollBy(core::Point{40.0f, 0.0f});
        ev.accept();
        break;
    case platform::Key::Left:
        scrollBy(core::Point{-40.0f, 0.0f});
        ev.accept();
        break;
    default:
        break;
    }
}

void ScrollView::paint(renderer::PaintContext& ctx) {
    if (bgColor_.a > 0.0f) {
        ctx.drawRect(screenRect(), bgColor_);
    }
    if (hasKeyboardFocus()) {
        theme::paintFocusRing(ctx, screenRect(), 0.0f);
    }
}

void ScrollView::paintOverlay(renderer::PaintContext& ctx) {
    core::Rect sRect = screenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return;
    }

    const auto& pal = colorPalette();

    // Render vertical scrollbar
    if (hasVerticalScrollbar()) {
        core::Rect tRect = verticalTrackRect();
        core::Rect thRect = verticalThumbRect();
        if (thRect.size.width > 0.0f && thRect.size.height > 0.0f) {
            core::Rect screenTrack{
                core::Point{sRect.origin.x + tRect.origin.x, sRect.origin.y + tRect.origin.y},
                tRect.size};
            core::Rect screenThumb{
                core::Point{sRect.origin.x + thRect.origin.x, sRect.origin.y + thRect.origin.y},
                thRect.size};

            if (isHoveringVertical_ || dragTarget_ == DragTarget::VerticalThumb) {
                float trackRadii[4]{3.0f, 3.0f, 3.0f, 3.0f};
                ctx.drawRoundedRect(screenTrack, pal.surfaceRaised.withAlpha(0.25f), trackRadii);
            }

            core::Color thumbCol;
            if (customScrollbarHoverColor_ &&
                (dragTarget_ == DragTarget::VerticalThumb || isHoveringVertical_)) {
                thumbCol = *customScrollbarHoverColor_;
            } else if (customScrollbarColor_) {
                thumbCol = *customScrollbarColor_;
            } else {
                if (dragTarget_ == DragTarget::VerticalThumb) {
                    thumbCol = pal.textSecondary;
                } else if (isHoveringVertical_) {
                    thumbCol = pal.textSecondary.withAlpha(0.8f);
                } else {
                    thumbCol = pal.textTertiary.withAlpha(0.45f);
                }
            }
            float thumbRadii[4]{scrollbarThickness_ * 0.5f, scrollbarThickness_ * 0.5f,
                                scrollbarThickness_ * 0.5f, scrollbarThickness_ * 0.5f};
            ctx.drawRoundedRect(screenThumb, thumbCol, thumbRadii);
        }
    }

    // Render horizontal scrollbar
    if (hasHorizontalScrollbar()) {
        core::Rect tRect = horizontalTrackRect();
        core::Rect thRect = horizontalThumbRect();
        if (thRect.size.width > 0.0f && thRect.size.height > 0.0f) {
            core::Rect screenTrack{
                core::Point{sRect.origin.x + tRect.origin.x, sRect.origin.y + tRect.origin.y},
                tRect.size};
            core::Rect screenThumb{
                core::Point{sRect.origin.x + thRect.origin.x, sRect.origin.y + thRect.origin.y},
                thRect.size};

            if (isHoveringHorizontal_ || dragTarget_ == DragTarget::HorizontalThumb) {
                float trackRadii[4]{3.0f, 3.0f, 3.0f, 3.0f};
                ctx.drawRoundedRect(screenTrack, pal.surfaceRaised.withAlpha(0.25f), trackRadii);
            }

            core::Color thumbCol;
            if (customScrollbarHoverColor_ &&
                (dragTarget_ == DragTarget::HorizontalThumb || isHoveringHorizontal_)) {
                thumbCol = *customScrollbarHoverColor_;
            } else if (customScrollbarColor_) {
                thumbCol = *customScrollbarColor_;
            } else {
                if (dragTarget_ == DragTarget::HorizontalThumb) {
                    thumbCol = pal.textSecondary;
                } else if (isHoveringHorizontal_) {
                    thumbCol = pal.textSecondary.withAlpha(0.8f);
                } else {
                    thumbCol = pal.textTertiary.withAlpha(0.45f);
                }
            }
            float thumbRadii[4]{scrollbarThickness_ * 0.5f, scrollbarThickness_ * 0.5f,
                                scrollbarThickness_ * 0.5f, scrollbarThickness_ * 0.5f};
            ctx.drawRoundedRect(screenThumb, thumbCol, thumbRadii);
        }
    }
}

} // namespace northern::widgets
