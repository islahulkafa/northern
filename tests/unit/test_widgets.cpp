#include "platform/clipboard.hpp"
#include "renderer/svg/svg_document.hpp"
#include "text/font_manager.hpp"
#include "text/text_shaper.hpp"
#include "widgets/controls/button.hpp"
#include "widgets/controls/checkbox.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/controls/label.hpp"
#include "widgets/containers/scroll_view.hpp"
#include "widgets/controls/slider.hpp"
#include "widgets/controls/text_input.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget.hpp"
#include "widgets/base/widget_context.hpp"

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

    // Mouse leaves window completely
    ctx.handleMouseLeave();
    EXPECT_EQ(ctx.hoveredWidget(), nullptr);
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

TEST(WidgetTest, SliderMouseDragOutsideBounds) {
    WidgetContext ctx;
    auto slider = std::make_unique<Slider>(0.0f, 100.0f, 0.0f);
    slider->style().width = layout::Length::points(200.0f);
    slider->style().height = layout::Length::points(28.0f);
    Slider* sPtr = slider.get();

    ctx.setRootWidget(std::move(slider));
    ctx.handleResize(core::Size{200.0f, 28.0f});

    EXPECT_FLOAT_EQ(sPtr->value(), 0.0f);

    // 1. Mouse down inside slider at center (100, 14)
    platform::MouseEvent evDown{};
    evDown.button = platform::MouseButton::Left;
    evDown.position = core::Point{100.0f, 14.0f};
    ctx.handleMouseDown(evDown);
    EXPECT_EQ(ctx.mouseGrabWidget(), sPtr);
    EXPECT_GT(sPtr->value(), 40.0f);
    EXPECT_LT(sPtr->value(), 60.0f);

    // 2. Mouse move far outside bounds: y is -200 (far above), x is 150
    platform::MouseEvent evMoveOutside{};
    evMoveOutside.button = platform::MouseButton::Left;
    evMoveOutside.position = core::Point{150.0f, -200.0f};
    ctx.handleMouseMove(evMoveOutside);
    EXPECT_GT(sPtr->value(), 70.0f);
    EXPECT_LT(sPtr->value(), 85.0f);

    // 3. Mouse move far to the right outside bounds (300, 500) -> clamps to max (100.0f)
    evMoveOutside.position = core::Point{300.0f, 500.0f};
    ctx.handleMouseMove(evMoveOutside);
    EXPECT_FLOAT_EQ(sPtr->value(), 100.0f);

    // 4. Mouse move far to the left outside bounds (-100, -50) -> clamps to min (0.0f)
    evMoveOutside.position = core::Point{-100.0f, -50.0f};
    ctx.handleMouseMove(evMoveOutside);
    EXPECT_FLOAT_EQ(sPtr->value(), 0.0f);

    // 5. Release mouse button while outside bounds (-100, -50)
    platform::MouseEvent evUpOutside{};
    evUpOutside.button = platform::MouseButton::Left;
    evUpOutside.position = core::Point{-100.0f, -50.0f};
    ctx.handleMouseUp(evUpOutside);
    EXPECT_EQ(ctx.mouseGrabWidget(), nullptr);

    // 6. Further mouse moves without drag should no longer affect slider
    platform::MouseEvent evMoveNoGrab{};
    evMoveNoGrab.position = core::Point{150.0f, 14.0f};
    ctx.handleMouseMove(evMoveNoGrab);
    EXPECT_FLOAT_EQ(sPtr->value(), 0.0f);
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

TEST(WidgetTest, RetainedDisplayListCachesCommands) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    root->setBackgroundColor(core::Color{0.1f, 0.1f, 0.1f, 1.0f});
    root->layoutNode().setLayoutRect({{0.0f, 0.0f}, {200.0f, 200.0f}});
    auto label = std::make_unique<Label>("Hello Northern");
    label->layoutNode().setLayoutRect({{0.0f, 0.0f}, {50.0f, 20.0f}});
    root->addChild(std::move(label));
    ctx.setRootWidget(std::move(root));

    EXPECT_TRUE(ctx.isPaintDirty());

    renderer::PaintContext p1;
    ctx.render(p1, {200.0f, 200.0f});
    EXPECT_FALSE(ctx.isPaintDirty());
    EXPECT_FALSE(p1.commands().empty());

    renderer::PaintContext p2;
    ctx.render(p2, {200.0f, 200.0f});
    EXPECT_EQ(p1.commands().size(), p2.commands().size());
}

