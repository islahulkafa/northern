#pragma once

#include "renderer/svg/svg_document.hpp"
#include "widgets/icons.hpp"
#include "widgets/theme.hpp"
#include "widgets/widget.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace northern::widgets {

class Icon : public Widget {
public:
    explicit Icon(IconId iconId, float size = 16.0f,
                  core::Color color = theme::ColorTokens::TextPrimary);
    explicit Icon(std::shared_ptr<renderer::SvgDocument> doc, float size = 16.0f,
                  core::Color color = theme::ColorTokens::TextPrimary);
    explicit Icon(std::string_view svgContent, float size = 16.0f,
                  core::Color color = theme::ColorTokens::TextPrimary);

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

    [[nodiscard]] core::Color color() const noexcept { return color_; }
    void setColor(core::Color color);

    [[nodiscard]] float iconSize() const noexcept { return iconSize_; }
    void setIconSize(float size);

    void paint(renderer::PaintContext& ctx) override;

private:
    std::optional<IconId> iconId_{};
    std::shared_ptr<renderer::SvgDocument> doc_{};
    float iconSize_{16.0f};
    core::Color color_{theme::ColorTokens::TextPrimary};
};

} // namespace northern::widgets
