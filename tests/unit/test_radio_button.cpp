#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/radio_button.hpp"
#include "widgets/controls/radio_group.hpp"
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

TEST(RadioButtonTest, DefaultFontSizePresetsAndDynamicTheming) {
    WidgetContext ctx;
    ctx.setTheme(theme::Theme::dark());

    auto owned = std::make_unique<RadioButton>("Dynamic Theme", true);
    RadioButton* rb = owned.get();
    ctx.setRootWidget(std::move(owned));

    ASSERT_NE(rb->effectiveFont(), nullptr);
    rb->layoutNode().computeLayout(200.0f, 40.0f);
    EXPECT_GT(rb->layoutRect().size.width, 20.0f);

    rb->setSize(RadioSize::Large);
    EXPECT_FLOAT_EQ(rb->circleSize(), 20.0f);

    EXPECT_EQ(rb->effectiveOuterColor(), theme::ColorPalette::dark().accentDefault);
    EXPECT_EQ(rb->effectiveDotColor(), core::Color::white());
    EXPECT_EQ(rb->effectiveTextColor(), theme::ColorPalette::dark().textPrimary);

    ctx.setTheme(theme::Theme::light());
    EXPECT_EQ(rb->effectiveOuterColor(), theme::ColorPalette::light().accentDefault);
    EXPECT_EQ(rb->effectiveDotColor(), core::Color::white());
    EXPECT_EQ(rb->effectiveTextColor(), theme::ColorPalette::light().textPrimary);

    rb->setEnabled(false);
    EXPECT_EQ(rb->effectiveTextColor(), theme::ColorPalette::light().textDisabled);

    renderer::PaintContext pctx;
    rb->paint(pctx);
    EXPECT_FALSE(pctx.commands().empty());
}

