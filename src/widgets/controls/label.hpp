#pragma once

#include "core/math/color.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace northern::widgets {

enum class VerticalAlignment : uint8_t { Top, Center, Bottom };

class Label : public Widget {
public:
    explicit Label(std::string text = "", std::shared_ptr<text::Font> font = nullptr);
    Label(std::string text, theme::TypographyStyle textStyle,
          theme::TextRole role = theme::TextRole::Primary);
    ~Label() override = default;

    void setText(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setTextStyle(theme::TypographyStyle style);
    [[nodiscard]] theme::TypographyStyle textStyle() const noexcept { return textStyle_; }

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void setRole(theme::TextRole role) noexcept;
    [[nodiscard]] theme::TextRole role() const noexcept { return role_; }

    void setTextColor(const core::Color& color) noexcept;
    void clearColorOverride() noexcept;
    [[nodiscard]] core::Color textColor() const noexcept { return effectiveColor(); }
    [[nodiscard]] core::Color effectiveColor() const noexcept;

    void setTextAlignment(text::TextAlignment alignment) noexcept;
    [[nodiscard]] text::TextAlignment textAlignment() const noexcept { return alignment_; }

    void setVerticalAlignment(VerticalAlignment alignment) noexcept;
    [[nodiscard]] VerticalAlignment verticalAlignment() const noexcept {
        return verticalAlignment_;
    }

    void setWordWrap(text::WordWrap wrapMode) noexcept;
    [[nodiscard]] text::WordWrap wordWrap() const noexcept { return wrapMode_; }

    void setOverflow(text::TextOverflow overflow) noexcept;
    [[nodiscard]] text::TextOverflow overflow() const noexcept { return overflow_; }

    void setMaxLines(uint32_t maxLines) noexcept;
    [[nodiscard]] uint32_t maxLines() const noexcept { return maxLines_; }

    void setLineHeightMultiplier(float multiplier) noexcept;
    [[nodiscard]] float lineHeightMultiplier() const noexcept { return lineHeightMultiplier_; }

    [[nodiscard]] bool isTruncated() const noexcept;

    void paint(renderer::PaintContext& ctx) override;

private:
    void invalidateLayoutCache() noexcept;
    void ensureUnwrappedSize(const std::shared_ptr<text::Font>& f) const;
    void ensureLayout(float maxWidth) const;

    std::string text_{};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};

    theme::TypographyStyle textStyle_{theme::TypographyStyle::Body};
    theme::TextRole role_{theme::TextRole::Primary};
    std::optional<core::Color> overrideColor_{std::nullopt};
    float dpiScale_{1.0f};

    text::TextAlignment alignment_{text::TextAlignment::Left};
    VerticalAlignment verticalAlignment_{VerticalAlignment::Center};
    text::WordWrap wrapMode_{text::WordWrap::Word};
    text::TextOverflow overflow_{text::TextOverflow::Clip};
    uint32_t maxLines_{0};
    float lineHeightMultiplier_{1.0f};

    mutable std::optional<text::TextLayout> cachedLayout_{};
    mutable float cachedWidth_{-1.0f};
    mutable core::Size unwrappedSize_{0.0f, 0.0f};
    mutable bool unwrappedSizeValid_{false};
};

} // namespace northern::widgets
