#include "widgets/widget_context.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/tooltip.hpp"

#include <algorithm>

namespace northern::widgets {

namespace {

void collectFocusableWidgets(Widget* node, std::vector<Widget*>& out) {
    if (!node || !node->isVisible() || !node->isEnabled() || node->isPendingDelete()) {
        return;
    }
    if (node->isFocusable()) {
        out.push_back(node);
    }
    for (const auto& child : node->children()) {
        collectFocusableWidgets(child.get(), out);
    }
}

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

WidgetContext::WidgetContext() = default;

WidgetContext::~WidgetContext() {
    dismissAllOverlays();
    dismissTooltip();
    flushDeletedWidgets();
    if (rootWidget_) {
        rootWidget_->setContext(nullptr);
    }
}

WidgetContext::WidgetContext(WidgetContext&&) noexcept = default;
WidgetContext& WidgetContext::operator=(WidgetContext&&) noexcept = default;

void WidgetContext::setRootWidget(std::unique_ptr<Widget> root) {
    flushDeletedWidgets();
    dismissTooltip();
    if (rootWidget_) {
        rootWidget_->setContext(nullptr);
    }
    hoveredWidget_ = nullptr;
    focusedWidget_ = nullptr;
    mouseGrabWidget_ = nullptr;
    rightPressTarget_ = nullptr;
    rootWidget_ = std::move(root);
    if (rootWidget_) {
        rootWidget_->setContext(this);
        rootWidget_->markLayoutDirty();
    }
    refreshCursorShape();
}

std::unique_ptr<Widget> WidgetContext::takeRootWidget() {
    flushDeletedWidgets();
    dismissTooltip();
    if (rootWidget_) {
        rootWidget_->setContext(nullptr);
    }
    hoveredWidget_ = nullptr;
    focusedWidget_ = nullptr;
    mouseGrabWidget_ = nullptr;
    rightPressTarget_ = nullptr;
    refreshCursorShape();
    return std::move(rootWidget_);
}

Widget* WidgetContext::hitTest(const core::Point& screenPos) const {
    for (auto it = overlayStack_.rbegin(); it != overlayStack_.rend(); ++it) {
        if (it->widget) {
            Widget* hit = findHitWidgetRecursive(it->widget.get(), screenPos);
            if (hit) {
                return hit;
            }
            if (it->desc.isModal) {
                return nullptr;
            }
        }
    }
    if (!rootWidget_) {
        return nullptr;
    }
    return findHitWidgetRecursive(rootWidget_.get(), screenPos);
}

void WidgetContext::handleMouseMove(const platform::MouseEvent& ev) {
    flushDeletedWidgets();

    if (mouseGrabWidget_) {
        Widget* curr = mouseGrabWidget_;
        while (curr) {
            core::Rect curRect = curr->screenRect();
            core::Point curLocalPos{ev.position.x - curRect.origin.x,
                                    ev.position.y - curRect.origin.y};
            WidgetMouseEvent wme{curLocalPos, ev.button, ev.modifiers, ev.wheelDelta};
            curr->onMouseMove(wme);
            if (wme.isAccepted()) {
                break;
            }
            curr = curr->parent();
        }
        return;
    }

    Widget* target = hitTest(ev.position);

    if (target != hoveredWidget_) {
        if (hoveredWidget_) {
            hoveredWidget_->onMouseLeave();
        }
        hoveredWidget_ = target;
        hoverDurationSeconds_ = 0.0f;
        dismissTooltip();
        if (hoveredWidget_) {
            hoveredWidget_->onMouseEnter();
        }
        refreshCursorShape();
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
    dismissTooltip();
    hoverDurationSeconds_ = 0.0f;

    if (!overlayStack_.empty()) {
        auto& top = overlayStack_.back();
        if (top.widget) {
            core::Rect topRect = top.widget->screenRect();
            if (!topRect.contains(ev.position)) {
                if (top.desc.dismissOnOutsideClick) {
                    Widget* owner = top.owner;
                    dismissTopOverlay();
                    if (owner && owner->screenRect().contains(ev.position)) {
                        return;
                    }
                } else if (top.desc.isModal) {
                    return;
                }
            }
        }
    }

    Widget* target = hitTest(ev.position);
    if (ev.button == platform::MouseButton::Right) {
        rightPressTarget_ = target;
    }

    if (target) {
        if (target->isFocusable()) {
            setFocusedWidget(target, FocusReason::Pointer);
        }
        Widget* curr = target;
        while (curr) {
            core::Rect curRect = curr->screenRect();
            core::Point curLocalPos{ev.position.x - curRect.origin.x,
                                    ev.position.y - curRect.origin.y};
            WidgetMouseEvent wme{curLocalPos, ev.button, ev.modifiers, ev.wheelDelta};
            curr->onMouseDown(wme);
            if (wme.isAccepted()) {
                mouseGrabWidget_ = curr;
                refreshCursorShape();
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
    Widget* target = mouseGrabWidget_ ? mouseGrabWidget_ : hitTest(ev.position);
    mouseGrabWidget_ = nullptr;

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

    if (ev.button == platform::MouseButton::Right) {
        Widget* releaseTarget = hitTest(ev.position);
        if (rightPressTarget_ && releaseTarget &&
            (releaseTarget == rightPressTarget_ ||
             rightPressTarget_->screenRect().contains(ev.position))) {
            Widget* curr = releaseTarget;
            while (curr) {
                core::Rect curRect = curr->screenRect();
                core::Point curLocalPos{ev.position.x - curRect.origin.x,
                                        ev.position.y - curRect.origin.y};
                WidgetMouseEvent wme{curLocalPos, ev.button, ev.modifiers, ev.wheelDelta};
                curr->onContextMenu(wme);
                if (wme.isAccepted()) {
                    break;
                }
                curr = curr->parent();
            }
        }
        rightPressTarget_ = nullptr;
    }

    Widget* actualHover = hitTest(ev.position);
    if (actualHover != hoveredWidget_) {
        if (hoveredWidget_) {
            hoveredWidget_->onMouseLeave();
        }
        hoveredWidget_ = actualHover;
        hoverDurationSeconds_ = 0.0f;
        dismissTooltip();
        if (hoveredWidget_) {
            hoveredWidget_->onMouseEnter();
        }
    }
    refreshCursorShape();
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

void WidgetContext::handleMouseLeave() {
    flushDeletedWidgets();
    if (hoveredWidget_) {
        hoveredWidget_->onMouseLeave();
        hoveredWidget_ = nullptr;
    }
    hoverDurationSeconds_ = 0.0f;
    dismissTooltip();
    refreshCursorShape();
}

void WidgetContext::handleKeyEvent(const platform::KeyEvent& ev) {
    flushDeletedWidgets();
    if (ev.isDown) {
        dismissTooltip();
    }

    if (ev.isDown && ev.key == platform::Key::Escape && !overlayStack_.empty()) {
        auto& top = overlayStack_.back();
        if (top.desc.dismissOnEscape) {
            dismissTopOverlay();
            return;
        }
    }

    bool accepted = false;
    Widget* curr = focusedWidget_;
    while (curr) {
        WidgetKeyEvent wke{ev.key, ev.isDown, ev.modifiers, ev.textCodepoint};
        if (ev.isDown) {
            curr->onKeyDown(wke);
        } else {
            curr->onKeyUp(wke);
        }
        if (wke.isAccepted()) {
            accepted = true;
            break;
        }
        curr = curr->parent();
    }

    if (!accepted && ev.isDown && ev.key == platform::Key::Tab) {
        bool reverse = platform::hasModifier(ev.modifiers, platform::KeyModifier::Shift);
        focusNext(reverse);
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

void WidgetContext::setFocusedWidget(Widget* widget, FocusReason reason) {
    if (focusedWidget_ == widget) {
        if (widget && reason != FocusReason::Programmatic && focusReason_ != reason) {
            focusReason_ = reason;
            focusedWidget_->markPaintDirty();
        }
        return;
    }
    if (focusedWidget_) {
        focusedWidget_->onFocusLost();
        focusedWidget_->markPaintDirty();
    }
    focusedWidget_ = widget;
    focusReason_ = reason;
    if (focusedWidget_) {
        focusedWidget_->onFocusGained();
        focusedWidget_->markPaintDirty();
    }
}

void WidgetContext::clearFocus() {
    setFocusedWidget(nullptr, FocusReason::Programmatic);
}

void WidgetContext::focusNext(bool reverse) {
    Widget* scope = nullptr;
    if (!overlayStack_.empty() && overlayStack_.back().widget) {
        scope = overlayStack_.back().widget.get();
    } else {
        scope = rootWidget_.get();
    }
    if (!scope) {
        return;
    }

    std::vector<Widget*> focusables;
    collectFocusableWidgets(scope, focusables);
    if (focusables.empty()) {
        return;
    }

    auto it = std::find(focusables.begin(), focusables.end(), focusedWidget_);
    Widget* next = nullptr;
    if (it == focusables.end()) {
        next = reverse ? focusables.back() : focusables.front();
    } else {
        size_t idx = static_cast<size_t>(std::distance(focusables.begin(), it));
        size_t count = focusables.size();
        size_t nextIdx = reverse ? (idx + count - 1) % count : (idx + 1) % count;
        next = focusables[nextIdx];
    }

    setFocusedWidget(next, FocusReason::Keyboard);
}

void WidgetContext::refreshCursorShape() {
    Widget* target = mouseGrabWidget_ ? mouseGrabWidget_ : hoveredWidget_;
    platform::CursorShape next = platform::CursorShape::Arrow;
    if (target && target->isEnabled()) {
        next = target->cursorShape();
    }
    if (currentCursorShape_ != next) {
        currentCursorShape_ = next;
        onCursorShapeChanged.emit(currentCursorShape_);
    }
}

void WidgetContext::setTooltipFont(std::shared_ptr<text::Font> font) {
    tooltipFont_ = std::move(font);
    if (activeTooltip_) {
        activeTooltip_->setFont(tooltipFont_);
    }
}

void WidgetContext::dismissTooltip() {
    tooltipVisible_ = false;
    tooltipAlphaAnim_.stop();
    activeTooltip_.reset();
}

void WidgetContext::showTooltipForHoveredWidget() {
    if (!hoveredWidget_ || hoveredWidget_->tooltipText().empty() || !overlayStack_.empty()) {
        return;
    }

    auto tip = std::make_unique<Tooltip>(hoveredWidget_->tooltipText(),
                                         hoveredWidget_->tooltipShortcut(), tooltipFont_);
    tip->setContext(this);

    core::Size tipSize = tip->preferredSize();
    core::Size viewport = (currentViewportSize_.width > 0.0f && currentViewportSize_.height > 0.0f)
                              ? currentViewportSize_
                              : core::Size{1920.0f, 1080.0f};

    OverlayDesc desc{};
    desc.anchorRect = hoveredWidget_->screenRect();
    desc.preferredPlacement = OverlayPlacement::Bottom;
    desc.autoFlip = true;
    desc.gap = 6.0f;

    core::Point pos = computeOverlayPosition(viewport, tipSize, desc);
    tip->layoutNode().setLayoutRect(core::Rect{pos, tipSize});
    tip->setAlpha(0.0f);

    activeTooltip_ = std::move(tip);
    tooltipVisible_ = true;
    tooltipAlphaAnim_.start(0.0f, 1.0f, 0.12f, core::animation::EasingCurve::OutCubic);
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
            hoverDurationSeconds_ = 0.0f;
            dismissTooltip();
        }
        if (focusedWidget_ == w) {
            focusedWidget_ = nullptr;
        }
        if (mouseGrabWidget_ == w) {
            mouseGrabWidget_ = nullptr;
        }
        if (rightPressTarget_ == w) {
            rightPressTarget_ = nullptr;
        }
        dismissOverlaysForOwner(w);
        for (auto it = overlayStack_.begin(); it != overlayStack_.end();) {
            if (it->widget.get() == w) {
                it = overlayStack_.erase(it);
            } else {
                ++it;
            }
        }
        if (w == rootWidget_.get()) {
            rootWidget_.reset();
        } else if (w->parent()) {
            w->parent()->removeChild(w);
        }
    }
    refreshCursorShape();
}

void WidgetContext::unregisterWidget(Widget* widget) {
    bool needCursorRefresh = false;
    if (hoveredWidget_ == widget) {
        hoveredWidget_ = nullptr;
        hoverDurationSeconds_ = 0.0f;
        dismissTooltip();
        needCursorRefresh = true;
    }
    if (focusedWidget_ == widget) {
        focusedWidget_ = nullptr;
    }
    if (mouseGrabWidget_ == widget) {
        mouseGrabWidget_ = nullptr;
        needCursorRefresh = true;
    }
    if (rightPressTarget_ == widget) {
        rightPressTarget_ = nullptr;
    }
    dismissOverlaysForOwner(widget);
    for (auto it = overlayStack_.begin(); it != overlayStack_.end();) {
        if (it->widget.get() == widget) {
            it = overlayStack_.erase(it);
        } else {
            ++it;
        }
    }
    std::erase(pendingDeletions_, widget);
    std::erase(animatedWidgets_, widget);
    if (needCursorRefresh) {
        refreshCursorShape();
    }
}

void WidgetContext::registerAnimatedWidget(Widget* widget) {
    if (widget && std::find(animatedWidgets_.begin(), animatedWidgets_.end(), widget) ==
                      animatedWidgets_.end()) {
        animatedWidgets_.push_back(widget);
    }
}

void WidgetContext::unregisterAnimatedWidget(Widget* widget) {
    std::erase(animatedWidgets_, widget);
}

void WidgetContext::advanceAnimations(float deltaSeconds) {
    if (deltaSeconds <= 0.0f) {
        return;
    }

    animationManager_.advance(deltaSeconds);

    if (!animatedWidgets_.empty()) {
        auto widgetsCopy = animatedWidgets_;
        for (Widget* w : widgetsCopy) {
            if (std::find(animatedWidgets_.begin(), animatedWidgets_.end(), w) !=
                animatedWidgets_.end()) {
                w->updateAnimations(deltaSeconds);
                if (!w->hasActiveAnimations()) {
                    std::erase(animatedWidgets_, w);
                }
            }
        }
    }

    if (hoveredWidget_ && !hoveredWidget_->tooltipText().empty() && !mouseGrabWidget_ &&
        overlayStack_.empty()) {
        if (!tooltipVisible_) {
            hoverDurationSeconds_ += deltaSeconds;
            if (hoverDurationSeconds_ >= tooltipDelaySeconds_) {
                showTooltipForHoveredWidget();
            }
        }
    }

    if (tooltipVisible_ && activeTooltip_ && tooltipAlphaAnim_.isRunning()) {
        tooltipAlphaAnim_.update(deltaSeconds);
        activeTooltip_->setAlpha(tooltipAlphaAnim_.currentValue());
    }
}

void WidgetContext::render(renderer::PaintContext& ctx, const core::Size& viewportSize) {
    flushDeletedWidgets();
    if (currentViewportSize_ != viewportSize) {
        currentViewportSize_ = viewportSize;
        if (rootWidget_) {
            rootWidget_->markLayoutDirty();
        }
    }

    if (rootWidget_) {
        if (rootWidget_->isLayoutDirty()) {
            rootWidget_->layoutNode().computeLayout(viewportSize.width, viewportSize.height);
            rootWidget_->clearLayoutDirty();
        }
        renderWidget(rootWidget_.get(), ctx);
    }

    // Render active overlays above the root hierarchy
    for (auto& entry : overlayStack_) {
        if (entry.widget && entry.widget->isVisible()) {
            if (entry.desc.isModal) {
                ctx.drawRect(core::Rect{{0.0f, 0.0f}, viewportSize},
                             core::Color{0.0f, 0.0f, 0.0f, 0.55f});
            }
            if (entry.widget->isLayoutDirty()) {
                entry.widget->layoutNode().computeLayout(viewportSize.width, viewportSize.height);
                core::Size overlaySize = entry.widget->layoutNode().layoutRect().size;
                entry.computedPosition =
                    computeOverlayPosition(viewportSize, overlaySize, entry.desc);
                entry.widget->layoutNode().setLayoutRect(
                    core::Rect{entry.computedPosition, overlaySize});
                entry.widget->clearLayoutDirty();
            }
            renderWidget(entry.widget.get(), ctx);
        }
    }

    if (tooltipVisible_ && activeTooltip_) {
        activeTooltip_->paint(ctx);
        activeTooltip_->clearPaintDirty();
    }
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

Widget* WidgetContext::showOverlay(std::unique_ptr<Widget> content, const OverlayDesc& desc,
                                   Widget* owner) {
    if (!content) {
        return nullptr;
    }
    dismissTooltip();
    content->setContext(this);

    float availW = currentViewportSize_.width > 0.0f ? currentViewportSize_.width : 1920.0f;
    float availH = currentViewportSize_.height > 0.0f ? currentViewportSize_.height : 1080.0f;
    content->layoutNode().computeLayout(availW, availH);

    core::Size overlaySize = content->layoutNode().layoutRect().size;
    core::Point pos = computeOverlayPosition(core::Size{availW, availH}, overlaySize, desc);
    content->layoutNode().setLayoutRect(core::Rect{pos, overlaySize});

    OverlayEntry entry{};
    entry.widget = std::move(content);
    entry.desc = desc;
    entry.owner = owner;
    entry.computedPosition = pos;

    Widget* ptr = entry.widget.get();
    overlayStack_.push_back(std::move(entry));
    return ptr;
}

void WidgetContext::dismissTopOverlay() {
    if (overlayStack_.empty()) {
        return;
    }
    auto entry = std::move(overlayStack_.back());
    overlayStack_.pop_back();

    Widget* w = entry.widget.get();
    if (hoveredWidget_ && (hoveredWidget_ == w || hoveredWidget_->isDescendantOf(w))) {
        hoveredWidget_->onMouseLeave();
        hoveredWidget_ = nullptr;
    }
    if (focusedWidget_ && (focusedWidget_ == w || focusedWidget_->isDescendantOf(w))) {
        clearFocus();
    }
    if (mouseGrabWidget_ && (mouseGrabWidget_ == w || mouseGrabWidget_->isDescendantOf(w))) {
        mouseGrabWidget_ = nullptr;
    }
    if (entry.desc.onDismiss) {
        entry.desc.onDismiss();
    }
}

void WidgetContext::dismissAllOverlays() {
    while (!overlayStack_.empty()) {
        dismissTopOverlay();
    }
}

void WidgetContext::dismissOverlaysForOwner(Widget* owner) {
    if (!owner) {
        return;
    }
    for (auto it = overlayStack_.begin(); it != overlayStack_.end();) {
        if (it->owner == owner) {
            Widget* w = it->widget.get();
            if (hoveredWidget_ && (hoveredWidget_ == w || hoveredWidget_->isDescendantOf(w))) {
                hoveredWidget_->onMouseLeave();
                hoveredWidget_ = nullptr;
            }
            if (focusedWidget_ && (focusedWidget_ == w || focusedWidget_->isDescendantOf(w))) {
                clearFocus();
            }
            if (mouseGrabWidget_ &&
                (mouseGrabWidget_ == w || mouseGrabWidget_->isDescendantOf(w))) {
                mouseGrabWidget_ = nullptr;
            }
            auto onDismiss = std::move(it->desc.onDismiss);
            it = overlayStack_.erase(it);
            if (onDismiss) {
                onDismiss();
            }
        } else {
            ++it;
        }
    }
}

} // namespace northern::widgets
