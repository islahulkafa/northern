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

enum class SwitchRole : uint8_t {
    Accent,
    Success,
    Warning,
    Danger,
};

enum class SwitchSize : uint8_t {
    Compact,
    Standard,
    Large,
};

enum class SwitchLabelPlacement : uint8_t {
    Trailing,
    Leading,
};

class Switch : public Widget {
public:
    explicit Switch(std::string text = "", bool checked = false,
                    std::shared_ptr<text::Font> font = nullptr);
    Switch(std::string text, SwitchSize size, SwitchRole role = SwitchRole::Accent,
           bool checked = false);
    ~Switch() override = default;

    core::Signal<bool> onToggled;

    void setChecked(bool checked);
    [[nodiscard]] bool isChecked() const noexcept { return isChecked_; }
    void toggle();

    void setText(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setRole(SwitchRole role) noexcept;
    [[nodiscard]] SwitchRole role() const noexcept { return role_; }

    void setSize(SwitchSize size) noexcept;
    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void setLabelPlacement(SwitchLabelPlacement placement) noexcept;
    [[nodiscard]] SwitchLabelPlacement labelPlacement() const noexcept { return labelPlacement_; }

    void setTrackSize(float width, float height) noexcept;
    [[nodiscard]] float trackWidth() const noexcept { return trackWidth_; }
    [[nodiscard]] float trackHeight() const noexcept { return trackHeight_; }

    void setCheckedTrackColor(const core::Color& color) noexcept;
    void setUncheckedTrackColor(const core::Color& color) noexcept;
    void setBorderColor(const core::Color& color) noexcept;
    void setCheckedThumbColor(const core::Color& color) noexcept;
    void setUncheckedThumbColor(const core::Color& color) noexcept;
    void setTextColor(const core::Color& color) noexcept;
    void clearColorOverrides() noexcept;

    [[nodiscard]] core::Color effectiveTrackColor() const noexcept;
    [[nodiscard]] core::Color effectiveBorderColor() const noexcept;
    [[nodiscard]] core::Color effectiveThumbColor() const noexcept;
    [[nodiscard]] core::Color effectiveTextColor() const noexcept;

    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    void ensureTextLayout() const;

    std::string text_{};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    float dpiScale_{1.0f};

    SwitchRole role_{SwitchRole::Accent};
    SwitchLabelPlacement labelPlacement_{SwitchLabelPlacement::Trailing};

    bool isChecked_{false};
    bool isHovered_{false};
    bool isPressed_{false};

    core::animation::PropertyAnimation<float> thumbAnim_{0.0f};
    core::animation::PropertyAnimation<float> hoverAnim_{0.0f};
    core::animation::PropertyAnimation<float> pressAnim_{0.0f};

    float trackWidth_{36.0f};
    float trackHeight_{20.0f};
    float thumbDiameter_{16.0f};
    float gap_{8.0f};

    std::optional<core::Color> overrideCheckedTrackColor_{std::nullopt};
    std::optional<core::Color> overrideUncheckedTrackColor_{std::nullopt};
    std::optional<core::Color> overrideBorderColor_{std::nullopt};
    std::optional<core::Color> overrideCheckedThumbColor_{std::nullopt};
    std::optional<core::Color> overrideUncheckedThumbColor_{std::nullopt};
    std::optional<core::Color> overrideTextColor_{std::nullopt};

    mutable std::optional<text::TextLayout> cachedTextLayout_{std::nullopt};
};

} // namespace northern::widgets
