#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "widgets/base/widget.hpp"
#include "widgets/containers/container.hpp"

#include <optional>

namespace northern::widgets {

enum class ScrollAxis { Vertical, Horizontal, Both };

enum class ScrollbarVisibility : uint8_t { Automatic, AlwaysVisible, Never };

class ScrollView : public Widget {
public:
    ScrollView();
    ~ScrollView() override = default;

    core::Signal<const core::Point&> onScrolled;

    void setScrollAxis(ScrollAxis axis) noexcept { axis_ = axis; }
    [[nodiscard]] ScrollAxis scrollAxis() const noexcept { return axis_; }

    void setScrollbarVisibility(ScrollbarVisibility visibility) noexcept;
    [[nodiscard]] ScrollbarVisibility scrollbarVisibility() const noexcept {
        return scrollbarVisibility_;
    }

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void scrollTo(const core::Point& offset);
    void scrollBy(const core::Point& delta);

    [[nodiscard]] core::Point scrollOffset() const noexcept override { return scrollOffset_; }
    [[nodiscard]] bool clipsChildren() const noexcept override { return true; }

    [[nodiscard]] core::Size contentSize() const noexcept;
    [[nodiscard]] core::Point maxScrollOffset() const noexcept;

    void setScrollbarThickness(float thickness) noexcept;
    [[nodiscard]] float scrollbarThickness() const noexcept { return scrollbarThickness_; }

    void setScrollbarPadding(float padding) noexcept;
    [[nodiscard]] float scrollbarPadding() const noexcept { return scrollbarPadding_; }

    void setScrollbarHitThickness(float hitThickness) noexcept;
    [[nodiscard]] float scrollbarHitThickness() const noexcept { return scrollbarHitThickness_; }

    void setScrollbarColor(const core::Color& color) noexcept;
    void setScrollbarHoverColor(const core::Color& color) noexcept;

    void setSurface(SurfaceRole role) noexcept;
    [[nodiscard]] SurfaceRole surface() const noexcept { return surfaceRole_; }

    void setBackgroundColor(const core::Color& color) noexcept;
    [[nodiscard]] core::Color effectiveBackgroundColor() const noexcept;
    [[nodiscard]] const core::Color& backgroundColor() const noexcept { return bgColor_; }

    [[nodiscard]] bool hasVerticalScrollbar() const noexcept;
    [[nodiscard]] bool hasHorizontalScrollbar() const noexcept;

    [[nodiscard]] core::Rect verticalTrackRect() const noexcept;
    [[nodiscard]] core::Rect verticalThumbRect() const noexcept;
    [[nodiscard]] core::Rect verticalHitRect() const noexcept;

    [[nodiscard]] core::Rect horizontalTrackRect() const noexcept;
    [[nodiscard]] core::Rect horizontalThumbRect() const noexcept;
    [[nodiscard]] core::Rect horizontalHitRect() const noexcept;

    void onContextAttached() override;
    void onThemeChanged() override;
    void onEnabledChanged() override;
    void onMouseWheel(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onMouseLeave() override;
    void onKeyDown(WidgetKeyEvent& ev) override;

    void paint(renderer::PaintContext& ctx) override;
    void paintOverlay(renderer::PaintContext& ctx) override;

    [[nodiscard]] float scrollbarOpacity() const noexcept {
        return scrollbarOpacityAnim_.currentValue();
    }

protected:
    void attachChildInternal(std::unique_ptr<Widget> child) override;

private:
    enum class DragTarget { None, VerticalThumb, HorizontalThumb };

    void applyDpiMetrics(float scale);
    void clampScrollOffset();
    void revealScrollbars() noexcept;
    void paintAxisScrollbar(renderer::PaintContext& ctx, const core::Rect& sRect,
                            const core::Rect& tRect, const core::Rect& thRect, bool isVertical,
                            bool isHovered, bool isDragging, float hoverT, float opacity) const;

    ScrollAxis axis_{ScrollAxis::Vertical};
    ScrollbarVisibility scrollbarVisibility_{ScrollbarVisibility::Automatic};
    SurfaceRole surfaceRole_{SurfaceRole::None};
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};
    core::Point scrollOffset_{0.0f, 0.0f};

    float scrollbarThickness_{6.0f};
    float scrollbarPadding_{3.0f};
    float scrollbarHitThickness_{14.0f};
    bool hasCustomThickness_{false};
    bool hasCustomPadding_{false};
    bool hasCustomHitThickness_{false};

    core::Color bgColor_{core::Color::transparent()};
    bool hasCustomBg_{false};
    std::optional<core::Color> customScrollbarColor_{};
    std::optional<core::Color> customScrollbarHoverColor_{};

    DragTarget dragTarget_{DragTarget::None};
    bool isHoveringVertical_{false};
    bool isHoveringHorizontal_{false};
    float dragStartMousePos_{0.0f};
    float dragStartScrollOffset_{0.0f};

    float inactivityTimer_{0.0f};
    core::animation::PropertyAnimation<float> scrollbarOpacityAnim_{1.0f};
    core::animation::PropertyAnimation<float> verticalHoverAnim_{0.0f};
    core::animation::PropertyAnimation<float> horizontalHoverAnim_{0.0f};

    static constexpr float kInactivityFadeDelay = 0.80f;
    static constexpr float kFadeInDuration = 0.12f;
    static constexpr float kFadeOutDuration = 0.18f;
    static constexpr float kHoverExpandDuration = 0.12f;
};

} // namespace northern::widgets
