#include "widgets/scroll_view.hpp"
#include "renderer/paint_context.hpp"

#include <algorithm>

namespace northern::widgets {

ScrollView::ScrollView() {
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
        markPaintDirty();
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

void ScrollView::setScrollbarColor(const core::Color& color) noexcept {
    scrollbarColor_ = color;
    markPaintDirty();
}

void ScrollView::setScrollbarHoverColor(const core::Color& color) noexcept {
    scrollbarHoverColor_ = color;
    markPaintDirty();
}

void ScrollView::setBackgroundColor(const core::Color& color) noexcept {
    bgColor_ = color;
    markPaintDirty();
}

core::Rect ScrollView::verticalThumbRect() const noexcept {
    core::Point maxOffset = maxScrollOffset();
    if (maxOffset.y <= 0.0f) {
        return core::Rect{0.0f, 0.0f, 0.0f, 0.0f};
    }
    core::Rect sRect = screenRect();
    float viewH = sRect.size.height;
    float contentH = contentSize().height;
    if (viewH <= 0.0f || contentH <= 0.0f) {
        return core::Rect{0.0f, 0.0f, 0.0f, 0.0f};
    }

    float trackH = std::max(0.0f, viewH - 2.0f * scrollbarPadding_);
    float thumbH = std::clamp(trackH * (viewH / contentH), 24.0f, trackH);
    float travel = trackH - thumbH;
    float thumbY =
        scrollbarPadding_ + (travel > 0.0f ? travel * (scrollOffset_.y / maxOffset.y) : 0.0f);
    float thumbX = sRect.size.width - scrollbarThickness_ - scrollbarPadding_;

    return core::Rect{
        core::Point{sRect.origin.x + thumbX, sRect.origin.y + thumbY},
        core::Size{scrollbarThickness_, thumbH},
    };
}

void ScrollView::onMouseWheel(WidgetMouseEvent& ev) {
    float step = 32.0f;
    float dy = (ev.wheelDelta().y != 0.0f) ? -ev.wheelDelta().y * step : 0.0f;
    float dx = (ev.wheelDelta().x != 0.0f) ? -ev.wheelDelta().x * step : 0.0f;
    if (dy == 0.0f && dx == 0.0f) {
        dy = 32.0f;
    }
    core::Point old = scrollOffset_;
    scrollBy(core::Point{dx, dy});
    if (old.x != scrollOffset_.x || old.y != scrollOffset_.y) {
        ev.accept();
    }
}

void ScrollView::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        core::Rect thumb = verticalThumbRect();
        core::Rect sRect = screenRect();
        core::Point screenMouse{sRect.origin.x + ev.position().x, sRect.origin.y + ev.position().y};
        if (thumb.contains(screenMouse)) {
            isDraggingThumb_ = true;
            dragStartMouseY_ = screenMouse.y;
            dragStartScrollY_ = scrollOffset_.y;
            ev.accept();
            markPaintDirty();
            return;
        }
    }
}

void ScrollView::onMouseMove(WidgetMouseEvent& ev) {
    core::Rect thumb = verticalThumbRect();
    core::Rect sRect = screenRect();
    core::Point screenMouse{sRect.origin.x + ev.position().x, sRect.origin.y + ev.position().y};

    bool hover = thumb.contains(screenMouse);
    if (hover != isHoveringThumb_) {
        isHoveringThumb_ = hover;
        markPaintDirty();
    }

    if (isDraggingThumb_) {
        float dy = screenMouse.y - dragStartMouseY_;
        float viewH = sRect.size.height;
        float contentH = contentSize().height;
        float trackH = std::max(0.0f, viewH - 2.0f * scrollbarPadding_);
        float thumbH = std::clamp(trackH * (viewH / contentH), 24.0f, trackH);
        float travel = trackH - thumbH;

        if (travel > 0.0f) {
            float scrollPerPixel = maxScrollOffset().y / travel;
            scrollTo(core::Point{scrollOffset_.x, dragStartScrollY_ + dy * scrollPerPixel});
        }
        ev.accept();
    }
}

void ScrollView::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left && isDraggingThumb_) {
        isDraggingThumb_ = false;
        ev.accept();
        markPaintDirty();
    }
}

void ScrollView::paint(renderer::PaintContext& ctx) {
    if (bgColor_.a > 0.0f) {
        ctx.drawRect(screenRect(), bgColor_);
    }
}

void ScrollView::paintOverlay(renderer::PaintContext& ctx) {
    core::Rect thumb = verticalThumbRect();
    if (thumb.size.width > 0.0f && thumb.size.height > 0.0f) {
        const core::Color& col =
            (isDraggingThumb_ || isHoveringThumb_) ? scrollbarHoverColor_ : scrollbarColor_;
        ctx.drawRoundedRect(thumb, col, scrollbarThickness_ * 0.5f);
    }
}

} // namespace northern::widgets
