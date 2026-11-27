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

enum class RadioSize : uint8_t {
    Compact,
    Standard,
    Large,
};

[[nodiscard]] constexpr float toPoints(RadioSize size) noexcept {
    switch (size) {
    case RadioSize::Compact:
        return 14.0f;
    case RadioSize::Standard:
        return 16.0f;
    case RadioSize::Large:
        return 20.0f;
    }
    return 16.0f;
}

class RadioButton : public Widget {
public:
    explicit RadioButton(std::string text = "", bool checked = false,
                         std::shared_ptr<text::Font> font = nullptr);
    ~RadioButton() override = default;

    core::Signal<bool> onToggled;
    core::Signal<RadioButton*> onSelected;

    void setChecked(bool checked);
    [[nodiscard]] bool isChecked() const noexcept { return isChecked_; }

    void setText(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    void setValue(std::string value);
    [[nodiscard]] const std::string& value() const noexcept { return value_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setSize(RadioSize size) noexcept;
    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }
    [[nodiscard]] bool hasCustomDpiScale() const noexcept { return hasCustomDpiScale_; }

    void setCircleSize(float diameter) noexcept;
    [[nodiscard]] float circleSize() const noexcept { return circleDiameter_; }
    void setDotSize(float diameter) noexcept;

    void setCheckedOuterColor(const core::Color& color) noexcept;
    void setUncheckedOuterColor(const core::Color& color) noexcept;
    void setBorderColor(const core::Color& color) noexcept;
    void setDotColor(const core::Color& color) noexcept;
    void setTextColor(const core::Color& color) noexcept;
    void clearColorOverrides() noexcept;

    [[nodiscard]] core::Color effectiveOuterColor() const noexcept;
    [[nodiscard]] core::Color effectiveBorderColor() const noexcept;
    [[nodiscard]] core::Color effectiveDotColor() const noexcept;
    [[nodiscard]] core::Color effectiveTextColor() const noexcept;

    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;
    void onFocusGained() override;
    void onFocusLost() override;
    void onContextAttached() override;
    void onThemeChanged() override;
    void onEnabledChanged() override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    friend class RadioGroup;
    void applyDpiScale(float scale);
    void cancelActiveInteraction();
    void ensureTextLayout() const;

    std::string text_{};
    std::string value_{};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};

    bool isChecked_{false};
    bool isHovered_{false};
    bool isPressed_{false};

    core::animation::PropertyAnimation<float> checkAnim_{0.0f};
    core::animation::PropertyAnimation<float> hoverAnim_{0.0f};

    float circleDiameter_{16.0f};
    float dotDiameter_{8.0f};
    float gap_{8.0f};

    std::optional<core::Color> overrideCheckedOuterColor_{std::nullopt};
    std::optional<core::Color> overrideUncheckedOuterColor_{std::nullopt};
    std::optional<core::Color> overrideBorderColor_{std::nullopt};
    std::optional<core::Color> overrideDotColor_{std::nullopt};
    std::optional<core::Color> overrideTextColor_{std::nullopt};

    mutable std::optional<text::TextLayout> cachedTextLayout_{std::nullopt};
};

} // namespace northern::widgets
