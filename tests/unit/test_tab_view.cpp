#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/containers/tab_view.hpp"
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

TEST(TabBarTest, VisualStylesAndSlidingIndicatorAnimation) {
    WidgetContext ctx;
    auto barUnique = std::make_unique<TabBar>();
    TabBar* bar = barUnique.get();
    bar->addTab("Overview");
    bar->addTab("Editor");
    bar->addTab("Terminal");

    ctx.setRootWidget(std::move(barUnique));
    ctx.handleResize(core::Size{500.0f, 32.0f});

    bar->setSelectedIndex(2);
    EXPECT_TRUE(bar->hasActiveAnimations());
    bar->updateAnimations(0.08f);
    EXPECT_TRUE(bar->hasActiveAnimations());
    bar->updateAnimations(0.10f);
    EXPECT_FALSE(bar->hasActiveAnimations());

    // Render all 3 styles
    for (auto s : {TabBarStyle::Underline, TabBarStyle::Pill, TabBarStyle::EditorCard}) {
        bar->setTabBarStyle(s);
        EXPECT_EQ(bar->tabBarStyle(), s);
        renderer::PaintContext paintCtx;
        bar->paint(paintCtx);
        EXPECT_FALSE(paintCtx.commands().empty());
    }
}

TEST(TabBarTest, IconsModifiedDotAndMiddleClickClose) {
    TabBar bar;
    bar.addTab("main.cpp", IconId::File, true);
    bar.addTab("config.json", IconId::Settings, false);

    ASSERT_NE(bar.tab(0), nullptr);
    EXPECT_NE(bar.tab(0)->icon, nullptr);
    EXPECT_FALSE(bar.isTabModified(0));

    bar.setTabModified(0, true);
    EXPECT_TRUE(bar.isTabModified(0));

    bar.layoutNode().setLayoutRect(core::Rect{core::Point{0.0f, 0.0f}, core::Size{400.0f, 32.0f}});
    renderer::PaintContext paintCtx;
    bar.paint(paintCtx);
    EXPECT_FALSE(paintCtx.commands().empty());

    // Middle-click on closable tab 0 emits close request
    int closeReqIdx = -1;
    bar.onTabCloseRequested.connect([&](int idx) { closeReqIdx = idx; });

    core::Point tab0Center{bar.tab(0)->tabRect.origin.x + 15.0f, 16.0f};
    WidgetMouseEvent midEv{tab0Center, platform::MouseButton::Middle, platform::KeyModifier::None};
    bar.onMouseDown(midEv);
    EXPECT_TRUE(midEv.isAccepted());
    EXPECT_EQ(closeReqIdx, 0);

    // Middle-click on non-closable tab 1 does not emit
    closeReqIdx = -1;
    core::Point tab1Center{bar.tab(1)->tabRect.origin.x + 15.0f, 16.0f};
    WidgetMouseEvent midEv1{tab1Center, platform::MouseButton::Middle, platform::KeyModifier::None};
    bar.onMouseDown(midEv1);
    EXPECT_FALSE(midEv1.isAccepted());
    EXPECT_EQ(closeReqIdx, -1);
}