TEST(RadioButtonTest, DisabledGuardsPointerDragCancelAndFocusReason) {
    WidgetContext ctx;
    auto owned = std::make_unique<RadioButton>("Clickable", false);
    RadioButton* rb = owned.get();
    ctx.setRootWidget(std::move(owned));
    rb->layoutNode().computeLayout(120.0f, 30.0f);

    // Dragging pointer outside bounds cancels toggle on mouse up
    WidgetMouseEvent downEv{core::Point{8.0f, 8.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    rb->onMouseDown(downEv);
    EXPECT_TRUE(downEv.isAccepted());
    EXPECT_FALSE(rb->hasKeyboardFocus());

    WidgetMouseEvent outsideUp{core::Point{500.0f, 500.0f}, platform::MouseButton::Left,
                               platform::KeyModifier::None};
    rb->onMouseUp(outsideUp);
    EXPECT_FALSE(rb->isChecked());

    // Disabled radio button ignores key and pointer clicks
    rb->setEnabled(false);
    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    rb->onKeyDown(spaceEv);
    EXPECT_FALSE(spaceEv.isAccepted());
    EXPECT_FALSE(rb->isChecked());
}

TEST(RadioGroupTest, OrientationValuesRemovalAndDisabledSkipNavigation) {
    RadioGroup group(RadioGroupOrientation::Horizontal);
    EXPECT_EQ(group.orientation(), RadioGroupOrientation::Horizontal);
    EXPECT_EQ(group.style().flexDirection, layout::FlexDirection::Row);

    auto* opt1 = group.addOption("Fast", "fast", RadioSize::Compact);
    auto* opt2 = group.addOption("Balanced", "balanced");
    auto* opt3 = group.addOption("Quality", "quality");

    opt2->setEnabled(false);
    EXPECT_TRUE(group.setSelectedValue("fast"));
    EXPECT_EQ(group.selectedValue(), "fast");
    EXPECT_TRUE(opt1->isChecked());

    // Down/Right skips disabled opt2 and lands on opt3
    WidgetKeyEvent rightKey{platform::Key::Right, true, platform::KeyModifier::None};
    group.onKeyDown(rightKey);
    EXPECT_TRUE(rightKey.isAccepted());
    EXPECT_EQ(group.selectedButton(), opt3);
    EXPECT_EQ(group.selectedValue(), "quality");

    // Removing an earlier item shifts selectedIndex_ cleanly
    group.removeRadioButton(opt1);
    EXPECT_EQ(group.buttons().size(), 2u);
    EXPECT_EQ(group.selectedIndex(), 1);
    EXPECT_EQ(group.selectedButton(), opt3);

    group.clearRadioButtons();
    EXPECT_TRUE(group.buttons().empty());
    EXPECT_EQ(group.selectedIndex(), -1);
}

TEST(RadioGroupTest, CenteredAutoWidthGroupShrinkWrapsInColumnRoot) {
    Container root;
    root.style().width = layout::Length::points(800.0f);
    root.style().height = layout::Length::points(600.0f);
    root.setFlexDirection(layout::FlexDirection::Column);
    root.setJustifyContent(layout::JustifyContent::Center);
    root.setAlignItems(layout::AlignItems::Center);

    auto group = std::make_unique<RadioGroup>(RadioGroupOrientation::Vertical);
    group->addOption("Short", "s");
    group->addOption("Longer Option Label", "l");
    RadioGroup* groupPtr = root.addChild(std::move(group));

    root.layoutNode().computeLayout(800.0f, 600.0f);

    EXPECT_LT(groupPtr->layoutRect().size.width, 400.0f);
    EXPECT_GT(groupPtr->layoutRect().origin.x, 200.0f);
    EXPECT_GT(groupPtr->layoutRect().origin.y, 200.0f);

    // Verify clicking a child RadioButton at non-zero screen coordinates toggles selection
    WidgetContext ctx;
    auto ownedRoot = std::make_unique<Container>();
    ownedRoot->style().width = layout::Length::points(800.0f);
    ownedRoot->style().height = layout::Length::points(600.0f);
    ownedRoot->setFlexDirection(layout::FlexDirection::Column);
    ownedRoot->setJustifyContent(layout::JustifyContent::Center);
    ownedRoot->setAlignItems(layout::AlignItems::Center);

    auto g2 = std::make_unique<RadioGroup>(RadioGroupOrientation::Vertical);
    auto* optA = g2->addOption("Option A", "a");
    auto* optB = g2->addOption("Option B", "b");
    ownedRoot->addChild(std::move(g2));

    ctx.setRootWidget(std::move(ownedRoot));
    renderer::PaintContext dummyCtx;
    ctx.render(dummyCtx, {800.0f, 600.0f});

    core::Rect bRect = optB->screenRect();
    ASSERT_GT(bRect.origin.x, 100.0f);
    ASSERT_GT(bRect.origin.y, 100.0f);

    platform::MouseEvent down{};
    down.type = platform::MouseEventType::ButtonDown;
    down.button = platform::MouseButton::Left;
    down.position = {bRect.origin.x + 8.0f, bRect.origin.y + bRect.size.height * 0.5f};
    ctx.handleMouseDown(down);

    platform::MouseEvent up{};
    up.type = platform::MouseEventType::ButtonUp;
    up.button = platform::MouseButton::Left;
    up.position = down.position;
    ctx.handleMouseUp(up);

    EXPECT_TRUE(optB->isChecked());
    EXPECT_FALSE(optA->isChecked());
}

TEST(RadioButtonTest, ContextDpiAdoptionAndMouseGrabCleanup) {
    WidgetContext ctx;
    ctx.setDpiScale(1.5f);
    auto owned = std::make_unique<RadioButton>("Scaled", false);
    RadioButton* rb = owned.get();
    ctx.setRootWidget(std::move(owned));
    ctx.handleResize(core::Size{160.0f, 32.0f});

    EXPECT_FLOAT_EQ(rb->dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(rb->circleSize(), 24.0f);
    ASSERT_NE(rb->effectiveFont(), nullptr);
    EXPECT_FLOAT_EQ(rb->effectiveFont()->pixelSize(), 21.0f);

    // Explicit setDpiScale is preserved on context attachment
    WidgetContext ctx2;
    ctx2.setDpiScale(1.5f);
    auto explicitOwned = std::make_unique<RadioButton>("Explicit", false);
    RadioButton* explicitRb = explicitOwned.get();
    explicitRb->setDpiScale(2.0f);
    ctx2.setRootWidget(std::move(explicitOwned));
    EXPECT_FLOAT_EQ(explicitRb->dpiScale(), 2.0f);

    // Idle updateAnimations does not dirty paint
    rb->clearPaintDirty();
    rb->updateAnimations(0.016f);
    EXPECT_FALSE(rb->isPaintDirty());

    // Theme switch marks paint dirty
    rb->clearPaintDirty();
    ctx.setTheme(theme::Theme::light());
    EXPECT_TRUE(rb->isPaintDirty());

    // Press acquires mouse grab; release outside cancels toggle and releases grab
    WidgetMouseEvent pressIn{core::Point{8.0f, 12.0f}, platform::MouseButton::Left,
                             platform::KeyModifier::None};
    rb->onMouseDown(pressIn);
    EXPECT_EQ(ctx.mouseGrabWidget(), rb);

    WidgetMouseEvent releaseOut{core::Point{400.0f, 12.0f}, platform::MouseButton::Left,
                                platform::KeyModifier::None};
    rb->onMouseUp(releaseOut);
    EXPECT_EQ(ctx.mouseGrabWidget(), nullptr);
    EXPECT_FALSE(rb->isChecked());

    // Disable mid-press releases grab and switches cursor to Arrow
    rb->onMouseDown(pressIn);
    EXPECT_EQ(ctx.mouseGrabWidget(), rb);
    rb->setEnabled(false);
    EXPECT_EQ(ctx.mouseGrabWidget(), nullptr);
    EXPECT_EQ(rb->cursorShape(), platform::CursorShape::Arrow);

    rb->setEnabled(true);
    EXPECT_EQ(rb->cursorShape(), platform::CursorShape::PointingHand);

    // Focus lost mid-press releases grab
    rb->onMouseDown(pressIn);
    EXPECT_EQ(ctx.mouseGrabWidget(), rb);
    rb->onFocusLost();
    EXPECT_EQ(ctx.mouseGrabWidget(), nullptr);
}

TEST(RadioGroupTest, DpiPropagationAndDisabledKeyGuard) {
    WidgetContext ctx;
    ctx.setDpiScale(1.5f);
    auto ownedGroup = std::make_unique<RadioGroup>(RadioGroupOrientation::Vertical);
    RadioGroup* group = ownedGroup.get();

    auto* opt1 = group->addOption("One", "1");
    auto customRb = std::make_unique<RadioButton>("Custom");
    customRb->setDpiScale(2.0f);
    RadioButton* opt2 = group->addRadioButton(std::move(customRb));

    ctx.setRootWidget(std::move(ownedGroup));

    EXPECT_FLOAT_EQ(group->dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(group->style().rowGap, 12.0f);
    EXPECT_FLOAT_EQ(opt1->dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(opt2->dpiScale(), 2.0f);

    // Disabled RadioGroup ignores arrow-key navigation
    group->setSelectedIndex(0);
    group->setEnabled(false);
    WidgetKeyEvent downKey{platform::Key::Down, true, platform::KeyModifier::None};
    group->onKeyDown(downKey);
    EXPECT_FALSE(downKey.isAccepted());
    EXPECT_EQ(group->selectedIndex(), 0);
}
