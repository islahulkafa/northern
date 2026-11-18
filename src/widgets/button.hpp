#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/widget.hpp"

#include <memory>
#include <string>

namespace northern::widgets {

enum class ButtonState { Idle, Hovered, Pressed };
enum class ButtonVariant { Secondary, Primary, Ghost, Destructive };

class Button : public Widget {
public:
    explicit Button(std::string text = "", std::shared_ptr<text::Font> font = nullptr,
                    ButtonVariant variant = ButtonVariant::Secondary);
    ~Button() override = default;

    core::Signal<> onClicked;

    void setText(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    [[nodiscard]] ButtonVariant variant() const noexcept { return variant_; }
    void setVariant(ButtonVariant variant) noexcept;

    [[nodiscard]] ButtonState state() const noexcept;

    void setIdleBackgroundColor(const core::Color& color) noexcept;
    void setHoverBackgroundColor(const core::Color& color) noexcept;
    void setPressedBackgroundColor(const core::Color& color) noexcept;

    void setIdleBorderColor(const core::Color& color) noexcept;
    void setHoverBorderColor(const core::Color& color) noexcept;
    void setPressedBorderColor(const core::Color& color) noexcept;

    void setIdleTextColor(const core::Color& color) noexcept;
    void setHoverTextColor(const core::Color& color) noexcept;
    void setPressedTextColor(const core::Color& color) noexcept;

    void setBorderWidth(float width) noexcept;
    void setCornerRadius(float radius) noexcept;
    void setCornerRadii(float r0, float r1, float r2, float r3) noexcept;

    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    void applyVariantStyle() noexcept;
    void updateTargetColors(bool animated = true);

    std::string text_{};
    std::shared_ptr<text::Font> font_{nullptr};
    ButtonVariant variant_{ButtonVariant::Secondary};

    core::animation::PropertyAnimation<core::Color> bgAnim_{};
    core::animation::PropertyAnimation<core::Color> borderAnim_{};

    core::Color idleBgColor_{core::Color{0.153f, 0.153f, 0.165f, 1.0f}};
    core::Color hoverBgColor_{core::Color{0.188f, 0.188f, 0.204f, 1.0f}};
    core::Color pressedBgColor_{core::Color{0.094f, 0.094f, 0.106f, 1.0f}};

    core::Color idleBorderColor_{core::Color{1.0f, 1.0f, 1.0f, 0.12f}};
    core::Color hoverBorderColor_{core::Color{1.0f, 1.0f, 1.0f, 0.16f}};
    core::Color pressedBorderColor_{core::Color{1.0f, 1.0f, 1.0f, 0.12f}};

    core::Color idleTextColor_{core::Color{0.980f, 0.980f, 0.980f, 1.0f}};
    core::Color hoverTextColor_{core::Color{0.980f, 0.980f, 0.980f, 1.0f}};
    core::Color pressedTextColor_{core::Color{0.631f, 0.631f, 0.667f, 1.0f}};

    float borderWidth_{1.0f};
    float cornerRadii_[4]{6.0f, 6.0f, 6.0f, 6.0f};

    bool isHovered_{false};
    bool isPressed_{false};
};

} // namespace northern::widgets
