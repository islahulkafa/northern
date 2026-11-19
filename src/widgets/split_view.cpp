#include "widgets/split_view.hpp"

#include "renderer/paint_context.hpp"
#include "widgets/theme.hpp"
#include "widgets/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

SplitView::SplitView(SplitOrientation orientation, float splitRatio)
    : orientation_(orientation), splitRatio_(std::clamp(splitRatio, 0.05f, 0.95f)) {
    setFocusable(false);
    style().width = layout::Length::percent(100.0f);
    style().height = layout::Length::percent(100.0f);
    updatePaneStyles();
}

void SplitView::setOrientation(SplitOrientation orientation) {
    if (orientation_ != orientation) {
        orientation_ = orientation;
        isHoveredSash_ = false;
        setCursorShape(platform::CursorShape::Arrow);
        updatePaneStyles();
        markLayoutDirty();
    }
}

void SplitView::setSplitRatio(float ratio) {
    float clamped = std::clamp(ratio, 0.01f, 0.99f);
    if (splitRatio_ != clamped) {
        splitRatio_ = clamped;
        updatePaneStyles();
        markLayoutDirty();
        onSplitChanged.emit(splitRatio_);
    }
}

void SplitView::setMinPaneSize(float minSize) noexcept {
    minPaneSize_ = std::max(0.0f, minSize);
    markLayoutDirty();
}

void SplitView::setSashThickness(float thickness) noexcept {
    float clamped = std::max(2.0f, thickness);
    if (sashThickness_ != clamped) {
        sashThickness_ = clamped;
        updatePaneStyles();
        markLayoutDirty();
    }
}

void SplitView::setFirstPane(std::unique_ptr<Widget> pane) {
    if (firstPane_) {
        removeChild(firstPane_);
        firstPane_ = nullptr;
    }
    if (pane) {
        firstPane_ = pane.get();
        if (secondPane_) {
            // Re-order so firstPane is index 0
            auto sec = removeChild(secondPane_);
            addChild(std::move(pane));
            secondPane_ = addChild(std::move(sec));
        } else {
            addChild(std::move(pane));
        }
    }
    updatePaneStyles();
    markLayoutDirty();
}

void SplitView::setSecondPane(std::unique_ptr<Widget> pane) {
    if (secondPane_) {
        removeChild(secondPane_);
        secondPane_ = nullptr;
    }
    if (pane) {
        secondPane_ = addChild(std::move(pane));
    }
    updatePaneStyles();
    markLayoutDirty();
}

void SplitView::setPanes(std::unique_ptr<Widget> first, std::unique_ptr<Widget> second) {
    clearChildren();
    firstPane_ = nullptr;
    secondPane_ = nullptr;
    if (first) {
        firstPane_ = addChild(std::move(first));
    }
    if (second) {
        secondPane_ = addChild(std::move(second));
    }
    updatePaneStyles();
    markLayoutDirty();
}

void SplitView::updatePaneStyles() {
    if (orientation_ == SplitOrientation::Horizontal) {
        style().flexDirection = layout::FlexDirection::Row;
        style().columnGap = sashThickness_;
        style().rowGap = 0.0f;

        if (firstPane_) {
            firstPane_->style().width = layout::Length::points(0.0f);
            firstPane_->style().height = layout::Length::percent(100.0f);
            firstPane_->style().flexGrow = splitRatio_;
            firstPane_->style().flexShrink = 1.0f;
        }
        if (secondPane_) {
            secondPane_->style().width = layout::Length::points(0.0f);
            secondPane_->style().height = layout::Length::percent(100.0f);
            secondPane_->style().flexGrow = 1.0f - splitRatio_;
            secondPane_->style().flexShrink = 1.0f;
        }
    } else {
        style().flexDirection = layout::FlexDirection::Column;
        style().rowGap = sashThickness_;
        style().columnGap = 0.0f;

        if (firstPane_) {
            firstPane_->style().height = layout::Length::points(0.0f);
            firstPane_->style().width = layout::Length::percent(100.0f);
            firstPane_->style().flexGrow = splitRatio_;
            firstPane_->style().flexShrink = 1.0f;
        }
        if (secondPane_) {
            secondPane_->style().height = layout::Length::points(0.0f);
            secondPane_->style().width = layout::Length::percent(100.0f);
            secondPane_->style().flexGrow = 1.0f - splitRatio_;
            secondPane_->style().flexShrink = 1.0f;
        }
    }
}

