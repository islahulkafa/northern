#include "widgets/controls/label.hpp"
#include "renderer/paint_context.hpp"

#include <algorithm>

namespace northern::widgets {

Label::Label(std::string text, std::shared_ptr<text::Font> font)
    : text_(std::move(text)), font_(std::move(font)) {
    layoutNode_.setMeasureFunc(
        [this](const layout::LayoutNode&, float availW, float) -> core::Size {
            if (text_.empty()) {
                return core::Size{0.0f, 0.0f};
            }
            auto f = effectiveFont();
            if (!f) {
                return core::Size{0.0f, 0.0f};
            }
            float maxW = availW > 0.0f ? availW : 0.0f;
            ensureLayout(maxW);
            return cachedLayout_ ? cachedLayout_->size() : core::Size{0.0f, 0.0f};
        });
}

Label::Label(std::string text, theme::TypographyStyle textStyle, theme::TextRole role)
    : Label(std::move(text), nullptr) {
    textStyle_ = textStyle;
    role_ = role;
}

void Label::invalidateLayoutCache() noexcept {
    cachedLayout_.reset();
    cachedWidth_ = -1.0f;
    unwrappedSizeValid_ = false;
}

std::shared_ptr<text::Font> Label::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        resolvedDefaultFont_ = theme::resolveFont(textStyle_, dpiScale_);
    }
    return resolvedDefaultFont_;
}

void Label::ensureUnwrappedSize(const std::shared_ptr<text::Font>& f) const {
    if (unwrappedSizeValid_ || !f || text_.empty()) {
        return;
    }
    text::TextLayoutOptions opts{};
    opts.maxWidth = 0.0f;
    opts.alignment = text::TextAlignment::Left;
    opts.wrapMode = text::WordWrap::None;
    opts.overflow = text::TextOverflow::Clip;
    opts.maxLines = maxLines_;
    opts.lineHeightMultiplier = lineHeightMultiplier_;
    auto unwrapped = text::TextLayout::create(text_, f, opts);
    unwrappedSize_ = unwrapped.size();
    unwrappedSizeValid_ = true;
}

void Label::ensureLayout(float maxWidth) const {
    auto f = effectiveFont();
    if (!f || text_.empty()) {
        cachedLayout_.reset();
        cachedWidth_ = maxWidth;
        return;
    }

    if (cachedLayout_ && cachedWidth_ == maxWidth) {
        return;
    }

    if (cachedLayout_ && alignment_ == text::TextAlignment::Left && unwrappedSizeValid_) {
        bool fitsPrev = (cachedWidth_ <= 0.0f || unwrappedSize_.width <= cachedWidth_);
        bool fitsNext = (maxWidth <= 0.0f || unwrappedSize_.width <= maxWidth);
        if (fitsPrev && fitsNext) {
            cachedWidth_ = maxWidth;
            return;
        }
    }

    ensureUnwrappedSize(f);

    text::TextLayoutOptions opts{};
    opts.maxWidth = maxWidth;
    opts.alignment = alignment_;
    opts.wrapMode = wrapMode_;
    opts.overflow = overflow_;
    opts.maxLines = maxLines_;
    opts.lineHeightMultiplier = lineHeightMultiplier_;

    cachedLayout_ = text::TextLayout::create(text_, f, opts);
    cachedWidth_ = maxWidth;
}

void Label::setText(std::string text) {
    if (text_ != text) {
        text_ = std::move(text);
        invalidateLayoutCache();
        markLayoutDirty();
    }
}

void Label::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        invalidateLayoutCache();
        markLayoutDirty();
    }
}

void Label::setTextStyle(theme::TypographyStyle style) {
    if (textStyle_ != style) {
        textStyle_ = style;
        resolvedDefaultFont_.reset();
        if (!font_) {
            invalidateLayoutCache();
            markLayoutDirty();
        }
    }
}

void Label::setDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        resolvedDefaultFont_.reset();
        if (!font_) {
            invalidateLayoutCache();
            markLayoutDirty();
        }
    }
}

void Label::setRole(theme::TextRole role) noexcept {
    if (role_ != role) {
        role_ = role;
        markPaintDirty();
    }
}

void Label::setTextColor(const core::Color& color) noexcept {
    overrideColor_ = color;
    markPaintDirty();
}

