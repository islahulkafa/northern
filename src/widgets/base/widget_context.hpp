#pragma once

#include "core/animation/animation_manager.hpp"
#include "core/animation/property_animation.hpp"
#include "core/math/geometry.hpp"
#include "core/signal/signal.hpp"
#include "platform/events.hpp"
#include "platform/mouse_types.hpp"
#include "widgets/base/overlay.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget.hpp"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace northern::renderer {
class PaintContext;
}

namespace northern::text {
class Font;
}

namespace northern::widgets {

class Tooltip;

struct KeyChord {
    platform::Key key{platform::Key::Unknown};
    platform::KeyModifier modifiers{platform::KeyModifier::None};

    auto operator<=>(const KeyChord&) const = default;
};

class WidgetContext {
public:
    WidgetContext();
    ~WidgetContext();

    WidgetContext(const WidgetContext&) = delete;
    WidgetContext& operator=(const WidgetContext&) = delete;
    WidgetContext(WidgetContext&&) noexcept;
    WidgetContext& operator=(WidgetContext&&) noexcept;

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
    void handleMouseLeave();
    void handleKeyEvent(const platform::KeyEvent& ev);
    void handleFileDrop(const platform::FileDropEvent& ev);
    void handleResize(const core::Size& logicalSize);

    // Global Accelerators
    static std::optional<KeyChord> parseShortcut(std::string_view str);
    void registerAccelerator(const std::string& shortcutStr, std::function<void()> callback);
    void registerAccelerator(const KeyChord& chord, std::function<void()> callback);
    void unregisterAccelerator(const std::string& shortcutStr);
    void unregisterAccelerator(const KeyChord& chord);
    bool dispatchAccelerator(platform::Key key, platform::KeyModifier modifiers);

    // Focus Management
    void setFocusedWidget(Widget* widget, FocusReason reason = FocusReason::Programmatic);
    [[nodiscard]] Widget* focusedWidget() const noexcept { return focusedWidget_; }
    [[nodiscard]] FocusReason focusReason() const noexcept { return focusReason_; }
    void clearFocus();
    void focusNext(bool reverse = false);

    // Theme Management
    void setTheme(theme::Theme theme);
    [[nodiscard]] const theme::Theme& theme() const noexcept { return theme_; }
    core::Signal<const theme::Theme&> onThemeChanged;

    // Cursor Management
    [[nodiscard]] platform::CursorShape currentCursorShape() const noexcept {
        return currentCursorShape_;
    }
    void refreshCursorShape();
    core::Signal<platform::CursorShape> onCursorShapeChanged;

    // Tooltip Management
    void setTooltipFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> tooltipFont() const noexcept { return tooltipFont_; }
    void setTooltipDpiScale(float scale);
    [[nodiscard]] float tooltipDpiScale() const noexcept { return tooltipDpiScale_; }
    void setTooltipDelay(float seconds) noexcept { tooltipDelaySeconds_ = seconds; }
    void setTooltipWarmupDelay(float seconds) noexcept { tooltipWarmupDelaySeconds_ = seconds; }
    void setTooltipWarmupWindow(float seconds) noexcept { tooltipWarmupWindowSeconds_ = seconds; }
    void markTooltipDirty() noexcept { tooltipDirty_ = true; }
    [[nodiscard]] bool isTooltipVisible() const noexcept { return tooltipVisible_; }
    [[nodiscard]] const Tooltip* activeTooltip() const noexcept { return activeTooltip_.get(); }
    void dismissTooltip();

    // Hover Tracking
    [[nodiscard]] Widget* hoveredWidget() const noexcept { return hoveredWidget_; }

    // Mouse Grab Tracking
    [[nodiscard]] Widget* mouseGrabWidget() const noexcept { return mouseGrabWidget_; }
    void setMouseGrabWidget(Widget* widget) noexcept { mouseGrabWidget_ = widget; }
    void releaseMouseGrab() noexcept { mouseGrabWidget_ = nullptr; }

