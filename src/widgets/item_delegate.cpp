#include "widgets/item_delegate.hpp"

#include "widgets/theme.hpp"

#include <algorithm>

namespace northern::widgets {

void ItemDelegate::paint(renderer::PaintContext& ctx, const StyleOptionViewItem& option,
                         const ModelIndex&) {
    if (option.bounds.size.width <= 0.0f || option.bounds.size.height <= 0.0f) {
        return;
    }

    if (option.isSelected) {
        core::Color bg = option.isHovered ? theme::ColorTokens::AccentDefault.withAlpha(0.28f)
                                          : theme::ColorTokens::AccentDefault.withAlpha(0.18f);
        ctx.drawRect(option.bounds, bg);
    } else if (option.isHovered) {
        ctx.drawRect(option.bounds, theme::ColorTokens::SurfaceRaised);
    } else if (option.isAlternate) {
        ctx.drawRect(option.bounds, theme::ColorTokens::SurfaceDefault);
    } else if (option.backgroundColor.a > 0.0f) {
        ctx.drawRect(option.bounds, option.backgroundColor);
    }

    if (option.font && !option.text.empty()) {
        auto layout = text::TextLayout::create(option.text, option.font);
        float textY = theme::computeCenteredTextY(
            option.bounds.origin.y + option.bounds.size.height * 0.5f, *option.font);
        float textX = option.bounds.origin.x + option.paddingLeft;

        if (option.alignment == text::TextAlignment::Right) {
            textX = option.bounds.origin.x + option.bounds.size.width - option.paddingRight -
                    layout.size().width;
        } else if (option.alignment == text::TextAlignment::Center) {
            textX =
                option.bounds.origin.x + (option.bounds.size.width - layout.size().width) * 0.5f;
        }

        ctx.drawTextLayout(core::Point{textX, textY}, layout, option.textColor);
    }

    if (option.hasFocus) {
        theme::paintFocusRing(ctx, option.bounds, 0.0f);
    }
}

core::Size ItemDelegate::sizeHint(const StyleOptionViewItem& option, const ModelIndex&) {
    float w = 100.0f;
    float h = 28.0f;
    if (option.font) {
        if (!option.text.empty()) {
            auto l = text::TextLayout::create(option.text, option.font);
            w = l.size().width + option.paddingLeft + option.paddingRight;
        }
        h = std::max(h, option.font->lineHeight() + 8.0f);
    }
    return core::Size{w, h};
}

} // namespace northern::widgets
