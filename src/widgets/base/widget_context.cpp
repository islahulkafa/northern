#include "widgets/base/widget_context.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/controls/tooltip.hpp"

#include <algorithm>
#include <cctype>

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
    updateHoverTarget(target);

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
        bool clickedInsideAny = false;
        for (const auto& entry : overlayStack_) {
            if (entry.widget && entry.widget->screenRect().contains(ev.position)) {
                clickedInsideAny = true;
                break;
            }
        }
        if (!clickedInsideAny) {
            auto& top = overlayStack_.back();
            if (top.desc.dismissOnOutsideClick) {
                Widget* owner = top.owner;
                if (owner && owner->screenRect().contains(ev.position)) {
                    dismissTopOverlay();
                    return;
                }
                dismissAllOverlays();
            } else if (top.desc.isModal) {
                return;
            }
        } else {
            while (!overlayStack_.empty()) {
                auto& top = overlayStack_.back();
                if (top.widget && top.widget->screenRect().contains(ev.position)) {
                    break;
                }
                if (top.desc.dismissOnOutsideClick) {
                    dismissTopOverlay();
                } else {
                    break;
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
    updateHoverTarget(actualHover);
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
    updateHoverTarget(nullptr);
}

std::optional<KeyChord> WidgetContext::parseShortcut(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    KeyChord chord{};
    size_t start = 0;
    while (start < str.size()) {
        size_t plusPos = str.find('+', start);
        std::string_view token;
        if (plusPos == std::string_view::npos) {
            token = str.substr(start);
            start = str.size();
        } else if (plusPos == start && start > 0) {
            token = "+";
            start = str.size();
        } else {
            token = str.substr(start, plusPos - start);
            start = plusPos + 1;
        }

        while (!token.empty() && std::isspace(static_cast<unsigned char>(token.front()))) {
            token.remove_prefix(1);
        }
        while (!token.empty() && std::isspace(static_cast<unsigned char>(token.back()))) {
            token.remove_suffix(1);
        }

        if (token.empty()) {
            continue;
        }

        std::string lower;
        lower.reserve(token.size());
        for (char c : token) {
            lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }

        if (lower == "ctrl" || lower == "control") {
            chord.modifiers |= platform::KeyModifier::Control;
        } else if (lower == "shift") {
            chord.modifiers |= platform::KeyModifier::Shift;
        } else if (lower == "alt") {
            chord.modifiers |= platform::KeyModifier::Alt;
        } else if (lower == "super" || lower == "cmd" || lower == "win") {
            chord.modifiers |= platform::KeyModifier::Super;
        } else {
            if (lower.size() == 1) {
                char c = lower[0];
                if (c >= 'a' && c <= 'z') {
                    chord.key = static_cast<platform::Key>(static_cast<uint32_t>(platform::Key::A) +
                                                           (c - 'a'));
                } else if (c >= '0' && c <= '9') {
                    chord.key = static_cast<platform::Key>(
                        static_cast<uint32_t>(platform::Key::Num0) + (c - '0'));
                } else if (c == '+') {
                    chord.key = platform::Key::Equal;
                } else if (c == '-') {
                    chord.key = platform::Key::Minus;
                } else if (c == '/') {
                    chord.key = platform::Key::Slash;
                } else if (c == ',') {
                    chord.key = platform::Key::Comma;
                } else if (c == '.') {
                    chord.key = platform::Key::Period;
                }
            } else if (lower[0] == 'f' && lower.size() >= 2 && lower.size() <= 3) {
                int fNum = 0;
                for (size_t i = 1; i < lower.size(); ++i) {
                    if (lower[i] >= '0' && lower[i] <= '9') {
                        fNum = fNum * 10 + (lower[i] - '0');
                    } else {
                        fNum = 0;
                        break;
                    }
                }
                if (fNum >= 1 && fNum <= 24) {
                    chord.key = static_cast<platform::Key>(
                        static_cast<uint32_t>(platform::Key::F1) + (fNum - 1));
                }
            } else if (lower == "escape" || lower == "esc") {
                chord.key = platform::Key::Escape;
            } else if (lower == "enter" || lower == "return") {
                chord.key = platform::Key::Enter;
            } else if (lower == "tab") {
                chord.key = platform::Key::Tab;
            } else if (lower == "space") {
                chord.key = platform::Key::Space;
            } else if (lower == "backspace") {
                chord.key = platform::Key::Backspace;
            } else if (lower == "delete" || lower == "del") {
                chord.key = platform::Key::Delete;
            } else if (lower == "insert" || lower == "ins") {
                chord.key = platform::Key::Insert;
            } else if (lower == "home") {
                chord.key = platform::Key::Home;
            } else if (lower == "end") {
                chord.key = platform::Key::End;
            } else if (lower == "pageup" || lower == "pgup") {
                chord.key = platform::Key::PageUp;
            } else if (lower == "pagedown" || lower == "pgdn") {
                chord.key = platform::Key::PageDown;
            } else if (lower == "up") {
                chord.key = platform::Key::Up;
            } else if (lower == "down") {
                chord.key = platform::Key::Down;
            } else if (lower == "left") {
                chord.key = platform::Key::Left;
            } else if (lower == "right") {
                chord.key = platform::Key::Right;
            }
        }
    }

    if (chord.key == platform::Key::Unknown) {
        return std::nullopt;
    }
    return chord;
}

void WidgetContext::registerAccelerator(const std::string& shortcutStr,
                                        std::function<void()> callback) {
    if (auto chord = parseShortcut(shortcutStr)) {
        registerAccelerator(*chord, std::move(callback));
    }
}

void WidgetContext::registerAccelerator(const KeyChord& chord, std::function<void()> callback) {
    if (chord.key != platform::Key::Unknown && callback) {
        accelerators_[chord] = std::move(callback);
    }
}

void WidgetContext::unregisterAccelerator(const std::string& shortcutStr) {
    if (auto chord = parseShortcut(shortcutStr)) {
        unregisterAccelerator(*chord);
    }
}

void WidgetContext::unregisterAccelerator(const KeyChord& chord) {
    accelerators_.erase(chord);
}

bool WidgetContext::dispatchAccelerator(platform::Key key, platform::KeyModifier modifiers) {
    if (accelerators_.empty() || key == platform::Key::Unknown) {
        return false;
    }
    platform::KeyModifier relevant =
        modifiers & (platform::KeyModifier::Shift | platform::KeyModifier::Control |
                     platform::KeyModifier::Alt | platform::KeyModifier::Super);
    KeyChord chord{key, relevant};
    auto it = accelerators_.find(chord);
    if (it != accelerators_.end() && it->second) {
        it->second();
        return true;
    }
    return false;
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

    if (ev.isDown && dispatchAccelerator(ev.key, ev.modifiers)) {
        return;
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
    for (auto& entry : overlayStack_) {
        if (entry.widget) {
            entry.widget->markLayoutDirty();
        }
    }
}

void WidgetContext::handleFileDrop(const platform::FileDropEvent& ev) {
    flushDeletedWidgets();
    Widget* target = hitTest(ev.position);
    if (!target) {
        target = rootWidget_.get();
    }
    Widget* curr = target;
    while (curr) {
        core::Rect sRect = curr->screenRect();
        core::Point localPos{ev.position.x - sRect.origin.x, ev.position.y - sRect.origin.y};
        WidgetFileDropEvent wfde{localPos, ev.position, ev.paths};
        curr->onFileDrop(wfde);
        if (wfde.isAccepted()) {
            break;
        }
        curr = curr->parent();
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
    Widget* oldFocused = focusedWidget_;
    focusedWidget_ = widget;
    focusReason_ = reason;
    if (oldFocused) {
        oldFocused->onFocusLost();
        oldFocused->markPaintDirty();
    }
    if (focusedWidget_) {
        focusedWidget_->onFocusGained();
        focusedWidget_->markPaintDirty();
    }
}

void WidgetContext::clearFocus() {
    setFocusedWidget(nullptr, FocusReason::Programmatic);
}

void WidgetContext::setDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    dpiScale_ = clamped;
    setTooltipDpiScale(clamped);
}

void WidgetContext::setTheme(theme::Theme theme) {
    theme_ = std::move(theme);
    isPaintDirty_ = true;

    auto notifyThemeRecursive = [](auto& self, Widget* w) -> void {
        if (!w) {
            return;
        }
        w->onThemeChanged();
        w->markPaintDirty();
        for (const auto& child : w->children()) {
            self(self, child.get());
        }
    };

    notifyThemeRecursive(notifyThemeRecursive, rootWidget_.get());
    if (activeTooltip_) {
        activeTooltip_->onThemeChanged();
        tooltipDirty_ = true;
    }
    for (const auto& entry : overlayStack_) {
        notifyThemeRecursive(notifyThemeRecursive, entry.widget.get());
    }
    onThemeChanged.emit(theme_);
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

void WidgetContext::updateHoverTarget(Widget* newHover) {
    if (newHover == hoveredWidget_) {
        return;
    }

    Widget* oldHover = hoveredWidget_;
    hoveredWidget_ = newHover;
    hoverDurationSeconds_ = 0.0f;
    dismissTooltip();

    std::vector<Widget*> oldChain;
    for (Widget* w = oldHover; w; w = w->parent()) {
        oldChain.push_back(w);
    }

    std::vector<Widget*> newChain;
    for (Widget* w = newHover; w; w = w->parent()) {
        newChain.push_back(w);
    }

    // Notify leave to widgets that are in oldChain but not in newChain
    for (Widget* w : oldChain) {
        if (std::find(newChain.begin(), newChain.end(), w) == newChain.end()) {
            w->onMouseLeave();
        }
    }

    // Notify enter to widgets that are in newChain but not in oldChain (ancestors first)
    for (auto it = newChain.rbegin(); it != newChain.rend(); ++it) {
        Widget* w = *it;
        if (std::find(oldChain.begin(), oldChain.end(), w) == oldChain.end()) {
            w->onMouseEnter();
        }
    }

    refreshCursorShape();
}

void WidgetContext::refreshCursorShape() {
    Widget* target = mouseGrabWidget_ ? mouseGrabWidget_ : hoveredWidget_;
    platform::CursorShape next = platform::CursorShape::Arrow;
    for (Widget* cur = target; cur && cur->isEnabled(); cur = cur->parent()) {
        if (cur->cursorShape() != platform::CursorShape::Arrow) {
            next = cur->cursorShape();
            break;
        }
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

void WidgetContext::setTooltipDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (tooltipDpiScale_ != clamped) {
        tooltipDpiScale_ = clamped;
        if (activeTooltip_) {
            activeTooltip_->setDpiScale(tooltipDpiScale_);
        }
    }
}

Widget* WidgetContext::resolveTooltipSource(Widget* hovered) const noexcept {
    for (Widget* curr = hovered; curr; curr = curr->parent()) {
        if (curr->isVisible() && curr->isEnabled() && !curr->tooltipText().empty()) {
            return curr;
        }
    }
    return nullptr;
}

void WidgetContext::dismissTooltip() {
    bool wasVisible = tooltipVisible_ || activeTooltip_ != nullptr;
    if (tooltipVisible_) {
        tooltipWarmupRemainingSeconds_ = tooltipWarmupWindowSeconds_;
    }
    tooltipVisible_ = false;
    tooltipAlphaAnim_.stop();
    activeTooltip_.reset();
    if (wasVisible) {
        markTooltipDirty();
    }
}

void WidgetContext::showTooltipForHoveredWidget() {
    Widget* source = resolveTooltipSource(hoveredWidget_);
    if (!source || !overlayStack_.empty()) {
        return;
    }

    auto tip = std::make_unique<Tooltip>(source->tooltipText(), source->tooltipShortcut(),
                                         tooltipFont_, source->tooltipDescription());
    tip->setDpiScale(tooltipDpiScale_);

    core::Size tipSize = tip->preferredSize();
    core::Size viewport = (currentViewportSize_.width > 0.0f && currentViewportSize_.height > 0.0f)
                              ? currentViewportSize_
                              : core::Size{1920.0f, 1080.0f};

    OverlayDesc desc{};
    desc.anchorRect = source->screenRect();
    desc.preferredPlacement = source->tooltipPlacement();
    desc.autoFlip = true;
    desc.gap = 6.0f * tooltipDpiScale_;

    core::Point pos = computeOverlayPosition(viewport, tipSize, desc);
    tip->layoutNode().setLayoutRect(core::Rect{pos, tipSize});
    tip->setAlpha(0.0f);
    tip->setContext(this);

    activeTooltip_ = std::move(tip);
    tooltipVisible_ = true;
    tooltipWarmupRemainingSeconds_ = tooltipWarmupWindowSeconds_;
    tooltipDirty_ = true;
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
    if (pendingDeletions_.empty()) {
        return;
    }
    std::vector<Widget*> toDelete;
    toDelete.swap(pendingDeletions_);

    for (Widget* w : toDelete) {
        if (!w) {
            continue;
        }
        if (hoveredWidget_ && (hoveredWidget_ == w || hoveredWidget_->isDescendantOf(w))) {
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

void WidgetContext::clearInteractionForSubtree(Widget* subtree) {
    if (!subtree) {
        return;
    }
    auto matchesSubtree = [subtree](Widget* w) {
        return w && (w == subtree || w->isDescendantOf(subtree));
    };

    bool needCursorRefresh = false;
    if (matchesSubtree(hoveredWidget_)) {
        Widget* prevHover = hoveredWidget_;
        hoveredWidget_ = nullptr;
        hoverDurationSeconds_ = 0.0f;
        dismissTooltip();
        if (prevHover) {
            prevHover->onMouseLeave();
        }
        needCursorRefresh = true;
    }
    if (matchesSubtree(focusedWidget_)) {
        clearFocus();
    }
    if (matchesSubtree(mouseGrabWidget_)) {
        mouseGrabWidget_ = nullptr;
        needCursorRefresh = true;
    }
    if (matchesSubtree(rightPressTarget_)) {
        rightPressTarget_ = nullptr;
    }
    if (needCursorRefresh) {
        refreshCursorShape();
    }
}

void WidgetContext::unregisterWidget(Widget* widget) {
    if (!widget) {
        return;
    }
    auto matchesSubtree = [widget](Widget* w) {
        return w && (w == widget || w->isDescendantOf(widget));
    };

    bool needCursorRefresh = false;
    if (matchesSubtree(hoveredWidget_)) {
        hoveredWidget_ = nullptr;
        hoverDurationSeconds_ = 0.0f;
        dismissTooltip();
        needCursorRefresh = true;
    }
    if (matchesSubtree(focusedWidget_)) {
        focusedWidget_ = nullptr;
    }
    if (matchesSubtree(mouseGrabWidget_)) {
        mouseGrabWidget_ = nullptr;
        needCursorRefresh = true;
    }
    if (matchesSubtree(rightPressTarget_)) {
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
    std::erase_if(pendingDeletions_, matchesSubtree);
    std::erase_if(animatedWidgets_, matchesSubtree);
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

    if (!tooltipVisible_ && tooltipWarmupRemainingSeconds_ > 0.0f) {
        tooltipWarmupRemainingSeconds_ =
            std::max(0.0f, tooltipWarmupRemainingSeconds_ - deltaSeconds);
    }

    if (resolveTooltipSource(hoveredWidget_) && !mouseGrabWidget_ && overlayStack_.empty()) {
        if (!tooltipVisible_) {
            hoverDurationSeconds_ += deltaSeconds;
            float activeDelay = (tooltipWarmupRemainingSeconds_ > 0.0f)
                                    ? std::min(tooltipDelaySeconds_, tooltipWarmupDelaySeconds_)
                                    : tooltipDelaySeconds_;
            if (hoverDurationSeconds_ >= activeDelay) {
                showTooltipForHoveredWidget();
            }
        }
    }

    if (tooltipVisible_ && activeTooltip_ && tooltipAlphaAnim_.isRunning()) {
        tooltipAlphaAnim_.update(deltaSeconds);
        activeTooltip_->setAlpha(tooltipAlphaAnim_.currentValue());
    }

    for (auto& entry : overlayStack_) {
        if (entry.desc.isModal && entry.scrimAnim.isRunning()) {
            entry.scrimAnim.update(deltaSeconds);
            isPaintDirty_ = true;
        }
    }
}

void WidgetContext::render(renderer::PaintContext& ctx, const core::Size& viewportSize) {
    flushDeletedWidgets();
    if (currentViewportSize_ != viewportSize) {
        currentViewportSize_ = viewportSize;
        isPaintDirty_ = true;
        if (rootWidget_) {
            rootWidget_->markLayoutDirty();
        }
        for (auto& entry : overlayStack_) {
            if (entry.widget) {
                entry.widget->markLayoutDirty();
            }
        }
    }

    if (!retainedPaintContext_) {
        retainedPaintContext_ = std::make_unique<renderer::PaintContext>();
    }

    bool needsRepaint = isPaintDirty_ || !animatedWidgets_.empty() ||
                        animationManager_.hasActiveAnimations() ||
                        retainedPaintContext_->commands().empty();
    if (!needsRepaint && rootWidget_ &&
        (rootWidget_->isLayoutDirty() || rootWidget_->isPaintDirty())) {
        needsRepaint = true;
    }
    if (!needsRepaint) {
        for (const auto& entry : overlayStack_) {
            if (entry.widget && (entry.widget->isLayoutDirty() || entry.widget->isPaintDirty())) {
                needsRepaint = true;
                break;
            }
        }
    }

    if (needsRepaint) {
        retainedPaintContext_->clear();
        core::Rect rootVisibleRect{{0.0f, 0.0f}, viewportSize};

        if (rootWidget_) {
            if (rootWidget_->isLayoutDirty()) {
                rootWidget_->layoutNode().computeLayout(viewportSize.width, viewportSize.height);
                rootWidget_->clearLayoutDirty();
            }
            renderWidget(rootWidget_.get(), *retainedPaintContext_, rootVisibleRect);
        }

        // Render active overlays above the root hierarchy
        for (auto& entry : overlayStack_) {
            if (entry.widget && entry.widget->isVisible()) {
                if (entry.desc.isModal) {
                    retainedPaintContext_->drawRect(
                        rootVisibleRect,
                        core::Color{0.0f, 0.0f, 0.0f, entry.scrimAnim.currentValue()});
                }
                if (entry.widget->isLayoutDirty()) {
                    entry.widget->layoutNode().computeLayout(viewportSize.width,
                                                             viewportSize.height);
                    core::Size overlaySize = entry.widget->layoutNode().layoutRect().size;
                    entry.computedPosition =
                        computeOverlayPosition(viewportSize, overlaySize, entry.desc);
                    entry.widget->layoutNode().setLayoutRect(
                        core::Rect{entry.computedPosition, overlaySize});
                    entry.widget->clearLayoutDirty();
                }
                renderWidget(entry.widget.get(), *retainedPaintContext_, rootVisibleRect);
            }
        }

        isPaintDirty_ = false;
    }

    ctx.append(*retainedPaintContext_);

    if (tooltipVisible_ && activeTooltip_) {
        activeTooltip_->paint(ctx);
        activeTooltip_->clearPaintDirty();
    }
    tooltipDirty_ = false;
}

void WidgetContext::renderWidget(Widget* widget, renderer::PaintContext& ctx,
                                 const core::Rect& visibleBounds) {
    if (!widget || !widget->isVisible()) {
        return;
    }

    core::Rect sRect = widget->screenRect();
    bool clip = widget->clipsChildren();

    // Frustum / viewport culling: if offscreen and clips children, skip entirely
    if (visibleBounds.size.width > 0.0f && visibleBounds.size.height > 0.0f) {
        if (!sRect.intersects(visibleBounds)) {
            if (clip) {
                return;
            }
        }
    }

    core::Rect childVisibleBounds = visibleBounds;
    if (clip) {
        ctx.pushClip(sRect);
        childVisibleBounds = visibleBounds.intersect(sRect);
    }

    if (sRect.intersects(visibleBounds) || visibleBounds.size.width <= 0.0f) {
        widget->paint(ctx);
    }
    widget->clearPaintDirty();

    for (const auto& child : widget->children()) {
        renderWidget(child.get(), ctx, childVisibleBounds);
    }

    if (clip) {
        ctx.popClip();
        if (sRect.intersects(visibleBounds) || visibleBounds.size.width <= 0.0f) {
            widget->paintOverlay(ctx);
        }
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
    entry.isOwned = true;
    if (desc.isModal) {
        entry.scrimAnim.start(0.0f, 0.55f, 0.14f, core::animation::EasingCurve::OutCubic);
    }

    Widget* ptr = entry.widget.get();
    overlayStack_.push_back(std::move(entry));
    isPaintDirty_ = true;
    return ptr;
}

Widget* WidgetContext::showOverlay(Widget* content, const OverlayDesc& desc, Widget* owner) {
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
    entry.widget = std::unique_ptr<Widget>(content);
    entry.desc = desc;
    entry.owner = owner;
    entry.computedPosition = pos;
    entry.isOwned = false;
    if (desc.isModal) {
        entry.scrimAnim.start(0.0f, 0.55f, 0.14f, core::animation::EasingCurve::OutCubic);
    }

    Widget* ptr = entry.widget.get();
    overlayStack_.push_back(std::move(entry));
    isPaintDirty_ = true;
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
    if (!entry.isOwned) {
        entry.widget.release();
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
            if (!it->isOwned) {
                it->widget.release();
            }
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
