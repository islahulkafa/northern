#include "widgets/views/item_delegate.hpp"

#include "widgets/base/theme.hpp"
#include "widgets/controls/badge.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

void ItemDelegate::paint(renderer::PaintContext& ctx, const StyleOptionViewItem& option,
                         const ModelIndex&) {
    if (option.bounds.size.width <= 0.0f || option.bounds.size.height <= 0.0f) {
        return;
    }

    float scale = option.dpiScale > 0.0f ? option.dpiScale : 1.0f;
    const auto& pal = option.palette ? *option.palette : theme::ColorPalette::dark();

    core::Rect bgRect = option.bounds;
    if (option.rowInsetX > 0.0f || option.rowInsetY > 0.0f) {
        bgRect.origin.x += option.rowInsetX;
        bgRect.origin.y += option.rowInsetY;
        bgRect.size.width = std::max(0.0f, bgRect.size.width - 2.0f * option.rowInsetX);
        bgRect.size.height = std::max(0.0f, bgRect.size.height - 2.0f * option.rowInsetY);
    }

    auto fillBg = [&](const core::Color& color) {
        if (option.cornerRadius > 0.0f) {
            float r[4] = {option.cornerRadius, option.cornerRadius, option.cornerRadius,
                          option.cornerRadius};
            ctx.drawRoundedRect(bgRect, color, r);
        } else {
            ctx.drawRect(bgRect, color);
        }
    };

    if (option.isSelected) {
        core::Color bg = option.isHovered ? pal.accentDefault.withAlpha(0.24f)
                                          : pal.accentDefault.withAlpha(0.17f);
        fillBg(bg);
    } else if (option.isHovered) {
        fillBg(pal.surfaceRaised);
    } else if (option.isAlternate) {
        fillBg(pal.surfaceBase.withAlpha(0.55f));
    } else if (option.backgroundColor.a > 0.0f) {
        fillBg(option.backgroundColor);
    }

    if (option.drawSeparator && !option.isSelected && !option.isHovered) {
        float divInset = option.rowInsetX > 0.0f ? option.rowInsetX + 8.0f * scale : 12.0f * scale;
        float divY = option.bounds.origin.y + option.bounds.size.height - 0.5f * scale;
        if (option.bounds.size.width > 2.0f * divInset) {
            ctx.drawLine(
                core::Point{option.bounds.origin.x + divInset, divY},
                core::Point{option.bounds.origin.x + option.bounds.size.width - divInset, divY},
                pal.borderSubtle, theme::Metrics::BorderWidthHairline * scale);
        }
    }

    if (option.hasFocus) {
        theme::paintFocusRing(ctx, bgRect, option.cornerRadius, -1.0f);
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
            ctx.drawRoundedRect(cbRect, pal.accentDefault, r);
            core::Color checkCol{1.0f, 1.0f, 1.0f, 1.0f};
            core::Point p0{cbRect.origin.x + 3.5f * scale, centerY};
            core::Point p1{cbRect.origin.x + 6.5f * scale, centerY + 3.0f * scale};
            core::Point p2{cbRect.origin.x + 12.5f * scale, centerY - 3.5f * scale};
            ctx.drawLine(p0, p1, checkCol, 1.5f * scale);
            ctx.drawLine(p1, p2, checkCol, 1.5f * scale);
        } else {
            ctx.drawBox(cbRect, pal.surfaceRaised, pal.borderDefault, 1.0f * scale, r);
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

    // Optional Status Badge/Chip
    if (!option.badgeText.empty() && option.font && contentRight > contentLeft) {
        float availW = contentRight - contentLeft;
        float bPad = 7.0f * scale;
        text::TextLayoutOptions bOpts{};
        bOpts.maxWidth = std::max(0.0f, availW - bPad * 2.0f);
        bOpts.wrapMode = text::WordWrap::None;
        bOpts.overflow = text::TextOverflow::Ellipsis;
        bOpts.maxLines = 1;
        auto bLayout = text::TextLayout::create(option.badgeText, option.font, bOpts);
        float bW = std::min(availW, bLayout.size().width + bPad * 2.0f);
        float bH = 20.0f * scale;
        float bX = option.text.empty() ? contentLeft : (contentRight - bW);
        float bY = std::round(centerY - bH * 0.5f);
        core::Rect bRect{core::Point{bX, bY}, core::Size{bW, bH}};

        std::optional<core::Color> customBg;
        if (option.badgeColor.a > 0.0f) {
            customBg = option.badgeColor;
        }
        std::optional<core::Color> customFg;
        if (option.badgeTextColor.a > 0.0f) {
            customFg = option.badgeTextColor;
        }
        Badge::paintInline(ctx, bRect, bLayout, *option.font, pal, scale, BadgeRole::Neutral,
                           BadgeVariant::Subtle, BadgeSize::Compact, customBg, customFg);

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
