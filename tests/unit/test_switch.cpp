#include "widgets/switch.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(SwitchTest, InitialStateAndDefaults) {
    Switch sw;
    EXPECT_FALSE(sw.isChecked());
    EXPECT_TRUE(sw.isFocusable());
    EXPECT_TRUE(sw.text().empty());
}

TEST(SwitchTest, SetCheckedAndToggle) {
    Switch sw;
    int toggleCount = 0;
    bool lastState = false;

    sw.onToggled.connect([&](bool checked) {
        ++toggleCount;
        lastState = checked;
    });

    sw.setChecked(true);
    EXPECT_TRUE(sw.isChecked());
    EXPECT_EQ(toggleCount, 1);
    EXPECT_TRUE(lastState);

    // No redundant emission if value doesn't change
    sw.setChecked(true);
    EXPECT_EQ(toggleCount, 1);

    sw.toggle();
    EXPECT_FALSE(sw.isChecked());
    EXPECT_EQ(toggleCount, 2);
    EXPECT_FALSE(lastState);
}

TEST(SwitchTest, MouseInteraction) {
    Switch sw;
    bool toggled = false;
    sw.onToggled.connect([&](bool c) { toggled = c; });

    WidgetMouseEvent downEv{core::Point{10.0f, 10.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    sw.onMouseDown(downEv);
    EXPECT_TRUE(downEv.isAccepted());
    EXPECT_FALSE(sw.isChecked()); // Toggles on mouse up

    WidgetMouseEvent upEv{core::Point{10.0f, 10.0f}, platform::MouseButton::Left,
                          platform::KeyModifier::None};
    sw.onMouseUp(upEv);
    EXPECT_TRUE(upEv.isAccepted());
    EXPECT_TRUE(sw.isChecked());
    EXPECT_TRUE(toggled);
}

TEST(SwitchTest, KeyboardActivation) {
    Switch sw;
    int toggleCount = 0;
    sw.onToggled.connect([&](bool) { ++toggleCount; });

    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    sw.onKeyDown(spaceEv);
    EXPECT_TRUE(spaceEv.isAccepted());
    EXPECT_TRUE(sw.isChecked());
    EXPECT_EQ(toggleCount, 1);

    WidgetKeyEvent enterEv{platform::Key::Enter, true, platform::KeyModifier::None};
    sw.onKeyDown(enterEv);
    EXPECT_TRUE(enterEv.isAccepted());
    EXPECT_FALSE(sw.isChecked());
    EXPECT_EQ(toggleCount, 2);
}
