#include "widgets/widget_context.hpp"
#include "renderer/paint_context.hpp"

#include <algorithm>

namespace northern::widgets {

namespace {

Widget* findHitWidgetRecursive(Widget* node, const core::Point& screenPos) {
    if (!node || !node->isVisible() || !node->isEnabled() || node->isPendingDelete()) {
        return nullptr;
    }
    core::Rect sRect = node->screenRect();
    if (!sRect.contains(screenPos)) {
        return nullptr;
    }
    core::Point localPoint{screenPos.x - sRect.origin.x, screenPos.y - sRect.origin.y};
    if (!node->hitTest(localPoint)) {
        return nullptr;
    }

    for (size_t i = node->childCount(); i > 0; --i) {
        Widget* child = node->childAt(i - 1);
        Widget* hit = findHitWidgetRecursive(child, screenPos);
        if (hit) {
            return hit;
        }
    }
    return node;
}

} // namespace

WidgetContext::~WidgetContext() {
    flushDeletedWidgets();
    if (rootWidget_) {
        rootWidget_->setContext(nullptr);
    }
}

void WidgetContext::setRootWidget(std::unique_ptr<Widget> root) {
    flushDeletedWidgets();
    if (rootWidget_) {
        rootWidget_->setContext(nullptr);
    }
    hoveredWidget_ = nullptr;
    focusedWidget_ = nullptr;
    rootWidget_ = std::move(root);
    if (rootWidget_) {
        rootWidget_->setContext(this);
        rootWidget_->markLayoutDirty();
    }
}

std::unique_ptr<Widget> WidgetContext::takeRootWidget() {
    flushDeletedWidgets();
    if (rootWidget_) {
        rootWidget_->setContext(nullptr);
    }
    hoveredWidget_ = nullptr;
    focusedWidget_ = nullptr;
    return std::move(rootWidget_);
}

Widget* WidgetContext::hitTest(const core::Point& screenPos) const {
    if (!rootWidget_) {
        return nullptr;
    }
    return findHitWidgetRecursive(rootWidget_.get(), screenPos);
}

void WidgetContext::handleMouseMove(const platform::MouseEvent& ev) {
    flushDeletedWidgets();
    Widget* target = hitTest(ev.position);

    if (target != hoveredWidget_) {
        if (hoveredWidget_) {
            hoveredWidget_->onMouseLeave();
        }
        hoveredWidget_ = target;
        if (hoveredWidget_) {
            hoveredWidget_->onMouseEnter();
        }
    }

    Widget* curr = hoveredWidget_;
    while (curr) {
        core::Rect curRect = curr->screenRect();
        core::Point curLocalPos{ev.position.x - curRect.origin.x, ev.position.y - curRect.origin.y};
        WidgetMouseEvent wme{curLocalPos, ev.button, ev.modifiers, ev.wheelDelta};
        curr->onMouseMove(wme);
        if (wme.isAccepted()) {
            break;
        }
        curr = curr->parent();
    }
}

void WidgetContext::handleMouseDown(const platform::MouseEvent& ev) {
    flushDeletedWidgets();
    Widget* target = hitTest(ev.position);

    if (target) {
        if (target->isFocusable() && target != focusedWidget_) {
            setFocusedWidget(target);
        }
        Widget* curr = target;
        while (curr) {
            core::Rect curRect = curr->screenRect();
            core::Point curLocalPos{ev.position.x - curRect.origin.x,
                                    ev.position.y - curRect.origin.y};
            WidgetMouseEvent wme{curLocalPos, ev.button, ev.modifiers, ev.wheelDelta};
            curr->onMouseDown(wme);
            if (wme.isAccepted()) {
                break;
            }
            curr = curr->parent();
        }
    } else {
        clearFocus();
    }
}

void WidgetContext::handleMouseUp(const platform::MouseEvent& ev) {
    flushDeletedWidgets();
    Widget* target = hitTest(ev.position);

    if (target) {
        Widget* curr = target;
        while (curr) {
            core::Rect curRect = curr->screenRect();
            core::Point curLocalPos{ev.position.x - curRect.origin.x,
                                    ev.position.y - curRect.origin.y};
            WidgetMouseEvent wme{curLocalPos, ev.button, ev.modifiers, ev.wheelDelta};
            curr->onMouseUp(wme);
            if (wme.isAccepted()) {
                break;
            }
            curr = curr->parent();
        }
    }
}

void WidgetContext::handleMouseWheel(const platform::MouseEvent& ev) {
    flushDeletedWidgets();
    Widget* target = hitTest(ev.position);
    if (!target) {
        target = hoveredWidget_;
    }
    if (!target) {
        target = rootWidget_.get();
    }

    Widget* curr = target;
    while (curr) {
        core::Rect curRect = curr->screenRect();
        core::Point curLocalPos{ev.position.x - curRect.origin.x, ev.position.y - curRect.origin.y};
        WidgetMouseEvent wme{curLocalPos, ev.button, ev.modifiers, ev.wheelDelta};
        curr->onMouseWheel(wme);
        if (wme.isAccepted()) {
            break;
        }
        curr = curr->parent();
    }
}

void WidgetContext::handleKeyEvent(const platform::KeyEvent& ev) {
    flushDeletedWidgets();
    if (!focusedWidget_) {
        return;
    }

    Widget* curr = focusedWidget_;
    while (curr) {
        WidgetKeyEvent wke{ev.key, ev.isDown, ev.modifiers, ev.textCodepoint};
        if (ev.isDown) {
            curr->onKeyDown(wke);
        } else {
            curr->onKeyUp(wke);
        }
        if (wke.isAccepted()) {
            break;
        }
        curr = curr->parent();
    }
}

void WidgetContext::handleResize(const core::Size& logicalSize) {
    currentViewportSize_ = logicalSize;
    if (rootWidget_) {
        rootWidget_->layoutNode().computeLayout(logicalSize.width, logicalSize.height);
        rootWidget_->clearLayoutDirty();
        rootWidget_->markPaintDirty();
    }
}

void WidgetContext::setFocusedWidget(Widget* widget) {
    if (focusedWidget_ == widget) {
        return;
    }
    if (focusedWidget_) {
        focusedWidget_->onFocusLost();
    }
    focusedWidget_ = widget;
    if (focusedWidget_) {
        focusedWidget_->onFocusGained();
    }
}

void WidgetContext::clearFocus() {
    setFocusedWidget(nullptr);
}

void WidgetContext::queueDeferredDelete(Widget* widget) {
    if (!widget) {
        return;
    }
    if (std::find(pendingDeletions_.begin(), pendingDeletions_.end(), widget) ==
        pendingDeletions_.end()) {
        pendingDeletions_.push_back(widget);
    }
}

void WidgetContext::flushDeletedWidgets() {
    while (!pendingDeletions_.empty()) {
        Widget* w = pendingDeletions_.front();
        pendingDeletions_.erase(pendingDeletions_.begin());
        if (!w) {
            continue;
        }
        if (hoveredWidget_ == w) {
            hoveredWidget_ = nullptr;
        }
        if (focusedWidget_ == w) {
            focusedWidget_ = nullptr;
        }
        if (w == rootWidget_.get()) {
            rootWidget_.reset();
        } else if (w->parent()) {
            w->parent()->removeChild(w);
        }
    }
}

void WidgetContext::unregisterWidget(Widget* widget) {
    if (hoveredWidget_ == widget) {
        hoveredWidget_ = nullptr;
    }
    if (focusedWidget_ == widget) {
        focusedWidget_ = nullptr;
    }
    std::erase(pendingDeletions_, widget);
}

void WidgetContext::render(renderer::PaintContext& ctx, const core::Size& viewportSize) {
    flushDeletedWidgets();
    if (!rootWidget_) {
        return;
    }

    if (currentViewportSize_ != viewportSize || rootWidget_->isLayoutDirty()) {
        currentViewportSize_ = viewportSize;
        rootWidget_->layoutNode().computeLayout(viewportSize.width, viewportSize.height);
        rootWidget_->clearLayoutDirty();
    }

    renderWidget(rootWidget_.get(), ctx);
}

void WidgetContext::renderWidget(Widget* widget, renderer::PaintContext& ctx) {
    if (!widget || !widget->isVisible()) {
        return;
    }
    bool clip = widget->clipsChildren();
    if (clip) {
        ctx.pushClip(widget->screenRect());
    }
    widget->paint(ctx);
    widget->clearPaintDirty();
    for (const auto& child : widget->children()) {
        renderWidget(child.get(), ctx);
    }
    if (clip) {
        ctx.popClip();
        widget->paintOverlay(ctx);
    }
}

} // namespace northern::widgets