TEST(WidgetBaseTest, CachedScreenRectCalculations) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    root->layoutNode().setLayoutRect({{10.0f, 10.0f}, {400.0f, 300.0f}});

    auto child = std::make_unique<Container>();
    child->layoutNode().setLayoutRect({{20.0f, 30.0f}, {200.0f, 150.0f}});

    auto subchild = std::make_unique<Container>();
    subchild->layoutNode().setLayoutRect({{5.0f, 5.0f}, {50.0f, 50.0f}});

    Container* subPtr = child->addChild(std::move(subchild));
    Container* childPtr = root->addChild(std::move(child));
    Container* rootPtr = root.get();
    ctx.setRootWidget(std::move(root));

    // Initial cached resolution
    EXPECT_EQ(rootPtr->screenRect(), (core::Rect{{10.0f, 10.0f}, {400.0f, 300.0f}}));
    EXPECT_EQ(childPtr->screenRect(), (core::Rect{{30.0f, 40.0f}, {200.0f, 150.0f}}));
    EXPECT_EQ(subPtr->screenRect(), (core::Rect{{35.0f, 45.0f}, {50.0f, 50.0f}}));

    // Modifying root layout and marking dirty should cascade down
    rootPtr->layoutNode().setLayoutRect({{50.0f, 60.0f}, {400.0f, 300.0f}});
    rootPtr->markLayoutDirty();

    EXPECT_EQ(rootPtr->screenRect(), (core::Rect{{50.0f, 60.0f}, {400.0f, 300.0f}}));
    EXPECT_EQ(childPtr->screenRect(), (core::Rect{{70.0f, 90.0f}, {200.0f, 150.0f}}));
    EXPECT_EQ(subPtr->screenRect(), (core::Rect{{75.0f, 95.0f}, {50.0f, 50.0f}}));

    // Test with ScrollView scrolling
    auto scrollRoot = std::make_unique<ScrollView>();
    scrollRoot->layoutNode().setLayoutRect({{0.0f, 0.0f}, {300.0f, 200.0f}});
    auto scrollChild = std::make_unique<Container>();
    scrollChild->layoutNode().setLayoutRect({{10.0f, 20.0f}, {100.0f, 500.0f}});
    Container* scChildPtr = scrollRoot->addChild(std::move(scrollChild));
    ScrollView* scRootPtr = scrollRoot.get();

    WidgetContext scrollCtx;
    scrollCtx.setRootWidget(std::move(scrollRoot));

    EXPECT_EQ(scChildPtr->screenRect(), (core::Rect{{10.0f, 20.0f}, {100.0f, 500.0f}}));
    scRootPtr->scrollTo({0.0f, 50.0f});
    EXPECT_EQ(scChildPtr->screenRect(), (core::Rect{{10.0f, -30.0f}, {100.0f, 500.0f}}));
}

TEST(WidgetBaseTest, DynamicThemeSwitching) {
    EXPECT_EQ(theme::ColorPalette::dark().mode, theme::ThemeMode::Dark);
    EXPECT_EQ(theme::ColorPalette::light().mode, theme::ThemeMode::Light);

    EXPECT_EQ(theme::ColorPalette::dark().surfaceCanvas, theme::ColorTokens::SurfaceCanvas);
    EXPECT_EQ(theme::ColorPalette::dark().accentDefault, theme::ColorTokens::AccentDefault);

    EXPECT_EQ(theme::ColorPalette::light().surfaceCanvas, (core::Color{0.957f, 0.957f, 0.961f, 1.0f}));
    EXPECT_EQ(theme::ColorPalette::light().textPrimary, (core::Color{0.035f, 0.035f, 0.043f, 1.0f}));

    WidgetContext ctx;
    EXPECT_EQ(ctx.theme().mode(), theme::ThemeMode::Dark);

    auto root = std::make_unique<Container>();
    Container* rootPtr = root.get();
    ctx.setRootWidget(std::move(root));

    EXPECT_EQ(rootPtr->theme().mode(), theme::ThemeMode::Dark);
    EXPECT_EQ(rootPtr->colorPalette().surfaceCanvas, theme::ColorTokens::SurfaceCanvas);

    int themeChangedEmissions = 0;
    ctx.onThemeChanged.connect([&](const theme::Theme& t) {
        themeChangedEmissions++;
        EXPECT_EQ(t.mode(), theme::ThemeMode::Light);
    });

    ctx.setTheme(theme::Theme::light());
    EXPECT_EQ(themeChangedEmissions, 1);
    EXPECT_EQ(ctx.theme().mode(), theme::ThemeMode::Light);
    EXPECT_EQ(rootPtr->theme().mode(), theme::ThemeMode::Light);
    EXPECT_EQ(rootPtr->colorPalette().surfaceCanvas, (core::Color{0.957f, 0.957f, 0.961f, 1.0f}));
    EXPECT_TRUE(ctx.isPaintDirty());
}

