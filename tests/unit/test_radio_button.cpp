#include "widgets/radio_button.hpp"
#include "widgets/radio_group.hpp"
#include "widgets/widget_context.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(RadioButtonTest, InitialStateAndDefaults) {
    RadioButton rb{"Option A"};
    EXPECT_FALSE(rb.isChecked());
    EXPECT_TRUE(rb.isFocusable());
    EXPECT_EQ(rb.text(), "Option A");
}

TEST(RadioButtonTest, SetCheckedAndSignals) {
    RadioButton rb;
    int toggleCount = 0;
    int selectCount = 0;

    rb.onToggled.connect([&](bool) { ++toggleCount; });
    rb.onSelected.connect([&](RadioButton*) { ++selectCount; });

    rb.setChecked(true);
    EXPECT_TRUE(rb.isChecked());
    EXPECT_EQ(toggleCount, 1);
    EXPECT_EQ(selectCount, 1);

    // Redundant setChecked should not re-emit
    rb.setChecked(true);
    EXPECT_EQ(toggleCount, 1);
    EXPECT_EQ(selectCount, 1);

    rb.setChecked(false);
    EXPECT_FALSE(rb.isChecked());
    EXPECT_EQ(toggleCount, 2);
    EXPECT_EQ(selectCount, 1); // onSelected only emits when checked becomes true
}

TEST(RadioButtonTest, AnimationProgressAndSettling) {
    WidgetContext ctx;
    auto rb = std::make_unique<RadioButton>("Option Animated", false);
    RadioButton* rbPtr = rb.get();

    ctx.setRootWidget(std::move(rb));
    EXPECT_FALSE(rbPtr->hasActiveAnimations());

    rbPtr->setChecked(true);
    EXPECT_TRUE(rbPtr->hasActiveAnimations());

    // Advance 80ms (half-way through 160ms animation)
    ctx.advanceAnimations(0.08f);
    EXPECT_TRUE(rbPtr->hasActiveAnimations());

    // Advance remaining 100ms
    ctx.advanceAnimations(0.10f);
    EXPECT_FALSE(rbPtr->hasActiveAnimations());
}

TEST(RadioGroupTest, MutualExclusivity) {
    RadioGroup group;
    int selectionChangedCount = 0;
    int lastSelectedIndex = -1;

    group.onSelectionChanged.connect([&](int idx, RadioButton*) {
        ++selectionChangedCount;
        lastSelectedIndex = idx;
    });

    auto* rb1 = group.addRadioButton(std::make_unique<RadioButton>("Option 1"));
    auto* rb2 = group.addRadioButton(std::make_unique<RadioButton>("Option 2"));
    auto* rb3 = group.addRadioButton(std::make_unique<RadioButton>("Option 3"));

    EXPECT_EQ(group.selectedIndex(), -1);
    EXPECT_EQ(group.selectedButton(), nullptr);

    rb1->setChecked(true);
    EXPECT_TRUE(rb1->isChecked());
    EXPECT_FALSE(rb2->isChecked());
    EXPECT_FALSE(rb3->isChecked());
    EXPECT_EQ(group.selectedIndex(), 0);
    EXPECT_EQ(group.selectedButton(), rb1);
    EXPECT_EQ(selectionChangedCount, 1);
    EXPECT_EQ(lastSelectedIndex, 0);

    // Selecting option 2 unchecks option 1
    rb2->setChecked(true);
    EXPECT_FALSE(rb1->isChecked());
    EXPECT_TRUE(rb2->isChecked());
    EXPECT_FALSE(rb3->isChecked());
    EXPECT_EQ(group.selectedIndex(), 1);
    EXPECT_EQ(group.selectedButton(), rb2);
    EXPECT_EQ(selectionChangedCount, 2);
    EXPECT_EQ(lastSelectedIndex, 1);

    // Select by index
    group.setSelectedIndex(2);
    EXPECT_FALSE(rb1->isChecked());
    EXPECT_FALSE(rb2->isChecked());
    EXPECT_TRUE(rb3->isChecked());
    EXPECT_EQ(group.selectedIndex(), 2);
    EXPECT_EQ(group.selectedButton(), rb3);
}

TEST(RadioGroupTest, ArrowKeyNavigation) {
    RadioGroup group;
    group.addRadioButton(std::make_unique<RadioButton>("1"));
    group.addRadioButton(std::make_unique<RadioButton>("2"));
    group.addRadioButton(std::make_unique<RadioButton>("3"));

    group.setSelectedIndex(0);

    WidgetKeyEvent downKey{platform::Key::Down, true, platform::KeyModifier::None};
    group.onKeyDown(downKey);
    EXPECT_TRUE(downKey.isAccepted());
    EXPECT_EQ(group.selectedIndex(), 1);

    group.onKeyDown(downKey);
    EXPECT_EQ(group.selectedIndex(), 2);

    WidgetKeyEvent upKey{platform::Key::Up, true, platform::KeyModifier::None};
    group.onKeyDown(upKey);
    EXPECT_TRUE(upKey.isAccepted());
    EXPECT_EQ(group.selectedIndex(), 1);
}
