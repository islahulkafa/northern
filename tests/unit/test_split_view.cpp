#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/containers/split_view.hpp"
#include <gtest/gtest.h>
#include <variant>

using namespace northern;
using namespace northern::widgets;

TEST(SplitViewTest, DefaultsAndSetters) {
    SplitView sv;
    EXPECT_EQ(sv.orientation(), SplitOrientation::Horizontal);
    EXPECT_EQ(sv.splitStyle(), SplitStyle::Hairline);
    EXPECT_FLOAT_EQ(sv.splitRatio(), 0.5f);
    EXPECT_FLOAT_EQ(sv.minPaneSize(), 40.0f);
    EXPECT_FLOAT_EQ(sv.sashThickness(), 1.0f);
    EXPECT_FLOAT_EQ(sv.grabMargin(), 4.0f);

    float reportedRatio = 0.0f;
    int emitCount = 0;
    sv.onSplitChanged.connect([&](float r) {
        reportedRatio = r;
        ++emitCount;
    });

    sv.setSplitRatio(0.35f);
    EXPECT_FLOAT_EQ(sv.splitRatio(), 0.35f);
    EXPECT_FLOAT_EQ(reportedRatio, 0.35f);
    EXPECT_EQ(emitCount, 1);

    // Redundant setter does not re-emit
    sv.setSplitRatio(0.35f);
    EXPECT_EQ(emitCount, 1);

    // Clamping
    sv.setSplitRatio(1.5f);
    EXPECT_FLOAT_EQ(sv.splitRatio(), 0.99f);

    sv.setSplitRatio(-0.5f);
    EXPECT_FLOAT_EQ(sv.splitRatio(), 0.01f);
}

TEST(SplitViewTest, PanesAndLayout) {
    SplitView sv;
    auto pane1 = std::make_unique<Container>();
    auto pane2 = std::make_unique<Container>();

    Container* p1 = pane1.get();
    Container* p2 = pane2.get();

    sv.setPanes(std::move(pane1), std::move(pane2));
    EXPECT_EQ(sv.firstPane(), p1);
    EXPECT_EQ(sv.secondPane(), p2);
    EXPECT_EQ(sv.childCount(), 2u);

    // Check style assignment
    EXPECT_FLOAT_EQ(p1->style().flexGrow, 0.5f);
    EXPECT_FLOAT_EQ(p2->style().flexGrow, 0.5f);

    sv.setSplitRatio(0.7f);
    EXPECT_FLOAT_EQ(p1->style().flexGrow, 0.7f);
    EXPECT_FLOAT_EQ(p2->style().flexGrow, 0.3f);
}

TEST(SplitViewTest, SashHoverAndCursorShape) {
    SplitView sv(SplitOrientation::Horizontal, 0.5f);
    sv.layoutNode().setLayoutRect(core::Rect{core::Point{0.0f, 0.0f}, core::Size{200.0f, 100.0f}});

    core::Rect sash = sv.sashLocalRect();
    EXPECT_NEAR(sash.origin.x, 99.5f, 4.0f); // ~ (200 - 1) * 0.5 = 99.5
    EXPECT_FLOAT_EQ(sash.size.width, 1.0f);

    // Hover outside sash
    WidgetMouseEvent moveOut{core::Point{10.0f, 50.0f}, platform::MouseButton::None,
                             platform::KeyModifier::None};
    sv.onMouseMove(moveOut);
    EXPECT_EQ(sv.cursorShape(), platform::CursorShape::Arrow);

    // Hover on sash
    WidgetMouseEvent moveOn{core::Point{sash.origin.x + 2.0f, 50.0f}, platform::MouseButton::None,
                            platform::KeyModifier::None};
    sv.onMouseMove(moveOn);
    EXPECT_EQ(sv.cursorShape(), platform::CursorShape::ResizeEW);

    // Switch to vertical
    sv.setOrientation(SplitOrientation::Vertical);
    sv.layoutNode().setLayoutRect(core::Rect{core::Point{0.0f, 0.0f}, core::Size{100.0f, 200.0f}});
    core::Rect vSash = sv.sashLocalRect();
    WidgetMouseEvent moveOnV{core::Point{50.0f, vSash.origin.y + 2.0f}, platform::MouseButton::None,
                             platform::KeyModifier::None};
    sv.onMouseMove(moveOnV);
    EXPECT_EQ(sv.cursorShape(), platform::CursorShape::ResizeNS);
}