    // Deferred Deletions
    void queueDeferredDelete(Widget* widget);
    void flushDeletedWidgets();
    void unregisterWidget(Widget* widget);

    // Animation Management
    void advanceAnimations(float deltaSeconds);
    void registerAnimatedWidget(Widget* widget);
    void unregisterAnimatedWidget(Widget* widget);
    [[nodiscard]] core::animation::AnimationManager& animationManager() noexcept {
        return animationManager_;
    }
    [[nodiscard]] const core::animation::AnimationManager& animationManager() const noexcept {
        return animationManager_;
    }

    // Rendering & Dirty Tracking
    void render(renderer::PaintContext& ctx, const core::Size& viewportSize);
    void markNeedsPaint() noexcept { isPaintDirty_ = true; }
    [[nodiscard]] bool isPaintDirty() const noexcept {
        return isPaintDirty_ || tooltipDirty_ || hasActiveAnimations();
    }
    [[nodiscard]] bool hasActiveAnimations() const noexcept {
        return !animatedWidgets_.empty() || animationManager_.hasActiveAnimations() ||
               (tooltipVisible_ && tooltipAlphaAnim_.isRunning());
    }

    // Overlay Management
    Widget* showOverlay(std::unique_ptr<Widget> content, const OverlayDesc& desc,
                        Widget* owner = nullptr);
    Widget* showOverlay(Widget* content, const OverlayDesc& desc, Widget* owner = nullptr);
    void dismissTopOverlay();
    void dismissAllOverlays();
    void dismissOverlaysForOwner(Widget* owner);
    [[nodiscard]] bool hasOverlays() const noexcept { return !overlayStack_.empty(); }
    [[nodiscard]] Widget* topOverlay() const noexcept {
        return overlayStack_.empty() ? nullptr : overlayStack_.back().widget.get();
    }
    [[nodiscard]] const std::vector<OverlayEntry>& overlayStack() const noexcept {
        return overlayStack_;
    }

private:
    void renderWidget(Widget* widget, renderer::PaintContext& ctx, const core::Rect& visibleBounds);
    [[nodiscard]] Widget* resolveTooltipSource(Widget* hovered) const noexcept;
    void showTooltipForHoveredWidget();
    void updateHoverTarget(Widget* newHover);

    std::unique_ptr<Widget> rootWidget_{nullptr};
    std::vector<OverlayEntry> overlayStack_{};
    Widget* hoveredWidget_{nullptr};
    Widget* focusedWidget_{nullptr};
    Widget* mouseGrabWidget_{nullptr};
    Widget* rightPressTarget_{nullptr};
    FocusReason focusReason_{FocusReason::Programmatic};
    platform::CursorShape currentCursorShape_{platform::CursorShape::Arrow};

    std::unique_ptr<Tooltip> activeTooltip_{nullptr};
    std::shared_ptr<text::Font> tooltipFont_{nullptr};
    core::animation::PropertyAnimation<float> tooltipAlphaAnim_{0.0f};
    float tooltipDpiScale_{1.0f};
    float hoverDurationSeconds_{0.0f};
    float tooltipDelaySeconds_{0.5f};
    float tooltipWarmupDelaySeconds_{0.08f};
    float tooltipWarmupWindowSeconds_{0.30f};
    float tooltipWarmupRemainingSeconds_{0.0f};
    bool tooltipVisible_{false};
    bool tooltipDirty_{false};

    std::vector<Widget*> pendingDeletions_{};
    std::vector<Widget*> animatedWidgets_{};
    core::animation::AnimationManager animationManager_{};
    core::Size currentViewportSize_{0.0f, 0.0f};
    bool isPaintDirty_{true};
    std::unique_ptr<renderer::PaintContext> retainedPaintContext_{nullptr};
    theme::Theme theme_{theme::Theme::dark()};
    std::map<KeyChord, std::function<void()>> accelerators_{};
};

} // namespace northern::widgets
