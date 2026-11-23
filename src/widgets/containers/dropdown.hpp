#pragma once

#include "core/animation/property_animation.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace northern::widgets {

class DropdownPopup;

class Dropdown : public Widget {
public:
    explicit Dropdown(std::vector<std::string> items = {}, int selectedIndex = -1,
                      std::shared_ptr<text::Font> font = nullptr);
    ~Dropdown() override;

    core::Signal<int, const std::string&> onSelectionChanged;

    void setItems(std::vector<std::string> items);
    void addItem(std::string item);
    void clearItems();
    [[nodiscard]] const std::vector<std::string>& items() const noexcept { return items_; }
    [[nodiscard]] size_t itemCount() const noexcept { return items_.size(); }

    void setSelectedIndex(int index);
    [[nodiscard]] int selectedIndex() const noexcept { return selectedIndex_; }
    [[nodiscard]] std::string selectedText() const;

    void setPlaceholder(std::string placeholder);
    [[nodiscard]] const std::string& placeholder() const noexcept { return placeholder_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    void open();
    void close();
    void toggle();
    [[nodiscard]] bool isOpen() const noexcept { return isOpen_; }

    [[nodiscard]] int highlightedIndex() const noexcept { return highlightedIndex_; }
    void setPopupHighlightedIndex(int idx);
    void navigateHighlight(int delta);
    void confirmHighlight();

    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;
    void onFocusLost() override;

    void paint(renderer::PaintContext& ctx) override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;
    [[nodiscard]] float chevronAnimationProgress() const noexcept {
        return chevronAnim_.currentValue();
    }
    [[nodiscard]] float hoverAnimationProgress() const noexcept {
        return hoverAnim_.currentValue();
    }
    [[nodiscard]] float popupAnimationProgress() const noexcept;

    // Internal hook for popup selection callback
    void selectAndClose(int index);
    void onPopupDismissed();

    [[nodiscard]] Widget* activePopup() const noexcept { return activePopup_; }

private:
    void invalidateTextLayouts();

    std::vector<std::string> items_{};
    int selectedIndex_{-1};
    int highlightedIndex_{-1};
    std::string placeholder_{"Select an option..."};
    std::shared_ptr<text::Font> font_{nullptr};

    bool isOpen_{false};
    bool isHovered_{false};

    Widget* activePopup_{nullptr};

    core::animation::PropertyAnimation<float> chevronAnim_{0.0f};
    core::animation::PropertyAnimation<float> hoverAnim_{0.0f};

    mutable std::optional<text::TextLayout> cachedSelectedLayout_{};
    mutable std::optional<text::TextLayout> cachedPlaceholderLayout_{};
    mutable std::string cachedSelectedText_{};

    float borderWidth_{1.0f};
    float cornerRadii_[4]{theme::Metrics::CornerRadiusControl, theme::Metrics::CornerRadiusControl,
                          theme::Metrics::CornerRadiusControl, theme::Metrics::CornerRadiusControl};
};

} // namespace northern::widgets
