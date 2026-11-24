#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/base/widget.hpp"

#include <memory>
#include <optional>
#include <string>

namespace northern::widgets {

enum class CheckState : uint8_t { Unchecked, Checked, Indeterminate };

class Checkbox : public Widget {
public:
    explicit Checkbox(std::string text = "", bool checked = false,
                      std::shared_ptr<text::Font> font = nullptr);
    explicit Checkbox(std::string text, CheckState state,
                      std::shared_ptr<text::Font> font = nullptr);
    ~Checkbox() override = default;

    core::Signal<CheckState> onToggled;
    core::Signal<bool> onCheckedChanged;

    void setChecked(bool checked);
    [[nodiscard]] bool isChecked() const noexcept { return checkState_ == CheckState::Checked; }
    [[nodiscard]] bool isIndeterminate() const noexcept {
        return checkState_ == CheckState::Indeterminate;
    }

    void setCheckState(CheckState state);
    [[nodiscard]] CheckState checkState() const noexcept { return checkState_; }

    void setTristate(bool tristate) noexcept { tristate_ = tristate; }
    [[nodiscard]] bool isTristate() const noexcept { return tristate_; }

    void toggle();

    void setText(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    void setBoxSize(float size) noexcept;
    [[nodiscard]] float boxSize() const noexcept { return boxSize_; }

    void setBoxCornerRadius(float radius) noexcept;
    [[nodiscard]] float boxCornerRadius() const noexcept { return boxCornerRadius_; }

    void setCheckedColor(const core::Color& color) noexcept;
    void setUncheckedColor(const core::Color& color) noexcept;
    void setBorderColor(const core::Color& color) noexcept;
    void setBorderHoverColor(const core::Color& color) noexcept;
    void setCheckmarkColor(const core::Color& color) noexcept;
    void setTextColor(const core::Color& color) noexcept;

    [[nodiscard]] core::Color effectiveCheckedColor() const noexcept;
    [[nodiscard]] core::Color effectiveUncheckedColor() const noexcept;
    [[nodiscard]] core::Color effectiveBorderColor(bool hovered) const noexcept;
    [[nodiscard]] core::Color effectiveCheckmarkColor() const noexcept;
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
    void drawCheckmark(renderer::PaintContext& ctx, const core::Rect& inner, float progress,
                       const core::Color& color) const;
    void drawIndeterminate(renderer::PaintContext& ctx, const core::Rect& inner, float progress,
                           const core::Color& color) const;

    std::string text_{};
    std::shared_ptr<text::Font> font_{nullptr};
    CheckState checkState_{CheckState::Unchecked};
    CheckState displayedGlyphState_{CheckState::Checked};
    bool tristate_{false};
    bool isHovered_{false};
    bool isPressed_{false};

    core::animation::PropertyAnimation<float> checkAnim_{0.0f};
    core::animation::PropertyAnimation<float> hoverAnim_{0.0f};

    float boxSize_{16.0f};
    float boxCornerRadius_{4.0f};
    float gap_{8.0f};

    std::optional<core::Color> overrideCheckedColor_{};
    std::optional<core::Color> overrideUncheckedColor_{};
    std::optional<core::Color> overrideBorderColor_{};
    std::optional<core::Color> overrideBorderHoverColor_{};
    std::optional<core::Color> overrideCheckmarkColor_{};
    std::optional<core::Color> overrideTextColor_{};
};

} // namespace northern::widgets
