#include "platform/clipboard.hpp"
#include "text/font_manager.hpp"
#include "text/text_shaper.hpp"
#include "widgets/button.hpp"
#include "widgets/checkbox.hpp"
#include "widgets/container.hpp"
#include "widgets/label.hpp"
#include "widgets/scroll_view.hpp"
#include "widgets/slider.hpp"
#include "widgets/text_input.hpp"
#include "widgets/theme.hpp"
#include "widgets/widget.hpp"
#include "widgets/widget_context.hpp"

#include <gtest/gtest.h>

namespace northern::widgets {
namespace {

class DestructionTrackingWidget : public Widget {
public:
    explicit DestructionTrackingWidget(int* counter) : counter_(counter) {}
    ~DestructionTrackingWidget() override {
        if (counter_) {
            (*counter_)++;
        }
    }

private:
    int* counter_{nullptr};
};

class EventTrackingWidget : public Widget {
public:
    int enterCount{0};
    int leaveCount{0};
    int downCount{0};
    bool shouldAcceptDown{false};

    void onMouseEnter() override { enterCount++; }
    void onMouseLeave() override { leaveCount++; }
    void onMouseDown(WidgetMouseEvent& ev) override {
        downCount++;
        if (shouldAcceptDown) {
            ev.accept();
        }
    }
};

} // namespace

TEST(WidgetTest, WidgetOwnershipAndDestructionCascade) {
    int destroyedCount = 0;
    {
        auto parent = std::make_unique<DestructionTrackingWidget>(&destroyedCount);
        auto* c1 = parent->addChild(std::make_unique<DestructionTrackingWidget>(&destroyedCount));
        auto* c2 = parent->addChild(std::make_unique<DestructionTrackingWidget>(&destroyedCount));
        auto* c3 = parent->addChild(std::make_unique<DestructionTrackingWidget>(&destroyedCount));
        c1->addChild(std::make_unique<DestructionTrackingWidget>(&destroyedCount));

        EXPECT_EQ(parent->childCount(), 3u);
        EXPECT_EQ(c1->parent(), parent.get());
        EXPECT_EQ(c2->parent(), parent.get());
        EXPECT_EQ(c3->parent(), parent.get());
    }
    // 1 parent + 3 children + 1 grandchild = 5 destroyed
    EXPECT_EQ(destroyedCount, 5);
}

TEST(WidgetTest, ReparentingSafety) {
    auto p1 = std::make_unique<Container>();
    auto p2 = std::make_unique<Container>();
    auto c = std::make_unique<Container>();
    Container* childRaw = c.get();

    p1->addChild(std::move(c));
    EXPECT_EQ(childRaw->parent(), p1.get());
    EXPECT_EQ(p1->layoutNode().childCount(), 1u);

    std::unique_ptr<Widget> detached = p1->removeChild(childRaw);
    EXPECT_EQ(detached.get(), childRaw);
    EXPECT_EQ(childRaw->parent(), nullptr);
    EXPECT_EQ(p1->layoutNode().childCount(), 0u);

    p2->addChild(std::move(detached));
    EXPECT_EQ(childRaw->parent(), p2.get());
    EXPECT_EQ(p2->layoutNode().childCount(), 1u);
}

TEST(WidgetTest, DeleteLaterDeferredQueue) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    Container* rootPtr = root.get();
    auto child = std::make_unique<Container>();
    Container* childPtr = child.get();
    root->addChild(std::move(child));

    ctx.setRootWidget(std::move(root));
    EXPECT_EQ(rootPtr->childCount(), 1u);

    childPtr->deleteLater();
    EXPECT_TRUE(childPtr->isPendingDelete());
    EXPECT_EQ(rootPtr->childCount(), 1u);

    ctx.flushDeletedWidgets();
    EXPECT_EQ(rootPtr->childCount(), 0u);
}

TEST(WidgetTest, DirtyStateSeparation) {
    Container container;
    container.style().width = layout::Length::points(100.0f);
    container.style().height = layout::Length::points(100.0f);
    container.layoutNode().computeLayout(100.0f, 100.0f);
    container.clearLayoutDirty();
    container.clearPaintDirty();

    EXPECT_FALSE(container.isLayoutDirty());
    EXPECT_FALSE(container.isPaintDirty());

    // Changing background color invalidates paint only
    container.setBackgroundColor(core::Color::red());
    EXPECT_TRUE(container.isPaintDirty());
    EXPECT_FALSE(container.isLayoutDirty());

    container.clearPaintDirty();

    // Changing layout style invalidates layout and paint
    container.style().width = layout::Length::points(200.0f);
    container.markLayoutDirty();
    EXPECT_TRUE(container.isLayoutDirty());
    EXPECT_TRUE(container.isPaintDirty());
}

