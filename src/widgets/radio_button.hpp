#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/theme.hpp"
#include "widgets/widget.hpp"

#include <memory>
#include <string>

namespace northern::widgets {

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

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    void setCircleSize(float diameter) noexcept;
    void setDotSize(float diameter) noexcept;
    void setCheckedOuterColor(const core::Color& color) noexcept;
    void setUncheckedOuterColor(const core::Color& color) noexcept;
    void setBorderColor(const core::Color& color) noexcept;
    void setDotColor(const core::Color& color) noexcept;
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

    float circleDiameter_{16.0f};
    float dotDiameter_{8.0f};
    float gap_{8.0f};

    core::Color checkedOuterColor_{theme::ColorTokens::AccentDefault};
    core::Color uncheckedOuterColor_{theme::ColorTokens::SurfaceRaised};
    core::Color borderColor_{theme::ColorTokens::BorderDefault};
    core::Color borderHoverColor_{theme::ColorTokens::BorderStrong};
    core::Color dotColor_{theme::ColorTokens::TextPrimary};
    core::Color textColor_{theme::ColorTokens::TextPrimary};
};

} // namespace northern::widgets
