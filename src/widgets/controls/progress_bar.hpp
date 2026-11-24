#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget.hpp"

#include <cstdint>
#include <memory>
#include <optional>

namespace northern::widgets {

enum class ProgressRole : uint8_t {
    Accent,
    Success,
    Warning,
    Danger,
    Info,
};

enum class ProgressSize : uint8_t {
    Thin,
    Standard,
    Thick,
};

[[nodiscard]] constexpr float toPoints(ProgressSize size) noexcept {
    switch (size) {
    case ProgressSize::Thin:
        return 4.0f;
    case ProgressSize::Standard:
        return 6.0f;
    case ProgressSize::Thick:
        return 8.0f;
    }
    return 4.0f;
}

class ProgressBar : public Widget {
public:
    explicit ProgressBar(float progress = 0.0f, bool isIndeterminate = false);
    ProgressBar(ProgressSize size, ProgressRole role = ProgressRole::Accent, float progress = 0.0f,
                bool isIndeterminate = false);
    ~ProgressBar() override = default;

    void setProgress(float progress);
    [[nodiscard]] float progress() const noexcept { return progress_; }

    void setRange(float minimum, float maximum);
    void setValue(float value);
    [[nodiscard]] float minimum() const noexcept { return minimum_; }
    [[nodiscard]] float maximum() const noexcept { return maximum_; }
    [[nodiscard]] float value() const noexcept;

    void setIndeterminate(bool isIndeterminate);
    [[nodiscard]] bool isIndeterminate() const noexcept { return isIndeterminate_; }

    void setSize(ProgressSize size) noexcept;
    void setBarHeight(float height) noexcept;
    [[nodiscard]] float barHeight() const noexcept { return barHeight_; }

    void setRole(ProgressRole role) noexcept;
    [[nodiscard]] ProgressRole role() const noexcept { return role_; }

    void setShowValue(bool show);
    [[nodiscard]] bool showValue() const noexcept { return showValue_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void setTrackColor(const core::Color& color) noexcept;
    void setFillColor(const core::Color& color) noexcept;
    void setTrackBorderColor(const core::Color& color) noexcept;
    void clearColorOverrides() noexcept;

    [[nodiscard]] core::Color effectiveTrackColor() const noexcept;
    [[nodiscard]] core::Color effectiveFillColor() const noexcept;
    [[nodiscard]] core::Color effectiveBorderColor() const noexcept;

    void advanceAnimation(float deltaSeconds) noexcept;
    [[nodiscard]] float indeterminateOffset() const noexcept { return indeterminateOffset_; }

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

    core::Signal<float> onProgressChanged;
    core::Signal<> onCompleted;

private:
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;
    void updateReservedLabelWidth() const;
    void ensureValueLayout(int percent) const;

    float progress_{0.0f};
    float minimum_{0.0f};
    float maximum_{1.0f};
    bool isIndeterminate_{false};
    float barHeight_{4.0f};
    float indeterminateOffset_{0.0f};

    ProgressRole role_{ProgressRole::Accent};
    bool showValue_{false};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    float dpiScale_{1.0f};

    core::animation::PropertyAnimation<float> progressAnim_{0.0f};

    std::optional<core::Color> overrideTrackColor_{std::nullopt};
    std::optional<core::Color> overrideFillColor_{std::nullopt};
    std::optional<core::Color> overrideBorderColor_{std::nullopt};

    mutable std::optional<text::TextLayout> cachedValueLayout_{std::nullopt};
    mutable int cachedPercent_{-1};
    mutable float reservedLabelWidth_{0.0f};
    mutable float reservedLabelHeight_{0.0f};
};

} // namespace northern::widgets