TEST(WidgetTest, HitTestingAndHoverTransitions) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    root->style().width = layout::Length::points(400.0f);
    root->style().height = layout::Length::points(400.0f);
    root->style().flexDirection = layout::FlexDirection::Row;

    auto w1 = std::make_unique<EventTrackingWidget>();
    w1->style().width = layout::Length::points(100.0f);
    w1->style().height = layout::Length::points(100.0f);
    EventTrackingWidget* p1 = w1.get();

    auto w2 = std::make_unique<EventTrackingWidget>();
    w2->style().width = layout::Length::points(100.0f);
    w2->style().height = layout::Length::points(100.0f);
    EventTrackingWidget* p2 = w2.get();

    root->addChild(std::move(w1));
    root->addChild(std::move(w2));
    ctx.setRootWidget(std::move(root));
    ctx.handleResize(core::Size{400.0f, 400.0f});

    // Move onto w1 (50, 50)
    platform::MouseEvent ev1{};
    ev1.position = core::Point{50.0f, 50.0f};
    ctx.handleMouseMove(ev1);

    EXPECT_EQ(ctx.hoveredWidget(), p1);
    EXPECT_EQ(p1->enterCount, 1);
    EXPECT_EQ(p1->leaveCount, 0);

    // Move onto w2 (150, 50)
    platform::MouseEvent ev2{};
    ev2.position = core::Point{150.0f, 50.0f};
    ctx.handleMouseMove(ev2);

    EXPECT_EQ(ctx.hoveredWidget(), p2);
    EXPECT_EQ(p1->leaveCount, 1);
    EXPECT_EQ(p2->enterCount, 1);

    // Move outside children onto root background (350, 50)
    platform::MouseEvent ev3{};
    ev3.position = core::Point{350.0f, 50.0f};
    ctx.handleMouseMove(ev3);

    EXPECT_EQ(ctx.hoveredWidget(), ctx.rootWidget());
    EXPECT_EQ(p2->leaveCount, 1);
}

TEST(WidgetTest, EventBubbling) {
    WidgetContext ctx;
    auto parent = std::make_unique<EventTrackingWidget>();
    parent->style().width = layout::Length::points(200.0f);
    parent->style().height = layout::Length::points(200.0f);
    EventTrackingWidget* parentPtr = parent.get();

    auto child = std::make_unique<EventTrackingWidget>();
    child->style().width = layout::Length::points(100.0f);
    child->style().height = layout::Length::points(100.0f);
    child->shouldAcceptDown = false; // unaccepted, should bubble
    EventTrackingWidget* childPtr = child.get();

    parent->addChild(std::move(child));
    ctx.setRootWidget(std::move(parent));
    ctx.handleResize(core::Size{200.0f, 200.0f});

    platform::MouseEvent evDown{};
    evDown.button = platform::MouseButton::Left;
    evDown.position = core::Point{50.0f, 50.0f};
    ctx.handleMouseDown(evDown);

    EXPECT_EQ(childPtr->downCount, 1);
    EXPECT_EQ(parentPtr->downCount, 1); // Bubbled up to parent!

    // Now test with event accepted
    childPtr->shouldAcceptDown = true;
    ctx.handleMouseDown(evDown);

    EXPECT_EQ(childPtr->downCount, 2);
    EXPECT_EQ(parentPtr->downCount, 1); // Not bubbled because accepted!
}

TEST(WidgetTest, ButtonClickSignal) {
    WidgetContext ctx;
    auto button = std::make_unique<Button>("Click Me");
    button->style().width = layout::Length::points(120.0f);
    button->style().height = layout::Length::points(40.0f);
    Button* btnPtr = button.get();

    ctx.setRootWidget(std::move(button));
    ctx.handleResize(core::Size{120.0f, 40.0f});

    int clickCount = 0;
    btnPtr->onClicked.connect([&]() { clickCount++; });

    // Hover over button
    platform::MouseEvent evMove{};
    evMove.position = core::Point{60.0f, 20.0f};
    ctx.handleMouseMove(evMove);
    EXPECT_EQ(btnPtr->state(), ButtonState::Hovered);

    // Press down
    platform::MouseEvent evDown{};
    evDown.button = platform::MouseButton::Left;
    evDown.position = core::Point{60.0f, 20.0f};
    ctx.handleMouseDown(evDown);
    EXPECT_EQ(btnPtr->state(), ButtonState::Pressed);

    // Release up
    platform::MouseEvent evUp{};
    evUp.button = platform::MouseButton::Left;
    evUp.position = core::Point{60.0f, 20.0f};
    ctx.handleMouseUp(evUp);
    EXPECT_EQ(btnPtr->state(), ButtonState::Hovered);
    EXPECT_EQ(clickCount, 1);
}