TEST(TabBarTest, DragToReorderAndTabViewPageSync) {
    WidgetContext ctx;
    auto tvUnique = std::make_unique<TabView>();
    TabView* tv = tvUnique.get();
    tv->setReorderable(true);

    auto p0 = std::make_unique<Container>();
    auto p1 = std::make_unique<Container>();
    auto p2 = std::make_unique<Container>();
    Widget* p0Ptr = p0.get();
    Widget* p1Ptr = p1.get();
    Widget* p2Ptr = p2.get();

    tv->addTab("First", std::move(p0));
    tv->addTab("Second", std::move(p1));
    tv->addTab("Third", std::move(p2));

    int movedFrom = -1;
    int movedTo = -1;
    tv->onTabMoved.connect([&](int from, int to) {
        movedFrom = from;
        movedTo = to;
    });

    ctx.setRootWidget(std::move(tvUnique));
    ctx.handleResize(core::Size{500.0f, 300.0f});

    // Programmatic moveTab
    tv->moveTab(0, 2);
    EXPECT_EQ(movedFrom, 0);
    EXPECT_EQ(movedTo, 2);
    EXPECT_EQ(tv->tabBar()->tabTitle(0), "Second");
    EXPECT_EQ(tv->tabBar()->tabTitle(2), "First");
    EXPECT_EQ(tv->pageAt(0), p1Ptr);
    EXPECT_EQ(tv->pageAt(2), p0Ptr);
    EXPECT_EQ(tv->currentIndex(), 2);

    // Interactive drag reorder
    TabBar* bar = tv->tabBar();
    core::Rect tab0Rect = bar->tab(0)->tabRect;
    core::Rect tab1Rect = bar->tab(1)->tabRect;

    WidgetMouseEvent downEv{core::Point{tab0Rect.origin.x + 10.0f, 16.0f},
                            platform::MouseButton::Left, platform::KeyModifier::None};
    bar->onMouseDown(downEv);

    float targetX = tab1Rect.origin.x + tab1Rect.size.width * 0.75f;
    WidgetMouseEvent dragEv{core::Point{targetX, 16.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    bar->onMouseMove(dragEv);
    EXPECT_TRUE(dragEv.isAccepted());
    EXPECT_EQ(bar->tabTitle(0), "Third");
    EXPECT_EQ(bar->tabTitle(1), "Second");
    EXPECT_EQ(tv->pageAt(0), p2Ptr);

    WidgetMouseEvent upEv{core::Point{targetX, 16.0f}, platform::MouseButton::Left,
                          platform::KeyModifier::None};
    bar->onMouseUp(upEv);
}

TEST(TabBarTest, TrailingAddButtonAndOverflowScrolling) {
    WidgetContext ctx;
    auto barUnique = std::make_unique<TabBar>();
    TabBar* bar = barUnique.get();
    bar->setAddButtonVisible(true);
    bar->setMinTabWidth(100.0f);

    int addClicks = 0;
    bar->onTabAddRequested.connect([&]() { ++addClicks; });

    for (int i = 0; i < 6; ++i) {
        bar->addTab("Tab " + std::to_string(i));
    }

    ctx.setRootWidget(std::move(barUnique));
    ctx.handleResize(core::Size{300.0f, 32.0f});

    EXPECT_GT(bar->maxScrollOffset(), 0.0f);

    // Click '+' button
    core::Rect addRect = bar->addButtonRect();
    EXPECT_GT(addRect.size.width, 0.0f);
    WidgetMouseEvent clickAdd{core::Point{addRect.origin.x + 10.0f, addRect.origin.y + 16.0f},
                              platform::MouseButton::Left, platform::KeyModifier::None};
    bar->onMouseDown(clickAdd);
    EXPECT_TRUE(clickAdd.isAccepted());
    EXPECT_EQ(addClicks, 1);

    // Selecting last off-screen tab automatically scrolls it into view
    bar->setSelectedIndex(5);
    EXPECT_GT(bar->tabScrollOffset(), 0.0f);

    // Mouse wheel scrolls horizontally
    float beforeWheel = bar->tabScrollOffset();
    WidgetMouseEvent wheelEv{core::Point{50.0f, 16.0f}, platform::MouseButton::None,
                             platform::KeyModifier::None, core::Point{0.0f, 1.0f}};
    bar->onMouseWheel(wheelEv);
    EXPECT_TRUE(wheelEv.isAccepted());
    EXPECT_LT(bar->tabScrollOffset(), beforeWheel);
}

TEST(TabBarTest, KeyboardShortcutsAndSkipDisabled) {
    TabBar bar;
    bar.addTab("Zero", true, true);
    bar.addTab("One (Disabled)", true, false);
    bar.addTab("Two", true, true);
    bar.addTab("Three", true, true);
    bar.setSelectedIndex(0);

    // Right skips disabled tab 1 -> lands on 2
    WidgetKeyEvent rightEv{platform::Key::Right, true, platform::KeyModifier::None};
    bar.onKeyDown(rightEv);
    EXPECT_TRUE(rightEv.isAccepted());
    EXPECT_EQ(bar.selectedIndex(), 2);

    // End jumps to 3
    WidgetKeyEvent endEv{platform::Key::End, true, platform::KeyModifier::None};
    bar.onKeyDown(endEv);
    EXPECT_EQ(bar.selectedIndex(), 3);

    // Home jumps to 0
    WidgetKeyEvent homeEv{platform::Key::Home, true, platform::KeyModifier::None};
    bar.onKeyDown(homeEv);
    EXPECT_EQ(bar.selectedIndex(), 0);

    // Ctrl+Tab cycles forward skipping disabled tab 1 -> 2
    WidgetKeyEvent ctrlTab{platform::Key::Tab, true, platform::KeyModifier::Control};
    bar.onKeyDown(ctrlTab);
    EXPECT_EQ(bar.selectedIndex(), 2);

    // Ctrl+W requests close of selected tab 2
    int closedIdx = -1;
    bar.onTabCloseRequested.connect([&](int idx) { closedIdx = idx; });
    WidgetKeyEvent ctrlW{platform::Key::W, true, platform::KeyModifier::Control};
    bar.onKeyDown(ctrlW);
    EXPECT_TRUE(ctrlW.isAccepted());
    EXPECT_EQ(closedIdx, 2);
}
