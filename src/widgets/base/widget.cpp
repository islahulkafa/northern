#include "widgets/base/widget.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/context_menu.hpp"
#include "widgets/controls/tooltip.hpp"

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
      isScreenRectDirty_(other.isScreenRectDirty_), cachedScreenRect_(other.cachedScreenRect_),
      isPendingDelete_(other.isPendingDelete_), isFocusedStandalone_(other.isFocusedStandalone_),
      standaloneFocusReason_(other.standaloneFocusReason_), cursorShape_(other.cursorShape_),
      tooltipText_(std::move(other.tooltipText_)),
      tooltipShortcut_(std::move(other.tooltipShortcut_)),
      tooltipDescription_(std::move(other.tooltipDescription_)),
      tooltipPlacement_(other.tooltipPlacement_),
      contextMenuBuilder_(std::move(other.contextMenuBuilder_)) {
    other.parent_ = nullptr;
    other.context_ = nullptr;
    other.isFocusedStandalone_ = false;
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
        isScreenRectDirty_ = other.isScreenRectDirty_;
        cachedScreenRect_ = other.cachedScreenRect_;
        isPendingDelete_ = other.isPendingDelete_;
        isFocusedStandalone_ = other.isFocusedStandalone_;
        standaloneFocusReason_ = other.standaloneFocusReason_;
        cursorShape_ = other.cursorShape_;
        tooltipText_ = std::move(other.tooltipText_);
        tooltipShortcut_ = std::move(other.tooltipShortcut_);
        tooltipDescription_ = std::move(other.tooltipDescription_);
        tooltipPlacement_ = other.tooltipPlacement_;
        contextMenuBuilder_ = std::move(other.contextMenuBuilder_);

        other.parent_ = nullptr;
        other.context_ = nullptr;
        other.isFocusedStandalone_ = false;
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
    child->markScreenRectDirty();
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
        removed->markScreenRectDirty();
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
            child->markScreenRectDirty();
        }
    }
    children_.clear();
    markLayoutDirty();
}

bool Widget::isDescendantOf(const Widget* ancestor) const noexcept {
    if (!ancestor) {
        return false;
    }
    const Widget* cur = parent_;
    while (cur) {
        if (cur == ancestor) {
            return true;
        }
        cur = cur->parent_;
    }
    return false;
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
    if (isScreenRectDirty_) {
        core::Rect local = layoutNode_.layoutRect();
        if (parent_) {
            core::Rect parentScreen = parent_->screenRect();
            core::Point pScroll = parent_->scrollOffset();
            cachedScreenRect_ = core::Rect{
                parentScreen.origin.x + local.origin.x - pScroll.x,
                parentScreen.origin.y + local.origin.y - pScroll.y,
                local.size.width,
                local.size.height,
            };
        } else {
            cachedScreenRect_ = local;
        }
        isScreenRectDirty_ = false;
    }
    return cachedScreenRect_;
}

void Widget::markLayoutDirty() noexcept {
    isLayoutDirty_ = true;
    isPaintDirty_ = true;
    markScreenRectDirty();
    layoutNode_.markDirty();
    if (context_) {
        context_->markNeedsPaint();
    }
    if (parent_) {
        parent_->markLayoutDirty();
    }
}

void Widget::markScreenRectDirty() noexcept {
    if (!isScreenRectDirty_) {
        isScreenRectDirty_ = true;
        for (const auto& child : children_) {
            if (child) {
                child->markScreenRectDirty();
            }
        }
    }
}

void Widget::markPaintDirty() noexcept {
    isPaintDirty_ = true;
    if (context_) {
        if (this == static_cast<const Widget*>(context_->activeTooltip())) {
            context_->markTooltipDirty();
        } else {
            context_->markNeedsPaint();
        }
    }
    if (parent_ && !parent_->isPaintDirty_) {
        parent_->markPaintDirty();
    }
}

bool Widget::isLayoutDirty() const noexcept {
    return isLayoutDirty_ || layoutNode_.isDirty();
}

void Widget::clearLayoutDirty() noexcept {
    isLayoutDirty_ = false;
    markScreenRectDirty();
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
        if (!visible) {
            if (context_) {
                context_->clearInteractionForSubtree(this);
            } else if (isFocusedStandalone_) {
                isFocusedStandalone_ = false;
                onFocusLost();
            }
        }
        markLayoutDirty();
    }
}

