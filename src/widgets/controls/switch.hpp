#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget.hpp"

#include <memory>
#include <string>

namespace northern::widgets {

class Switch : public Widget {
public:
    explicit Switch(std::string text = "", bool checked = false,
                    std::shared_ptr<text::Font> font = nullptr);
    ~Switch() override = default;

    core::Signal<bool> onToggled;

    void setChecked(bool checked);
    [[nodiscard]] bool isChecked() const noexcept { return isChecked_; }
    void toggle();

    void setText(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    void setTrackSize(float width, float height) noexcept;
    void setCheckedTrackColor(const core::Color& color) noexcept;
    void setUncheckedTrackColor(const core::Color& color) noexcept;
    void setBorderColor(const core::Color& color) noexcept;
    void setCheckedThumbColor(const core::Color& color) noexcept;
    void setUncheckedThumbColor(const core::Color& color) noexcept;
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

    core::animation::PropertyAnimation<float> thumbAnim_{0.0f};

    float trackWidth_{36.0f};
    float trackHeight_{20.0f};
    float thumbDiameter_{16.0f};
    float gap_{8.0f};

    core::Color checkedTrackColor_{theme::ColorTokens::AccentDefault};
    core::Color uncheckedTrackColor_{theme::ColorTokens::SurfaceRaised};
    core::Color borderColor_{theme::ColorTokens::BorderDefault};
    core::Color borderHoverColor_{theme::ColorTokens::BorderStrong};
    core::Color checkedThumbColor_{theme::ColorTokens::TextPrimary};
    core::Color uncheckedThumbColor_{theme::ColorTokens::TextSecondary};
    core::Color textColor_{theme::ColorTokens::TextPrimary};
};

} // namespace northern::widgets
