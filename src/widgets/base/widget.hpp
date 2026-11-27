#pragma once

#include "core/math/geometry.hpp"
#include "layout/layout_node.hpp"
#include "platform/mouse_types.hpp"
#include "widgets/base/overlay.hpp"
#include "widgets/base/widget_events.hpp"

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace northern::renderer {
class PaintContext;
}

namespace northern::theme {
class Theme;
struct ColorPalette;
} // namespace northern::theme

namespace northern::widgets {

class ContextMenu;
class WidgetContext;

enum class FocusReason : uint8_t { Pointer, Keyboard, Programmatic };

class Widget {
public:
    Widget();
    virtual ~Widget();

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
    Widget(Widget&&) noexcept;
    Widget& operator=(Widget&&) noexcept;

    // Hierarchy & Ownership
    [[nodiscard]] Widget* parent() const noexcept { return parent_; }
    [[nodiscard]] std::span<const std::unique_ptr<Widget>> children() const noexcept {
        return children_;
    }
    [[nodiscard]] size_t childCount() const noexcept { return children_.size(); }
    [[nodiscard]] Widget* childAt(size_t index) const noexcept;

    template <typename T>
    T* addChild(std::unique_ptr<T> child) {
        if (!child) {
            return nullptr;
        }
        T* rawPtr = child.get();
        attachChildInternal(std::move(child));
        return rawPtr;
    }

    std::unique_ptr<Widget> removeChild(Widget* child);
    std::unique_ptr<Widget> removeFromParent();
    void clearChildren();
    [[nodiscard]] bool isDescendantOf(const Widget* ancestor) const noexcept;

    // Deferred Deletion
    void deleteLater();
    [[nodiscard]] bool isPendingDelete() const noexcept { return isPendingDelete_; }

    // Layout Integration
    [[nodiscard]] layout::LayoutNode& layoutNode() noexcept { return layoutNode_; }
    [[nodiscard]] const layout::LayoutNode& layoutNode() const noexcept { return layoutNode_; }
    [[nodiscard]] layout::Style& style() noexcept { return layoutNode_.style(); }
    [[nodiscard]] const layout::Style& style() const noexcept { return layoutNode_.style(); }
    void setStyle(const layout::Style& style) noexcept;

    [[nodiscard]] core::Rect layoutRect() const noexcept { return layoutNode_.layoutRect(); }
    [[nodiscard]] core::Rect screenRect() const noexcept;
    [[nodiscard]] virtual core::Point scrollOffset() const noexcept {
        return core::Point{0.0f, 0.0f};
    }
    [[nodiscard]] virtual bool clipsChildren() const noexcept { return false; }

    // Invalidation & Dirty Flags
    void markLayoutDirty() noexcept;
    void markPaintDirty() noexcept;
    void markScreenRectDirty() noexcept;
    [[nodiscard]] bool isLayoutDirty() const noexcept;
    [[nodiscard]] bool isPaintDirty() const noexcept { return isPaintDirty_; }
    void clearPaintDirty() noexcept { isPaintDirty_ = false; }
    void clearLayoutDirty() noexcept;

    // Visibility & Interaction
    [[nodiscard]] bool isVisible() const noexcept { return isVisible_; }
    void setVisible(bool visible);

    [[nodiscard]] bool isEnabled() const noexcept { return isEnabled_; }
    void setEnabled(bool enabled);

    [[nodiscard]] bool isFocusable() const noexcept { return isFocusable_; }
    void setFocusable(bool focusable) noexcept { isFocusable_ = focusable; }
    [[nodiscard]] bool hasFocus() const noexcept;
    [[nodiscard]] bool hasKeyboardFocus() const noexcept;
    void setFocus(FocusReason reason = FocusReason::Programmatic);
    [[nodiscard]] bool isHovered() const noexcept;
    [[nodiscard]] bool isAttached() const noexcept { return context_ != nullptr; }
    [[nodiscard]] float contextDpiScale() const noexcept;

    // Cursor & Tooltip
    void setCursorShape(platform::CursorShape shape) noexcept;
    [[nodiscard]] platform::CursorShape cursorShape() const noexcept { return cursorShape_; }

    void setTooltip(std::string text, std::string shortcut = {}, std::string description = {},
                    OverlayPlacement placement = OverlayPlacement::Bottom);
    void setTooltipPlacement(OverlayPlacement placement) noexcept;
    [[nodiscard]] const std::string& tooltipText() const noexcept { return tooltipText_; }
    [[nodiscard]] const std::string& tooltipShortcut() const noexcept { return tooltipShortcut_; }
    [[nodiscard]] const std::string& tooltipDescription() const noexcept {
        return tooltipDescription_;
    }
    [[nodiscard]] OverlayPlacement tooltipPlacement() const noexcept { return tooltipPlacement_; }

    // Context Menu
    void setContextMenuBuilder(std::function<std::unique_ptr<ContextMenu>()> builder);

    // Context linkage & Theming
    [[nodiscard]] WidgetContext* context() const noexcept { return context_; }
    void setContext(WidgetContext* context) noexcept;
    [[nodiscard]] const theme::Theme& theme() const noexcept;
    [[nodiscard]] const theme::ColorPalette& colorPalette() const noexcept;

    // Animations & Frame Updates
    virtual void updateAnimations(float deltaSeconds);
    [[nodiscard]] virtual bool hasActiveAnimations() const noexcept { return false; }

    // Virtual Paint & Event Hooks
    virtual void paint(renderer::PaintContext& ctx);
    virtual void paintOverlay(renderer::PaintContext& ctx);
    virtual bool hitTest(const core::Point& localPoint) const;

    virtual void onMouseEnter();
    virtual void onMouseLeave();
    virtual void onMouseMove(WidgetMouseEvent& ev);
    virtual void onMouseDown(WidgetMouseEvent& ev);
    virtual void onMouseUp(WidgetMouseEvent& ev);
    virtual void onMouseWheel(WidgetMouseEvent& ev);
    virtual void onContextMenu(WidgetMouseEvent& ev);
    virtual void onFileDrop(WidgetFileDropEvent& ev);
    virtual void onKeyDown(WidgetKeyEvent& ev);
    virtual void onKeyUp(WidgetKeyEvent& ev);
    virtual void onFocusGained();
    virtual void onFocusLost();
    virtual void onContextAttached();
    virtual void onThemeChanged();
    virtual void onEnabledChanged();

protected:
    void paintFocusRing(renderer::PaintContext& ctx, const core::Rect& rect,
                        float cornerRadius) const;
    virtual void attachChildInternal(std::unique_ptr<Widget> child);

    layout::LayoutNode layoutNode_{};
    Widget* parent_{nullptr};
    WidgetContext* context_{nullptr};
    std::vector<std::unique_ptr<Widget>> children_{};

    bool isVisible_{true};
    bool isEnabled_{true};
    bool isFocusable_{false};
    bool isPaintDirty_{true};
    bool isLayoutDirty_{true};
    mutable bool isScreenRectDirty_{true};
    mutable core::Rect cachedScreenRect_{};
    bool isPendingDelete_{false};
    bool isFocusedStandalone_{false};
    FocusReason standaloneFocusReason_{FocusReason::Programmatic};
    platform::CursorShape cursorShape_{platform::CursorShape::Arrow};
    std::string tooltipText_{};
    std::string tooltipShortcut_{};
    std::string tooltipDescription_{};
    OverlayPlacement tooltipPlacement_{OverlayPlacement::Bottom};
    std::function<std::unique_ptr<ContextMenu>()> contextMenuBuilder_{nullptr};
};

} // namespace northern::widgets
