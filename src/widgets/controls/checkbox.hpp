#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/base/widget.hpp"

#include <memory>
#include <string>

namespace northern::widgets {

class Checkbox : public Widget {
public:
    explicit Checkbox(std::string text = "", bool checked = false,
                      std::shared_ptr<text::Font> font = nullptr);
    ~Checkbox() override = default;

    core::Signal<bool> onToggled;

    void setChecked(bool checked);
    [[nodiscard]] bool isChecked() const noexcept { return isChecked_; }
    void toggle();

    void setText(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    void setBoxSize(float size) noexcept;
    void setBoxCornerRadius(float radius) noexcept;
    void setCheckedColor(const core::Color& color) noexcept;
    void setUncheckedColor(const core::Color& color) noexcept;
    void setBorderColor(const core::Color& color) noexcept;
    void setCheckmarkColor(const core::Color& color) noexcept;
    void setTextColor(const core::Color& color) noexcept;

    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    std::string text_{};
    std::shared_ptr<text::Font> font_{nullptr};
    bool isChecked_{false};
    bool isHovered_{false};
    bool isPressed_{false};

    core::animation::PropertyAnimation<float> checkAnim_{0.0f};

    float boxSize_{16.0f};
    float boxCornerRadius_{4.0f};
    float gap_{8.0f};

    core::Color checkedColor_{core::Color{0.145f, 0.388f, 0.922f, 1.0f}};
    core::Color uncheckedColor_{core::Color::transparent()};
    core::Color borderColor_{core::Color{1.0f, 1.0f, 1.0f, 0.30f}};
    core::Color borderHoverColor_{core::Color{1.0f, 1.0f, 1.0f, 0.50f}};
    core::Color checkmarkColor_{core::Color::white()};
    core::Color textColor_{core::Color{0.980f, 0.980f, 0.980f, 1.0f}};
};

} // namespace northern::widgets
