#pragma once

#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace northern::widgets {

enum class TextInputSize : uint8_t {
    Compact,
    Standard,
    Comfortable,
};

class TextInput : public Widget {
public:
    explicit TextInput(std::string placeholder = "", std::shared_ptr<text::Font> font = nullptr);
    TextInput(std::string placeholder, TextInputSize size);
    ~TextInput() override = default;

    core::Signal<const std::string&> onTextChanged;
    core::Signal<const std::string&> onEnterPressed;

    void setText(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    void setPlaceholder(std::string placeholder);
    [[nodiscard]] const std::string& placeholder() const noexcept { return placeholder_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setSize(TextInputSize size) noexcept;
    [[nodiscard]] TextInputSize size() const noexcept { return size_; }
    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void setReadOnly(bool readOnly) noexcept;
    [[nodiscard]] bool isReadOnly() const noexcept { return readOnly_; }

    void setPasswordMode(bool enabled);
    [[nodiscard]] bool isPasswordMode() const noexcept { return passwordMode_; }

    void setMaxLength(size_t maxChars);
    [[nodiscard]] size_t maxLength() const noexcept { return maxLength_; }

    [[nodiscard]] size_t cursorIndex() const noexcept { return cursorIndex_; }
    [[nodiscard]] size_t selectionAnchor() const noexcept { return selectionAnchor_; }
    [[nodiscard]] bool hasSelection() const noexcept { return cursorIndex_ != selectionAnchor_; }
    [[nodiscard]] std::string selectedText() const;

    void selectAll();
    void setCursorIndex(size_t index, bool keepSelection = false);

    void setBackgroundColor(const core::Color& color) noexcept;
    void setIdleBorderColor(const core::Color& color) noexcept;
    void setFocusedBorderColor(const core::Color& color) noexcept;
    void setTextColor(const core::Color& color) noexcept;
    void setPlaceholderColor(const core::Color& color) noexcept;
    void setSelectionColor(const core::Color& color) noexcept;
    void setCursorColor(const core::Color& color) noexcept;
    void setBorderWidth(float width) noexcept;
    void setCornerRadius(float radius) noexcept;
    void clearColorOverrides() noexcept;

    [[nodiscard]] core::Color effectiveBackgroundColor() const noexcept;
    [[nodiscard]] core::Color effectiveBorderColor() const noexcept;
    [[nodiscard]] core::Color effectiveTextColor() const noexcept;
    [[nodiscard]] core::Color effectivePlaceholderColor() const noexcept;
    [[nodiscard]] core::Color effectiveSelectionColor() const noexcept;
    [[nodiscard]] core::Color effectiveCursorColor() const noexcept;

    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;
    void onFocusGained() override;
    void onFocusLost() override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    void invalidateTextCache() noexcept;
    void ensureTextCache() const;
    void ensurePlaceholderCache() const;
    [[nodiscard]] float measureOffsetAt(size_t byteIndex) const;
    void deleteSelection();
    void insertText(std::string_view str);
    size_t hitTestCharacter(float localX) const;
    void ensureCursorVisible();

    std::string text_{};
    std::string placeholder_{};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    TextInputSize size_{TextInputSize::Standard};
    float dpiScale_{1.0f};

    bool readOnly_{false};
    bool passwordMode_{false};
    size_t maxLength_{0}; // 0 = unlimited UTF-8 codepoints

    size_t cursorIndex_{0};
    size_t selectionAnchor_{0};
    float scrollOffset_{0.0f};
    bool isHovered_{false};
    bool isDraggingSelection_{false};

    float cursorBlinkTimer_{0.0f};
    bool cursorVisible_{true};

    std::optional<core::Color> overrideBgColor_{std::nullopt};
    std::optional<core::Color> overrideIdleBorderColor_{std::nullopt};
    std::optional<core::Color> overrideFocusedBorderColor_{std::nullopt};
    std::optional<core::Color> overrideTextColor_{std::nullopt};
    std::optional<core::Color> overridePlaceholderColor_{std::nullopt};
    std::optional<core::Color> overrideSelectionColor_{std::nullopt};
    std::optional<core::Color> overrideCursorColor_{std::nullopt};

    float borderWidth_{1.0f};
    float cornerRadii_[4]{theme::Metrics::CornerRadiusControl, theme::Metrics::CornerRadiusControl,
                          theme::Metrics::CornerRadiusControl, theme::Metrics::CornerRadiusControl};

    mutable std::optional<text::TextLayout> cachedTextLayout_{std::nullopt};
    mutable std::vector<float> clusterPenX_{};
    mutable bool textCacheValid_{false};

    mutable std::optional<text::TextLayout> cachedPlaceholderLayout_{std::nullopt};
    mutable bool placeholderCacheValid_{false};
};

} // namespace northern::widgets
