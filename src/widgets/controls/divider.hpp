#pragma once

#include "core/math/color.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/widget.hpp"

#include <memory>
#include <optional>
#include <string>

namespace northern::widgets {

enum class DividerOrientation : uint8_t { Horizontal, Vertical };
enum class DividerAlignment : uint8_t { Center, Start, End };
enum class DividerVariant : uint8_t { Subtle, Default, Strong };

class Divider : public Widget {
public:
    explicit Divider(DividerOrientation orientation = DividerOrientation::Horizontal,
                     float thickness = 1.0f);
    explicit Divider(std::string label, std::shared_ptr<text::Font> font = nullptr,
                     float thickness = 1.0f);

    void setOrientation(DividerOrientation orientation) noexcept;
    [[nodiscard]] DividerOrientation orientation() const noexcept { return orientation_; }

    void setThickness(float thickness) noexcept;
    [[nodiscard]] float thickness() const noexcept { return thickness_; }

    void setVariant(DividerVariant variant) noexcept;
    [[nodiscard]] DividerVariant variant() const noexcept { return variant_; }

    void setLabelAlignment(DividerAlignment alignment) noexcept;
    [[nodiscard]] DividerAlignment labelAlignment() const noexcept { return labelAlignment_; }

    void setLabelOffset(float offset) noexcept;
    [[nodiscard]] float labelOffset() const noexcept { return labelOffset_; }

    void setInset(float both) noexcept;
    void setInset(float start, float end) noexcept;
    [[nodiscard]] float insetStart() const noexcept { return insetStart_; }
    [[nodiscard]] float insetEnd() const noexcept { return insetEnd_; }

    void setColor(const core::Color& color) noexcept;
    [[nodiscard]] core::Color color() const noexcept { return effectiveColor(); }

    void setLabel(std::string label);
    [[nodiscard]] const std::string& label() const noexcept { return label_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void setTextColor(const core::Color& color) noexcept;
    [[nodiscard]] core::Color textColor() const noexcept { return effectiveTextColor(); }

    void setGap(float gap) noexcept;
    [[nodiscard]] float gap() const noexcept { return gap_; }

    [[nodiscard]] core::Color effectiveColor() const noexcept;
    [[nodiscard]] core::Color effectiveTextColor() const noexcept;

    void onContextAttached() override;
    void onThemeChanged() override;
    void onEnabledChanged() override;

    void paint(renderer::PaintContext& ctx) override;

private:
    void applyDpiScale(float scale);
    void updateMeasureAndStyle();
    void ensureTextLayout() const;

    DividerOrientation orientation_{DividerOrientation::Horizontal};
    DividerVariant variant_{DividerVariant::Subtle};
    DividerAlignment labelAlignment_{DividerAlignment::Center};
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};
    bool hasCustomThickness_{false};
    bool hasCustomGap_{false};
    bool hasCustomLabelOffset_{false};
    float thickness_{1.0f};
    float gap_{8.0f};
    float labelOffset_{24.0f};
    float insetStart_{0.0f};
    float insetEnd_{0.0f};

    std::string label_{};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};

    std::optional<core::Color> overrideColor_{};
    std::optional<core::Color> overrideTextColor_{};

    mutable std::optional<text::TextLayout> cachedTextLayout_{};
    mutable bool isTextLayoutDirty_{true};
};

} // namespace northern::widgets
