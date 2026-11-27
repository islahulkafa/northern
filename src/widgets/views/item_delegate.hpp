#pragma once

#include "core/math/color.hpp"
#include "core/math/geometry.hpp"
#include "renderer/paint_context.hpp"
#include "renderer/svg/svg_document.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/views/item_model.hpp"

#include <memory>
#include <string>

namespace northern::theme {
struct ColorPalette;
}

namespace northern::widgets {

struct StyleOptionViewItem {
    core::Rect bounds{};
    bool isSelected{false};
    bool isHovered{false};
    bool hasFocus{false};
    bool isAlternate{false};
    std::string text{};
    std::shared_ptr<text::Font> font{nullptr};
    core::Color textColor{core::Color{0.96f, 0.96f, 0.96f, 1.0f}};
    core::Color backgroundColor{core::Color::transparent()};
    text::TextAlignment alignment{text::TextAlignment::Left};
    float paddingLeft{8.0f};
    float paddingRight{8.0f};

    // Rich delegate capabilities
    std::shared_ptr<renderer::SvgDocument> icon{nullptr};
    float iconSize{16.0f};
    bool showCheckbox{false};
    bool checkboxChecked{false};
    std::string badgeText{};
    core::Color badgeColor{core::Color::transparent()};
    core::Color badgeTextColor{core::Color{0.96f, 0.96f, 0.96f, 1.0f}};
    float dpiScale{1.0f};
    const theme::ColorPalette* palette{nullptr};
    float rowInsetX{0.0f};
    float rowInsetY{0.0f};
    float cornerRadius{0.0f};
    bool drawSeparator{false};
};

class ItemDelegate {
public:
    virtual ~ItemDelegate() = default;

    virtual void paint(renderer::PaintContext& ctx, const StyleOptionViewItem& option,
                       const ModelIndex& index);

    virtual core::Size sizeHint(const StyleOptionViewItem& option, const ModelIndex& index);
};

} // namespace northern::widgets