TEST(WidgetTest, LabelIntrinsicMeasurement) {
    auto fontRes = text::FontManager::instance().defaultFont();
    ASSERT_TRUE(fontRes.isOk());
    auto font = fontRes.value();
    ASSERT_NE(font, nullptr);

    Label label("Short", font);
    label.layoutNode().computeLayout(0.0f, 0.0f);
    float shortWidth = label.layoutRect().size.width;
    EXPECT_GT(shortWidth, 0.0f);

    label.setText("A substantially longer string of text to measure");
    EXPECT_TRUE(label.isLayoutDirty());

    label.layoutNode().computeLayout(0.0f, 0.0f);
    float longWidth = label.layoutRect().size.width;
    EXPECT_GT(longWidth, shortWidth);
}

TEST(WidgetTest, TextInputCursorAndTyping) {
    auto fontRes = text::FontManager::instance().defaultFont();
    ASSERT_TRUE(fontRes.isOk());

    TextInput input("Type here...", fontRes.value());
    input.setFocus();

    // Type 'H', 'i'
    WidgetKeyEvent ev1{platform::Key::H, true, platform::KeyModifier::None, 'H'};
    input.onKeyDown(ev1);
    WidgetKeyEvent ev2{platform::Key::I, true, platform::KeyModifier::None, 'i'};
    input.onKeyDown(ev2);

    EXPECT_EQ(input.text(), "Hi");
    EXPECT_EQ(input.cursorIndex(), 2u);

    // Left arrow moves cursor
    WidgetKeyEvent evLeft{platform::Key::Left, true, platform::KeyModifier::None};
    input.onKeyDown(evLeft);
    EXPECT_EQ(input.cursorIndex(), 1u);

    // Backspace deletes 'H'
    WidgetKeyEvent evBs{platform::Key::Backspace, true, platform::KeyModifier::None};
    input.onKeyDown(evBs);
    EXPECT_EQ(input.text(), "i");
    EXPECT_EQ(input.cursorIndex(), 0u);
}

TEST(WidgetTest, TextInputSelectionAndClipboard) {
    auto fontRes = text::FontManager::instance().defaultFont();
    ASSERT_TRUE(fontRes.isOk());

    TextInput input("", fontRes.value());
    input.setText("Hello World");
    input.setFocus();

    input.selectAll();
    EXPECT_TRUE(input.hasSelection());
    EXPECT_EQ(input.selectedText(), "Hello World");

    // Copy to clipboard
    WidgetKeyEvent evCopy{platform::Key::C, true, platform::KeyModifier::Control};
    input.onKeyDown(evCopy);

    auto clipRes = platform::Clipboard::getText();
    ASSERT_TRUE(clipRes.isOk());
    EXPECT_EQ(clipRes.value(), "Hello World");

    // Type 'X' replaces entire selection
    WidgetKeyEvent evX{platform::Key::X, true, platform::KeyModifier::None, 'X'};
    input.onKeyDown(evX);
    EXPECT_EQ(input.text(), "X");
    EXPECT_EQ(input.cursorIndex(), 1u);
}

TEST(WidgetTest, ScrollViewWheelAndClamping) {
    ScrollView scroll;
    scroll.style().width = layout::Length::points(200.0f);
    scroll.style().height = layout::Length::points(100.0f);

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(200.0f);
    content->style().height = layout::Length::points(400.0f);
    scroll.addChild(std::move(content));

    scroll.layoutNode().computeLayout(200.0f, 100.0f);

    EXPECT_FLOAT_EQ(scroll.maxScrollOffset().y, 300.0f);
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 0.0f);

    // Wheel down
    WidgetMouseEvent evWheel{core::Point{50.0f, 50.0f}, platform::MouseButton::None,
                             platform::KeyModifier::None, core::Point{0.0f, -2.0f}};
    scroll.onMouseWheel(evWheel);
    EXPECT_GT(scroll.scrollOffset().y, 0.0f);

    // Scroll beyond max clamps to 300
    scroll.scrollTo(core::Point{0.0f, 1000.0f});
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 300.0f);

    // Scroll negative clamps to 0
    scroll.scrollTo(core::Point{0.0f, -50.0f});
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 0.0f);
}

