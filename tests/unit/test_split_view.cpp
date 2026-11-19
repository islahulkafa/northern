#include "widgets/container.hpp"
#include "widgets/split_view.hpp"
#include "widgets/widget_context.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(SplitViewTest, DefaultsAndSetters) {
    SplitView sv;
    EXPECT_EQ(sv.orientation(), SplitOrientation::Horizontal);
    EXPECT_FLOAT_EQ(sv.splitRatio(), 0.5f);
    EXPECT_FLOAT_EQ(sv.minPaneSize(), 40.0f);
    EXPECT_FLOAT_EQ(sv.sashThickness(), 6.0f);

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
    EXPECT_NEAR(sash.origin.x, 97.0f, 4.0f); // ~ (200 - 6) * 0.5 = 97
    EXPECT_FLOAT_EQ(sash.size.width, 6.0f);

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
