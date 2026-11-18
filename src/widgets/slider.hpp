#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "widgets/widget.hpp"

namespace northern::widgets {

class Slider : public Widget {
public:
    explicit Slider(float min = 0.0f, float max = 1.0f, float value = 0.0f);
    ~Slider() override = default;

    core::Signal<float> onValueChanged;

    void setValue(float value);
    [[nodiscard]] float value() const noexcept { return value_; }

    void setRange(float min, float max);
    [[nodiscard]] float minValue() const noexcept { return minValue_; }
    [[nodiscard]] float maxValue() const noexcept { return maxValue_; }

    void setStep(float step) noexcept;
    [[nodiscard]] float step() const noexcept { return step_; }

    void setTrackHeight(float height) noexcept;
    void setThumbRadius(float radius) noexcept;

    void setInactiveTrackColor(const core::Color& color) noexcept;
    void setActiveTrackColor(const core::Color& color) noexcept;
    void setThumbColor(const core::Color& color) noexcept;
    void setThumbBorderColor(const core::Color& color) noexcept;

    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onKeyDown(WidgetKeyEvent& ev) override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    void updateValueFromPosition(float localX);
    void updateHoverAnimation();

    float minValue_{0.0f};
    float maxValue_{1.0f};
    float value_{0.0f};
    float step_{0.0f};

    float trackHeight_{4.0f};
    float thumbRadius_{7.0f};

    bool isDragging_{false};
    bool isHovered_{false};

    core::animation::PropertyAnimation<float> thumbScaleAnim_{1.0f};

    core::Color inactiveTrackColor_{core::Color{0.153f, 0.153f, 0.165f, 1.0f}};
    core::Color activeTrackColor_{core::Color{0.145f, 0.388f, 0.922f, 1.0f}};
    core::Color thumbColor_{core::Color{0.980f, 0.980f, 0.980f, 1.0f}};
    core::Color thumbBorderColor_{core::Color{0.0f, 0.0f, 0.0f, 0.20f}};
};

} // namespace northern::widgets
