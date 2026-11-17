#pragma once

#include "core/math/color.hpp"
#include "widgets/widget.hpp"

namespace northern::widgets {

class Container : public Widget {
public:
    Container() = default;
    ~Container() override = default;

    void setBackgroundColor(const core::Color& color) noexcept;
    [[nodiscard]] const core::Color& backgroundColor() const noexcept { return backgroundColor_; }

    void setBorderColor(const core::Color& color) noexcept;
    [[nodiscard]] const core::Color& borderColor() const noexcept { return borderColor_; }

    void setBorderWidth(float width) noexcept;
    [[nodiscard]] float borderWidth() const noexcept { return borderWidth_; }

    void setCornerRadius(float radius) noexcept;
    void setCornerRadii(float r0, float r1, float r2, float r3) noexcept;
    [[nodiscard]] const float* cornerRadii() const noexcept { return cornerRadii_; }

    void paint(renderer::PaintContext& ctx) override;

private:
    core::Color backgroundColor_{core::Color::transparent()};
    core::Color borderColor_{core::Color::transparent()};
    float borderWidth_{0.0f};
    float cornerRadii_[4]{0.0f, 0.0f, 0.0f, 0.0f};
};

} // namespace northern::widgets
