#pragma once

#include "core/math/color.hpp"
#include "widgets/widget.hpp"

namespace northern::widgets {

enum class ScrollAxis { Vertical, Horizontal, Both };

class ScrollView : public Widget {
public:
    ScrollView();
    ~ScrollView() override = default;

    void setScrollAxis(ScrollAxis axis) noexcept { axis_ = axis; }
    [[nodiscard]] ScrollAxis scrollAxis() const noexcept { return axis_; }

    void scrollTo(const core::Point& offset);
    void scrollBy(const core::Point& delta);

    [[nodiscard]] core::Point scrollOffset() const noexcept override { return scrollOffset_; }
    [[nodiscard]] bool clipsChildren() const noexcept override { return true; }

    [[nodiscard]] core::Size contentSize() const noexcept;
    [[nodiscard]] core::Point maxScrollOffset() const noexcept;

    void setScrollbarThickness(float thickness) noexcept;
    void setScrollbarColor(const core::Color& color) noexcept;
    void setScrollbarHoverColor(const core::Color& color) noexcept;
    void setBackgroundColor(const core::Color& color) noexcept;

    void onMouseWheel(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;

    void paint(renderer::PaintContext& ctx) override;
    void paintOverlay(renderer::PaintContext& ctx) override;

protected:
    void attachChildInternal(std::unique_ptr<Widget> child) override;

private:
    [[nodiscard]] core::Rect verticalThumbRect() const noexcept;

    ScrollAxis axis_{ScrollAxis::Vertical};
    core::Point scrollOffset_{0.0f, 0.0f};

    float scrollbarThickness_{6.0f};
    float scrollbarPadding_{3.0f};
    core::Color bgColor_{core::Color::transparent()};
    core::Color scrollbarColor_{core::Color{1.0f, 1.0f, 1.0f, 0.20f}};
    core::Color scrollbarHoverColor_{core::Color{1.0f, 1.0f, 1.0f, 0.45f}};

    bool isDraggingThumb_{false};
    bool isHoveringThumb_{false};
    float dragStartMouseY_{0.0f};
    float dragStartScrollY_{0.0f};
};

} // namespace northern::widgets
