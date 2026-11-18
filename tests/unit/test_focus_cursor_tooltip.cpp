#include "widgets/button.hpp"
#include "widgets/checkbox.hpp"
#include "widgets/container.hpp"
#include "widgets/radio_button.hpp"
#include "widgets/radio_group.hpp"
#include "widgets/slider.hpp"
#include "widgets/text_input.hpp"
#include "widgets/tooltip.hpp"
#include "widgets/widget_context.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

namespace {

platform::MouseEvent makeMouseEvent(platform::MouseEventType type, platform::MouseButton button,
                                    core::Point pos) {
    platform::MouseEvent ev{};
    ev.type = type;
    ev.button = button;
    ev.position = pos;
    return ev;
}

platform::KeyEvent makeKeyEvent(platform::Key key, bool isDown,
                                platform::KeyModifier mods = platform::KeyModifier::None) {
    platform::KeyEvent ev{};
    ev.key = key;
    ev.isDown = isDown;
    ev.modifiers = mods;
    return ev;
}

} // namespace

TEST(FocusCursorTooltipTest, TabAndShiftTabTraversalWithFiltering) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    root->style().flexDirection = layout::FlexDirection::Column;

    auto* input = root->addChild(std::make_unique<TextInput>("Input"));
    auto* disabledBtn = root->addChild(std::make_unique<Button>("Disabled"));
    disabledBtn->setEnabled(false);
    auto* hiddenBtn = root->addChild(std::make_unique<Button>("Hidden"));
    hiddenBtn->setVisible(false);
    auto* activeBtn = root->addChild(std::make_unique<Button>("Apply"));

    ctx.setRootWidget(std::move(root));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    auto tabDown = makeKeyEvent(platform::Key::Tab, true);
    ctx.handleKeyEvent(tabDown);
    EXPECT_EQ(ctx.focusedWidget(), input);
    EXPECT_TRUE(input->hasKeyboardFocus());

    ctx.handleKeyEvent(tabDown);
    EXPECT_EQ(ctx.focusedWidget(), activeBtn);
    EXPECT_TRUE(activeBtn->hasKeyboardFocus());

    // Wraps around forward
    ctx.handleKeyEvent(tabDown);
    EXPECT_EQ(ctx.focusedWidget(), input);

    // Shift+Tab wraps backward
    auto shiftTabDown = makeKeyEvent(platform::Key::Tab, true, platform::KeyModifier::Shift);
    ctx.handleKeyEvent(shiftTabDown);
    EXPECT_EQ(ctx.focusedWidget(), activeBtn);
}

TEST(FocusCursorTooltipTest, FocusReasonPointerVsKeyboard) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    root->style().flexDirection = layout::FlexDirection::Column;

    auto btn = std::make_unique<Button>("Click Me");
    btn->style().width = layout::Length::points(120.0f);
    btn->style().height = layout::Length::points(32.0f);
    Button* btnPtr = root->addChild(std::move(btn));

    ctx.setRootWidget(std::move(root));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    core::Rect btnRect = btnPtr->screenRect();
    core::Point center{btnRect.origin.x + btnRect.size.width * 0.5f,
                       btnRect.origin.y + btnRect.size.height * 0.5f};

    ctx.handleMouseDown(
        makeMouseEvent(platform::MouseEventType::ButtonDown, platform::MouseButton::Left, center));

    EXPECT_TRUE(btnPtr->hasFocus());
    EXPECT_FALSE(btnPtr->hasKeyboardFocus());

    // Tabbing onto it switches to keyboard focus visibility
    ctx.clearFocus();
    ctx.handleKeyEvent(makeKeyEvent(platform::Key::Tab, true));
    EXPECT_TRUE(btnPtr->hasFocus());
    EXPECT_TRUE(btnPtr->hasKeyboardFocus());
}

