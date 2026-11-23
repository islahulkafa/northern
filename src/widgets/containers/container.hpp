#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "layout/style.hpp"
#include "widgets/base/widget.hpp"

namespace northern::widgets {

enum class SurfaceRole {
    None,    // Transparent or custom color
    Canvas,  // colorPalette().surfaceCanvas
    Default, // colorPalette().surfaceDefault
    Raised,  // colorPalette().surfaceRaised
    Overlay  // colorPalette().surfaceOverlay
};

enum class BorderRole {
    None,    // Transparent or custom width/color
    Subtle,  // 1px hairline colorPalette().borderSubtle
    Default  // 1px hairline colorPalette().borderDefault
};

class Container : public Widget {
public:
    Container() = default;
    ~Container() override = default;

    // Surface & Theming
    void setSurface(SurfaceRole role) noexcept;
    [[nodiscard]] SurfaceRole surface() const noexcept { return surfaceRole_; }

    void setBorderRole(BorderRole role) noexcept;
    [[nodiscard]] BorderRole borderRole() const noexcept { return borderRole_; }

    void setBackgroundColor(const core::Color& color) noexcept;
    [[nodiscard]] core::Color effectiveBackgroundColor() const noexcept;
    [[nodiscard]] const core::Color& backgroundColor() const noexcept { return backgroundColor_; }

    void setBorderColor(const core::Color& color) noexcept;
    [[nodiscard]] core::Color effectiveBorderColor() const noexcept;
    [[nodiscard]] const core::Color& borderColor() const noexcept { return borderColor_; }

    void setBorderWidth(float width) noexcept;
    [[nodiscard]] float effectiveBorderWidth() const noexcept;
    [[nodiscard]] float borderWidth() const noexcept { return borderWidth_; }

    void setCornerRadius(float radius) noexcept;
    void setCornerRadii(float r0, float r1, float r2, float r3) noexcept;
    [[nodiscard]] const float* cornerRadii() const noexcept { return cornerRadii_; }

    // Clipping Control
    void setClipsChildren(bool clips) noexcept;
    [[nodiscard]] bool clipsChildren() const noexcept override { return clipsChildren_; }

    // Fluent Flexbox Layout API
    Container* setFlexDirection(layout::FlexDirection dir) noexcept;
    Container* setJustifyContent(layout::JustifyContent justify) noexcept;
    Container* setAlignItems(layout::AlignItems align) noexcept;
    Container* setFlexWrap(layout::FlexWrap wrap) noexcept;
    Container* setPadding(float all) noexcept;
    Container* setPadding(float vertical, float horizontal) noexcept;
    Container* setPadding(const layout::RectLength& padding) noexcept;
    Container* setGap(float gap) noexcept;
    Container* setRowGap(float gap) noexcept;
    Container* setColumnGap(float gap) noexcept;

    // Interactive Card Mode
    void setInteractive(bool interactive) noexcept;
    [[nodiscard]] bool isInteractive() const noexcept { return isInteractive_; }

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;

    void paint(renderer::PaintContext& ctx) override;

    core::Signal<> onClicked;

private:
    void updateInteractiveTargetColors(bool animated);
    [[nodiscard]] core::Color targetInteractiveBg() const noexcept;
    [[nodiscard]] core::Color targetInteractiveBorder() const noexcept;

    SurfaceRole surfaceRole_{SurfaceRole::None};
    BorderRole borderRole_{BorderRole::None};

    core::Color backgroundColor_{core::Color::transparent()};
    core::Color borderColor_{core::Color::transparent()};
    float borderWidth_{0.0f};
    float cornerRadii_[4]{0.0f, 0.0f, 0.0f, 0.0f};

    bool hasCustomBg_{false};
    bool hasCustomBorder_{false};
    bool hasCustomBorderWidth_{false};
    bool clipsChildren_{false};

    // Interactive state
    bool isInteractive_{false};
    bool isHovered_{false};
    bool isPressed_{false};

    core::animation::PropertyAnimation<core::Color> bgAnim_{};
    core::animation::PropertyAnimation<core::Color> borderAnim_{};
};

} // namespace northern::widgets
