#include "widgets/controls/tooltip.hpp"

#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "widgets/base/theme.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

namespace {
constexpr float kPrimaryDefaultPt = 10.5f;
constexpr float kSecondaryDefaultPt = 9.5f;
} // namespace

Tooltip::Tooltip(std::string text, std::string shortcut, std::shared_ptr<text::Font> font,
                 std::string description)
    : text_(std::move(text)), shortcut_(std::move(shortcut)), description_(std::move(description)),
      font_(std::move(font)) {
    setFocusable(false);
}

void Tooltip::invalidateLayoutCache() noexcept {
    layoutCacheValid_ = false;
    cachedTextLayout_.reset();
    cachedDescLayout_.reset();
    cachedShortcutLayout_.reset();
    cachedPreferredSize_ = {0.0f, 0.0f};
    cachedHeaderHeight_ = 0.0f;
    cachedBadgeWidth_ = 0.0f;
    cachedBadgeHeight_ = 0.0f;
}

void Tooltip::setContent(std::string text, std::string shortcut, std::string description) {
    if (text_ != text || shortcut_ != shortcut || description_ != description) {
        text_ = std::move(text);
        shortcut_ = std::move(shortcut);
        description_ = std::move(description);
        invalidateLayoutCache();
        markPaintDirty();
    }
}

void Tooltip::setDescription(std::string description) {
    if (description_ != description) {
        description_ = std::move(description);
        invalidateLayoutCache();
        markPaintDirty();
    }
}

void Tooltip::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedPrimaryFont_.reset();
        resolvedDescFont_.reset();
        resolvedShortcutFont_.reset();
        invalidateLayoutCache();
        markPaintDirty();
    }
}

std::shared_ptr<text::Font> Tooltip::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedPrimaryFont_) {
        float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
        resolvedPrimaryFont_ =
            text::FontManager::instance().defaultFont(kPrimaryDefaultPt * scale).valueOr(nullptr);
    }
    return resolvedPrimaryFont_;
}

std::shared_ptr<text::Font> Tooltip::effectiveDescriptionFont() const {
    if (!resolvedDescFont_) {
        float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
        float pt =
            font_ ? std::max(8.0f, font_->pointSize() - 1.0f) : (kSecondaryDefaultPt * scale);
        resolvedDescFont_ = text::FontManager::instance().defaultFont(pt).valueOr(nullptr);
    }
    return resolvedDescFont_;
}

std::shared_ptr<text::Font> Tooltip::effectiveShortcutFont() const {
    if (!resolvedShortcutFont_) {
        float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
        float pt =
            font_ ? std::max(8.0f, font_->pointSize() - 1.0f) : (kSecondaryDefaultPt * scale);
        resolvedShortcutFont_ = text::FontManager::instance().monospaceFont(pt).valueOr(nullptr);
    }
    return resolvedShortcutFont_;
}

void Tooltip::setDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        resolvedPrimaryFont_.reset();
        resolvedDescFont_.reset();
        resolvedShortcutFont_.reset();
        invalidateLayoutCache();
        markPaintDirty();
    }
}

void Tooltip::setMaxWidth(float maxWidth) {
    float clamped = std::max(80.0f, maxWidth);
    if (maxWidth_ != clamped) {
        maxWidth_ = clamped;
        invalidateLayoutCache();
        markPaintDirty();
    }
}

void Tooltip::setAlpha(float alpha) noexcept {
    float clamped = std::clamp(alpha, 0.0f, 1.0f);
    if (alpha_ != clamped) {
        alpha_ = clamped;
        markPaintDirty();
    }
}

void Tooltip::setBackgroundColor(const core::Color& color) noexcept {
    overrideBgColor_ = color;
    markPaintDirty();
}

void Tooltip::setBorderColor(const core::Color& color) noexcept {
    overrideBorderColor_ = color;
    markPaintDirty();
}

void Tooltip::setTextColor(const core::Color& color) noexcept {
    overrideTextColor_ = color;
    markPaintDirty();
}

void Tooltip::setDescriptionColor(const core::Color& color) noexcept {
    overrideDescColor_ = color;
    markPaintDirty();
}

void Tooltip::setShortcutColor(const core::Color& color) noexcept {
    overrideShortcutColor_ = color;
    markPaintDirty();
}

void Tooltip::setShortcutBackgroundColor(const core::Color& color) noexcept {
    overrideShortcutBgColor_ = color;
    markPaintDirty();
}

void Tooltip::setShortcutBorderColor(const core::Color& color) noexcept {
    overrideShortcutBorderColor_ = color;
    markPaintDirty();
}

void Tooltip::clearColorOverrides() noexcept {
    overrideBgColor_.reset();
    overrideBorderColor_.reset();
    overrideTextColor_.reset();
    overrideDescColor_.reset();
    overrideShortcutColor_.reset();
    overrideShortcutBgColor_.reset();
    overrideShortcutBorderColor_.reset();
    markPaintDirty();
}

