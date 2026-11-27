#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace northern::widgets {

class TextArea : public Widget {
public:
    explicit TextArea(std::string placeholder = "", std::shared_ptr<text::Font> font = nullptr);
    ~TextArea() override = default;

    core::Signal<const std::string&> onTextChanged;
    core::Signal<> onSelectionChanged;
    core::Signal<int, int> onCursorPositionChanged;

    void setText(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    void setPlaceholder(std::string placeholder);
    [[nodiscard]] const std::string& placeholder() const noexcept { return placeholder_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void setReadOnly(bool readOnly) noexcept;
    [[nodiscard]] bool isReadOnly() const noexcept { return readOnly_; }

    void setMaxLength(size_t maxChars);
    [[nodiscard]] size_t maxLength() const noexcept { return maxLength_; }

    void setAcceptsTab(bool accepts) noexcept { acceptsTab_ = accepts; }
    [[nodiscard]] bool acceptsTab() const noexcept { return acceptsTab_; }

    void setTabWidth(int spaces) noexcept;
    [[nodiscard]] int tabWidth() const noexcept { return tabWidth_; }

    void setWordWrap(text::WordWrap wrap);
    [[nodiscard]] text::WordWrap wordWrap() const noexcept { return wordWrap_; }

    void setLineHeightMultiplier(float multiplier);
    [[nodiscard]] float lineHeightMultiplier() const noexcept { return lineHeightMultiplier_; }

    void setShowLineNumbers(bool show);
    [[nodiscard]] bool showLineNumbers() const noexcept { return showLineNumbers_; }

    [[nodiscard]] size_t cursorIndex() const noexcept { return cursorIndex_; }
    [[nodiscard]] size_t selectionAnchor() const noexcept { return selectionAnchor_; }
    [[nodiscard]] bool hasSelection() const noexcept { return cursorIndex_ != selectionAnchor_; }
    [[nodiscard]] std::string selectedText() const;
    [[nodiscard]] int cursorLine() const noexcept;
    [[nodiscard]] int cursorColumn() const noexcept;
    [[nodiscard]] int lineCount() const noexcept;

    void selectAll();
    void setCursorIndex(size_t index, bool keepSelection = false);

    void undo();
    void redo();
    [[nodiscard]] bool canUndo() const noexcept { return !undoStack_.empty(); }
    [[nodiscard]] bool canRedo() const noexcept { return !redoStack_.empty(); }
    void clearUndoHistory() noexcept;

    [[nodiscard]] float scrollOffsetX() const noexcept { return scrollOffsetX_; }
    [[nodiscard]] float scrollOffsetY() const noexcept { return scrollOffsetY_; }
    [[nodiscard]] core::Point scrollOffset() const noexcept override {
        return core::Point{scrollOffsetX_, scrollOffsetY_};
    }
    [[nodiscard]] float maxScrollOffsetX() const noexcept;
    [[nodiscard]] float maxScrollOffsetY() const noexcept;
    void scrollBy(float deltaX, float deltaY);
    void ensureCursorVisible();

    void setBackgroundColor(const core::Color& color) noexcept;
    void setIdleBorderColor(const core::Color& color) noexcept;
    void setFocusedBorderColor(const core::Color& color) noexcept;
    void setTextColor(const core::Color& color) noexcept;
    void setPlaceholderColor(const core::Color& color) noexcept;
    void setSelectionColor(const core::Color& color) noexcept;
    void setCursorColor(const core::Color& color) noexcept;
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
    void onMouseWheel(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;
    void onFocusGained() override;
    void onFocusLost() override;
    void onContextAttached() override;
    void onThemeChanged() override;
    void onEnabledChanged() override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    struct VisualLine {
        size_t startByte{0};
        size_t endByte{0};
        int logicalLine{0};
        bool isFirstInParagraph{false};
        float y{0.0f};
        float width{0.0f};
        std::optional<text::TextLayout> layout{std::nullopt};
        std::vector<float> clusterPenX{};
    };

    struct HistoryEntry {
        std::string text{};
        size_t cursorIndex{0};
        size_t selectionAnchor{0};
    };

    enum class EditKind : uint8_t { None, InsertChar, Delete };

    void applyDpiScale(float scale);
    void cancelActiveInteraction();
    void invalidateLineCache() noexcept;
    void ensureLineCache() const;
    void ensurePlaceholderCache() const;
    [[nodiscard]] float effectiveLineHeight() const;
    [[nodiscard]] float gutterWidth() const;
    [[nodiscard]] core::Rect textViewportLocalRect() const;
    [[nodiscard]] core::Rect verticalThumbRect() const;
    [[nodiscard]] core::Rect horizontalThumbRect() const;
    [[nodiscard]] size_t visualLineForByte(size_t byteIndex) const;
    [[nodiscard]] float xOffsetInVisualLine(const VisualLine& vl, size_t byteIndex) const;
    [[nodiscard]] size_t hitTestPosition(core::Point localPos) const;
    void clampScrollOffsets() noexcept;

    void pushUndoSnapshot(EditKind kind);
    void applyEdit(std::string newText, size_t newCursor, size_t newAnchor, EditKind kind);
    void deleteSelection();
    void insertText(std::string_view str, EditKind kind);
    void notifyCursorAndSelection(size_t prevCursor, size_t prevAnchor);
    void resetBlink() noexcept;

    std::string text_{};
    std::string placeholder_{};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};

    bool readOnly_{false};
    size_t maxLength_{0};
    bool acceptsTab_{false};
    int tabWidth_{4};
    text::WordWrap wordWrap_{text::WordWrap::Word};
    float lineHeightMultiplier_{1.08f};
    bool showLineNumbers_{false};

    size_t cursorIndex_{0};
    size_t selectionAnchor_{0};
    float desiredCaretX_{-1.0f};
    float scrollOffsetX_{0.0f};
    float scrollOffsetY_{0.0f};
    float scrollbarThickness_{6.0f};

    bool isHovered_{false};
    bool isDraggingSelection_{false};
    bool isHoveringVThumb_{false};
    bool isDraggingVThumb_{false};
    bool isHoveringHThumb_{false};
    bool isDraggingHThumb_{false};
    float dragStartMouseY_{0.0f};
    float dragStartScrollY_{0.0f};
    float dragStartMouseX_{0.0f};
    float dragStartScrollX_{0.0f};

    int clickCount_{0};
    std::chrono::steady_clock::time_point lastClickTime_{};
    core::Point lastClickPos_{};

    float cursorBlinkTimer_{0.0f};
    bool cursorVisible_{true};
    core::animation::PropertyAnimation<float> hoverAnim_{0.0f};
    core::animation::PropertyAnimation<float> focusAnim_{0.0f};

    std::vector<HistoryEntry> undoStack_{};
    std::vector<HistoryEntry> redoStack_{};
    EditKind lastEditKind_{EditKind::None};
    std::chrono::steady_clock::time_point lastEditTime_{};

    std::optional<core::Color> overrideBgColor_{std::nullopt};
    std::optional<core::Color> overrideIdleBorderColor_{std::nullopt};
    std::optional<core::Color> overrideFocusedBorderColor_{std::nullopt};
    std::optional<core::Color> overrideTextColor_{std::nullopt};
    std::optional<core::Color> overridePlaceholderColor_{std::nullopt};
    std::optional<core::Color> overrideSelectionColor_{std::nullopt};
    std::optional<core::Color> overrideCursorColor_{std::nullopt};
    float cornerRadius_{theme::Metrics::CornerRadiusControl};

    mutable std::vector<VisualLine> visualLines_{};
    mutable float contentMaxWidth_{0.0f};
    mutable float contentHeight_{0.0f};
    mutable int logicalLineCount_{1};
    mutable float cachedWrapWidth_{-1.0f};
    mutable bool lineCacheValid_{false};

    mutable std::optional<text::TextLayout> cachedPlaceholderLayout_{std::nullopt};
    mutable bool placeholderCacheValid_{false};
};

} // namespace northern::widgets
