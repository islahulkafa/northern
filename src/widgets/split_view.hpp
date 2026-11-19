#pragma once

#include "core/signal/signal.hpp"
#include "widgets/widget.hpp"

#include <memory>

namespace northern::widgets {

enum class SplitOrientation : uint8_t { Horizontal, Vertical };

class SplitView : public Widget {
public:
    explicit SplitView(SplitOrientation orientation = SplitOrientation::Horizontal,
                       float splitRatio = 0.5f);
    ~SplitView() override = default;

    void setOrientation(SplitOrientation orientation);
    [[nodiscard]] SplitOrientation orientation() const noexcept { return orientation_; }

    void setSplitRatio(float ratio);
    [[nodiscard]] float splitRatio() const noexcept { return splitRatio_; }

    void setMinPaneSize(float minSize) noexcept;
    [[nodiscard]] float minPaneSize() const noexcept { return minPaneSize_; }

    void setSashThickness(float thickness) noexcept;
    [[nodiscard]] float sashThickness() const noexcept { return sashThickness_; }

    void setFirstPane(std::unique_ptr<Widget> pane);
    [[nodiscard]] Widget* firstPane() const noexcept { return firstPane_; }

    void setSecondPane(std::unique_ptr<Widget> pane);
    [[nodiscard]] Widget* secondPane() const noexcept { return secondPane_; }

    void setPanes(std::unique_ptr<Widget> first, std::unique_ptr<Widget> second);

    [[nodiscard]] core::Rect sashLocalRect() const noexcept;
    [[nodiscard]] core::Rect sashScreenRect() const noexcept;

    core::Signal<float> onSplitChanged;

    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onMouseLeave() override;

    void paint(renderer::PaintContext& ctx) override;

private:
    void updatePaneStyles();

    SplitOrientation orientation_{SplitOrientation::Horizontal};
    float splitRatio_{0.5f};
    float minPaneSize_{40.0f};
    float sashThickness_{6.0f};

    Widget* firstPane_{nullptr};
    Widget* secondPane_{nullptr};

    bool isHoveredSash_{false};
    bool isDraggingSash_{false};
};

} // namespace northern::widgets