void Widget::setEnabled(bool enabled) {
    if (isEnabled_ != enabled) {
        isEnabled_ = enabled;
        if (!enabled) {
            if (context_) {
                context_->clearInteractionForSubtree(this);
            } else if (isFocusedStandalone_) {
                isFocusedStandalone_ = false;
                onFocusLost();
            }
        }
        onEnabledChanged();
        markPaintDirty();
    }
}

bool Widget::hasFocus() const noexcept {
    return context_ ? (context_->focusedWidget() == this) : isFocusedStandalone_;
}

bool Widget::hasKeyboardFocus() const noexcept {
    if (!hasFocus()) {
        return false;
    }
    return context_ ? (context_->focusReason() == FocusReason::Keyboard)
                    : (standaloneFocusReason_ == FocusReason::Keyboard);
}

void Widget::setFocus(FocusReason reason) {
    if (context_) {
        context_->setFocusedWidget(this, reason);
    } else {
        isFocusedStandalone_ = true;
        standaloneFocusReason_ = reason;
    }
}

bool Widget::isHovered() const noexcept {
    return context_ && context_->hoveredWidget() == this;
}

float Widget::contextDpiScale() const noexcept {
    return context_ ? context_->dpiScale() : 1.0f;
}

void Widget::setCursorShape(platform::CursorShape shape) noexcept {
    if (cursorShape_ != shape) {
        cursorShape_ = shape;
        if (context_) {
            context_->refreshCursorShape();
        }
    }
}

void Widget::setTooltip(std::string text, std::string shortcut, std::string description,
                        OverlayPlacement placement) {
    tooltipText_ = std::move(text);
    tooltipShortcut_ = std::move(shortcut);
    tooltipDescription_ = std::move(description);
    tooltipPlacement_ = placement;
}

void Widget::setTooltipPlacement(OverlayPlacement placement) noexcept {
    tooltipPlacement_ = placement;
}

void Widget::setContextMenuBuilder(std::function<std::unique_ptr<ContextMenu>()> builder) {
    contextMenuBuilder_ = std::move(builder);
}

void Widget::setContext(WidgetContext* context) noexcept {
    WidgetContext* oldCtx = context_;
    if (oldCtx == context) {
        return;
    }
    if (oldCtx) {
        oldCtx->unregisterWidget(this);
    }
    context_ = context;
    for (const auto& child : children_) {
        if (child) {
            child->setContext(context);
        }
    }
    if (context_) {
        onContextAttached();
    }
}

void Widget::paintFocusRing(renderer::PaintContext& ctx, const core::Rect& rect,
                            float cornerRadius) const {
    theme::paintFocusRing(ctx, rect, cornerRadius, colorPalette(), contextDpiScale());
}

void Widget::onContextAttached() {}
void Widget::onThemeChanged() {}
void Widget::onEnabledChanged() {}

const theme::Theme& Widget::theme() const noexcept {
    if (context_) {
        return context_->theme();
    }
    return theme::Theme::dark();
}

const theme::ColorPalette& Widget::colorPalette() const noexcept {
    return theme().colors();
}

void Widget::paint(renderer::PaintContext&) {}
void Widget::paintOverlay(renderer::PaintContext&) {}
void Widget::updateAnimations(float) {}

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
void Widget::onContextMenu(WidgetMouseEvent& ev) {
    if (contextMenuBuilder_ && context_) {
        auto menu = contextMenuBuilder_();
        if (menu) {
            core::Rect sRect = screenRect();
            core::Point screenPos{sRect.origin.x + ev.position().x,
                                  sRect.origin.y + ev.position().y};
            ContextMenu::showAt(*context_, std::move(menu), screenPos, this);
            ev.accept();
        }
    }
}
void Widget::onFileDrop(WidgetFileDropEvent&) {}
void Widget::onKeyDown(WidgetKeyEvent&) {}
void Widget::onKeyUp(WidgetKeyEvent&) {}
void Widget::onFocusGained() {}
void Widget::onFocusLost() {
    isFocusedStandalone_ = false;
    standaloneFocusReason_ = FocusReason::Programmatic;
}

} // namespace northern::widgets
