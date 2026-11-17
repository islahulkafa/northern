#pragma once

#include "core/math/color.hpp"
#include "core/math/geometry.hpp"
#include "renderer/paint_context.hpp"
#include "text/font.hpp"

namespace northern::theme {

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

struct Metrics {
    static constexpr float GridBase = 4.0f;
    static constexpr float ControlHeightCompact = 28.0f;
    static constexpr float ControlHeightStandard = 32.0f;
    static constexpr float ControlHeightComfortable = 36.0f;
    static constexpr float CornerRadiusControl = 6.0f;
    static constexpr float CornerRadiusCard = 8.0f;
    static constexpr float CornerRadiusOverlay = 10.0f;
    static constexpr float BorderWidthHairline = 1.0f;
    static constexpr float FocusRingWidth = 2.0f;
    static constexpr float FocusRingOffset = 2.0f;
};

inline void paintFocusRing(renderer::PaintContext& ctx, const core::Rect& rect, float cornerRadius,
                           float ringOffset = Metrics::FocusRingOffset,
                           float ringWidth = Metrics::FocusRingWidth,
                           const core::Color& ringColor = ColorTokens::AccentFocusRing) {
    core::Rect ringRect = rect.outset(core::Insets{ringOffset});
    float outerRadius = cornerRadius > 0.0f ? cornerRadius + ringOffset : 0.0f;
    ctx.drawBorder(ringRect, ringColor, ringWidth, outerRadius);
}

inline float computeCenteredTextY(float targetCenterY, const text::Font& font) {
    float capHeight = font.pointSize() * 0.70f;
    return targetCenterY + capHeight * 0.5f - font.ascender();
}

} // namespace northern::theme