void Label::clearColorOverride() noexcept {
    if (overrideColor_) {
        overrideColor_.reset();
        markPaintDirty();
    }
}

core::Color Label::effectiveColor() const noexcept {
    if (overrideColor_) {
        return isEnabled() ? *overrideColor_ : overrideColor_->withAlpha(overrideColor_->a * 0.40f);
    }
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.textDisabled;
    }
    switch (role_) {
    case theme::TextRole::Primary:
        return pal.textPrimary;
    case theme::TextRole::Secondary:
        return pal.textSecondary;
    case theme::TextRole::Tertiary:
        return pal.textTertiary;
    case theme::TextRole::Disabled:
        return pal.textDisabled;
    case theme::TextRole::Accent:
        return pal.accentDefault;
    case theme::TextRole::Success:
        return pal.success;
    case theme::TextRole::Warning:
        return pal.warning;
    case theme::TextRole::Danger:
        return pal.danger;
    case theme::TextRole::Info:
        return pal.info;
    }
    return pal.textPrimary;
}

void Label::setTextAlignment(text::TextAlignment alignment) noexcept {
    if (alignment_ != alignment) {
        alignment_ = alignment;
        cachedLayout_.reset();
        cachedWidth_ = -1.0f;
        markPaintDirty();
    }
}

void Label::setVerticalAlignment(VerticalAlignment alignment) noexcept {
    if (verticalAlignment_ != alignment) {
        verticalAlignment_ = alignment;
        markPaintDirty();
    }
}

void Label::setWordWrap(text::WordWrap wrapMode) noexcept {
    if (wrapMode_ != wrapMode) {
        wrapMode_ = wrapMode;
        cachedLayout_.reset();
        cachedWidth_ = -1.0f;
        markLayoutDirty();
    }
}

void Label::setOverflow(text::TextOverflow overflow) noexcept {
    if (overflow_ != overflow) {
        overflow_ = overflow;
        cachedLayout_.reset();
        cachedWidth_ = -1.0f;
        markPaintDirty();
    }
}

void Label::setMaxLines(uint32_t maxLines) noexcept {
    if (maxLines_ != maxLines) {
        maxLines_ = maxLines;
        invalidateLayoutCache();
        markLayoutDirty();
    }
}

void Label::setLineHeightMultiplier(float multiplier) noexcept {
    float clamped = multiplier > 0.0f ? multiplier : 1.0f;
    if (lineHeightMultiplier_ != clamped) {
        lineHeightMultiplier_ = clamped;
        invalidateLayoutCache();
        markLayoutDirty();
    }
}

bool Label::isTruncated() const noexcept {
    return cachedLayout_ && cachedLayout_->isTruncated();
}

void Label::paint(renderer::PaintContext& ctx) {
    if (text_.empty()) {
        return;
    }
    core::Color col = effectiveColor();
    if (col.a <= 0.0f) {
        return;
    }
    auto f = effectiveFont();
    if (!f) {
        return;
    }

    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    ensureLayout(rect.size.width);
    if (!cachedLayout_ || cachedLayout_->lines().empty()) {
        return;
    }

    core::Size layoutSize = cachedLayout_->size();
    float textY = rect.origin.y;

    switch (verticalAlignment_) {
    case VerticalAlignment::Top:
        textY = rect.origin.y;
        break;
    case VerticalAlignment::Bottom:
        textY = rect.origin.y + std::max(0.0f, rect.size.height - layoutSize.height);
        break;
    case VerticalAlignment::Center:
        if (cachedLayout_->lines().size() <= 1 && rect.size.height > layoutSize.height) {
            textY = theme::computeCenteredTextY(rect.origin.y + rect.size.height * 0.5f, *f);
        } else if (rect.size.height > layoutSize.height) {
            textY = rect.origin.y + (rect.size.height - layoutSize.height) * 0.5f;
        }
        break;
    }

    bool needsClip =
        (overflow_ == text::TextOverflow::Clip) &&
        (layoutSize.width > rect.size.width + 0.5f || layoutSize.height > rect.size.height + 0.5f);

    if (needsClip) {
        ctx.pushClip(rect);
    }
    ctx.drawTextLayout(core::Point{rect.origin.x, textY}, *cachedLayout_, col);
    if (needsClip) {
        ctx.popClip();
    }
}

} // namespace northern::widgets
