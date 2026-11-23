#include "widgets/controls/tooltip.hpp"

#include "renderer/paint_context.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"

#include <algorithm>

namespace northern::widgets {

Tooltip::Tooltip(std::string text, std::string shortcut, std::shared_ptr<text::Font> font)
    : text_(std::move(text)), shortcut_(std::move(shortcut)), font_(std::move(font)) {
    setFocusable(false);
}

void Tooltip::setContent(std::string text, std::string shortcut) {
    text_ = std::move(text);
    shortcut_ = std::move(shortcut);
    markPaintDirty();
}

void Tooltip::setFont(std::shared_ptr<text::Font> font) {
    font_ = std::move(font);
    markPaintDirty();
}

void Tooltip::setAlpha(float alpha) noexcept {
    float clamped = std::clamp(alpha, 0.0f, 1.0f);
    if (alpha_ != clamped) {
        alpha_ = clamped;
        markPaintDirty();
    }
}

core::Size Tooltip::preferredSize() const {
    constexpr float kPadX = 10.0f;
    constexpr float kPadY = 6.0f;
    constexpr float kShortcutGap = 10.0f;

    float textW = static_cast<float>(text_.size()) * 6.5f;
    float shortcutW = shortcut_.empty() ? 0.0f : static_cast<float>(shortcut_.size()) * 6.0f;
    float contentH = 14.0f;

    if (font_) {
        if (!text_.empty()) {
            auto textLayout = text::TextLayout::create(text_, font_);
            textW = textLayout.size().width;
            contentH = std::max(contentH, font_->lineHeight());
        }
        if (!shortcut_.empty()) {
            auto scLayout = text::TextLayout::create(shortcut_, font_);
            shortcutW = scLayout.size().width;
            contentH = std::max(contentH, font_->lineHeight());
        }
    }

    float totalW = kPadX * 2.0f + textW;
    if (!shortcut_.empty()) {
        totalW += kShortcutGap + shortcutW;
    }

    return core::Size{std::max(40.0f, totalW), std::max(24.0f, contentH + kPadY * 2.0f)};
}

void Tooltip::paint(renderer::PaintContext& ctx) {
    if (alpha_ <= 0.01f) {
        return;
    }

    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    constexpr float kRadius = theme::Metrics::CornerRadiusControl;
    float radii[4] = {kRadius, kRadius, kRadius, kRadius};
    core::Color bg = theme::ColorTokens::SurfaceOverlay.withAlpha(alpha_);
    core::Color border =
        theme::ColorTokens::BorderStrong.withAlpha(theme::ColorTokens::BorderStrong.a * alpha_);

    ctx.drawBox(rect, bg, border, 1.0f, radii);

    if (font_) {
        constexpr float kPadX = 10.0f;
        float centerY = rect.origin.y + rect.size.height * 0.5f;
        float textY = theme::computeCenteredTextY(centerY, *font_);

        if (!text_.empty()) {
            auto textLayout = text::TextLayout::create(text_, font_);
            core::Color textCol = theme::ColorTokens::TextPrimary.withAlpha(alpha_);
            ctx.drawTextLayout(core::Point{rect.origin.x + kPadX, textY}, textLayout, textCol);
        }

        if (!shortcut_.empty()) {
            auto scLayout = text::TextLayout::create(shortcut_, font_);
            float scX = rect.origin.x + rect.size.width - kPadX - scLayout.size().width;
            core::Color scCol = theme::ColorTokens::TextTertiary.withAlpha(alpha_);
            ctx.drawTextLayout(core::Point{scX, textY}, scLayout, scCol);
        }
    }
}

} // namespace northern::widgets
