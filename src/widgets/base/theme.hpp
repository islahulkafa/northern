#pragma once

#include "core/math/color.hpp"
#include "core/math/geometry.hpp"
#include "renderer/paint_context.hpp"
#include "text/font.hpp"
#include "text/font_manager.hpp"

namespace northern::theme {

enum class ThemeMode : uint8_t { Dark, Light };

struct ColorTokens {
    // Dark Mode Optical Zinc Ramp
    static constexpr core::Color SurfaceCanvas{0.035f, 0.035f, 0.043f, 1.0f};  // #09090B
    static constexpr core::Color SurfaceBase{0.071f, 0.071f, 0.082f, 1.0f};    // #121215
    static constexpr core::Color SurfaceDefault{0.094f, 0.094f, 0.106f, 1.0f}; // #18181B
    static constexpr core::Color SurfaceRaised{0.153f, 0.153f, 0.165f, 1.0f};  // #27272A
    static constexpr core::Color SurfaceOverlay{0.196f, 0.196f, 0.220f, 1.0f}; // #323238

    // Hairline Borders
    static constexpr core::Color BorderSubtle{1.0f, 1.0f, 1.0f, 0.08f};
    static constexpr core::Color BorderDefault{1.0f, 1.0f, 1.0f, 0.12f};
    static constexpr core::Color BorderStrong{1.0f, 1.0f, 1.0f, 0.16f};

    // Accent System (Cobalt #2563EB)
    static constexpr core::Color AccentDefault{0.145f, 0.388f, 0.922f, 1.0f}; // #2563EB
    static constexpr core::Color AccentHover{0.114f, 0.306f, 0.847f, 1.0f};   // #1D4ED8
    static constexpr core::Color AccentActive{0.118f, 0.251f, 0.686f, 1.0f};  // #1E40AF
    static constexpr core::Color AccentMuted{0.145f, 0.388f, 0.922f, 0.20f};
    static constexpr core::Color AccentFocusRing{0.145f, 0.388f, 0.922f, 0.70f};

    // Text Hierarchy
    static constexpr core::Color TextPrimary{0.980f, 0.980f, 0.980f, 1.0f};   // #FAFAFA
    static constexpr core::Color TextSecondary{0.631f, 0.631f, 0.667f, 1.0f}; // #A1A1AA
    static constexpr core::Color TextTertiary{0.443f, 0.443f, 0.478f, 1.0f};  // #71717A
    static constexpr core::Color TextDisabled{0.322f, 0.322f, 0.357f, 1.0f};  // #52525B

    // Status Semantics
    static constexpr core::Color Success{0.133f, 0.773f, 0.369f, 1.0f}; // #22C55E
    static constexpr core::Color Warning{0.961f, 0.620f, 0.043f, 1.0f}; // #F59E0B
    static constexpr core::Color Danger{0.937f, 0.267f, 0.267f, 1.0f};  // #EF4444
    static constexpr core::Color Info{0.231f, 0.510f, 0.965f, 1.0f};    // #3B82F6
};

struct ColorPalette {
    ThemeMode mode{ThemeMode::Dark};
    core::Color surfaceCanvas{ColorTokens::SurfaceCanvas};
    core::Color surfaceBase{ColorTokens::SurfaceBase};
    core::Color surfaceDefault{ColorTokens::SurfaceDefault};
    core::Color surfaceRaised{ColorTokens::SurfaceRaised};
    core::Color surfaceOverlay{ColorTokens::SurfaceOverlay};

    core::Color borderSubtle{ColorTokens::BorderSubtle};
    core::Color borderDefault{ColorTokens::BorderDefault};
    core::Color borderStrong{ColorTokens::BorderStrong};

    core::Color accentDefault{ColorTokens::AccentDefault};
    core::Color accentHover{ColorTokens::AccentHover};
    core::Color accentActive{ColorTokens::AccentActive};
    core::Color accentMuted{ColorTokens::AccentMuted};
    core::Color accentFocusRing{ColorTokens::AccentFocusRing};

    core::Color textPrimary{ColorTokens::TextPrimary};
    core::Color textSecondary{ColorTokens::TextSecondary};
    core::Color textTertiary{ColorTokens::TextTertiary};
    core::Color textDisabled{ColorTokens::TextDisabled};

    core::Color success{ColorTokens::Success};
    core::Color warning{ColorTokens::Warning};
    core::Color danger{ColorTokens::Danger};
    core::Color info{ColorTokens::Info};

