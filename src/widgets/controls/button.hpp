#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/widget.hpp"

#include <memory>
#include <optional>
#include <string>

namespace northern::renderer {
class SvgDocument;
}

namespace northern::widgets {

enum class IconId;
enum class ButtonState { Idle, Hovered, Pressed, Disabled };
enum class ButtonVariant { Secondary, Primary, Outline, Ghost, Destructive, Danger = Destructive };
enum class ButtonSize : uint8_t { Compact, Standard, Comfortable };
enum class IconPlacement : uint8_t { Leading, Trailing };

class Button : public Widget {
public:
    explicit Button(std::string text = "", std::shared_ptr<text::Font> font = nullptr,
                    ButtonVariant variant = ButtonVariant::Secondary,
                    ButtonSize size = ButtonSize::Standard);
    Button(std::string text, IconId icon, ButtonVariant variant = ButtonVariant::Secondary,
           ButtonSize size = ButtonSize::Standard);
    explicit Button(IconId icon, ButtonVariant variant = ButtonVariant::Secondary,
                    ButtonSize size = ButtonSize::Standard);
    ~Button() override = default;

    core::Signal<> onClicked;

    void setText(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    [[nodiscard]] ButtonVariant variant() const noexcept { return variant_; }
    void setVariant(ButtonVariant variant) noexcept;

    [[nodiscard]] ButtonSize size() const noexcept { return size_; }
    void setSize(ButtonSize size) noexcept;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    // Loading state & spinner
    void setLoading(bool loading) noexcept;
    [[nodiscard]] bool isLoading() const noexcept { return isLoading_; }

    // Vector Icon integration
    void setIcon(std::shared_ptr<renderer::SvgDocument> icon,
                 IconPlacement placement = IconPlacement::Leading, float iconSize = 0.0f) noexcept;
    void setIcon(IconId icon, IconPlacement placement = IconPlacement::Leading,
                 float iconSize = 0.0f) noexcept;
    [[nodiscard]] std::shared_ptr<renderer::SvgDocument> icon() const noexcept { return icon_; }
    [[nodiscard]] IconPlacement iconPlacement() const noexcept { return iconPlacement_; }
    [[nodiscard]] float iconSize() const noexcept { return iconSize_; }
    void setIconSize(float size) noexcept;
    [[nodiscard]] float iconGap() const noexcept { return iconGap_; }
    void setIconGap(float gap) noexcept;

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

    // Effective resolved colors (Theme-derived or custom overridden)
    [[nodiscard]] core::Color effectiveIdleBackgroundColor() const noexcept;
    [[nodiscard]] core::Color effectiveHoverBackgroundColor() const noexcept;
    [[nodiscard]] core::Color effectivePressedBackgroundColor() const noexcept;

    [[nodiscard]] core::Color effectiveIdleBorderColor() const noexcept;
    [[nodiscard]] core::Color effectiveHoverBorderColor() const noexcept;
    [[nodiscard]] core::Color effectivePressedBorderColor() const noexcept;

    [[nodiscard]] core::Color effectiveIdleTextColor() const noexcept;
    [[nodiscard]] core::Color effectiveHoverTextColor() const noexcept;
    [[nodiscard]] core::Color effectivePressedTextColor() const noexcept;

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
    void applySizeMetrics() noexcept;
    void applyVariantStyle() noexcept;
    void updateTargetColors(bool animated = true);

    std::string text_{};
    std::shared_ptr<text::Font> font_{nullptr};
    ButtonVariant variant_{ButtonVariant::Secondary};
    ButtonSize size_{ButtonSize::Standard};
    float dpiScale_{1.0f};
    bool isLoading_{false};

    std::shared_ptr<renderer::SvgDocument> icon_{nullptr};
    IconPlacement iconPlacement_{IconPlacement::Leading};
    float iconSize_{16.0f};
    float iconGap_{6.0f};

    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    mutable std::optional<text::TextLayout> cachedTextLayout_{};
    mutable bool isTextLayoutDirty_{true};

    core::animation::PropertyAnimation<core::Color> bgAnim_{};
    core::animation::PropertyAnimation<core::Color> borderAnim_{};
    core::animation::PropertyAnimation<float> spinnerAnim_{};

    bool customBg_{false};
    bool customBorder_{false};
    bool customText_{false};

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
