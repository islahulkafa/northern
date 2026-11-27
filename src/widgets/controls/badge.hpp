#pragma once

#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/widget.hpp"
#include "widgets/controls/icons.hpp"

#include <memory>
#include <optional>
#include <string>

namespace northern::widgets {

enum class BadgeVariant : uint8_t {
    Subtle,
    Solid,
    Outline,
};

enum class BadgeRole : uint8_t {
    Neutral,
    Accent,
    Success,
    Warning,
    Danger,
    Info,
};

enum class BadgeSize : uint8_t {
    Compact,
    Standard,
};

struct BadgeStyleColors {
    core::Color background{};
    core::Color border{};
    core::Color foreground{};
};

class Badge : public Widget {
public:
    explicit Badge(std::string text = "", BadgeRole role = BadgeRole::Neutral,
                   BadgeVariant variant = BadgeVariant::Subtle,
                   BadgeSize size = BadgeSize::Standard);
    ~Badge() override = default;

    void setText(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    void setVariant(BadgeVariant variant) noexcept;
    [[nodiscard]] BadgeVariant variant() const noexcept { return variant_; }

    void setRole(BadgeRole role) noexcept;
    [[nodiscard]] BadgeRole role() const noexcept { return role_; }

    void setSize(BadgeSize size) noexcept;
    [[nodiscard]] BadgeSize size() const noexcept { return size_; }

    void setIcon(std::optional<IconId> icon);
    [[nodiscard]] std::optional<IconId> icon() const noexcept { return icon_; }

    void setShowDot(bool show) noexcept;
    [[nodiscard]] bool showDot() const noexcept { return showDot_; }

    void setDismissible(bool dismissible) noexcept;
    [[nodiscard]] bool isDismissible() const noexcept { return dismissible_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] const std::shared_ptr<text::Font>& font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    core::Signal<> onDismissed;

    [[nodiscard]] static float cornerRadiusForSize(BadgeSize size, float dpiScale = 1.0f) noexcept;
    [[nodiscard]] static BadgeStyleColors resolveColors(const theme::ColorPalette& palette,
                                                        BadgeRole role, BadgeVariant variant,
                                                        bool enabled = true) noexcept;
    static void paintInline(renderer::PaintContext& ctx, const core::Rect& rect,
                            const text::TextLayout& layout, const text::Font& font,
                            const theme::ColorPalette& palette, float dpiScale,
                            BadgeRole role = BadgeRole::Neutral,
                            BadgeVariant variant = BadgeVariant::Subtle,
                            BadgeSize size = BadgeSize::Compact,
                            std::optional<core::Color> customBg = std::nullopt,
                            std::optional<core::Color> customFg = std::nullopt);

    void paint(renderer::PaintContext& ctx) override;
    void onMouseMove(WidgetMouseEvent& event) override;
    void onMouseDown(WidgetMouseEvent& event) override;
    void onMouseUp(WidgetMouseEvent& event) override;
    void onMouseLeave() override;
    void onKeyDown(WidgetKeyEvent& event) override;

protected:
    void onContextAttached() override;
    void onThemeChanged() override;

private:
    void applySizeMetrics();
    void ensureTextLayout() const;
    core::Rect dismissButtonRect(const core::Rect& pillRect) const noexcept;
    core::Color roleColor() const noexcept;

    std::string text_;
    BadgeVariant variant_{BadgeVariant::Subtle};
    BadgeRole role_{BadgeRole::Neutral};
    BadgeSize size_{BadgeSize::Standard};
    std::optional<IconId> icon_;
    bool showDot_{false};
    bool dismissible_{false};
    bool closeHovered_{false};
    bool closePressed_{false};

    std::shared_ptr<text::Font> font_;
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_;
    mutable std::optional<text::TextLayout> cachedLayout_;

    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};
};

} // namespace northern::widgets