TEST(WidgetTest, ScrollViewHitTestingOffset) {
    WidgetContext ctx;
    auto scroll = std::make_unique<ScrollView>();
    scroll->style().width = layout::Length::points(200.0f);
    scroll->style().height = layout::Length::points(200.0f);

    auto child1 = std::make_unique<Container>();
    child1->style().width = layout::Length::points(200.0f);
    child1->style().height = layout::Length::points(150.0f);
    Container* c1 = child1.get();

    auto child2 = std::make_unique<Container>();
    child2->style().width = layout::Length::points(200.0f);
    child2->style().height = layout::Length::points(250.0f);
    Container* c2 = child2.get();

    scroll->addChild(std::move(child1));
    scroll->addChild(std::move(child2));
    ScrollView* scrollPtr = scroll.get();

    ctx.setRootWidget(std::move(scroll));
    ctx.handleResize(core::Size{200.0f, 200.0f});

    // Point (50, 50) hits child1
    EXPECT_EQ(ctx.hitTest(core::Point{50.0f, 50.0f}), c1);

    // Scroll down by 160px so child2 moves into top of viewport
    scrollPtr->scrollTo(core::Point{0.0f, 160.0f});

    // Point (50, 50) now hits child2!
    EXPECT_EQ(ctx.hitTest(core::Point{50.0f, 50.0f}), c2);
}

TEST(WidgetTest, CheckboxToggleSignal) {
    Checkbox cb("Option 1", false);
    bool toggledState = false;
    int toggleCount = 0;
    cb.onToggled.connect([&](bool s) {
        toggledState = s;
        toggleCount++;
    });

    EXPECT_FALSE(cb.isChecked());

    cb.toggle();
    EXPECT_TRUE(cb.isChecked());
    EXPECT_TRUE(toggledState);
    EXPECT_EQ(toggleCount, 1);

    // Mouse click toggles
    WidgetMouseEvent evDown{core::Point{5.0f, 5.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    cb.onMouseDown(evDown);
    WidgetMouseEvent evUp{core::Point{5.0f, 5.0f}, platform::MouseButton::Left,
                          platform::KeyModifier::None};
    cb.onMouseUp(evUp);

    EXPECT_FALSE(cb.isChecked());
    EXPECT_FALSE(toggledState);
    EXPECT_EQ(toggleCount, 2);
}

TEST(WidgetTest, SliderValueAndDrag) {
    Slider slider(0.0f, 100.0f, 25.0f);
    float lastVal = 0.0f;
    int valCount = 0;
    slider.onValueChanged.connect([&](float v) {
        lastVal = v;
        valCount++;
    });

    EXPECT_FLOAT_EQ(slider.value(), 25.0f);

    slider.setValue(75.0f);
    EXPECT_FLOAT_EQ(slider.value(), 75.0f);
    EXPECT_FLOAT_EQ(lastVal, 75.0f);
    EXPECT_EQ(valCount, 1);

    // Clamp limits
    slider.setValue(150.0f);
    EXPECT_FLOAT_EQ(slider.value(), 100.0f);
    slider.setValue(-20.0f);
    EXPECT_FLOAT_EQ(slider.value(), 0.0f);
}

TEST(WidgetTest, ButtonVariantsAndThemeTokens) {
    EXPECT_EQ(theme::ColorTokens::AccentDefault, (core::Color{0.145f, 0.388f, 0.922f, 1.0f}));
    EXPECT_FLOAT_EQ(theme::Metrics::ControlHeightStandard, 32.0f);
    EXPECT_FLOAT_EQ(theme::Metrics::CornerRadiusControl, 6.0f);

    Button btn("Submit", nullptr, ButtonVariant::Primary);
    EXPECT_EQ(btn.variant(), ButtonVariant::Primary);

    btn.setVariant(ButtonVariant::Secondary);
    EXPECT_EQ(btn.variant(), ButtonVariant::Secondary);

    btn.setVariant(ButtonVariant::Ghost);
    EXPECT_EQ(btn.variant(), ButtonVariant::Ghost);

    btn.setVariant(ButtonVariant::Destructive);
    EXPECT_EQ(btn.variant(), ButtonVariant::Destructive);

    // Verify focus ring painting does not throw or crash
    renderer::PaintContext pctx;
    btn.style().width = layout::Length::points(100.0f);
    btn.style().height = layout::Length::points(32.0f);
    btn.layoutNode().computeLayout(100.0f, 32.0f);
    btn.setFocus();
    EXPECT_TRUE(btn.hasFocus());
    btn.paint(pctx);
    EXPECT_FALSE(pctx.commands().empty());
}

} // namespace northern::widgets