core::Color Tooltip::effectiveBackgroundColor() const noexcept {
    return overrideBgColor_.value_or(colorPalette().surfaceOverlay);
}

core::Color Tooltip::effectiveBorderColor() const noexcept {
    return overrideBorderColor_.value_or(colorPalette().borderStrong);
}

core::Color Tooltip::effectiveTextColor() const noexcept {
    return overrideTextColor_.value_or(colorPalette().textPrimary);
}

core::Color Tooltip::effectiveDescriptionColor() const noexcept {
    return overrideDescColor_.value_or(colorPalette().textSecondary);
}

core::Color Tooltip::effectiveShortcutColor() const noexcept {
    return overrideShortcutColor_.value_or(colorPalette().textSecondary);
}

core::Color Tooltip::effectiveShortcutBackgroundColor() const noexcept {
    return overrideShortcutBgColor_.value_or(colorPalette().surfaceRaised);
}

core::Color Tooltip::effectiveShortcutBorderColor() const noexcept {
    return overrideShortcutBorderColor_.value_or(colorPalette().borderSubtle);
}

void Tooltip::ensureLayouts() const {
    if (layoutCacheValid_) {
        return;
    }

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float padX = 10.0f * scale;
    float shortcutGap = 10.0f * scale;
    float badgePadX = 5.0f * scale;
    float badgePadY = 2.0f * scale;
    float descGap = 4.0f * scale;
    float maxContentW = std::max(40.0f * scale, maxWidth_ * scale - padX * 2.0f);

    cachedTextLayout_.reset();
    cachedDescLayout_.reset();
    cachedShortcutLayout_.reset();
    cachedBadgeWidth_ = 0.0f;
    cachedBadgeHeight_ = 0.0f;

    auto primaryFont = effectiveFont();
    auto scFont = effectiveShortcutFont();
    auto descFont = effectiveDescriptionFont();

    if (!shortcut_.empty()) {
        if (scFont) {
            cachedShortcutLayout_ = text::TextLayout::create(shortcut_, scFont);
            float scW = cachedShortcutLayout_->size().width;
            float scH = std::max(scFont->lineHeight(), cachedShortcutLayout_->size().height);
            cachedBadgeWidth_ = scW + badgePadX * 2.0f;
            cachedBadgeHeight_ = scH + badgePadY * 2.0f;
        } else {
            cachedBadgeWidth_ =
                static_cast<float>(shortcut_.size()) * 6.0f * scale + badgePadX * 2.0f;
            cachedBadgeHeight_ = 16.0f * scale;
        }
    }

    float titleW = 0.0f;
    float titleH = 14.0f * scale;
    if (!text_.empty()) {
        float primaryMaxW =
            (!shortcut_.empty() && cachedBadgeWidth_ > 0.0f)
                ? std::max(40.0f * scale, maxContentW - cachedBadgeWidth_ - shortcutGap)
                : maxContentW;
        if (primaryFont) {
            text::TextLayoutOptions opts{};
            opts.maxWidth = primaryMaxW;
            opts.wrapMode = text::WordWrap::Word;
            opts.overflow = text::TextOverflow::Clip;
            cachedTextLayout_ = text::TextLayout::create(text_, primaryFont, opts);
            titleW = cachedTextLayout_->size().width;
            titleH = std::max(primaryFont->lineHeight(), cachedTextLayout_->size().height);
        } else {
            titleW = std::min(primaryMaxW, static_cast<float>(text_.size()) * 6.5f * scale);
        }
    }

    cachedHeaderHeight_ = std::max(titleH, cachedBadgeHeight_);

    float descW = 0.0f;
    float descH = 0.0f;
    if (!description_.empty()) {
        if (descFont) {
            text::TextLayoutOptions opts{};
            opts.maxWidth = maxContentW;
            opts.wrapMode = text::WordWrap::Word;
            opts.overflow = text::TextOverflow::Clip;
            cachedDescLayout_ = text::TextLayout::create(description_, descFont, opts);
            descW = cachedDescLayout_->size().width;
            descH = cachedDescLayout_->size().height;
        } else {
            descW = std::min(maxContentW, static_cast<float>(description_.size()) * 6.0f * scale);
            descH = 13.0f * scale;
        }
    }

    bool multiLineOrDesc =
        !description_.empty() || (cachedTextLayout_ && cachedTextLayout_->lines().size() > 1);
    float padY = multiLineOrDesc ? 8.0f * scale : 6.0f * scale;

    float headerW = titleW;
    if (!shortcut_.empty()) {
        headerW += (titleW > 0.0f ? shortcutGap : 0.0f) + cachedBadgeWidth_;
    }

    float totalW = padX * 2.0f + std::max(headerW, descW);
    float totalH = padY * 2.0f + cachedHeaderHeight_ + (descH > 0.0f ? descGap + descH : 0.0f);

    cachedPreferredSize_ =
        core::Size{std::max(40.0f * scale, totalW), std::max(24.0f * scale, totalH)};
    layoutCacheValid_ = true;
}

core::Size Tooltip::preferredSize() const {
    ensureLayouts();
    return cachedPreferredSize_;
}

