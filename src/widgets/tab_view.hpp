#pragma once

#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/widget.hpp"

#include <memory>
#include <string>
#include <vector>

namespace northern::widgets {

class Container;

struct TabItem {
    std::string title{};
    bool closable{false};
    bool enabled{true};
    core::Rect tabRect{};
    core::Rect closeBtnRect{};
};

class TabBar : public Widget {
public:
    explicit TabBar(std::shared_ptr<text::Font> font = nullptr);
    ~TabBar() override = default;

    int addTab(std::string title, bool closable = false, bool enabled = true);
    void insertTab(int index, std::string title, bool closable = false, bool enabled = true);
    void removeTab(int index);
    void clearTabs();

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

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    core::Signal<int> onTabSelected;
    core::Signal<int> onTabCloseRequested;

    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseLeave() override;
    void onKeyDown(WidgetKeyEvent& ev) override;

    void paint(renderer::PaintContext& ctx) override;

private:
    void updateMeasure();
    void layoutTabs(float totalWidth);
    int tabIndexAt(const core::Point& localPos) const noexcept;
    bool isCloseBtnHit(int tabIndex, const core::Point& localPos) const noexcept;

    std::vector<TabItem> items_{};
    int selectedIndex_{-1};
    int hoveredIndex_{-1};
    int hoveredCloseIndex_{-1};
    std::shared_ptr<text::Font> font_{nullptr};
    float tabHeight_{32.0f};
    float tabPaddingX_{14.0f};
};

class TabView : public Widget {
public:
    explicit TabView(std::shared_ptr<text::Font> font = nullptr);
    ~TabView() override = default;

    int addTab(std::string title, std::unique_ptr<Widget> page, bool closable = false);
    void removeTab(int index);
    void setCurrentIndex(int index);
    [[nodiscard]] int currentIndex() const noexcept;
    [[nodiscard]] size_t tabCount() const noexcept;
    [[nodiscard]] Widget* pageAt(int index) const;

    [[nodiscard]] TabBar* tabBar() noexcept { return tabBar_; }
    [[nodiscard]] const TabBar* tabBar() const noexcept { return tabBar_; }

    core::Signal<int> onTabChanged;
    core::Signal<int> onTabClosed;

private:
    void updatePageVisibility();

    TabBar* tabBar_{nullptr};
    Container* deck_{nullptr};
    std::vector<Widget*> pages_{};
};

} // namespace northern::widgets
