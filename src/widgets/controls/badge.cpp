#include "widgets/controls/badge.hpp"
#include "renderer/paint_context.hpp"
#include "renderer/svg/svg_document.hpp"
#include "text/font_manager.hpp"
#include "widgets/base/theme.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

Badge::Badge(std::string text, BadgeRole role, BadgeVariant variant, BadgeSize size)
    : text_(std::move(text)), variant_(variant), role_(role), size_(size) {
    applySizeMetrics();

    layoutNode_.setMeasureFunc([this](const layout::LayoutNode&, float, float) -> core::Size {
        float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
        float h = (size_ == BadgeSize::Compact ? 20.0f : 24.0f) * scale;
        float padH = (size_ == BadgeSize::Compact ? 7.0f : 9.0f) * scale;
        float gap = 5.0f * scale;

        float contentW = 0.0f;
        if (showDot_) {
            contentW += 6.0f * scale;
            if (!text_.empty() || icon_.has_value()) {
                contentW += gap;
            }
        }
        if (icon_) {
            float iconSz = (size_ == BadgeSize::Compact ? 11.0f : 12.5f) * scale;
            contentW += iconSz;
            if (!text_.empty()) {
                contentW += gap;
            }
        }
        ensureTextLayout();
        if (cachedLayout_) {
            contentW += cachedLayout_->size().width;
        }
        if (dismissible_) {
            float closeSz = (size_ == BadgeSize::Compact ? 12.0f : 14.0f) * scale;
            if (contentW > 0.0f) {
                contentW += gap * 0.8f;
            }
            contentW += closeSz;
        }
        return core::Size{std::max(h, contentW + padH * 2.0f), h};
    });
}

void Badge::setText(std::string text) {
    if (text_ != text) {
        text_ = std::move(text);
        cachedLayout_.reset();
        markLayoutDirty();
        markPaintDirty();
    }
}

void Badge::setVariant(BadgeVariant variant) noexcept {
    if (variant_ != variant) {
        variant_ = variant;
        markPaintDirty();
    }
}

void Badge::setRole(BadgeRole role) noexcept {
    if (role_ != role) {
        role_ = role;
        markPaintDirty();
    }
}

void Badge::setSize(BadgeSize size) noexcept {
    if (size_ != size) {
        size_ = size;
        resolvedDefaultFont_.reset();
        cachedLayout_.reset();
        applySizeMetrics();
        markPaintDirty();
    }
}

void Badge::setIcon(std::optional<IconId> icon) {
    if (icon_ != icon) {
        icon_ = icon;
        markLayoutDirty();
        markPaintDirty();
    }
}

void Badge::setShowDot(bool show) noexcept {
    if (showDot_ != show) {
        showDot_ = show;
        markLayoutDirty();
        markPaintDirty();
    }
}

void Badge::setDismissible(bool dismissible) noexcept {
    if (dismissible_ != dismissible) {
        dismissible_ = dismissible;
        setFocusable(dismissible);
        closeHovered_ = false;
        closePressed_ = false;
        markLayoutDirty();
        markPaintDirty();
    }
}

void Badge::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedDefaultFont_.reset();
        cachedLayout_.reset();
        markLayoutDirty();
        markPaintDirty();
    }
}

std::shared_ptr<text::Font> Badge::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float px = (size_ == BadgeSize::Compact ? 11.5f : 12.0f) * dpiScale_;
        resolvedDefaultFont_ = text::FontManager::instance().mediumFont(px).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void Badge::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        resolvedDefaultFont_.reset();
        cachedLayout_.reset();
        applySizeMetrics();
        markPaintDirty();
    }
}

void Badge::onContextAttached() {
    if (!hasCustomDpiScale_) {
        float scale = contextDpiScale();
        if (dpiScale_ != scale) {
            dpiScale_ = scale;
            resolvedDefaultFont_.reset();
            cachedLayout_.reset();
            applySizeMetrics();
        }
    }
}

void Badge::onThemeChanged() {
    cachedLayout_.reset();
    markPaintDirty();
}

void Badge::applySizeMetrics() {
    float h = (size_ == BadgeSize::Compact ? 20.0f : 24.0f) * dpiScale_;
    style().height = layout::Length::points(h);
    markLayoutDirty();
}

void Badge::ensureTextLayout() const {
    if (cachedLayout_ || text_.empty()) {
        return;
    }
    auto f = effectiveFont();
    if (f) {
        cachedLayout_ = text::TextLayout::create(text_, f);
    }
}