class DropTestWidget : public Widget {
public:
    bool acceptDrop{true};
    int dropCount{0};
    core::Point lastLocalPos{};
    core::Point lastScreenPos{};
    std::vector<std::string> lastPaths{};

    void onFileDrop(WidgetFileDropEvent& ev) override {
        dropCount++;
        lastLocalPos = ev.position();
        lastScreenPos = ev.screenPosition();
        lastPaths = ev.paths();
        if (acceptDrop) {
            ev.accept();
        }
    }
};

TEST(WidgetBaseTest, FileDropEventBubbling) {
    WidgetContext ctx;
    auto root = std::make_unique<DropTestWidget>();
    root->layoutNode().setLayoutRect({{0.0f, 0.0f}, {500.0f, 500.0f}});
    root->acceptDrop = true;

    auto child = std::make_unique<DropTestWidget>();
    child->layoutNode().setLayoutRect({{100.0f, 100.0f}, {200.0f, 200.0f}});
    child->acceptDrop = true;

    DropTestWidget* childPtr = child.get();
    DropTestWidget* rootPtr = root.get();
    root->addChild(std::move(child));
    ctx.setRootWidget(std::move(root));

    // 1. Drop directly on child with acceptance
    platform::FileDropEvent ev1{
        .paths = {"C:/doc.txt", "C:/img.png"},
        .position = {150.0f, 160.0f},
    };
    ctx.handleFileDrop(ev1);

    EXPECT_EQ(childPtr->dropCount, 1);
    EXPECT_EQ(childPtr->lastLocalPos, (core::Point{50.0f, 60.0f}));
    EXPECT_EQ(childPtr->lastScreenPos, (core::Point{150.0f, 160.0f}));
    EXPECT_EQ(childPtr->lastPaths.size(), 2u);
    EXPECT_EQ(rootPtr->dropCount, 0); // Not bubbled because child accepted

    // 2. Child rejects drop -> bubbles to parent
    childPtr->acceptDrop = false;
    platform::FileDropEvent ev2{
        .paths = {"C:/data.json"},
        .position = {120.0f, 130.0f},
    };
    ctx.handleFileDrop(ev2);

    EXPECT_EQ(childPtr->dropCount, 2);
    EXPECT_EQ(rootPtr->dropCount, 1);
    EXPECT_EQ(rootPtr->lastLocalPos, (core::Point{120.0f, 130.0f}));
    EXPECT_EQ(rootPtr->lastPaths.size(), 1u);
}

TEST(WidgetBaseTest, DeferredDeletionLinearScale) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    Container* rootPtr = root.get();
    ctx.setRootWidget(std::move(root));

    int destroyedCount = 0;
    constexpr int kCount = 100;
    for (int i = 0; i < kCount; ++i) {
        auto w = std::make_unique<DestructionTrackingWidget>(&destroyedCount);
        auto* ptr = rootPtr->addChild(std::move(w));
        ptr->deleteLater();
        EXPECT_TRUE(ptr->isPendingDelete());
    }

    EXPECT_EQ(rootPtr->childCount(), static_cast<size_t>(kCount));
    ctx.flushDeletedWidgets();
    EXPECT_EQ(rootPtr->childCount(), 0u);
    EXPECT_EQ(destroyedCount, kCount);
}

