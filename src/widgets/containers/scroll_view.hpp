#pragma once

#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "widgets/base/widget.hpp"

#include <optional>

namespace northern::widgets {

enum class ScrollAxis { Vertical, Horizontal, Both };

class ScrollView : public Widget {
public:
    ScrollView();
    ~ScrollView() override = default;

    core::Signal<const core::Point&> onScrolled;

    void setScrollAxis(ScrollAxis axis) noexcept { axis_ = axis; }
    [[nodiscard]] ScrollAxis scrollAxis() const noexcept { return axis_; }

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

    void setBackgroundColor(const core::Color& color) noexcept;
    [[nodiscard]] const core::Color& backgroundColor() const noexcept { return bgColor_; }

    [[nodiscard]] bool hasVerticalScrollbar() const noexcept;
    [[nodiscard]] bool hasHorizontalScrollbar() const noexcept;

    [[nodiscard]] core::Rect verticalTrackRect() const noexcept;
    [[nodiscard]] core::Rect verticalThumbRect() const noexcept;
    [[nodiscard]] core::Rect verticalHitRect() const noexcept;

    [[nodiscard]] core::Rect horizontalTrackRect() const noexcept;
    [[nodiscard]] core::Rect horizontalThumbRect() const noexcept;
    [[nodiscard]] core::Rect horizontalHitRect() const noexcept;

    void onMouseWheel(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onMouseLeave() override;
    void onKeyDown(WidgetKeyEvent& ev) override;

    void paint(renderer::PaintContext& ctx) override;
    void paintOverlay(renderer::PaintContext& ctx) override;

protected:
    void attachChildInternal(std::unique_ptr<Widget> child) override;

private:
    enum class DragTarget { None, VerticalThumb, HorizontalThumb };

    ScrollAxis axis_{ScrollAxis::Vertical};
    core::Point scrollOffset_{0.0f, 0.0f};

    float scrollbarThickness_{6.0f};
    float scrollbarPadding_{3.0f};
    float scrollbarHitThickness_{14.0f};

    core::Color bgColor_{core::Color::transparent()};
    std::optional<core::Color> customScrollbarColor_{};
    std::optional<core::Color> customScrollbarHoverColor_{};

    DragTarget dragTarget_{DragTarget::None};
    bool isHoveringVertical_{false};
    bool isHoveringHorizontal_{false};
    float dragStartMousePos_{0.0f};
    float dragStartScrollOffset_{0.0f};
};

} // namespace northern::widgets
