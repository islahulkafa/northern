#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace northern::widgets {

enum class SliderRole : uint8_t {
    Accent,
    Success,
    Warning,
    Danger,
    Info,
};

enum class SliderSize : uint8_t {
    Compact,
    Standard,
    Thick,
};

class Slider : public Widget {
public:
    explicit Slider(float min = 0.0f, float max = 1.0f, float value = 0.0f);
    Slider(SliderSize size, SliderRole role = SliderRole::Accent, float min = 0.0f,
           float max = 1.0f, float value = 0.0f);
    ~Slider() override = default;

    core::Signal<float> onValueChanged;
    core::Signal<float> onValueCommitted;

    void setValue(float value);
    [[nodiscard]] float value() const noexcept { return value_; }

    void setRange(float min, float max);
    [[nodiscard]] float minValue() const noexcept { return minValue_; }
    [[nodiscard]] float maxValue() const noexcept { return maxValue_; }

    void setStep(float step) noexcept;
    [[nodiscard]] float step() const noexcept { return step_; }

    void setRole(SliderRole role) noexcept;
    [[nodiscard]] SliderRole role() const noexcept { return role_; }

    void setSize(SliderSize size) noexcept;
    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void setTrackHeight(float height) noexcept;
    [[nodiscard]] float trackHeight() const noexcept { return trackHeight_; }
    void setThumbRadius(float radius) noexcept;
    [[nodiscard]] float thumbRadius() const noexcept { return thumbRadius_; }

    void setShowValue(bool show);
    [[nodiscard]] bool showValue() const noexcept { return showValue_; }

    void setValuePrecision(int decimalPlaces) noexcept;
    [[nodiscard]] int valuePrecision() const noexcept { return valuePrecision_; }

    void setShowTicks(bool show) noexcept;
    [[nodiscard]] bool showTicks() const noexcept { return showTicks_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    void setInactiveTrackColor(const core::Color& color) noexcept;
    void setActiveTrackColor(const core::Color& color) noexcept;
    void setThumbColor(const core::Color& color) noexcept;
    void setThumbBorderColor(const core::Color& color) noexcept;
    void clearColorOverrides() noexcept;

    [[nodiscard]] core::Color effectiveInactiveTrackColor() const noexcept;
    [[nodiscard]] core::Color effectiveActiveTrackColor() const noexcept;
    [[nodiscard]] core::Color effectiveThumbColor() const noexcept;
    [[nodiscard]] core::Color effectiveThumbBorderColor() const noexcept;

    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseWheel(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;
    [[nodiscard]] std::string formatValueString(float val) const;
    void updateReservedLabelWidth() const;
    void ensureValueLayout() const;
    [[nodiscard]] float computeTrackWidth(float totalWidth) const noexcept;
    void updateValueFromPosition(float localX);
    void updateHoverAnimation();

    float minValue_{0.0f};
    float maxValue_{1.0f};
    float value_{0.0f};
    float step_{0.0f};

    SliderRole role_{SliderRole::Accent};
    float trackHeight_{4.0f};
    float thumbRadius_{7.0f};
    float dpiScale_{1.0f};

    bool showValue_{false};
    int valuePrecision_{-1};
    bool showTicks_{false};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};

    bool isDragging_{false};
    bool isHovered_{false};

    core::animation::PropertyAnimation<float> thumbScaleAnim_{1.0f};

    std::optional<core::Color> overrideInactiveTrackColor_{std::nullopt};
    std::optional<core::Color> overrideActiveTrackColor_{std::nullopt};
    std::optional<core::Color> overrideThumbColor_{std::nullopt};
    std::optional<core::Color> overrideThumbBorderColor_{std::nullopt};

    mutable std::optional<text::TextLayout> cachedValueLayout_{std::nullopt};
    mutable std::string cachedValueText_{};
    mutable float reservedLabelWidth_{0.0f};
    mutable float reservedLabelHeight_{0.0f};
};

} // namespace northern::widgets