    [[nodiscard]] static const ColorPalette& dark() noexcept {
        static constexpr ColorPalette kDark{
            .mode = ThemeMode::Dark,
            .surfaceCanvas = ColorTokens::SurfaceCanvas,
            .surfaceBase = ColorTokens::SurfaceBase,
            .surfaceDefault = ColorTokens::SurfaceDefault,
            .surfaceRaised = ColorTokens::SurfaceRaised,
            .surfaceOverlay = ColorTokens::SurfaceOverlay,
            .borderSubtle = ColorTokens::BorderSubtle,
            .borderDefault = ColorTokens::BorderDefault,
            .borderStrong = ColorTokens::BorderStrong,
            .accentDefault = ColorTokens::AccentDefault,
            .accentHover = ColorTokens::AccentHover,
            .accentActive = ColorTokens::AccentActive,
            .accentMuted = ColorTokens::AccentMuted,
            .accentFocusRing = ColorTokens::AccentFocusRing,
            .textPrimary = ColorTokens::TextPrimary,
            .textSecondary = ColorTokens::TextSecondary,
            .textTertiary = ColorTokens::TextTertiary,
            .textDisabled = ColorTokens::TextDisabled,
            .success = ColorTokens::Success,
            .warning = ColorTokens::Warning,
            .danger = ColorTokens::Danger,
            .info = ColorTokens::Info,
        };
        return kDark;
    }

    [[nodiscard]] static const ColorPalette& light() noexcept {
        static constexpr ColorPalette kLight{
            .mode = ThemeMode::Light,
            .surfaceCanvas = core::Color{0.957f, 0.957f, 0.961f, 1.0f},
            .surfaceBase = core::Color{0.980f, 0.980f, 0.980f, 1.0f},
            .surfaceDefault = core::Color{1.0f, 1.0f, 1.0f, 1.0f},
            .surfaceRaised = core::Color{0.957f, 0.957f, 0.961f, 1.0f},
            .surfaceOverlay = core::Color{1.0f, 1.0f, 1.0f, 1.0f},
            .borderSubtle = core::Color{0.0f, 0.0f, 0.0f, 0.08f},
            .borderDefault = core::Color{0.0f, 0.0f, 0.0f, 0.12f},
            .borderStrong = core::Color{0.0f, 0.0f, 0.0f, 0.14f},
            .accentDefault = core::Color{0.145f, 0.388f, 0.922f, 1.0f},
            .accentHover = core::Color{0.114f, 0.306f, 0.847f, 1.0f},
            .accentActive = core::Color{0.118f, 0.251f, 0.686f, 1.0f},
            .accentMuted = core::Color{0.145f, 0.388f, 0.922f, 0.15f},
            .accentFocusRing = core::Color{0.145f, 0.388f, 0.922f, 0.70f},
            .textPrimary = core::Color{0.035f, 0.035f, 0.043f, 1.0f},
            .textSecondary = core::Color{0.322f, 0.322f, 0.357f, 1.0f},
            .textTertiary = core::Color{0.443f, 0.443f, 0.478f, 1.0f},
            .textDisabled = core::Color{0.631f, 0.631f, 0.667f, 1.0f},
            .success = core::Color{0.086f, 0.639f, 0.290f, 1.0f},
            .warning = core::Color{0.851f, 0.467f, 0.024f, 1.0f},
            .danger = core::Color{0.863f, 0.149f, 0.149f, 1.0f},
            .info = core::Color{0.145f, 0.388f, 0.922f, 1.0f},
        };
        return kLight;
    }
};

class Theme {
public:
    Theme() : palette_(ColorPalette::dark()) {}
    explicit Theme(ColorPalette palette) : palette_(std::move(palette)) {}

    [[nodiscard]] const ColorPalette& colors() const noexcept { return palette_; }
    [[nodiscard]] ThemeMode mode() const noexcept { return palette_.mode; }
    [[nodiscard]] bool isDark() const noexcept { return palette_.mode == ThemeMode::Dark; }
    void setPalette(ColorPalette palette) noexcept { palette_ = std::move(palette); }