void Tooltip::paint(renderer::PaintContext& ctx) {
    if (alpha_ <= 0.01f) {
        return;
    }

    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    ensureLayouts();

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float radius = theme::Metrics::CornerRadiusControl * scale;
    float radii[4] = {radius, radius, radius, radius};
    float borderW = std::max(1.0f, std::round(1.0f * scale));

    core::Color bg = effectiveBackgroundColor();
    bg = bg.withAlpha(bg.a * alpha_);
    core::Color border = effectiveBorderColor();
    border = border.withAlpha(border.a * alpha_);

    ctx.drawBox(rect, bg, border, borderW, radii);

    float padX = 10.0f * scale;
    bool multiLineOrDesc =
        !description_.empty() || (cachedTextLayout_ && cachedTextLayout_->lines().size() > 1);
    float padY = multiLineOrDesc ? 8.0f * scale : 6.0f * scale;

    auto primaryFont = effectiveFont();
    auto scFont = effectiveShortcutFont();

    if (!multiLineOrDesc) {
        float centerY = rect.origin.y + rect.size.height * 0.5f;
        if (cachedTextLayout_ && primaryFont) {
            float textY = theme::computeCenteredTextY(centerY, *primaryFont);
            core::Color textCol = effectiveTextColor();
            textCol = textCol.withAlpha(textCol.a * alpha_);
            ctx.drawTextLayout(core::Point{rect.origin.x + padX, textY}, *cachedTextLayout_,
                               textCol);
        }

        if (cachedShortcutLayout_ && scFont && cachedBadgeWidth_ > 0.0f) {
            float badgeX = rect.origin.x + rect.size.width - padX - cachedBadgeWidth_;
            float badgeY = centerY - cachedBadgeHeight_ * 0.5f;
            float badgeRadius = 4.0f * scale;
            float badgeRadii[4] = {badgeRadius, badgeRadius, badgeRadius, badgeRadius};

            core::Color scBg = effectiveShortcutBackgroundColor();
            scBg = scBg.withAlpha(scBg.a * alpha_);
            core::Color scBorder = effectiveShortcutBorderColor();
            scBorder = scBorder.withAlpha(scBorder.a * alpha_);
            ctx.drawBox(core::Rect{{badgeX, badgeY}, {cachedBadgeWidth_, cachedBadgeHeight_}}, scBg,
                        scBorder, borderW, badgeRadii);

            float badgePadX = 5.0f * scale;
            float scTextY = theme::computeCenteredTextY(centerY, *scFont);
            core::Color scCol = effectiveShortcutColor();
            scCol = scCol.withAlpha(scCol.a * alpha_);
            ctx.drawTextLayout(core::Point{badgeX + badgePadX, scTextY}, *cachedShortcutLayout_,
                               scCol);
        }
        return;
    }

    float rowTop = rect.origin.y + padY;
    if (cachedTextLayout_ && primaryFont) {
        core::Color textCol = effectiveTextColor();
        textCol = textCol.withAlpha(textCol.a * alpha_);
        ctx.drawTextLayout(core::Point{rect.origin.x + padX, rowTop}, *cachedTextLayout_, textCol);
    }

    if (cachedShortcutLayout_ && scFont && cachedBadgeWidth_ > 0.0f) {
        float badgeX = rect.origin.x + rect.size.width - padX - cachedBadgeWidth_;
        float badgeY = rowTop + (cachedHeaderHeight_ - cachedBadgeHeight_) * 0.5f;
        float badgeRadius = 4.0f * scale;
        float badgeRadii[4] = {badgeRadius, badgeRadius, badgeRadius, badgeRadius};

        core::Color scBg = effectiveShortcutBackgroundColor();
        scBg = scBg.withAlpha(scBg.a * alpha_);
        core::Color scBorder = effectiveShortcutBorderColor();
        scBorder = scBorder.withAlpha(scBorder.a * alpha_);
        ctx.drawBox(core::Rect{{badgeX, badgeY}, {cachedBadgeWidth_, cachedBadgeHeight_}}, scBg,
                    scBorder, borderW, badgeRadii);

        float badgePadX = 5.0f * scale;
        float badgeCenterY = badgeY + cachedBadgeHeight_ * 0.5f;
        float scTextY = theme::computeCenteredTextY(badgeCenterY, *scFont);
        core::Color scCol = effectiveShortcutColor();
        scCol = scCol.withAlpha(scCol.a * alpha_);
        ctx.drawTextLayout(core::Point{badgeX + badgePadX, scTextY}, *cachedShortcutLayout_, scCol);
    }

    if (cachedDescLayout_) {
        float descGap = 4.0f * scale;
        core::Color descCol = effectiveDescriptionColor();
        descCol = descCol.withAlpha(descCol.a * alpha_);
        ctx.drawTextLayout(
            core::Point{rect.origin.x + padX, rowTop + cachedHeaderHeight_ + descGap},
            *cachedDescLayout_, descCol);
    }
}

} // namespace northern::widgets