core::Color roleColorFromPalette(const theme::ColorPalette& pal, BadgeRole role) noexcept {
    switch (role) {
    case BadgeRole::Accent:
        return pal.accentDefault;
    case BadgeRole::Success:
        return pal.success;
    case BadgeRole::Warning:
        return pal.warning;
    case BadgeRole::Danger:
        return pal.danger;
    case BadgeRole::Info:
        return pal.info;
    case BadgeRole::Neutral:
    default:
        return pal.textSecondary;
    }
}

core::Color Badge::roleColor() const noexcept {
    return roleColorFromPalette(colorPalette(), role_);
}

float Badge::cornerRadiusForSize(BadgeSize size, float dpiScale) noexcept {
    float scale = dpiScale > 0.0f ? dpiScale : 1.0f;
    return (size == BadgeSize::Compact ? 4.0f : 6.0f) * scale;
}

BadgeStyleColors Badge::resolveColors(const theme::ColorPalette& pal, BadgeRole role,
                                      BadgeVariant variant, bool enabled) noexcept {
    core::Color rc = roleColorFromPalette(pal, role);
    BadgeStyleColors out{};
    out.foreground = pal.textPrimary;

    switch (variant) {
    case BadgeVariant::Solid:
        out.background = (role == BadgeRole::Neutral) ? pal.surfaceRaised : rc;
        out.foreground = (role == BadgeRole::Neutral) ? pal.textPrimary : core::Color::white();
        break;
    case BadgeVariant::Outline:
        out.border = (role == BadgeRole::Neutral) ? pal.borderStrong : rc.withAlpha(0.55f);
        out.foreground = (role == BadgeRole::Neutral) ? pal.textSecondary : rc;
        break;
    case BadgeVariant::Subtle:
    default:
        out.background = (role == BadgeRole::Neutral) ? pal.surfaceRaised : rc.withAlpha(0.16f);
        out.border = (role == BadgeRole::Neutral) ? pal.borderSubtle : rc.withAlpha(0.28f);
        out.foreground = (role == BadgeRole::Neutral) ? pal.textPrimary : rc;
        break;
    }

    if (!enabled) {
        out.background = out.background.withAlpha(out.background.a * 0.5f);
        out.border = out.border.withAlpha(out.border.a * 0.5f);
        out.foreground = pal.textDisabled;
    }
    return out;
}

void Badge::paintInline(renderer::PaintContext& ctx, const core::Rect& rect,
                        const text::TextLayout& layout, const text::Font& font,
                        const theme::ColorPalette& palette, float dpiScale, BadgeRole role,
                        BadgeVariant variant, BadgeSize size, std::optional<core::Color> customBg,
                        std::optional<core::Color> customFg) {
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }
    float scale = dpiScale > 0.0f ? dpiScale : 1.0f;
    float borderW = std::max(1.0f, std::round(1.0f * scale));
    float radius = cornerRadiusForSize(size, scale);
    float radii[4]{radius, radius, radius, radius};

    BadgeStyleColors colors = resolveColors(palette, role, variant, true);
    if (customBg && customBg->a > 0.0f) {
        colors.background = *customBg;
    }
    if (customFg && customFg->a > 0.0f) {
        colors.foreground = *customFg;
    }

    ctx.pushClip(rect);
    ctx.drawBox(rect, colors.background, colors.border, colors.border.a > 0.0f ? borderW : 0.0f,
                radii);
    float padH = (size == BadgeSize::Compact ? 7.0f : 9.0f) * scale;
    float centerY = rect.origin.y + rect.size.height * 0.5f;
    float textY = theme::computeCenteredTextY(centerY, font);
    ctx.drawTextLayout(core::Point{rect.origin.x + padH, textY}, layout, colors.foreground);
    ctx.popClip();
}

core::Rect Badge::dismissButtonRect(const core::Rect& pillRect) const noexcept {
    if (!dismissible_) {
        return {};
    }
    float padH = (size_ == BadgeSize::Compact ? 6.0f : 8.0f) * dpiScale_;
    float closeSz = (size_ == BadgeSize::Compact ? 13.0f : 15.0f) * dpiScale_;
    return core::Rect{
        pillRect.origin.x + pillRect.size.width - padH - closeSz,
        pillRect.origin.y + (pillRect.size.height - closeSz) * 0.5f,
        closeSz,
        closeSz,
    };
}