    [[nodiscard]] static const Theme& dark() noexcept {
        static const Theme kDark{ColorPalette::dark()};
        return kDark;
    }
    [[nodiscard]] static const Theme& light() noexcept {
        static const Theme kLight{ColorPalette::light()};
        return kLight;
    }

private:
    ColorPalette palette_;
};

struct Metrics {
    static constexpr float GridBase = 4.0f;
    static constexpr float ControlHeightCompact = 28.0f;
    static constexpr float ControlHeightStandard = 32.0f;
    static constexpr float ControlHeightComfortable = 36.0f;
    static constexpr float CornerRadiusControl = 6.0f;
    static constexpr float CornerRadiusCard = 8.0f;
    static constexpr float CornerRadiusOverlay = 10.0f;
    static constexpr float CornerRadiusButtonCompact = 6.0f;
    static constexpr float CornerRadiusButtonStandard = 8.0f;
    static constexpr float CornerRadiusButtonComfortable = 9.0f;
    static constexpr float BorderWidthHairline = 1.0f;
    static constexpr float FocusRingWidth = 2.0f;
    static constexpr float FocusRingOffset = 2.0f;
};

[[nodiscard]] constexpr float resolveConcentricRadius(float outerRadius, float inset,
                                                      float minRadius = 4.0f) noexcept {
    float candidate = outerRadius - inset;
    return candidate > minRadius ? candidate : minRadius;
}

inline void paintFocusRing(renderer::PaintContext& ctx, const core::Rect& rect, float cornerRadius,
                           float ringOffset = Metrics::FocusRingOffset,
                           float ringWidth = Metrics::FocusRingWidth,
                           const core::Color& ringColor = ColorTokens::AccentFocusRing) {
    core::Rect ringRect = rect.outset(core::Insets{ringOffset});
    float outerRadius = cornerRadius > 0.0f ? cornerRadius + ringOffset : 0.0f;
    ctx.drawBorder(ringRect, ringColor, ringWidth, outerRadius);
}

inline void paintFocusRing(renderer::PaintContext& ctx, const core::Rect& rect, float cornerRadius,
                           const ColorPalette& palette, float dpiScale = 1.0f) {
    float scale = dpiScale > 0.0f ? dpiScale : 1.0f;
    paintFocusRing(ctx, rect, cornerRadius, Metrics::FocusRingOffset * scale,
                   Metrics::FocusRingWidth * scale, palette.accentFocusRing);
}

inline float computeCenteredTextY(float targetCenterY, const text::Font& font) {
    float capHeight = font.capHeight();
    if (capHeight <= 0.0f) {
        float px = font.pixelSize();
        capHeight = (px > 0.0f) ? (px * 0.70f) : (font.pointSize() * (96.0f / 72.0f) * 0.70f);
    }
    return targetCenterY + capHeight * 0.5f - font.ascender();
}

enum class TextRole : uint8_t {
    Primary,
    Secondary,
    Tertiary,
    Disabled,
    Accent,
    Success,
    Warning,
    Danger,
    Info,
};

enum class TypographyStyle : uint8_t {
    Display,
    Title,
    Subtitle,
    Body,
    BodyStrong,
    Caption,
    Monospace,
};

[[nodiscard]] constexpr float typographyPointSize(TypographyStyle style) noexcept {
    switch (style) {
    case TypographyStyle::Display:
        return 24.0f;
    case TypographyStyle::Title:
        return 18.0f;
    case TypographyStyle::Subtitle:
        return 15.0f;
    case TypographyStyle::Body:
    case TypographyStyle::BodyStrong:
        return 14.0f;
    case TypographyStyle::Caption:
        return 12.0f;
    case TypographyStyle::Monospace:
        return 13.0f;
    }
    return 14.0f;
}

[[nodiscard]] inline std::shared_ptr<text::Font> resolveFont(TypographyStyle style,
                                                             float dpiScale = 1.0f) {
    float scale = dpiScale > 0.0f ? dpiScale : 1.0f;
    float pt = typographyPointSize(style) * scale;
    auto& mgr = text::FontManager::instance();
    switch (style) {
    case TypographyStyle::Display:
    case TypographyStyle::Title:
    case TypographyStyle::BodyStrong:
        return mgr.boldFont(pt).valueOr(nullptr);
    case TypographyStyle::Monospace:
        return mgr.monospaceFont(pt).valueOr(nullptr);
    case TypographyStyle::Subtitle:
    case TypographyStyle::Body:
    case TypographyStyle::Caption:
    default:
        return mgr.defaultFont(pt).valueOr(nullptr);
    }
}

} // namespace northern::theme
