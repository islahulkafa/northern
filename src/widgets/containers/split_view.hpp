#pragma once

#include "core/animation/property_animation.hpp"
#include "core/signal/signal.hpp"
#include "widgets/base/widget.hpp"

#include <chrono>
#include <limits>
#include <memory>

namespace northern::widgets {

enum class SplitOrientation : uint8_t { Horizontal, Vertical };

enum class SplitSizeMode : uint8_t { Proportional, FixedFirst, FixedSecond };

enum class SplitCollapseState : uint8_t { None, FirstCollapsed, SecondCollapsed };

class SplitView : public Widget {
public:
    explicit SplitView(SplitOrientation orientation = SplitOrientation::Horizontal,
                       float splitRatio = 0.5f);
    ~SplitView() override = default;

    void setOrientation(SplitOrientation orientation);
    [[nodiscard]] SplitOrientation orientation() const noexcept { return orientation_; }

    void setSizeMode(SplitSizeMode mode);
    [[nodiscard]] SplitSizeMode sizeMode() const noexcept { return sizeMode_; }

    void setSplitRatio(float ratio);
    [[nodiscard]] float splitRatio() const noexcept { return splitRatio_; }

    void setFixedPaneSize(float size);
    [[nodiscard]] float fixedPaneSize() const noexcept;

    void setFirstPaneSize(float size);
    [[nodiscard]] float firstPaneSize() const noexcept { return firstPaneSize_; }

    void setSecondPaneSize(float size);
    [[nodiscard]] float secondPaneSize() const noexcept { return secondPaneSize_; }

    void setMinPaneSize(float minSize) noexcept;
    [[nodiscard]] float minPaneSize() const noexcept { return minFirstPaneSize_; }

    void setMinFirstPaneSize(float minSize) noexcept;
    [[nodiscard]] float minFirstPaneSize() const noexcept { return minFirstPaneSize_; }

    void setMaxFirstPaneSize(float maxSize) noexcept;
    [[nodiscard]] float maxFirstPaneSize() const noexcept { return maxFirstPaneSize_; }

    void setMinSecondPaneSize(float minSize) noexcept;
    [[nodiscard]] float minSecondPaneSize() const noexcept { return minSecondPaneSize_; }

    void setMaxSecondPaneSize(float maxSize) noexcept;
    [[nodiscard]] float maxSecondPaneSize() const noexcept { return maxSecondPaneSize_; }

    void setSnapThreshold(float threshold) noexcept;
    [[nodiscard]] float snapThreshold() const noexcept { return snapThreshold_; }

    void setCollapsible(bool firstCollapsible, bool secondCollapsible) noexcept;
    [[nodiscard]] bool isFirstCollapsible() const noexcept { return firstCollapsible_; }
    [[nodiscard]] bool isSecondCollapsible() const noexcept { return secondCollapsible_; }

    void collapseFirstPane(bool animated = true);
    void collapseSecondPane(bool animated = true);
    void expandPane(bool animated = true);
    void toggleCollapse(bool animated = true);

    [[nodiscard]] SplitCollapseState collapseState() const noexcept { return collapseState_; }
    [[nodiscard]] bool isFirstPaneCollapsed() const noexcept {
        return collapseState_ == SplitCollapseState::FirstCollapsed;
    }
    [[nodiscard]] bool isSecondPaneCollapsed() const noexcept {
        return collapseState_ == SplitCollapseState::SecondCollapsed;
    }

    void setSashThickness(float thickness) noexcept;
    [[nodiscard]] float sashThickness() const noexcept { return sashThickness_; }

    void setGrabMargin(float margin) noexcept;
    [[nodiscard]] float grabMargin() const noexcept { return grabMargin_; }

    void setFirstPane(std::unique_ptr<Widget> pane);
    [[nodiscard]] Widget* firstPane() const noexcept { return firstPane_; }

    void setSecondPane(std::unique_ptr<Widget> pane);
    [[nodiscard]] Widget* secondPane() const noexcept { return secondPane_; }

    void setPanes(std::unique_ptr<Widget> first, std::unique_ptr<Widget> second);

    [[nodiscard]] core::Rect sashLocalRect() const noexcept;
    [[nodiscard]] core::Rect sashHitRect() const noexcept;
    [[nodiscard]] core::Rect sashScreenRect() const noexcept;
    [[nodiscard]] core::Rect gripScreenRect() const noexcept;

    core::Signal<float> onSplitChanged;
    core::Signal<float, float> onPaneResized;
    core::Signal<bool, bool> onCollapseChanged;

    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onMouseLeave() override;
    void onKeyDown(WidgetKeyEvent& ev) override;
    void onFocusGained() override;
    void onFocusLost() override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    void updatePaneStyles();
    void applyCollapseState(SplitCollapseState newState, bool animated);
    void nudgeSplit(float deltaPixels);
    void emitSizeSignals();
    [[nodiscard]] float availableSplitSpan() const noexcept;
    [[nodiscard]] float clampFirstPaneSize(float size, float avail) const noexcept;
    [[nodiscard]] float clampSecondPaneSize(float size, float avail) const noexcept;

    SplitOrientation orientation_{SplitOrientation::Horizontal};
    SplitSizeMode sizeMode_{SplitSizeMode::Proportional};
    SplitCollapseState collapseState_{SplitCollapseState::None};

    float splitRatio_{0.5f};
    float firstPaneSize_{240.0f};
    float secondPaneSize_{240.0f};
    float savedRatio_{0.5f};
    float savedFirstPaneSize_{240.0f};
    float savedSecondPaneSize_{240.0f};

    float minFirstPaneSize_{40.0f};
    float maxFirstPaneSize_{std::numeric_limits<float>::infinity()};
    float minSecondPaneSize_{40.0f};
    float maxSecondPaneSize_{std::numeric_limits<float>::infinity()};

    float snapThreshold_{24.0f};
    float sashThickness_{6.0f};
    float grabMargin_{2.0f};

    bool firstCollapsible_{true};
    bool secondCollapsible_{true};
    bool lastCollapsedIsFirst_{true};

    Widget* firstPane_{nullptr};
    Widget* secondPane_{nullptr};

    bool isHoveredSash_{false};
    bool isDraggingSash_{false};

    std::chrono::steady_clock::time_point lastClickTime_{};
    core::Point lastClickPos_{};

    core::animation::PropertyAnimation<float> collapseAnim_{1.0f};
    core::animation::PropertyAnimation<float> hoverAnim_{0.0f};
};

} // namespace northern::widgets
