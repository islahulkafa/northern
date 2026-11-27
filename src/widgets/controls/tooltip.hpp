#pragma once

#include "core/math/color.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget.hpp"

#include <memory>
#include <optional>
#include <string>

namespace northern::widgets {

class Tooltip : public Widget {
public:
    Tooltip();
    explicit Tooltip(std::string text, std::string shortcut = {},
                     std::shared_ptr<text::Font> font = nullptr, std::string description = {});

    void setContent(std::string text, std::string shortcut = {}, std::string description = {});
    void setText(std::string text);
    void setShortcut(std::string shortcut);
    void setDescription(std::string description);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }
    [[nodiscard]] const std::string& shortcut() const noexcept { return shortcut_; }
    [[nodiscard]] const std::string& description() const noexcept { return description_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;
    [[nodiscard]] std::shared_ptr<text::Font> effectiveDescriptionFont() const;
    [[nodiscard]] std::shared_ptr<text::Font> effectiveShortcutFont() const;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void setMaxWidth(float maxWidth);
    [[nodiscard]] float maxWidth() const noexcept { return maxWidth_; }

    void setAlpha(float alpha) noexcept;
    [[nodiscard]] float alpha() const noexcept { return alpha_; }

    void setBackgroundColor(const core::Color& color) noexcept;
    void setBorderColor(const core::Color& color) noexcept;
    void setTextColor(const core::Color& color) noexcept;
    void setDescriptionColor(const core::Color& color) noexcept;
    void setShortcutColor(const core::Color& color) noexcept;
    void setShortcutBackgroundColor(const core::Color& color) noexcept;
    void setShortcutBorderColor(const core::Color& color) noexcept;
    void clearColorOverrides() noexcept;

    [[nodiscard]] core::Color effectiveBackgroundColor() const noexcept;
    [[nodiscard]] core::Color effectiveBorderColor() const noexcept;
    [[nodiscard]] core::Color effectiveTextColor() const noexcept;
    [[nodiscard]] core::Color effectiveDescriptionColor() const noexcept;
    [[nodiscard]] core::Color effectiveShortcutColor() const noexcept;
    [[nodiscard]] core::Color effectiveShortcutBackgroundColor() const noexcept;
    [[nodiscard]] core::Color effectiveShortcutBorderColor() const noexcept;

    [[nodiscard]] core::Size preferredSize() const;

    void onContextAttached() override;
    void onThemeChanged() override;
    void paint(renderer::PaintContext& ctx) override;

private:
    void applyDpiScale(float scale);
    void invalidateLayoutCache() noexcept;
    void ensureLayouts() const;
    void drawShortcutBadge(renderer::PaintContext& ctx, float badgeX, float badgeY, float scale,
                           float borderW, float outerRadius) const;

    std::string text_{};
    std::string shortcut_{};
    std::string description_{};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedPrimaryFont_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDescFont_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedShortcutFont_{nullptr};

    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};
    float maxWidth_{280.0f};
    float alpha_{1.0f};

    std::optional<core::Color> overrideBgColor_{std::nullopt};
    std::optional<core::Color> overrideBorderColor_{std::nullopt};
    std::optional<core::Color> overrideTextColor_{std::nullopt};
    std::optional<core::Color> overrideDescColor_{std::nullopt};
    std::optional<core::Color> overrideShortcutColor_{std::nullopt};
    std::optional<core::Color> overrideShortcutBgColor_{std::nullopt};
    std::optional<core::Color> overrideShortcutBorderColor_{std::nullopt};

    mutable std::optional<text::TextLayout> cachedTextLayout_{std::nullopt};
    mutable std::optional<text::TextLayout> cachedDescLayout_{std::nullopt};
    mutable std::optional<text::TextLayout> cachedShortcutLayout_{std::nullopt};
    mutable core::Size cachedPreferredSize_{0.0f, 0.0f};
    mutable float cachedHeaderHeight_{0.0f};
    mutable float cachedBadgeWidth_{0.0f};
    mutable float cachedBadgeHeight_{0.0f};
    mutable bool layoutCacheValid_{false};
};

} // namespace northern::widgets
