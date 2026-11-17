#pragma once

#include "core/math/color.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/widget.hpp"

#include <memory>
#include <string>

namespace northern::widgets {

class Label : public Widget {
public:
    explicit Label(std::string text = "", std::shared_ptr<text::Font> font = nullptr);
    ~Label() override = default;

    void setText(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    void setTextColor(const core::Color& color) noexcept;
    [[nodiscard]] const core::Color& textColor() const noexcept { return textColor_; }

    void setTextAlignment(text::TextAlignment alignment) noexcept;
    [[nodiscard]] text::TextAlignment textAlignment() const noexcept { return alignment_; }

    void paint(renderer::PaintContext& ctx) override;

private:
    std::string text_{};
    std::shared_ptr<text::Font> font_{nullptr};
    core::Color textColor_{core::Color::white()};
    text::TextAlignment alignment_{text::TextAlignment::Left};
};

} // namespace northern::widgets
