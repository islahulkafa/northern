#pragma once

#include "core/animation/property_animation.hpp"
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
#include <vector>

namespace northern::renderer {
class SvgDocument;
}

namespace northern::widgets {

class DropdownPopup;

enum class DropdownSize : uint8_t {
    Compact,
    Standard,
    Comfortable,
};

struct DropdownItem {
    std::string text{};
    std::shared_ptr<renderer::SvgDocument> icon{nullptr};
    bool isSeparator{false};
    bool isEnabled{true};

    static DropdownItem separator() {
        DropdownItem it{};
        it.isSeparator = true;
        it.isEnabled = false;
        return it;
    }
};

class Dropdown : public Widget {
public:
    explicit Dropdown(std::vector<std::string> items = {}, int selectedIndex = -1,
                      std::shared_ptr<text::Font> font = nullptr,
                      DropdownSize size = DropdownSize::Standard);
    Dropdown(std::initializer_list<std::string> items, int selectedIndex = -1,
             std::shared_ptr<text::Font> font = nullptr,
             DropdownSize size = DropdownSize::Standard);
    explicit Dropdown(std::vector<DropdownItem> items, int selectedIndex = -1,
                      std::shared_ptr<text::Font> font = nullptr,
                      DropdownSize size = DropdownSize::Standard);
    ~Dropdown() override;

    core::Signal<int, const std::string&> onSelectionChanged;

    void setItems(std::vector<std::string> items);
    void setItems(std::vector<DropdownItem> items);
    void addItem(std::string item);
    void addItem(DropdownItem item);
    void addSeparator();
    void clearItems();

    [[nodiscard]] const std::vector<DropdownItem>& items() const noexcept { return items_; }
    [[nodiscard]] size_t itemCount() const noexcept { return items_.size(); }
    [[nodiscard]] const DropdownItem& itemAt(size_t index) const;
    [[nodiscard]] std::string itemText(size_t index) const;

    void setItemEnabled(int index, bool enabled);
    [[nodiscard]] bool isItemEnabled(int index) const noexcept;
    [[nodiscard]] bool isSeparator(size_t index) const noexcept;

    void setSelectedIndex(int index);
    [[nodiscard]] int selectedIndex() const noexcept { return selectedIndex_; }
    [[nodiscard]] std::string selectedText() const;

    void setPlaceholder(std::string placeholder);
    [[nodiscard]] const std::string& placeholder() const noexcept { return placeholder_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setSize(DropdownSize size) noexcept;
    [[nodiscard]] DropdownSize size() const noexcept { return size_; }
    [[nodiscard]] float preferredHeight() const noexcept;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void setMaxVisibleItems(size_t maxItems) noexcept;
    [[nodiscard]] size_t maxVisibleItems() const noexcept { return maxVisibleItems_; }

    void setBackgroundColor(const core::Color& color) noexcept;
    void setBorderColor(const core::Color& color) noexcept;
    void setTextColor(const core::Color& color) noexcept;
    void setPlaceholderColor(const core::Color& color) noexcept;
    void clearColorOverrides() noexcept;

    [[nodiscard]] core::Color effectiveBackgroundColor() const noexcept;
    [[nodiscard]] core::Color effectiveBorderColor() const noexcept;
    [[nodiscard]] core::Color effectiveTextColor() const noexcept;
    [[nodiscard]] core::Color effectivePlaceholderColor() const noexcept;

    void open();
    void close();
    void toggle();
    [[nodiscard]] bool isOpen() const noexcept { return isOpen_; }

    void clearTypeAhead() noexcept;

    [[nodiscard]] int highlightedIndex() const noexcept { return highlightedIndex_; }
    void setPopupHighlightedIndex(int idx);
    void navigateHighlight(int delta);
    void confirmHighlight();

    void onContextAttached() override;
    void onThemeChanged() override;
    void onEnabledChanged() override;
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
    void applyDpiMetrics(float scale);
    void invalidateTextLayouts();
    void updateDimensions() noexcept;
    [[nodiscard]] int findNextSelectable(int start, int delta) const noexcept;
    void handleTypeAhead(char c);

    std::vector<DropdownItem> items_{};
    int selectedIndex_{-1};
    int highlightedIndex_{-1};
    std::string placeholder_{"Select an option..."};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};

    DropdownSize size_{DropdownSize::Standard};
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};
    size_t maxVisibleItems_{6};

    bool isOpen_{false};
    bool isHovered_{false};

    Widget* activePopup_{nullptr};

    core::animation::PropertyAnimation<float> chevronAnim_{0.0f};
    core::animation::PropertyAnimation<float> hoverAnim_{0.0f};

    mutable std::optional<text::TextLayout> cachedSelectedLayout_{};
    mutable std::optional<text::TextLayout> cachedPlaceholderLayout_{};
    mutable std::string cachedSelectedText_{};

    std::string typeAheadBuffer_{};
    std::chrono::steady_clock::time_point lastTypeAheadTime_{};

    std::optional<core::Color> overrideBgColor_{std::nullopt};
    std::optional<core::Color> overrideBorderColor_{std::nullopt};
    std::optional<core::Color> overrideTextColor_{std::nullopt};
    std::optional<core::Color> overridePlaceholderColor_{std::nullopt};

    float borderWidth_{1.0f};
    float cornerRadii_[4]{theme::Metrics::CornerRadiusControl, theme::Metrics::CornerRadiusControl,
                          theme::Metrics::CornerRadiusControl, theme::Metrics::CornerRadiusControl};
};

} // namespace northern::widgets
