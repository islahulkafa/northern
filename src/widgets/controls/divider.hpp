#pragma once

#include "core/math/color.hpp"
#include "text/font.hpp"
#include "widgets/base/widget.hpp"

#include <memory>
#include <string>

namespace northern::widgets {

enum class DividerOrientation : uint8_t { Horizontal, Vertical };

class Divider : public Widget {
public:
    explicit Divider(DividerOrientation orientation = DividerOrientation::Horizontal,
                     float thickness = 1.0f);
    explicit Divider(std::string label, std::shared_ptr<text::Font> font = nullptr,
                     float thickness = 1.0f);

    void setOrientation(DividerOrientation orientation) noexcept;
    [[nodiscard]] DividerOrientation orientation() const noexcept { return orientation_; }

    void setThickness(float thickness) noexcept;
    [[nodiscard]] float thickness() const noexcept { return thickness_; }

    void setColor(const core::Color& color) noexcept;
    [[nodiscard]] const core::Color& color() const noexcept { return color_; }

    void setLabel(std::string label);
    [[nodiscard]] const std::string& label() const noexcept { return label_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    void setTextColor(const core::Color& color) noexcept;
    [[nodiscard]] const core::Color& textColor() const noexcept { return textColor_; }

    void setGap(float gap) noexcept;
    [[nodiscard]] float gap() const noexcept { return gap_; }

    void paint(renderer::PaintContext& ctx) override;

private:
    void updateMeasureAndStyle();

    DividerOrientation orientation_{DividerOrientation::Horizontal};
    float thickness_{1.0f};
    core::Color color_{core::Color{1.0f, 1.0f, 1.0f, 0.08f}};
    std::string label_{};
    std::shared_ptr<text::Font> font_{nullptr};
    core::Color textColor_{core::Color{0.443f, 0.443f, 0.478f, 1.0f}};
    float gap_{8.0f};
};

} // namespace northern::widgets