TEST(FocusCursorTooltipTest, KeyboardActivationForButtonCheckboxSliderAndRadioGroup) {
    Button btn("OK");
    int clicked = 0;
    btn.onClicked.connect([&]() { ++clicked; });
    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    btn.onKeyDown(spaceEv);
    EXPECT_TRUE(spaceEv.isAccepted());
    EXPECT_EQ(clicked, 1);

    Checkbox cb("Enable", false);
    cb.onKeyDown(spaceEv);
    EXPECT_TRUE(cb.isChecked());

    Slider slider(0.0f, 100.0f, 50.0f);
    WidgetKeyEvent rightEv{platform::Key::Right, true, platform::KeyModifier::None};
    slider.onKeyDown(rightEv);
    EXPECT_GT(slider.value(), 50.0f);
    WidgetKeyEvent homeEv{platform::Key::Home, true, platform::KeyModifier::None};
    slider.onKeyDown(homeEv);
    EXPECT_FLOAT_EQ(slider.value(), 0.0f);
    WidgetKeyEvent endEv{platform::Key::End, true, platform::KeyModifier::None};
    slider.onKeyDown(endEv);
    EXPECT_FLOAT_EQ(slider.value(), 100.0f);

    RadioGroup group;
    EXPECT_FALSE(group.isFocusable());
    group.addRadioButton(std::make_unique<RadioButton>("A"));
    group.addRadioButton(std::make_unique<RadioButton>("B"));
    group.setSelectedIndex(0);
    group.onKeyDown(rightEv);
    EXPECT_EQ(group.selectedIndex(), 1);
    WidgetKeyEvent leftEv{platform::Key::Left, true, platform::KeyModifier::None};
    group.onKeyDown(leftEv);
    EXPECT_EQ(group.selectedIndex(), 0);
}

TEST(FocusCursorTooltipTest, CursorShapeDispatchOnHover) {
    WidgetContext ctx;
    std::vector<platform::CursorShape> emitted;
    ctx.onCursorShapeChanged.connect([&](platform::CursorShape s) { emitted.push_back(s); });

    auto root = std::make_unique<Container>();
    root->style().flexDirection = layout::FlexDirection::Column;
    root->style().rowGap = 8.0f;

    auto input = std::make_unique<TextInput>("Type");
    input->style().width = layout::Length::points(120.0f);
    input->style().height = layout::Length::points(32.0f);
    TextInput* inputPtr = root->addChild(std::move(input));

    auto btn = std::make_unique<Button>("Go");
    btn->style().width = layout::Length::points(120.0f);
    btn->style().height = layout::Length::points(32.0f);
    Button* btnPtr = root->addChild(std::move(btn));

    ctx.setRootWidget(std::move(root));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    core::Rect inR = inputPtr->screenRect();
    ctx.handleMouseMove(makeMouseEvent(platform::MouseEventType::Move, platform::MouseButton::None,
                                       {inR.origin.x + 10.0f, inR.origin.y + 10.0f}));
    EXPECT_EQ(ctx.currentCursorShape(), platform::CursorShape::IBeam);

    core::Rect btnR = btnPtr->screenRect();
    ctx.handleMouseMove(makeMouseEvent(platform::MouseEventType::Move, platform::MouseButton::None,
                                       {btnR.origin.x + 10.0f, btnR.origin.y + 10.0f}));
    EXPECT_EQ(ctx.currentCursorShape(), platform::CursorShape::PointingHand);

    ctx.handleMouseMove(makeMouseEvent(platform::MouseEventType::Move, platform::MouseButton::None,
                                       {350.0f, 250.0f}));
    EXPECT_EQ(ctx.currentCursorShape(), platform::CursorShape::Arrow);
    EXPECT_EQ(emitted.size(), 3u);
}

TEST(FocusCursorTooltipTest, DelayedTooltipShowHideAndNonBlockingClick) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();

    auto btn = std::make_unique<Button>("Save");
    btn->style().width = layout::Length::points(100.0f);
    btn->style().height = layout::Length::points(32.0f);
    btn->setTooltip("Save document", "Ctrl+S");
    int clicked = 0;
    btn->onClicked.connect([&]() { ++clicked; });
    Button* btnPtr = root->addChild(std::move(btn));

    ctx.setRootWidget(std::move(root));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    core::Rect btnR = btnPtr->screenRect();
    core::Point hoverPos{btnR.origin.x + 20.0f, btnR.origin.y + 16.0f};
    ctx.handleMouseMove(
        makeMouseEvent(platform::MouseEventType::Move, platform::MouseButton::None, hoverPos));

    ctx.advanceAnimations(0.30f);
    EXPECT_FALSE(ctx.isTooltipVisible());

    ctx.advanceAnimations(0.25f);
    EXPECT_TRUE(ctx.isTooltipVisible());
    ASSERT_NE(ctx.activeTooltip(), nullptr);
    EXPECT_EQ(ctx.activeTooltip()->text(), "Save document");
    EXPECT_EQ(ctx.activeTooltip()->shortcut(), "Ctrl+S");

    // Clicking the button while tooltip is visible still hits and clicks the button
    ctx.handleMouseDown(makeMouseEvent(platform::MouseEventType::ButtonDown,
                                       platform::MouseButton::Left, hoverPos));
    EXPECT_FALSE(ctx.isTooltipVisible());
    ctx.handleMouseUp(
        makeMouseEvent(platform::MouseEventType::ButtonUp, platform::MouseButton::Left, hoverPos));
    EXPECT_EQ(clicked, 1);
}
