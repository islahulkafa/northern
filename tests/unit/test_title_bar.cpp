#include "widgets/dialogs/title_bar.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(TitleBarTest, DefaultsAndDimensions) {
    TitleBar bar("Test App");
    EXPECT_EQ(bar.title(), "Test App");
    EXPECT_FLOAT_EQ(bar.style().height.value, TitleBar::kDefaultHeight);
    EXPECT_FLOAT_EQ(bar.style().width.value, 100.0f); // percent

    bar.setTitle("Updated Title");
    EXPECT_EQ(bar.title(), "Updated Title");
}

TEST(TitleBarTest, HitTestingCaptionAndButtons) {
    TitleBar bar("HitTest Window");
    bar.layoutNode().setLayoutRect(core::Rect{core::Point{0.0f, 0.0f}, core::Size{800.0f, 36.0f}});

    // Center caption area
    EXPECT_EQ(bar.hitTestPoint(core::Point{400.0f, 18.0f}), platform::HitTestResult::Caption);

    // Left title area
    EXPECT_EQ(bar.hitTestPoint(core::Point{50.0f, 18.0f}), platform::HitTestResult::Caption);

    // Close button: width 800 - 44 = 756..800
    EXPECT_EQ(bar.hitTestPoint(core::Point{780.0f, 18.0f}), platform::HitTestResult::Client);

    // Maximize button: 756 - 44 = 712..756
    EXPECT_EQ(bar.hitTestPoint(core::Point{730.0f, 18.0f}), platform::HitTestResult::Client);

    // Minimize button: 712 - 44 = 668..712
    EXPECT_EQ(bar.hitTestPoint(core::Point{690.0f, 18.0f}), platform::HitTestResult::Client);

    // Outside bounds
    EXPECT_EQ(bar.hitTestPoint(core::Point{400.0f, 50.0f}), platform::HitTestResult::Client);
}

TEST(TitleBarTest, ClickSignals) {
    TitleBar bar("Signals");
    bar.layoutNode().setLayoutRect(core::Rect{core::Point{0.0f, 0.0f}, core::Size{800.0f, 36.0f}});

    bool minClicked = false;
    bool maxClicked = false;
    bool closeClicked = false;

    bar.onMinimizeClicked.connect([&]() { minClicked = true; });
    bar.onMaximizeClicked.connect([&]() { maxClicked = true; });
    bar.onCloseClicked.connect([&]() { closeClicked = true; });

    // Click close button
    WidgetMouseEvent downClose{core::Point{780.0f, 18.0f}, platform::MouseButton::Left,
                               platform::KeyModifier::None};
    bar.onMouseDown(downClose);
    EXPECT_TRUE(downClose.isAccepted());

    WidgetMouseEvent upClose{core::Point{780.0f, 18.0f}, platform::MouseButton::Left,
                             platform::KeyModifier::None};
    bar.onMouseUp(upClose);
    EXPECT_TRUE(upClose.isAccepted());
    EXPECT_TRUE(closeClicked);

    // Click minimize button
    WidgetMouseEvent downMin{core::Point{690.0f, 18.0f}, platform::MouseButton::Left,
                             platform::KeyModifier::None};
    bar.onMouseDown(downMin);
    WidgetMouseEvent upMin{core::Point{690.0f, 18.0f}, platform::MouseButton::Left,
                           platform::KeyModifier::None};
    bar.onMouseUp(upMin);
    EXPECT_TRUE(minClicked);

    // Click maximize button
    WidgetMouseEvent downMax{core::Point{730.0f, 18.0f}, platform::MouseButton::Left,
                             platform::KeyModifier::None};
    bar.onMouseDown(downMax);
    WidgetMouseEvent upMax{core::Point{730.0f, 18.0f}, platform::MouseButton::Left,
                           platform::KeyModifier::None};
    bar.onMouseUp(upMax);
    EXPECT_TRUE(maxClicked);
}

TEST(TitleBarTest, HoverAndLeave) {
    TitleBar bar("Hover Test");
    bar.layoutNode().setLayoutRect(core::Rect{core::Point{0.0f, 0.0f}, core::Size{800.0f, 36.0f}});

    WidgetMouseEvent moveClose{core::Point{780.0f, 18.0f}, platform::MouseButton::None,
                               platform::KeyModifier::None};
    bar.onMouseMove(moveClose);

    WidgetMouseEvent moveAway{core::Point{50.0f, 18.0f}, platform::MouseButton::None,
                              platform::KeyModifier::None};
    bar.onMouseMove(moveAway);

    bar.onMouseLeave();
}
