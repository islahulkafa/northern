#include "widgets/views/item_delegate.hpp"

#include "widgets/base/theme.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

void ItemDelegate::paint(renderer::PaintContext& ctx, const StyleOptionViewItem& option,
                         const ModelIndex&) {
    if (option.bounds.size.width <= 0.0f || option.bounds.size.height <= 0.0f) {
        return;
    }

    float scale = option.dpiScale > 0.0f ? option.dpiScale : 1.0f;

    if (option.isSelected) {
        core::Color bg = option.isHovered ? theme::ColorTokens::AccentDefault.withAlpha(0.25f)
                                          : theme::ColorTokens::AccentDefault.withAlpha(0.16f);
        ctx.drawRect(option.bounds, bg);
    } else if (option.isHovered) {
        ctx.drawRect(option.bounds, theme::ColorTokens::SurfaceRaised);
    } else if (option.isAlternate) {
        ctx.drawRect(option.bounds, theme::ColorTokens::SurfaceBase);
    } else if (option.backgroundColor.a > 0.0f) {
        ctx.drawRect(option.bounds, option.backgroundColor);
    }

    if (option.hasFocus) {
        theme::paintFocusRing(ctx, option.bounds, 0.0f, -1.0f);
    }

    float centerY = option.bounds.origin.y + option.bounds.size.height * 0.5f;
    float contentLeft = option.bounds.origin.x + option.paddingLeft;
    float contentRight = option.bounds.origin.x + option.bounds.size.width - option.paddingRight;

    // Optional Checkbox
    if (option.showCheckbox) {
        float cbSize = 16.0f * scale;
        float cbY = std::round(centerY - cbSize * 0.5f);
        core::Rect cbRect{core::Point{contentLeft, cbY}, core::Size{cbSize, cbSize}};
        float r[4] = {3.0f * scale, 3.0f * scale, 3.0f * scale, 3.0f * scale};

        if (option.checkboxChecked) {
            ctx.drawRoundedRect(cbRect, theme::ColorTokens::AccentDefault, r);
            core::Point p0{cbRect.origin.x + 3.5f * scale, centerY};
            core::Point p1{cbRect.origin.x + 6.5f * scale, centerY + 3.0f * scale};
            core::Point p2{cbRect.origin.x + 12.5f * scale, centerY - 3.5f * scale};
            ctx.drawLine(p0, p1, theme::ColorTokens::TextPrimary, 1.5f * scale);
            ctx.drawLine(p1, p2, theme::ColorTokens::TextPrimary, 1.5f * scale);
        } else {
            ctx.drawBox(cbRect, theme::ColorTokens::SurfaceRaised,
                        theme::ColorTokens::BorderDefault, 1.0f * scale, r);
        }
        contentLeft += cbSize + 8.0f * scale;
    }

    // Optional Leading SVG Icon
    if (option.icon && option.icon->isValid()) {
        float iSize = (option.iconSize > 0.0f ? option.iconSize : 16.0f) * scale;
        core::Rect iconRect{core::Point{contentLeft, std::round(centerY - iSize * 0.5f)},
                            core::Size{iSize, iSize}};
        ctx.drawSvg(*option.icon, iconRect, option.textColor);
        contentLeft += iSize + 8.0f * scale;
    }

    // Optional Status Badge/Chip on Right
    if (!option.badgeText.empty() && option.font) {
        auto bLayout = text::TextLayout::create(option.badgeText, option.font);
        float bPad = 6.0f * scale;
        float bW = bLayout.size().width + bPad * 2.0f;
        float bH = 18.0f * scale;
        float bX = contentRight - bW;
        float bY = std::round(centerY - bH * 0.5f);
        core::Rect bRect{core::Point{bX, bY}, core::Size{bW, bH}};
        float radius = 3.0f * scale;
        float br[4] = {radius, radius, radius, radius};

        core::Color bgCol =
            option.badgeColor.a > 0.0f ? option.badgeColor : theme::ColorTokens::SurfaceOverlay;
        ctx.drawRoundedRect(bRect, bgCol, br);
        ctx.drawBorder(bRect, theme::ColorTokens::BorderDefault, 1.0f);
        float bTextY = theme::computeCenteredTextY(centerY, *option.font);
        core::Color textCol = option.badgeTextColor.a > 0.0f ? option.badgeTextColor
                                                             : theme::ColorTokens::TextPrimary;
        ctx.drawTextLayout(core::Point{bX + bPad, bTextY}, bLayout, textCol);

        contentRight = bX - 8.0f * scale;
    }

    // Primary Text
    if (option.font && !option.text.empty() && contentRight > contentLeft) {
        auto layout = text::TextLayout::create(option.text, option.font);
        float textY = theme::computeCenteredTextY(centerY, *option.font);
        float textX = contentLeft;

        if (option.alignment == text::TextAlignment::Right) {
            textX = contentRight - layout.size().width;
        } else if (option.alignment == text::TextAlignment::Center) {
            textX = contentLeft + (contentRight - contentLeft - layout.size().width) * 0.5f;
        }

        core::Rect clipText{
            core::Point{contentLeft, option.bounds.origin.y},
            core::Size{std::max(0.0f, contentRight - contentLeft), option.bounds.size.height}};
        ctx.pushClip(clipText);
        ctx.drawTextLayout(core::Point{textX, textY}, layout, option.textColor);
        ctx.popClip();
    }
}

core::Size ItemDelegate::sizeHint(const StyleOptionViewItem& option, const ModelIndex&) {
    float scale = option.dpiScale > 0.0f ? option.dpiScale : 1.0f;
    float w = 100.0f * scale;
    float h = 28.0f * scale;
    float extraW = option.paddingLeft + option.paddingRight;

    if (option.showCheckbox) {
        extraW += 24.0f * scale;
    }
    if (option.icon && option.icon->isValid()) {
        float iSize = (option.iconSize > 0.0f ? option.iconSize : 16.0f) * scale;
        extraW += iSize + 8.0f * scale;
    }
    if (!option.badgeText.empty() && option.font) {
        auto bLayout = text::TextLayout::create(option.badgeText, option.font);
        extraW += bLayout.size().width + 20.0f * scale;
    }

    if (option.font) {
        if (!option.text.empty()) {
            auto l = text::TextLayout::create(option.text, option.font);
            w = l.size().width + extraW;
        } else {
            w = extraW;
        }
        h = std::max(h, option.font->lineHeight() + 8.0f * scale);
    }
    return core::Size{w, h};
}

} // namespace northern::widgets
