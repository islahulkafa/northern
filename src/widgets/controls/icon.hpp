#pragma once

#include "core/animation/property_animation.hpp"
#include "renderer/svg/svg_document.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget.hpp"
#include "widgets/controls/icons.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace northern::widgets {

enum class IconSize : uint8_t { Small = 12, Standard = 16, Medium = 20, Large = 24 };

constexpr float toPoints(IconSize size) noexcept {
    return static_cast<float>(static_cast<uint8_t>(size));
}

enum class IconRole : uint8_t {
    Primary,
    Secondary,
    Tertiary,
    Disabled,
    Accent,
    Success,
    Warning,
    Danger,
    Info
};

class Icon : public Widget {
public:
    explicit Icon(IconId iconId, float size = 16.0f,
                  std::optional<core::Color> color = std::nullopt);
    explicit Icon(IconId iconId, IconSize size, IconRole role = IconRole::Primary);
    explicit Icon(std::shared_ptr<renderer::SvgDocument> doc, float size = 16.0f,
                  std::optional<core::Color> color = std::nullopt);
    explicit Icon(std::shared_ptr<renderer::SvgDocument> doc, IconSize size,
                  IconRole role = IconRole::Primary);
    explicit Icon(std::string_view svgContent, float size = 16.0f,
                  std::optional<core::Color> color = std::nullopt);
    explicit Icon(std::string_view svgContent, IconSize size, IconRole role = IconRole::Primary);

    ~Icon() override = default;

    Icon(Icon&&) noexcept = default;
    Icon& operator=(Icon&&) noexcept = default;

    [[nodiscard]] std::optional<IconId> iconId() const noexcept { return iconId_; }
    void setIcon(IconId iconId);

    [[nodiscard]] std::shared_ptr<renderer::SvgDocument> svgDocument() const noexcept {
        return doc_;
    }
    void setSvgDocument(std::shared_ptr<renderer::SvgDocument> doc);
    void setSvgContent(std::string_view svgContent);

    [[nodiscard]] IconRole role() const noexcept { return role_; }
    void setRole(IconRole role) noexcept;

    [[nodiscard]] core::Color color() const noexcept { return effectiveColor(); }
    void setColor(core::Color color);
    [[nodiscard]] core::Color effectiveColor() const noexcept;

    [[nodiscard]] float iconSize() const noexcept { return iconSize_; }
    void setIconSize(float size);
    void setIconSize(IconSize size) { setIconSize(toPoints(size)); }

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void setInteractive(bool interactive) noexcept;
    [[nodiscard]] bool isInteractive() const noexcept { return isInteractive_; }

    core::Signal<> onClicked;

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
    void applyDpiScale(float scale);
    void cancelActiveInteraction();

    std::optional<IconId> iconId_{};
    std::shared_ptr<renderer::SvgDocument> doc_{};
    IconRole role_{IconRole::Primary};
    std::optional<core::Color> overrideColor_{};
    float baseIconSize_{16.0f};
    float iconSize_{16.0f};
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};
    bool hasCustomSize_{false};

    bool isInteractive_{false};
    bool isHovered_{false};
    bool isPressed_{false};

    core::animation::PropertyAnimation<float> hoverAnim_{0.0f};
};

} // namespace northern::widgets