void Badge::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    const auto& pal = colorPalette();
    core::Color rc = roleColor();
    float borderW = std::max(1.0f, std::round(1.0f * dpiScale_));
    float radius = cornerRadiusForSize(size_, dpiScale_);
    float radii[4]{radius, radius, radius, radius};

    BadgeStyleColors colors = resolveColors(pal, role_, variant_, isEnabled());
    core::Color bg = colors.background;
    core::Color border = colors.border;
    core::Color fg = colors.foreground;

    ctx.drawBox(rect, bg, border, border.a > 0.0f ? borderW : 0.0f, radii);

    if (isEnabled() && hasKeyboardFocus()) {
        theme::paintFocusRing(ctx, rect, radius, pal, dpiScale_);
    }

    float padH = (size_ == BadgeSize::Compact ? 7.0f : 9.0f) * dpiScale_;
    float gap = 5.0f * dpiScale_;
    float cursorX = rect.origin.x + padH;
    float centerY = rect.origin.y + rect.size.height * 0.5f;

    if (showDot_) {
        float dotSz = 6.0f * dpiScale_;
        float dotR = dotSz * 0.5f;
        float dotRadii[4]{dotR, dotR, dotR, dotR};
        core::Color dotCol = (variant_ == BadgeVariant::Solid && role_ != BadgeRole::Neutral)
                                 ? core::Color::white()
                                 : rc;
        ctx.drawRoundedRect(core::Rect{cursorX, centerY - dotR, dotSz, dotSz}, dotCol, dotRadii);
        cursorX += dotSz + gap;
    }

    if (icon_) {
        if (auto doc = getSvgDocument(*icon_)) {
            float iconSz = (size_ == BadgeSize::Compact ? 11.0f : 12.5f) * dpiScale_;
            ctx.drawSvg(*doc, core::Rect{cursorX, centerY - iconSz * 0.5f, iconSz, iconSz}, fg);
            cursorX += iconSz + gap;
        }
    }

    ensureTextLayout();
    if (cachedLayout_) {
        auto f = effectiveFont();
        float textY = f ? theme::computeCenteredTextY(centerY, *f) : centerY;
        ctx.drawTextLayout(core::Point{cursorX, textY}, *cachedLayout_, fg);
    }

    if (dismissible_) {
        core::Rect closeRect = dismissButtonRect(rect);
        if (closeHovered_ || closePressed_) {
            float cr = closeRect.size.height * 0.5f;
            float crRadii[4]{cr, cr, cr, cr};
            core::Color pillBg = fg.withAlpha(closePressed_ ? 0.24f : 0.14f);
            ctx.drawRoundedRect(closeRect, pillBg, crRadii);
        }
        if (auto closeDoc = getSvgDocument(IconId::Close)) {
            float glyphSz = closeRect.size.width * 0.65f;
            core::Rect glyphRect{
                closeRect.origin.x + (closeRect.size.width - glyphSz) * 0.5f,
                closeRect.origin.y + (closeRect.size.height - glyphSz) * 0.5f,
                glyphSz,
                glyphSz,
            };
            ctx.drawSvg(*closeDoc, glyphRect, fg);
        }
    }
}

void Badge::onMouseMove(WidgetMouseEvent& event) {
    if (!dismissible_ || !isEnabled()) {
        return;
    }
    bool hovered = dismissButtonRect(screenRect()).contains(event.position());
    if (closeHovered_ != hovered) {
        closeHovered_ = hovered;
        setCursorShape(hovered ? platform::CursorShape::PointingHand
                               : platform::CursorShape::Arrow);
        markPaintDirty();
    }
}

void Badge::onMouseDown(WidgetMouseEvent& event) {
    if (!dismissible_ || !isEnabled() || event.button() != platform::MouseButton::Left) {
        return;
    }
    if (dismissButtonRect(screenRect()).contains(event.position())) {
        closePressed_ = true;
        event.accept();
        markPaintDirty();
    }
}

void Badge::onMouseUp(WidgetMouseEvent& event) {
    if (!dismissible_ || !isEnabled() || event.button() != platform::MouseButton::Left) {
        return;
    }
    bool wasPressed = closePressed_;
    closePressed_ = false;
    if (wasPressed && dismissButtonRect(screenRect()).contains(event.position())) {
        event.accept();
        onDismissed.emit();
    }
    markPaintDirty();
}

void Badge::onMouseLeave() {
    if (closeHovered_ || closePressed_) {
        closeHovered_ = false;
        closePressed_ = false;
        setCursorShape(platform::CursorShape::Arrow);
        markPaintDirty();
    }
}

void Badge::onKeyDown(WidgetKeyEvent& event) {
    if (!dismissible_ || !isEnabled()) {
        return;
    }
    if (event.key() == platform::Key::Delete || event.key() == platform::Key::Backspace ||
        event.key() == platform::Key::Enter || event.key() == platform::Key::Space) {
        event.accept();
        onDismissed.emit();
    }
}

} // namespace northern::widgets
