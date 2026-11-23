#pragma once

#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/base/widget.hpp"

#include <memory>
#include <string>

namespace northern::widgets {

class TextInput : public Widget {
public:
    explicit TextInput(std::string placeholder = "", std::shared_ptr<text::Font> font = nullptr);
    ~TextInput() override = default;

    core::Signal<const std::string&> onTextChanged;
    core::Signal<const std::string&> onEnterPressed;

    void setText(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    void setPlaceholder(std::string placeholder);
    [[nodiscard]] const std::string& placeholder() const noexcept { return placeholder_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

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
    void deleteSelection();
    void insertText(std::string_view str);
    size_t hitTestCharacter(float localX) const;
    void ensureCursorVisible();

    std::string text_{};
    std::string placeholder_{};
    std::shared_ptr<text::Font> font_{nullptr};

    size_t cursorIndex_{0};
    size_t selectionAnchor_{0};
    float scrollOffset_{0.0f};
    bool isDraggingSelection_{false};

    float cursorBlinkTimer_{0.0f};
    bool cursorVisible_{true};

    core::Color bgColor_{core::Color{0.122f, 0.122f, 0.137f, 1.0f}};
    core::Color idleBorderColor_{core::Color{1.0f, 1.0f, 1.0f, 0.12f}};
    core::Color focusedBorderColor_{core::Color{0.145f, 0.388f, 0.922f, 1.0f}};
    core::Color textColor_{core::Color{0.980f, 0.980f, 0.980f, 1.0f}};
    core::Color placeholderColor_{core::Color{0.443f, 0.443f, 0.478f, 1.0f}};
    core::Color selectionColor_{core::Color{0.145f, 0.388f, 0.922f, 0.35f}};
    core::Color cursorColor_{core::Color{0.980f, 0.980f, 0.980f, 1.0f}};

    float borderWidth_{1.0f};
    float cornerRadii_[4]{6.0f, 6.0f, 6.0f, 6.0f};
};

} // namespace northern::widgets
