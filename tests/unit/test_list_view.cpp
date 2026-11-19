#include "widgets/list_view.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::core;
using namespace northern::widgets;

TEST(ListViewTest, VirtualizedRowMathAndIndexing) {
    StringListModel model;
    for (int i = 0; i < 10000; ++i) {
        model.append("Item " + std::to_string(i));
    }

    ListView list(&model);
    list.setRowHeight(30.0f);
    list.layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{300.0f, 300.0f}});

    EXPECT_FLOAT_EQ(list.maxScrollOffset(), 10000 * 30.0f - 300.0f);

    // Row hit testing
    EXPECT_EQ(list.rowAt(0.0f), 0);
    EXPECT_EQ(list.rowAt(45.0f), 1);
    EXPECT_EQ(list.rowAt(299.0f), 9);

    // Scroll down by 300px
    list.scrollBy(300.0f);
    EXPECT_FLOAT_EQ(list.scrollOffsetY(), 300.0f);
    EXPECT_EQ(list.rowAt(0.0f), 10);
}

TEST(ListViewTest, ClickSelectionAndSignals) {
    StringListModel model({"Zero", "One", "Two", "Three"});
    ListView list(&model);
    list.setRowHeight(25.0f);
    list.layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{200.0f, 200.0f}});

    ModelIndex clickedIndex;
    list.onItemClicked.connect([&](const ModelIndex& idx) { clickedIndex = idx; });

    // Click on row 2 (y = 55px)
    WidgetMouseEvent ev{core::Point{50.0f, 55.0f}, northern::platform::MouseButton::Left,
                        northern::platform::KeyModifier::None};
    list.onMouseDown(ev);

    EXPECT_TRUE(clickedIndex.isValid());
    EXPECT_EQ(clickedIndex.row, 2);
    EXPECT_TRUE(list.selectionModel().isSelected(model.index(2, 0)));
}

TEST(ListViewTest, KeyboardNavigation) {
    StringListModel model({"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"});
    ListView list(&model);
    list.setRowHeight(20.0f);
    list.layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{100.0f, 100.0f}});

    list.selectionModel().select(model.index(0, 0));
    list.selectionModel().setCurrentIndex(model.index(0, 0));

    // Down arrow
    WidgetKeyEvent downEv{northern::platform::Key::Down, true,
                          northern::platform::KeyModifier::None};
    list.onKeyDown(downEv);
    EXPECT_EQ(list.selectionModel().currentIndex().row, 1);

    // End key
    WidgetKeyEvent endEv{northern::platform::Key::End, true, northern::platform::KeyModifier::None};
    list.onKeyDown(endEv);
    EXPECT_EQ(list.selectionModel().currentIndex().row, 9);

    // Home key
    WidgetKeyEvent homeEv{northern::platform::Key::Home, true,
                          northern::platform::KeyModifier::None};
    list.onKeyDown(homeEv);
    EXPECT_EQ(list.selectionModel().currentIndex().row, 0);

    // Enter activates
    ModelIndex activatedIndex;
    list.onItemActivated.connect([&](const ModelIndex& idx) { activatedIndex = idx; });
    WidgetKeyEvent enterEv{northern::platform::Key::Enter, true,
                           northern::platform::KeyModifier::None};
    list.onKeyDown(enterEv);
    EXPECT_EQ(activatedIndex.row, 0);
}
