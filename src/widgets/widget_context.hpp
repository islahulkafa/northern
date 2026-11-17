#pragma once

#include "core/math/geometry.hpp"
#include "platform/events.hpp"
#include "widgets/widget.hpp"

#include <memory>
#include <vector>

namespace northern::renderer {
class PaintContext;
}

namespace northern::widgets {

class WidgetContext {
public:
    WidgetContext() = default;
    ~WidgetContext();

    WidgetContext(const WidgetContext&) = delete;
    WidgetContext& operator=(const WidgetContext&) = delete;
    WidgetContext(WidgetContext&&) noexcept = default;
    WidgetContext& operator=(WidgetContext&&) noexcept = default;

    // Root Widget Management
    void setRootWidget(std::unique_ptr<Widget> root);
    [[nodiscard]] Widget* rootWidget() const noexcept { return rootWidget_.get(); }
    std::unique_ptr<Widget> takeRootWidget();

    // Hit Testing
    [[nodiscard]] Widget* hitTest(const core::Point& screenPos) const;

    // Event Handling
    void handleMouseMove(const platform::MouseEvent& ev);
    void handleMouseDown(const platform::MouseEvent& ev);
    void handleMouseUp(const platform::MouseEvent& ev);
    void handleMouseWheel(const platform::MouseEvent& ev);
    void handleKeyEvent(const platform::KeyEvent& ev);
    void handleResize(const core::Size& logicalSize);

    // Focus Management
    void setFocusedWidget(Widget* widget);
    [[nodiscard]] Widget* focusedWidget() const noexcept { return focusedWidget_; }
    void clearFocus();

    // Hover Tracking
    [[nodiscard]] Widget* hoveredWidget() const noexcept { return hoveredWidget_; }

    // Deferred Deletions
    void queueDeferredDelete(Widget* widget);
    void flushDeletedWidgets();
    void unregisterWidget(Widget* widget);

    // Rendering
    void render(renderer::PaintContext& ctx, const core::Size& viewportSize);

private:
    void renderWidget(Widget* widget, renderer::PaintContext& ctx);

    std::unique_ptr<Widget> rootWidget_{nullptr};
    Widget* hoveredWidget_{nullptr};
    Widget* focusedWidget_{nullptr};
    std::vector<Widget*> pendingDeletions_{};
    core::Size currentViewportSize_{0.0f, 0.0f};
};

} // namespace northern::widgets
