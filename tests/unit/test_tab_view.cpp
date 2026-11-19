#include "widgets/container.hpp"
#include "widgets/tab_view.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(TabBarTest, AddRemoveAndSelect) {
    TabBar bar;
    EXPECT_EQ(bar.tabCount(), 0u);
    EXPECT_EQ(bar.selectedIndex(), -1);

    int selectedFromSignal = -1;
    bar.onTabSelected.connect([&](int idx) { selectedFromSignal = idx; });

    int idx0 = bar.addTab("General");
    EXPECT_EQ(idx0, 0);
    EXPECT_EQ(bar.tabCount(), 1u);
    EXPECT_EQ(bar.selectedIndex(), 0);

    int idx1 = bar.addTab("Settings", true);
    EXPECT_EQ(idx1, 1);
    EXPECT_EQ(bar.tabCount(), 2u);

    bar.setSelectedIndex(1);
    EXPECT_EQ(bar.selectedIndex(), 1);
    EXPECT_EQ(selectedFromSignal, 1);

    // Remove tab 0
    bar.removeTab(0);
    EXPECT_EQ(bar.tabCount(), 1u);
    EXPECT_EQ(bar.selectedIndex(), 0);
    ASSERT_NE(bar.tab(0), nullptr);
    EXPECT_EQ(bar.tab(0)->title, "Settings");
    EXPECT_TRUE(bar.tab(0)->closable);
}

TEST(TabBarTest, KeyboardArrowNavigation) {
    TabBar bar;
    bar.addTab("Tab 1");
    bar.addTab("Tab 2");
    bar.addTab("Tab 3");
    bar.setSelectedIndex(0);

    // Press Right arrow
    WidgetKeyEvent rightEv{platform::Key::Right, true, platform::KeyModifier::None};
    bar.onKeyDown(rightEv);
    EXPECT_TRUE(rightEv.isAccepted());
    EXPECT_EQ(bar.selectedIndex(), 1);

    // Press Right arrow again
    WidgetKeyEvent rightEv2{platform::Key::Right, true, platform::KeyModifier::None};
    bar.onKeyDown(rightEv2);
    EXPECT_EQ(bar.selectedIndex(), 2);

    // Wrap around to 0
    WidgetKeyEvent rightEv3{platform::Key::Right, true, platform::KeyModifier::None};
    bar.onKeyDown(rightEv3);
    EXPECT_EQ(bar.selectedIndex(), 0);

    // Press Left arrow wrap around to 2
    WidgetKeyEvent leftEv{platform::Key::Left, true, platform::KeyModifier::None};
    bar.onKeyDown(leftEv);
    EXPECT_EQ(bar.selectedIndex(), 2);
}

TEST(TabViewTest, AddPagesAndSwitchVisibility) {
    TabView tv;
    EXPECT_EQ(tv.tabCount(), 0u);
    EXPECT_EQ(tv.currentIndex(), -1);

    int changedSignal = -1;
    tv.onTabChanged.connect([&](int idx) { changedSignal = idx; });

    auto page1 = std::make_unique<Container>();
    auto page2 = std::make_unique<Container>();
    Widget* p1 = page1.get();
    Widget* p2 = page2.get();

    tv.addTab("First", std::move(page1));
    EXPECT_EQ(tv.tabCount(), 1u);
    EXPECT_EQ(tv.currentIndex(), 0);
    EXPECT_TRUE(p1->isVisible());

    tv.addTab("Second", std::move(page2));
    EXPECT_EQ(tv.tabCount(), 2u);
    EXPECT_EQ(tv.currentIndex(), 0);
    EXPECT_TRUE(p1->isVisible());
    EXPECT_FALSE(p2->isVisible());

    tv.setCurrentIndex(1);
    EXPECT_EQ(tv.currentIndex(), 1);
    EXPECT_EQ(changedSignal, 1);
    EXPECT_FALSE(p1->isVisible());
    EXPECT_TRUE(p2->isVisible());

    EXPECT_EQ(tv.pageAt(0), p1);
    EXPECT_EQ(tv.pageAt(1), p2);

    // Remove first tab
    tv.removeTab(0);
    EXPECT_EQ(tv.tabCount(), 1u);
    EXPECT_EQ(tv.currentIndex(), 0);
    EXPECT_TRUE(p2->isVisible());
}

TEST(TabViewTest, CloseTabSignal) {
    TabView tv;
    int closedIndex = -1;
    tv.onTabClosed.connect([&](int idx) { closedIndex = idx; });

    auto page1 = std::make_unique<Container>();
    auto page2 = std::make_unique<Container>();

    tv.addTab("Tab 1", std::move(page1), true);
    tv.addTab("Tab 2", std::move(page2), true);

    tv.tabBar()->onTabCloseRequested.emit(0);
    EXPECT_EQ(closedIndex, 0);
    EXPECT_EQ(tv.tabCount(), 1u);
}
