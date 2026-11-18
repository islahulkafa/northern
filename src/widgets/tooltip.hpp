#pragma once

#include "text/font.hpp"
#include "widgets/widget.hpp"

#include <memory>
#include <string>

namespace northern::widgets {

class Tooltip : public Widget {
public:
    Tooltip() = default;
    explicit Tooltip(std::string text, std::string shortcut = {},
                     std::shared_ptr<text::Font> font = nullptr);

    void setContent(std::string text, std::string shortcut = {});
    [[nodiscard]] const std::string& text() const noexcept { return text_; }
    [[nodiscard]] const std::string& shortcut() const noexcept { return shortcut_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    void setAlpha(float alpha) noexcept;
    [[nodiscard]] float alpha() const noexcept { return alpha_; }

    [[nodiscard]] core::Size preferredSize() const;

    void paint(renderer::PaintContext& ctx) override;

private:
    std::string text_{};
    std::string shortcut_{};
    std::shared_ptr<text::Font> font_{nullptr};
    float alpha_{1.0f};
};

} // namespace northern::widgets