TEST(ButtonTest, ButtonSizeMetrics) {
    Button btnCompact("Save", nullptr, ButtonVariant::Secondary, ButtonSize::Compact);
    EXPECT_EQ(btnCompact.size(), ButtonSize::Compact);
    EXPECT_FLOAT_EQ(btnCompact.style().height.value, theme::Metrics::ControlHeightCompact);
    EXPECT_FLOAT_EQ(btnCompact.iconSize(), 14.0f);

    Button btnStandard("Save", nullptr, ButtonVariant::Secondary, ButtonSize::Standard);
    EXPECT_EQ(btnStandard.size(), ButtonSize::Standard);
    EXPECT_FLOAT_EQ(btnStandard.style().height.value, theme::Metrics::ControlHeightStandard);
    EXPECT_FLOAT_EQ(btnStandard.iconSize(), 16.0f);

    Button btnComfortable("Save", nullptr, ButtonVariant::Secondary, ButtonSize::Comfortable);
    EXPECT_EQ(btnComfortable.size(), ButtonSize::Comfortable);
    EXPECT_FLOAT_EQ(btnComfortable.style().height.value, theme::Metrics::ControlHeightComfortable);
    EXPECT_FLOAT_EQ(btnComfortable.iconSize(), 18.0f);

    // Dynamic setSize
    btnCompact.setSize(ButtonSize::Comfortable);
    EXPECT_EQ(btnCompact.size(), ButtonSize::Comfortable);
    EXPECT_FLOAT_EQ(btnCompact.style().height.value, theme::Metrics::ControlHeightComfortable);
    EXPECT_FLOAT_EQ(btnCompact.iconSize(), 18.0f);
}

TEST(ButtonTest, TextLayoutMemoization) {
    Button btn("Submit", nullptr, ButtonVariant::Primary);
    btn.style().width = layout::Length::points(120.0f);
    btn.style().height = layout::Length::points(32.0f);
    btn.layoutNode().computeLayout(120.0f, 32.0f);

    renderer::PaintContext ctx1;
    btn.paint(ctx1);

    // Second paint reuses memoized layout without crash or reallocation
    renderer::PaintContext ctx2;
    btn.paint(ctx2);
    EXPECT_EQ(ctx1.commands().size(), ctx2.commands().size());

    // Modifying text triggers layout invalidation
    btn.setText("Submit Form");
    EXPECT_TRUE(btn.isLayoutDirty());
}

TEST(ButtonTest, VectorIconPlacement) {
    auto svgRes = renderer::SvgDocument::fromString(
        "<svg viewBox='0 0 24 24'><rect x='2' y='2' width='20' height='20'/></svg>");
    ASSERT_TRUE(svgRes.isOk());
    auto svgDoc = svgRes.value();

    // 1. Icon + Text (Leading)
    Button btnLeading("Open", nullptr, ButtonVariant::Secondary);
    btnLeading.setIcon(svgDoc, IconPlacement::Leading, 16.0f);
    EXPECT_EQ(btnLeading.icon(), svgDoc);
    EXPECT_EQ(btnLeading.iconPlacement(), IconPlacement::Leading);
    EXPECT_FLOAT_EQ(btnLeading.iconSize(), 16.0f);

    // 2. Icon + Text (Trailing)
    btnLeading.setIcon(svgDoc, IconPlacement::Trailing, 16.0f);
    EXPECT_EQ(btnLeading.iconPlacement(), IconPlacement::Trailing);

    // 3. Icon-only mode
    Button btnIconOnly("", nullptr, ButtonVariant::Ghost);
    btnIconOnly.setIcon(svgDoc, IconPlacement::Leading, 16.0f);
    btnIconOnly.setSize(ButtonSize::Standard);
    core::Size measured = btnIconOnly.layoutNode().measure(100.0f, 100.0f);
    // In icon-only mode with auto width, intrinsic measurement resolves to square (32x32)
    EXPECT_FLOAT_EQ(measured.width, theme::Metrics::ControlHeightStandard);
    EXPECT_FLOAT_EQ(measured.height, theme::Metrics::ControlHeightStandard);

    btnIconOnly.layoutNode().computeLayout(32.0f, 32.0f);
    renderer::PaintContext pctx;
    btnIconOnly.paint(pctx);
    EXPECT_FALSE(pctx.commands().empty());
}

