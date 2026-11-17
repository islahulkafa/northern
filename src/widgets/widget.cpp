#include "widgets/widget.hpp"
#include "widgets/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

Widget::Widget() = default;

Widget::~Widget() {
    if (context_) {
        context_->unregisterWidget(this);
    }
}

Widget::Widget(Widget&& other) noexcept
    : layoutNode_(std::move(other.layoutNode_)), parent_(other.parent_), context_(other.context_),
      children_(std::move(other.children_)), isVisible_(other.isVisible_),
      isEnabled_(other.isEnabled_), isFocusable_(other.isFocusable_),
      isPaintDirty_(other.isPaintDirty_), isLayoutDirty_(other.isLayoutDirty_),
      isPendingDelete_(other.isPendingDelete_) {
    other.parent_ = nullptr;
    other.context_ = nullptr;
    for (const auto& child : children_) {
        if (child) {
            child->parent_ = this;
        }
    }
}

Widget& Widget::operator=(Widget&& other) noexcept {
    if (this != &other) {
        if (context_) {
            context_->unregisterWidget(this);
        }

        layoutNode_ = std::move(other.layoutNode_);
        parent_ = other.parent_;
        context_ = other.context_;
        children_ = std::move(other.children_);
        isVisible_ = other.isVisible_;
        isEnabled_ = other.isEnabled_;
        isFocusable_ = other.isFocusable_;
        isPaintDirty_ = other.isPaintDirty_;
        isLayoutDirty_ = other.isLayoutDirty_;
        isPendingDelete_ = other.isPendingDelete_;

        other.parent_ = nullptr;
        other.context_ = nullptr;
        for (const auto& child : children_) {
            if (child) {
                child->parent_ = this;
            }
        }
    }
    return *this;
}

Widget* Widget::childAt(size_t index) const noexcept {
    if (index < children_.size()) {
        return children_[index].get();
    }
    return nullptr;
}

void Widget::attachChildInternal(std::unique_ptr<Widget> child) {
    if (!child) {
        return;
    }
    child->parent_ = this;
    child->setContext(context_);
    layoutNode_.addChild(&child->layoutNode_);
    children_.push_back(std::move(child));
    markLayoutDirty();
}

std::unique_ptr<Widget> Widget::removeChild(Widget* child) {
    if (!child) {
        return nullptr;
    }
    auto it = std::find_if(children_.begin(), children_.end(),
                           [child](const auto& ptr) { return ptr.get() == child; });
    if (it != children_.end()) {
        std::unique_ptr<Widget> removed = std::move(*it);
        children_.erase(it);
        layoutNode_.detachChild(&removed->layoutNode_);
        removed->parent_ = nullptr;
        removed->setContext(nullptr);
        markLayoutDirty();
        return removed;
    }
    return nullptr;
}

std::unique_ptr<Widget> Widget::removeFromParent() {
    if (parent_) {
        return parent_->removeChild(this);
    }
    return nullptr;
}

void Widget::clearChildren() {
    for (const auto& child : children_) {
        if (child) {
            layoutNode_.detachChild(&child->layoutNode_);
            child->parent_ = nullptr;
            child->setContext(nullptr);
        }
    }
    children_.clear();
    markLayoutDirty();
}

void Widget::deleteLater() {
    if (isPendingDelete_) {
        return;
    }
    isPendingDelete_ = true;
    if (context_) {
        context_->queueDeferredDelete(this);
    }
}

void Widget::setStyle(const layout::Style& style) noexcept {
    layoutNode_.setStyle(style);
    markLayoutDirty();
}

core::Rect Widget::screenRect() const noexcept {
    core::Rect r = layoutNode_.layoutRect();
    const Widget* p = parent_;
    while (p) {
        const core::Point& parentPos = p->layoutNode_.layoutRect().origin;
        core::Point pScroll = p->scrollOffset();
        r.origin.x += parentPos.x - pScroll.x;
        r.origin.y += parentPos.y - pScroll.y;
        p = p->parent_;
    }
    return r;
}

void Widget::markLayoutDirty() noexcept {
    isLayoutDirty_ = true;
    isPaintDirty_ = true;
    layoutNode_.markDirty();
    if (parent_) {
        parent_->markLayoutDirty();
    }
}

void Widget::markPaintDirty() noexcept {
    isPaintDirty_ = true;
    if (parent_ && !parent_->isPaintDirty_) {
        parent_->markPaintDirty();
    }
}

bool Widget::isLayoutDirty() const noexcept {
    return isLayoutDirty_ || layoutNode_.isDirty();
}

void Widget::clearLayoutDirty() noexcept {
    isLayoutDirty_ = false;
    for (const auto& child : children_) {
        if (child) {
            child->clearLayoutDirty();
        }
    }
}

void Widget::setVisible(bool visible) {
    if (isVisible_ != visible) {
        isVisible_ = visible;
        layoutNode_.style().display = visible ? layout::Display::Flex : layout::Display::None;
        markLayoutDirty();
    }
}

void Widget::setEnabled(bool enabled) {
    if (isEnabled_ != enabled) {
        isEnabled_ = enabled;
        markPaintDirty();
    }
}

bool Widget::hasFocus() const noexcept {
    return context_ ? (context_->focusedWidget() == this) : isFocusedStandalone_;
}

void Widget::setFocus() {
    if (context_) {
        context_->setFocusedWidget(this);
    } else {
        isFocusedStandalone_ = true;
    }
}

void Widget::setContext(WidgetContext* context) noexcept {
    context_ = context;
    for (const auto& child : children_) {
        if (child) {
            child->setContext(context);
        }
    }
}

void Widget::paint(renderer::PaintContext&) {}
void Widget::paintOverlay(renderer::PaintContext&) {}

bool Widget::hitTest(const core::Point& localPoint) const {
    if (!isVisible_ || !isEnabled_ || isPendingDelete_) {
        return false;
    }
    const core::Size& sz = layoutNode_.layoutRect().size;
    return (localPoint.x >= 0.0f && localPoint.x <= sz.width && localPoint.y >= 0.0f &&
            localPoint.y <= sz.height);
}

void Widget::onMouseEnter() {}
void Widget::onMouseLeave() {}
void Widget::onMouseMove(WidgetMouseEvent&) {}
void Widget::onMouseDown(WidgetMouseEvent&) {}
void Widget::onMouseUp(WidgetMouseEvent&) {}
void Widget::onMouseWheel(WidgetMouseEvent&) {}
void Widget::onKeyDown(WidgetKeyEvent&) {}
void Widget::onKeyUp(WidgetKeyEvent&) {}
void Widget::onFocusGained() {}
void Widget::onFocusLost() {
    isFocusedStandalone_ = false;
}

} // namespace northern::widgets