TEST(SplitViewTest, MouseDragUpdatesRatio) {
    WidgetContext ctx;
    auto svUnique = std::make_unique<SplitView>(SplitOrientation::Horizontal, 0.5f);
    SplitView* sv = svUnique.get();

    auto p1 = std::make_unique<Container>();
    auto p2 = std::make_unique<Container>();
    sv->setPanes(std::move(p1), std::move(p2));

    ctx.setRootWidget(std::move(svUnique));
    ctx.handleResize(core::Size{400.0f, 200.0f});

    core::Rect sash = sv->sashLocalRect();

    // Mouse down on sash
    WidgetMouseEvent downEv{core::Point{sash.origin.x + 2.0f, 50.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    sv->onMouseDown(downEv);
    EXPECT_TRUE(downEv.isAccepted());

    // Drag to right (e.g. x = 300)
    WidgetMouseEvent dragEv{core::Point{300.0f, 50.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    sv->onMouseMove(dragEv);
    EXPECT_TRUE(dragEv.isAccepted());
    EXPECT_GT(sv->splitRatio(), 0.5f);

    // Mouse up
    WidgetMouseEvent upEv{core::Point{300.0f, 50.0f}, platform::MouseButton::Left,
                          platform::KeyModifier::None};
    sv->onMouseUp(upEv);
    EXPECT_TRUE(upEv.isAccepted());
}

TEST(SplitViewTest, DualSizingModesAndFixedResizePreservation) {
    WidgetContext ctx;
    auto svUnique = std::make_unique<SplitView>(SplitOrientation::Horizontal);
    SplitView* sv = svUnique.get();

    auto p1 = std::make_unique<Container>();
    auto p2 = std::make_unique<Container>();
    Container* p1Ptr = p1.get();
    Container* p2Ptr = p2.get();
    sv->setPanes(std::move(p1), std::move(p2));

    sv->setSizeMode(SplitSizeMode::FixedFirst);
    sv->setFixedPaneSize(200.0f);

    ctx.setRootWidget(std::move(svUnique));
    ctx.handleResize(core::Size{800.0f, 400.0f});

    EXPECT_FLOAT_EQ(p1Ptr->layoutRect().size.width, 200.0f);
    EXPECT_FLOAT_EQ(p2Ptr->layoutRect().size.width, 800.0f - 200.0f - sv->sashThickness());

    // Resize window wider: first pane stays 200px, second pane expands
    ctx.handleResize(core::Size{1200.0f, 400.0f});
    EXPECT_FLOAT_EQ(p1Ptr->layoutRect().size.width, 200.0f);
    EXPECT_FLOAT_EQ(p2Ptr->layoutRect().size.width, 1200.0f - 200.0f - sv->sashThickness());

    // Switch to FixedSecond
    sv->setSizeMode(SplitSizeMode::FixedSecond);
    sv->setFixedPaneSize(300.0f);
    ctx.handleResize(core::Size{1000.0f, 400.0f});
    EXPECT_FLOAT_EQ(p2Ptr->layoutRect().size.width, 300.0f);
    EXPECT_FLOAT_EQ(p1Ptr->layoutRect().size.width, 1000.0f - 300.0f - sv->sashThickness());
}

TEST(SplitViewTest, PerPaneMinMaxConstraints) {
    WidgetContext ctx;
    auto svUnique = std::make_unique<SplitView>(SplitOrientation::Horizontal);
    SplitView* sv = svUnique.get();

    sv->setPanes(std::make_unique<Container>(), std::make_unique<Container>());
    sv->setSizeMode(SplitSizeMode::FixedFirst);
    sv->setMinFirstPaneSize(120.0f);
    sv->setMaxFirstPaneSize(350.0f);
    sv->setMinSecondPaneSize(200.0f);

    ctx.setRootWidget(std::move(svUnique));
    ctx.handleResize(core::Size{800.0f, 400.0f});

    sv->setFirstPaneSize(50.0f);
    EXPECT_FLOAT_EQ(sv->firstPaneSize(), 120.0f);

    sv->setFirstPaneSize(500.0f);
    EXPECT_FLOAT_EQ(sv->firstPaneSize(), 350.0f);

    // Shrink window so maxFirst is bounded by minSecondPaneSize (500 - 6 - 200 = 294)
    ctx.handleResize(core::Size{500.0f, 400.0f});
    sv->setFirstPaneSize(350.0f);
    EXPECT_FLOAT_EQ(sv->firstPaneSize(), 500.0f - sv->sashThickness() - 200.0f);
}

TEST(SplitViewTest, EdgeSnappingCollapseAndRestore) {
    WidgetContext ctx;
    auto svUnique = std::make_unique<SplitView>(SplitOrientation::Horizontal, 0.5f);
    SplitView* sv = svUnique.get();

    auto p1 = std::make_unique<Container>();
    auto p2 = std::make_unique<Container>();
    Container* p1Ptr = p1.get();
    Container* p2Ptr = p2.get();
    sv->setPanes(std::move(p1), std::move(p2));
    sv->setSnapThreshold(30.0f);

    bool collapseSignalReceived = false;
    bool signalIsFirst = false;
    bool signalIsCollapsed = false;
    sv->onCollapseChanged.connect([&](bool isFirst, bool collapsed) {
        collapseSignalReceived = true;
        signalIsFirst = isFirst;
        signalIsCollapsed = collapsed;
    });

    ctx.setRootWidget(std::move(svUnique));
    ctx.handleResize(core::Size{400.0f, 200.0f});

    core::Rect sash = sv->sashLocalRect();
    WidgetMouseEvent downEv{core::Point{sash.origin.x + 2.0f, 50.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    sv->onMouseDown(downEv);
    EXPECT_TRUE(downEv.isAccepted());

    // Drag inside snapThreshold (x = 10 < 30)
    WidgetMouseEvent dragSnapEv{core::Point{10.0f, 50.0f}, platform::MouseButton::Left,
                                platform::KeyModifier::None};
    sv->onMouseMove(dragSnapEv);
    EXPECT_TRUE(sv->isFirstPaneCollapsed());
    EXPECT_TRUE(collapseSignalReceived);
    EXPECT_TRUE(signalIsFirst);
    EXPECT_TRUE(signalIsCollapsed);
    EXPECT_FALSE(p1Ptr->isVisible());
    EXPECT_TRUE(p2Ptr->isVisible());

    WidgetMouseEvent upEv{core::Point{10.0f, 50.0f}, platform::MouseButton::Left,
                          platform::KeyModifier::None};
    sv->onMouseUp(upEv);

    // Sash should be at left edge x = 0
    EXPECT_FLOAT_EQ(sv->sashLocalRect().origin.x, 0.0f);

    // Expand restores previous ratio
    sv->expandPane(false);
    EXPECT_FALSE(sv->isFirstPaneCollapsed());
    EXPECT_TRUE(p1Ptr->isVisible());
    EXPECT_FLOAT_EQ(sv->splitRatio(), 0.5f);
}

TEST(SplitViewTest, AnimatedCollapseTransitions) {
    WidgetContext ctx;
    auto svUnique = std::make_unique<SplitView>(SplitOrientation::Horizontal, 0.4f);
    SplitView* sv = svUnique.get();

    auto p1 = std::make_unique<Container>();
    auto p2 = std::make_unique<Container>();
    Container* p1Ptr = p1.get();
    sv->setPanes(std::move(p1), std::move(p2));

    ctx.setRootWidget(std::move(svUnique));
    ctx.handleResize(core::Size{500.0f, 300.0f});

    sv->collapseFirstPane(true);
    EXPECT_TRUE(sv->isFirstPaneCollapsed());
    EXPECT_TRUE(sv->hasActiveAnimations());

    // Advance halfway
    sv->updateAnimations(0.10f);
    EXPECT_TRUE(sv->hasActiveAnimations());
    EXPECT_TRUE(p1Ptr->isVisible());

    // Advance to completion (> 0.22s)
    sv->updateAnimations(0.15f);
    EXPECT_FALSE(sv->hasActiveAnimations());
    EXPECT_FALSE(p1Ptr->isVisible());

    // Animated expand
    sv->expandPane(true);
    EXPECT_FALSE(sv->isFirstPaneCollapsed());
    EXPECT_TRUE(sv->hasActiveAnimations());
    EXPECT_TRUE(p1Ptr->isVisible());

    sv->updateAnimations(0.25f);
    EXPECT_FALSE(sv->hasActiveAnimations());
    EXPECT_FLOAT_EQ(sv->splitRatio(), 0.4f);
}

TEST(SplitViewTest, KeyboardNavigationAndToggle) {
    WidgetContext ctx;
    auto svUnique = std::make_unique<SplitView>(SplitOrientation::Horizontal);
    SplitView* sv = svUnique.get();
    sv->setPanes(std::make_unique<Container>(), std::make_unique<Container>());
    sv->setSizeMode(SplitSizeMode::FixedFirst);
    sv->setFirstPaneSize(200.0f);

    ctx.setRootWidget(std::move(svUnique));
    ctx.handleResize(core::Size{800.0f, 400.0f});

    EXPECT_TRUE(sv->isFocusable());

    // Right arrow nudges by 8px
    WidgetKeyEvent rightEv{platform::Key::Right, true, platform::KeyModifier::None};
    sv->onKeyDown(rightEv);
    EXPECT_TRUE(rightEv.isAccepted());
    EXPECT_FLOAT_EQ(sv->firstPaneSize(), 208.0f);

    // Shift + Left arrow nudges by -32px
    WidgetKeyEvent shiftLeftEv{platform::Key::Left, true, platform::KeyModifier::Shift};
    sv->onKeyDown(shiftLeftEv);
    EXPECT_TRUE(shiftLeftEv.isAccepted());
    EXPECT_FLOAT_EQ(sv->firstPaneSize(), 176.0f);

    // Enter toggles collapse
    WidgetKeyEvent enterEv{platform::Key::Enter, true, platform::KeyModifier::None};
    sv->onKeyDown(enterEv);
    EXPECT_TRUE(enterEv.isAccepted());
    EXPECT_TRUE(sv->isFirstPaneCollapsed());

    // Space toggles back to expanded
    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    sv->onKeyDown(spaceEv);
    EXPECT_TRUE(spaceEv.isAccepted());
    EXPECT_FALSE(sv->isFirstPaneCollapsed());
}

TEST(SplitViewTest, SignalsAndGripGeometry) {
    WidgetContext ctx;
    auto svUnique = std::make_unique<SplitView>(SplitOrientation::Horizontal, 0.5f);
    SplitView* sv = svUnique.get();
    sv->setPanes(std::make_unique<Container>(), std::make_unique<Container>());

    float lastFirstSize = 0.0f;
    float lastSecondSize = 0.0f;
    int resizeCount = 0;
    sv->onPaneResized.connect([&](float s1, float s2) {
        lastFirstSize = s1;
        lastSecondSize = s2;
        ++resizeCount;
    });

    ctx.setRootWidget(std::move(svUnique));
    ctx.handleResize(core::Size{401.0f, 200.0f}); // avail = 400.0f with 1px Hairline sash

    sv->setSplitRatio(0.25f);
    EXPECT_EQ(resizeCount, 1);
    EXPECT_FLOAT_EQ(lastFirstSize, 100.0f);
    EXPECT_FLOAT_EQ(lastSecondSize, 300.0f);

    core::Rect grip = sv->gripScreenRect();
    EXPECT_FLOAT_EQ(grip.size.width, 4.0f);
    EXPECT_FLOAT_EQ(grip.size.height, 24.0f);

    renderer::PaintContext paintCtx;
    sv->paint(paintCtx);
    EXPECT_FALSE(paintCtx.commands().empty());
}

TEST(SplitViewTest, SplitStyleAndDpiScaling) {
    SplitView sv;
    sv.setPanes(std::make_unique<Container>(), std::make_unique<Container>());
    EXPECT_EQ(sv.splitStyle(), SplitStyle::Hairline);
    EXPECT_FLOAT_EQ(sv.sashThickness(), 1.0f);
    EXPECT_FLOAT_EQ(sv.grabMargin(), 4.0f);

    sv.setDpiScale(1.5f);
    EXPECT_FLOAT_EQ(sv.dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(sv.sashThickness(), 1.5f);
    EXPECT_FLOAT_EQ(sv.grabMargin(), 6.0f);

    sv.setSplitStyle(SplitStyle::Classic);
    EXPECT_EQ(sv.splitStyle(), SplitStyle::Classic);
    EXPECT_FLOAT_EQ(sv.sashThickness(), 9.0f);
    EXPECT_FLOAT_EQ(sv.grabMargin(), 3.0f);

    sv.layoutNode().setLayoutRect(core::Rect{{0.0f, 0.0f}, {300.0f, 200.0f}});
    renderer::PaintContext paintCtx;
    sv.paint(paintCtx);
    EXPECT_FALSE(paintCtx.commands().empty());
}

TEST(SplitViewTest, DisabledReleasesGrabClearsHoverAndHidesFocusRing) {
    WidgetContext ctx;
    auto svUnique = std::make_unique<SplitView>(SplitOrientation::Horizontal, 0.5f);
    SplitView* sv = svUnique.get();
    sv->setPanes(std::make_unique<Container>(), std::make_unique<Container>());
    ctx.setRootWidget(std::move(svUnique));
    ctx.handleResize(core::Size{401.0f, 200.0f});

    core::Rect sash = sv->sashLocalRect();
    platform::MouseEvent moveEv{};
    moveEv.type = platform::MouseEventType::Move;
    moveEv.physicalPosition = {sash.origin.x + 1.0f, 50.0f};
    moveEv.position = moveEv.physicalPosition;
    ctx.handleMouseMove(moveEv);
    EXPECT_EQ(sv->cursorShape(), platform::CursorShape::ResizeEW);

    platform::MouseEvent downEv{};
    downEv.type = platform::MouseEventType::ButtonDown;
    downEv.button = platform::MouseButton::Left;
    downEv.physicalPosition = moveEv.physicalPosition;
    downEv.position = moveEv.physicalPosition;
    ctx.handleMouseDown(downEv);
    EXPECT_EQ(ctx.mouseGrabWidget(), sv);

    sv->setFocus(FocusReason::Keyboard);
    sv->setEnabled(false);

    EXPECT_EQ(ctx.mouseGrabWidget(), nullptr);
    EXPECT_EQ(sv->cursorShape(), platform::CursorShape::Arrow);

    float ratioBefore = sv->splitRatio();
    WidgetKeyEvent rightEv{platform::Key::Right, true, platform::KeyModifier::None};
    sv->onKeyDown(rightEv);
    EXPECT_FALSE(rightEv.isAccepted());
    EXPECT_FLOAT_EQ(sv->splitRatio(), ratioBefore);

    // Disabled SplitView emits only the single muted hairline (no grip or focus ring)
    renderer::PaintContext paintCtx;
    sv->paint(paintCtx);
    EXPECT_EQ(paintCtx.commands().size(), 1u);
    EXPECT_TRUE(std::holds_alternative<renderer::DrawLineCommand>(paintCtx.commands()[0]));
}

TEST(SplitViewTest, ContextAttachmentAdoptsDpiPreservesCustomSashAndScalesKeyStep) {
    WidgetContext ctx;
    ctx.setDpiScale(1.5f);

    auto svUnique = std::make_unique<SplitView>(SplitOrientation::Horizontal);
    SplitView* sv = svUnique.get();
    sv->setSizeMode(SplitSizeMode::FixedFirst);
    sv->setFirstPaneSize(200.0f);
    sv->setPanes(std::make_unique<Container>(), std::make_unique<Container>());
    ctx.setRootWidget(std::move(svUnique));
    ctx.handleResize(core::Size{800.0f, 400.0f});

    EXPECT_FLOAT_EQ(sv->dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(sv->sashThickness(), 1.5f);
    EXPECT_FLOAT_EQ(sv->grabMargin(), 6.0f);

    // Keyboard nudge step scales by 1.5x (8 * 1.5 = 12px)
    WidgetKeyEvent rightEv{platform::Key::Right, true, platform::KeyModifier::None};
    sv->onKeyDown(rightEv);
    EXPECT_TRUE(rightEv.isAccepted());
    EXPECT_FLOAT_EQ(sv->firstPaneSize(), 212.0f);

    // Explicit sashThickness/grabMargin overrides survive later DPI changes
    sv->setSashThickness(3.0f);
    sv->setGrabMargin(7.0f);
    sv->setDpiScale(2.0f);
    EXPECT_FLOAT_EQ(sv->sashThickness(), 3.0f);
    EXPECT_FLOAT_EQ(sv->grabMargin(), 7.0f);
}

TEST(SplitViewTest, SameMillisecondDoubleClickTogglesCollapseAndThemeMarksDirty) {
    WidgetContext ctx;
    auto svUnique = std::make_unique<SplitView>(SplitOrientation::Horizontal, 0.5f);
    SplitView* sv = svUnique.get();
    sv->setPanes(std::make_unique<Container>(), std::make_unique<Container>());
    ctx.setRootWidget(std::move(svUnique));
    ctx.handleResize(core::Size{401.0f, 200.0f});

    core::Rect sash = sv->sashLocalRect();
    WidgetMouseEvent click1{core::Point{sash.origin.x, 50.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    WidgetMouseEvent click2{core::Point{sash.origin.x, 50.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    sv->onMouseDown(click1);
    sv->onMouseUp(click1);
    sv->onMouseDown(click2);
    EXPECT_TRUE(sv->isFirstPaneCollapsed());

    renderer::PaintContext pctx;
    ctx.render(pctx, {401.0f, 200.0f});
    EXPECT_FALSE(sv->isPaintDirty());

    ctx.setTheme(theme::Theme::light());
    EXPECT_TRUE(sv->isPaintDirty());
}