TEST(ButtonTest, DynamicThemeSwitching) {
    WidgetContext ctx;
    auto btn = std::make_unique<Button>("Click Me", nullptr, ButtonVariant::Secondary);
    Button* btnPtr = btn.get();
    ctx.setRootWidget(std::move(btn));

    // Initially dark theme
    EXPECT_EQ(ctx.theme().mode(), theme::ThemeMode::Dark);
    EXPECT_EQ(btnPtr->effectiveIdleBackgroundColor(), theme::ColorPalette::dark().surfaceRaised);
    EXPECT_EQ(btnPtr->effectiveIdleTextColor(), theme::ColorPalette::dark().textPrimary);

    // Switch context theme to light
    ctx.setTheme(theme::Theme::light());
    EXPECT_EQ(btnPtr->effectiveIdleBackgroundColor(), theme::ColorPalette::light().surfaceRaised);
    EXPECT_EQ(btnPtr->effectiveIdleTextColor(), theme::ColorPalette::light().textPrimary);

    // Manual custom color override
    btnPtr->setIdleBackgroundColor(core::Color::red());
    EXPECT_EQ(btnPtr->effectiveIdleBackgroundColor(), core::Color::red());

    // Switch back to dark theme: custom override is preserved
    ctx.setTheme(theme::Theme::dark());
    EXPECT_EQ(btnPtr->effectiveIdleBackgroundColor(), core::Color::red());
}

TEST(ButtonTest, DisabledState) {
    Button btn("Action", nullptr, ButtonVariant::Primary);
    EXPECT_EQ(btn.state(), ButtonState::Idle);

    btn.setEnabled(false);
    EXPECT_EQ(btn.state(), ButtonState::Disabled);

    int clicks = 0;
    btn.onClicked.connect([&] { clicks++; });

    // Pointer events are ignored when disabled
    WidgetMouseEvent me{core::Point{10.0f, 10.0f}, platform::MouseButton::Left,
                        platform::KeyModifier::None};
    btn.onMouseDown(me);
    btn.onMouseUp(me);
    EXPECT_EQ(clicks, 0);

    // Keyboard events are ignored when disabled
    WidgetKeyEvent ke{platform::Key::Enter, true, platform::KeyModifier::None};
    btn.onKeyDown(ke);
    EXPECT_EQ(clicks, 0);

    // Paint verifies disabled text color is applied
    renderer::PaintContext pctx;
    btn.layoutNode().computeLayout(100.0f, 32.0f);
    btn.paint(pctx);
    EXPECT_FALSE(pctx.commands().empty());
}

TEST(ButtonTest, DynamicTextResizeInContainer) {
    auto fontRes = text::FontManager::instance().defaultFont(14.0f);
    ASSERT_TRUE(fontRes.isOk());
    auto font = fontRes.value();

    auto root = std::make_unique<Container>();
    root->style().flexDirection = layout::FlexDirection::Column;
    root->style().justifyContent = layout::JustifyContent::Center;
    root->style().alignItems = layout::AlignItems::Center;
    root->style().width = layout::Length::percent(100.0f);
    root->style().height = layout::Length::percent(100.0f);

    auto btn = std::make_unique<Button>("Click Me", font, ButtonVariant::Primary,
                                        ButtonSize::Standard);
    Button* btnPtr = btn.get();
    root->addChild(std::move(btn));

    WidgetContext ctx;
    ctx.setRootWidget(std::move(root));

    renderer::PaintContext pctx1;
    ctx.render(pctx1, {1024.0f, 768.0f});

    float w1 = btnPtr->layoutRect().size.width;
    core::Rect sRect1 = btnPtr->screenRect();
    EXPECT_EQ(sRect1.size.width, w1);

    btnPtr->onClicked.connect([btnPtr]() {
        btnPtr->setText("Clicked 1 time");
    });

    platform::MouseEvent downEv{};
    downEv.type = platform::MouseEventType::ButtonDown;
    downEv.button = platform::MouseButton::Left;
    downEv.physicalPosition = {sRect1.origin.x + 5.0f, sRect1.origin.y + 5.0f};
    downEv.position = downEv.physicalPosition;
    ctx.handleMouseDown(downEv);

    platform::MouseEvent upEv{};
    upEv.type = platform::MouseEventType::ButtonUp;
    upEv.button = platform::MouseButton::Left;
    upEv.physicalPosition = {sRect1.origin.x + 5.0f, sRect1.origin.y + 5.0f};
    upEv.position = upEv.physicalPosition;
    ctx.handleMouseUp(upEv);

    renderer::PaintContext pctx2;
    ctx.render(pctx2, {1024.0f, 768.0f});

    float w2 = btnPtr->layoutRect().size.width;
    core::Rect sRect2 = btnPtr->screenRect();

    EXPECT_GT(w2, w1);
    EXPECT_FLOAT_EQ(sRect2.size.width, w2);
}

} // namespace northern::widgets
