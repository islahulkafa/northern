#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/controls/button.hpp"
#include "widgets/controls/checkbox.hpp"
#include "widgets/controls/label.hpp"
#include "widgets/controls/radio_button.hpp"
#include "widgets/controls/radio_group.hpp"
#include "widgets/controls/slider.hpp"
#include "widgets/controls/text_input.hpp"
#include "widgets/controls/tooltip.hpp"
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

TEST(FocusCursorTooltipTest, TooltipTypographyWrappingThemeAndCaching) {
    WidgetContext ctx;
    ctx.setTheme(theme::Theme::dark());

    auto owned =
        std::make_unique<Tooltip>("Save Workspace", "Ctrl+S", nullptr,
                                  "Writes all modified buffers and layout state to disk cleanly.");
    Tooltip* tip = owned.get();
    ctx.setRootWidget(std::move(owned));

    ASSERT_NE(tip->effectiveFont(), nullptr);
    core::Size baseSize = tip->preferredSize();
    EXPECT_LE(baseSize.width, tip->maxWidth() + 0.01f);
    EXPECT_GT(baseSize.height, 36.0f);

    tip->layoutNode().setLayoutRect(core::Rect{{10.0f, 10.0f}, baseSize});
    renderer::PaintContext pctx;
    tip->paint(pctx);

    int boxCount = 0;
    int glyphCount = 0;
    for (const auto& cmd : pctx.commands()) {
        if (std::holds_alternative<renderer::DrawBoxCommand>(cmd)) {
            ++boxCount;
        } else if (std::holds_alternative<renderer::DrawGlyphsCommand>(cmd)) {
            ++glyphCount;
        }
    }
    EXPECT_GE(boxCount, 2);   // Outer surface + shortcut keycap pill
    EXPECT_GE(glyphCount, 3); // Title + shortcut + description

    tip->setDpiScale(1.5f);
    core::Size scaledSize = tip->preferredSize();
    EXPECT_GT(scaledSize.width, baseSize.width);
    EXPECT_GT(scaledSize.height, baseSize.height);

    EXPECT_EQ(tip->effectiveBackgroundColor(), theme::ColorPalette::dark().surfaceOverlay);
    EXPECT_EQ(tip->effectiveTextColor(), theme::ColorPalette::dark().textPrimary);

    ctx.setTheme(theme::Theme::light());
    EXPECT_EQ(tip->effectiveBackgroundColor(), theme::ColorPalette::light().surfaceOverlay);
    EXPECT_EQ(tip->effectiveTextColor(), theme::ColorPalette::light().textPrimary);
}

TEST(FocusCursorTooltipTest, TooltipBubblingPlacementWarmupAndO1Dismissal) {
    struct PaintCountContainer : Container {
        mutable int paintCount{0};
        void paint(renderer::PaintContext& ctx) override {
            ++paintCount;
            Container::paint(ctx);
        }
    };

    WidgetContext ctx;
    auto root = std::make_unique<PaintCountContainer>();
    root->setFlexDirection(layout::FlexDirection::Column);
    root->setGap(12.0f);
    root->style().padding.top = layout::Length::points(100.0f);
    PaintCountContainer* rootPtr = root.get();

    auto row = std::make_unique<Container>();
    row->style().width = layout::Length::points(180.0f);
    row->style().height = layout::Length::points(32.0f);
    row->setTooltip("Group Tooltip", "Ctrl+G", "Bubbles from child", OverlayPlacement::Top);
    auto child = std::make_unique<Label>("Child");
    child->style().width = layout::Length::points(100.0f);
    child->style().height = layout::Length::points(24.0f);
    Label* childPtr = row->addChild(std::move(child));
    Container* rowPtr = root->addChild(std::move(row));

    auto second = std::make_unique<Container>();
    second->style().width = layout::Length::points(180.0f);
    second->style().height = layout::Length::points(32.0f);
    second->setTooltip("Second Action");
    Container* secondPtr = root->addChild(std::move(second));

    ctx.setRootWidget(std::move(root));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    // Initial scene render records root tree once
    renderer::PaintContext pctx;
    ctx.render(pctx, core::Size{400.0f, 300.0f});
    EXPECT_EQ(rootPtr->paintCount, 1);

    // Hover child Label (no direct tooltip): should bubble up to rowPtr
    core::Rect childR = childPtr->screenRect();
    ctx.handleMouseMove(makeMouseEvent(platform::MouseEventType::Move, platform::MouseButton::None,
                                       {childR.origin.x + 10.0f, childR.origin.y + 10.0f}));
    ctx.advanceAnimations(0.55f);
    ASSERT_TRUE(ctx.isTooltipVisible());
    ASSERT_NE(ctx.activeTooltip(), nullptr);
    EXPECT_EQ(ctx.activeTooltip()->text(), "Group Tooltip");
    EXPECT_EQ(ctx.activeTooltip()->description(), "Bubbles from child");
    EXPECT_LT(ctx.activeTooltip()->layoutRect().origin.y, rowPtr->screenRect().origin.y);

    // Fade-in animation step & render should NOT re-traverse root widget tree
    ctx.advanceAnimations(0.04f);
    pctx.clear();
    ctx.render(pctx, core::Size{400.0f, 300.0f});
    EXPECT_EQ(rootPtr->paintCount, 1);

    // Moving to adjacent tooltipped widget within warmup window triggers fast (~80ms) display
    core::Rect secR = secondPtr->screenRect();
    ctx.handleMouseMove(makeMouseEvent(platform::MouseEventType::Move, platform::MouseButton::None,
                                       {secR.origin.x + 10.0f, secR.origin.y + 10.0f}));
    EXPECT_FALSE(ctx.isTooltipVisible());
    ctx.advanceAnimations(0.10f);
    EXPECT_TRUE(ctx.isTooltipVisible());
    ASSERT_NE(ctx.activeTooltip(), nullptr);
    EXPECT_EQ(ctx.activeTooltip()->text(), "Second Action");

    // Dismissing marks tooltip dirty and renders clean scene without re-traversing root
    ctx.dismissTooltip();
    EXPECT_FALSE(ctx.isTooltipVisible());
    EXPECT_TRUE(ctx.isPaintDirty());
    pctx.clear();
    ctx.render(pctx, core::Size{400.0f, 300.0f});
    EXPECT_FALSE(ctx.isPaintDirty());
    EXPECT_EQ(rootPtr->paintCount, 1);
}