core::Rect SplitView::sashLocalRect() const noexcept {
    core::Rect lRect = layoutRect();
    if (orientation_ == SplitOrientation::Horizontal) {
        float x = firstPane_
                      ? (firstPane_->layoutRect().origin.x + firstPane_->layoutRect().size.width)
                      : (lRect.size.width * splitRatio_);
        return core::Rect{core::Point{x, 0.0f}, core::Size{sashThickness_, lRect.size.height}};
    } else {
        float y = firstPane_
                      ? (firstPane_->layoutRect().origin.y + firstPane_->layoutRect().size.height)
                      : (lRect.size.height * splitRatio_);
        return core::Rect{core::Point{0.0f, y}, core::Size{lRect.size.width, sashThickness_}};
    }
}

core::Rect SplitView::sashScreenRect() const noexcept {
    core::Rect sRect = screenRect();
    core::Rect local = sashLocalRect();
    return core::Rect{core::Point{sRect.origin.x + local.origin.x, sRect.origin.y + local.origin.y},
                      local.size};
}

void SplitView::onMouseMove(WidgetMouseEvent& ev) {
    core::Rect sashL = sashLocalRect();
    core::Rect lRect = layoutRect();

    if (isDraggingSash_) {
        if (orientation_ == SplitOrientation::Horizontal) {
            float availW = lRect.size.width - sashThickness_;
            if (availW > 0.0f) {
                float minR = std::clamp(minPaneSize_ / availW, 0.01f, 0.49f);
                float maxR = 1.0f - minR;
                float newR =
                    std::clamp((ev.position().x - sashThickness_ * 0.5f) / availW, minR, maxR);
                setSplitRatio(newR);
            }
        } else {
            float availH = lRect.size.height - sashThickness_;
            if (availH > 0.0f) {
                float minR = std::clamp(minPaneSize_ / availH, 0.01f, 0.49f);
                float maxR = 1.0f - minR;
                float newR =
                    std::clamp((ev.position().y - sashThickness_ * 0.5f) / availH, minR, maxR);
                setSplitRatio(newR);
            }
        }
        ev.accept();
        return;
    }

    bool inside = sashL.contains(ev.position());
    if (inside != isHoveredSash_) {
        isHoveredSash_ = inside;
        setCursorShape(isHoveredSash_ ? (orientation_ == SplitOrientation::Horizontal
                                             ? platform::CursorShape::ResizeEW
                                             : platform::CursorShape::ResizeNS)
                                      : platform::CursorShape::Arrow);
        markPaintDirty();
    } else if (isHoveredSash_) {
        platform::CursorShape expected =
            (orientation_ == SplitOrientation::Horizontal ? platform::CursorShape::ResizeEW
                                                          : platform::CursorShape::ResizeNS);
        if (cursorShape() != expected) {
            setCursorShape(expected);
        }
    }
}

void SplitView::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        core::Rect sashL = sashLocalRect();
        if (sashL.contains(ev.position())) {
            isDraggingSash_ = true;
            if (context_) {
                context_->setMouseGrabWidget(this);
            }
            ev.accept();
            markPaintDirty();
        }
    }
}

void SplitView::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left && isDraggingSash_) {
        isDraggingSash_ = false;
        if (context_) {
            context_->releaseMouseGrab();
        }
        core::Rect sashL = sashLocalRect();
        if (!sashL.contains(ev.position())) {
            isHoveredSash_ = false;
            setCursorShape(platform::CursorShape::Arrow);
        }
        ev.accept();
        markPaintDirty();
    }
}

void SplitView::onMouseLeave() {
    if (!isDraggingSash_ && isHoveredSash_) {
        isHoveredSash_ = false;
        setCursorShape(platform::CursorShape::Arrow);
        markPaintDirty();
    }
}

void SplitView::paint(renderer::PaintContext& ctx) {
    core::Rect sRect = sashScreenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return;
    }

    if (isDraggingSash_) {
        ctx.drawRect(sRect, core::Color{0.145f, 0.388f, 0.922f, 0.18f});
    } else if (isHoveredSash_) {
        ctx.drawRect(sRect, core::Color{1.0f, 1.0f, 1.0f, 0.06f});
    }

    core::Color lineCol = isDraggingSash_ ? theme::ColorTokens::AccentDefault
                                          : (isHoveredSash_ ? theme::ColorTokens::BorderStrong
                                                            : theme::ColorTokens::BorderSubtle);

    if (orientation_ == SplitOrientation::Horizontal) {
        float midX = sRect.origin.x + sRect.size.width * 0.5f;
        ctx.drawLine(core::Point{midX, sRect.origin.y},
                     core::Point{midX, sRect.origin.y + sRect.size.height}, lineCol, 1.0f);
    } else {
        float midY = sRect.origin.y + sRect.size.height * 0.5f;
        ctx.drawLine(core::Point{sRect.origin.x, midY},
                     core::Point{sRect.origin.x + sRect.size.width, midY}, lineCol, 1.0f);
    }
}

} // namespace northern::widgets
