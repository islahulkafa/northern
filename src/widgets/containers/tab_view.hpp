#pragma once

#include "core/animation/property_animation.hpp"
#include "core/signal/signal.hpp"
#include "renderer/svg/svg_document.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/widget.hpp"
#include "widgets/controls/icons.hpp"

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace northern::widgets {

class Container;

enum class TabBarStyle : uint8_t { Underline, Pill, EditorCard };

struct TabItem {
    std::string title{};
    bool closable{false};
    bool enabled{true};
    bool modified{false};
    std::shared_ptr<renderer::SvgDocument> icon{nullptr};
    core::Rect tabRect{};
    core::Rect closeBtnRect{};
    mutable std::optional<text::TextLayout> cachedLayout{};
    mutable float cachedTextWidth{0.0f};
};

class TabBar : public Widget {
public:
    explicit TabBar(std::shared_ptr<text::Font> font = nullptr);
    ~TabBar() override = default;

    // Style & Sizing
    void setTabBarStyle(TabBarStyle style) noexcept;
    [[nodiscard]] TabBarStyle tabBarStyle() const noexcept { return style_; }

    void setMinTabWidth(float minW) noexcept;
    [[nodiscard]] float minTabWidth() const noexcept { return minTabWidth_; }

    void setMaxTabWidth(float maxW) noexcept;
    [[nodiscard]] float maxTabWidth() const noexcept { return maxTabWidth_; }

    void setTabHeight(float height) noexcept;
    [[nodiscard]] float tabHeight() const noexcept { return tabHeight_; }

    // Item Management
    int addTab(std::string title, bool closable = false, bool enabled = true);
    int addTab(std::string title, std::shared_ptr<renderer::SvgDocument> icon,
               bool closable = false, bool enabled = true);
    int addTab(std::string title, IconId iconId, bool closable = false, bool enabled = true);

    void insertTab(int index, std::string title, bool closable = false, bool enabled = true);
    void insertTab(int index, std::string title, std::shared_ptr<renderer::SvgDocument> icon,
                   bool closable = false, bool enabled = true);
    void insertTab(int index, std::string title, IconId iconId, bool closable = false,
                   bool enabled = true);

    void removeTab(int index);
    void clearTabs();
    void moveTab(int fromIndex, int toIndex);

    void setSelectedIndex(int index);
    [[nodiscard]] int selectedIndex() const noexcept { return selectedIndex_; }
    [[nodiscard]] size_t tabCount() const noexcept { return items_.size(); }
    [[nodiscard]] const std::vector<TabItem>& items() const noexcept { return items_; }
    [[nodiscard]] const TabItem* tab(size_t index) const noexcept {
        if (index < items_.size()) {
            return &items_[index];
        }
        return nullptr;
    }

    [[nodiscard]] const std::string& tabTitle(int index) const;
    void setTabTitle(int index, std::string title);

    void setTabIcon(int index, std::shared_ptr<renderer::SvgDocument> icon);
    void setTabIcon(int index, IconId iconId);

    void setTabModified(int index, bool modified);
    [[nodiscard]] bool isTabModified(int index) const noexcept;

    void setTabEnabled(int index, bool enabled);
    [[nodiscard]] bool isTabEnabled(int index) const noexcept;

    void setTabClosable(int index, bool closable);
    [[nodiscard]] bool isTabClosable(int index) const noexcept;

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    // Reordering & Trailing '+' Button
    void setReorderable(bool reorderable) noexcept { reorderable_ = reorderable; }
    [[nodiscard]] bool isReorderable() const noexcept { return reorderable_; }

    void setAddButtonVisible(bool visible) noexcept;
    [[nodiscard]] bool isAddButtonVisible() const noexcept { return showAddButton_; }
    [[nodiscard]] core::Rect addButtonRect() const noexcept;

    // Horizontal Overflow Scrolling
    void setTabScrollOffset(float offset) noexcept;
    [[nodiscard]] float tabScrollOffset() const noexcept { return scrollOffset_; }
    [[nodiscard]] float maxScrollOffset() const noexcept;
    [[nodiscard]] float totalTabsWidth() const noexcept { return totalTabsWidth_; }
    void ensureTabVisible(int index) noexcept;

    // Signals
    core::Signal<int> onTabSelected;
    core::Signal<int> onTabCloseRequested;
    core::Signal<int, int> onTabMoved;
    core::Signal<> onTabAddRequested;

    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onMouseWheel(WidgetMouseEvent& ev) override;
    void onMouseLeave() override;
    void onKeyDown(WidgetKeyEvent& ev) override;
    void onFocusGained() override;
    void onFocusLost() override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    void updateMeasure();
    void ensureTabsLayout() const;
    void startIndicatorAnimation(int targetIndex, bool animated);
    [[nodiscard]] float tabsViewportWidth() const noexcept;
    [[nodiscard]] int tabIndexAt(const core::Point& localPos) const noexcept;
    [[nodiscard]] bool isCloseBtnHit(int tabIndex, const core::Point& localPos) const noexcept;
    [[nodiscard]] int findNextEnabledTab(int startIndex, int delta) const noexcept;
    [[nodiscard]] int findFirstEnabledTab() const noexcept;
    [[nodiscard]] int findLastEnabledTab() const noexcept;

    mutable std::vector<TabItem> items_{};
    mutable bool isTabsLayoutDirty_{true};
    mutable float lastLayoutWidth_{-1.0f};
    mutable float totalTabsWidth_{0.0f};

    TabBarStyle style_{TabBarStyle::Underline};
    int selectedIndex_{-1};
    int hoveredIndex_{-1};
    int hoveredCloseIndex_{-1};
    bool hoveredAddBtn_{false};

    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    float dpiScale_{1.0f};
    float tabHeight_{32.0f};
    float tabPaddingX_{12.0f};
    float minTabWidth_{64.0f};
    float maxTabWidth_{220.0f};

    float scrollOffset_{0.0f};
    bool reorderable_{false};
    bool showAddButton_{false};

    // Drag-to-reorder state
    int pressTabIndex_{-1};
    bool isDraggingTab_{false};
    float dragStartX_{0.0f};
    float dragCurrentX_{0.0f};

    // Animations
    core::animation::PropertyAnimation<float> indicatorXAnim_{0.0f};
    core::animation::PropertyAnimation<float> indicatorWAnim_{0.0f};
    core::animation::PropertyAnimation<float> hoverAnim_{0.0f};
};

class TabView : public Widget {
public:
    explicit TabView(std::shared_ptr<text::Font> font = nullptr);
    ~TabView() override = default;

    int addTab(std::string title, std::unique_ptr<Widget> page, bool closable = false);
    int addTab(std::string title, IconId iconId, std::unique_ptr<Widget> page,
               bool closable = false);
    int addTab(std::string title, std::shared_ptr<renderer::SvgDocument> icon,
               std::unique_ptr<Widget> page, bool closable = false);

    void insertTab(int index, std::string title, std::unique_ptr<Widget> page,
                   bool closable = false);
    void removeTab(int index);
    void moveTab(int fromIndex, int toIndex);

    void setCurrentIndex(int index);
    [[nodiscard]] int currentIndex() const noexcept;
    [[nodiscard]] size_t tabCount() const noexcept;
    [[nodiscard]] Widget* pageAt(int index) const;

    void setTabModified(int index, bool modified);
    void setTabEnabled(int index, bool enabled);
    void setTabBarStyle(TabBarStyle style) noexcept;
    void setReorderable(bool reorderable) noexcept;
    void setAddButtonVisible(bool visible) noexcept;
    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    [[nodiscard]] TabBar* tabBar() noexcept { return tabBar_; }
    [[nodiscard]] const TabBar* tabBar() const noexcept { return tabBar_; }

    core::Signal<int> onTabChanged;
    core::Signal<int> onTabClosed;
    core::Signal<int, int> onTabMoved;
    core::Signal<> onTabAddRequested;

private:
    void updatePageVisibility();

    TabBar* tabBar_{nullptr};
    Container* deck_{nullptr};
    std::vector<Widget*> pages_{};
    float dpiScale_{1.0f};
};

} // namespace northern::widgets
