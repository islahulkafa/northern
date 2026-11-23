#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/containers/scroll_view.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(ScrollViewTest, InitialStateAndAxisConfiguration) {
    ScrollView scroll;
    EXPECT_EQ(scroll.scrollAxis(), ScrollAxis::Vertical);
    EXPECT_FLOAT_EQ(scroll.scrollOffset().x, 0.0f);
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 0.0f);
    EXPECT_FLOAT_EQ(scroll.scrollbarThickness(), 6.0f);
    EXPECT_FLOAT_EQ(scroll.scrollbarPadding(), 3.0f);
    EXPECT_FLOAT_EQ(scroll.scrollbarHitThickness(), 14.0f);
    EXPECT_TRUE(scroll.clipsChildren());
    EXPECT_TRUE(scroll.isFocusable());

    scroll.setScrollAxis(ScrollAxis::Horizontal);
    EXPECT_EQ(scroll.scrollAxis(), ScrollAxis::Horizontal);

    scroll.setScrollAxis(ScrollAxis::Both);
    EXPECT_EQ(scroll.scrollAxis(), ScrollAxis::Both);

    scroll.setScrollbarThickness(8.0f);
    EXPECT_FLOAT_EQ(scroll.scrollbarThickness(), 8.0f);

    scroll.setScrollbarPadding(4.0f);
    EXPECT_FLOAT_EQ(scroll.scrollbarPadding(), 4.0f);

    scroll.setScrollbarHitThickness(16.0f);
    EXPECT_FLOAT_EQ(scroll.scrollbarHitThickness(), 16.0f);
}

TEST(ScrollViewTest, DualAxisContentSizeAndMaxScroll) {
    ScrollView scroll;
    scroll.style().width = layout::Length::points(200.0f);
    scroll.style().height = layout::Length::points(100.0f);

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(500.0f);
    content->style().height = layout::Length::points(300.0f);
    scroll.addChild(std::move(content));

    scroll.layoutNode().computeLayout(200.0f, 100.0f);

    // Default vertical axis
    EXPECT_FLOAT_EQ(scroll.contentSize().width, 500.0f);
    EXPECT_FLOAT_EQ(scroll.contentSize().height, 300.0f);
    EXPECT_FLOAT_EQ(scroll.maxScrollOffset().x, 0.0f);
    EXPECT_FLOAT_EQ(scroll.maxScrollOffset().y, 200.0f);
    EXPECT_TRUE(scroll.hasVerticalScrollbar());
    EXPECT_FALSE(scroll.hasHorizontalScrollbar());

    // Switch to horizontal axis
    scroll.setScrollAxis(ScrollAxis::Horizontal);
    EXPECT_FLOAT_EQ(scroll.maxScrollOffset().x, 300.0f);
    EXPECT_FLOAT_EQ(scroll.maxScrollOffset().y, 0.0f);
    EXPECT_FALSE(scroll.hasVerticalScrollbar());
    EXPECT_TRUE(scroll.hasHorizontalScrollbar());

    // Switch to both axes
    scroll.setScrollAxis(ScrollAxis::Both);
    EXPECT_FLOAT_EQ(scroll.maxScrollOffset().x, 300.0f);
    EXPECT_FLOAT_EQ(scroll.maxScrollOffset().y, 200.0f);
    EXPECT_TRUE(scroll.hasVerticalScrollbar());
    EXPECT_TRUE(scroll.hasHorizontalScrollbar());
}

TEST(ScrollViewTest, TrackAndThumbGeometryWithCornerSpacer) {
    ScrollView scroll;
    scroll.setScrollAxis(ScrollAxis::Both);
    scroll.style().width = layout::Length::points(200.0f);
    scroll.style().height = layout::Length::points(150.0f);

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(400.0f);
    content->style().height = layout::Length::points(300.0f);
    scroll.addChild(std::move(content));

    scroll.layoutNode().computeLayout(200.0f, 150.0f);

    EXPECT_TRUE(scroll.hasVerticalScrollbar());
    EXPECT_TRUE(scroll.hasHorizontalScrollbar());

    core::Rect vTrack = scroll.verticalTrackRect();
    core::Rect hTrack = scroll.horizontalTrackRect();
    core::Rect vThumb = scroll.verticalThumbRect();
    core::Rect hThumb = scroll.horizontalThumbRect();

    // Verify track bounds
    EXPECT_GT(vTrack.size.height, 0.0f);
    EXPECT_GT(hTrack.size.width, 0.0f);

    // Verify corner reservation: tracks must not reach the opposing edge
    float cornerReservation = scroll.scrollbarThickness() + scroll.scrollbarPadding();
    EXPECT_FLOAT_EQ(vTrack.size.height,
                    150.0f - 2.0f * scroll.scrollbarPadding() - cornerReservation);
    EXPECT_FLOAT_EQ(hTrack.size.width,
                    200.0f - 2.0f * scroll.scrollbarPadding() - cornerReservation);

    // Verify thumb sizes clamped to minimum 24px
    EXPECT_GE(vThumb.size.height, 24.0f);
    EXPECT_GE(hThumb.size.width, 24.0f);

    // Initial position: thumbs at top and left of their tracks
    EXPECT_FLOAT_EQ(vThumb.origin.y, vTrack.origin.y);
    EXPECT_FLOAT_EQ(hThumb.origin.x, hTrack.origin.x);
}

TEST(ScrollViewTest, TrackClickJumpAndThumbDrag) {
    ScrollView scroll;
    scroll.setScrollAxis(ScrollAxis::Both);
    scroll.style().width = layout::Length::points(200.0f);
    scroll.style().height = layout::Length::points(200.0f);

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(600.0f);
    content->style().height = layout::Length::points(600.0f);
    scroll.addChild(std::move(content));

    scroll.layoutNode().computeLayout(200.0f, 200.0f);

    core::Rect vTrack = scroll.verticalTrackRect();
    core::Rect vThumb = scroll.verticalThumbRect();

    // 1. Click on vertical track below the thumb (jump to click position)
    float clickY = vTrack.origin.y + vTrack.size.height * 0.75f;
    WidgetMouseEvent clickTrack{core::Point{195.0f, clickY}, platform::MouseButton::Left,
                                platform::KeyModifier::None};
    scroll.onMouseDown(clickTrack);
    EXPECT_TRUE(clickTrack.isAccepted());
    EXPECT_GT(scroll.scrollOffset().y, 100.0f);

    // 2. Drag vertical thumb further down
    WidgetMouseEvent dragMove{core::Point{195.0f, clickY + 20.0f}, platform::MouseButton::Left,
                              platform::KeyModifier::None};
    scroll.onMouseMove(dragMove);
    EXPECT_TRUE(dragMove.isAccepted());

    // 3. Mouse up releases vertical drag
    WidgetMouseEvent releaseMouse{core::Point{195.0f, clickY + 20.0f}, platform::MouseButton::Left,
                                  platform::KeyModifier::None};
    scroll.onMouseUp(releaseMouse);
    EXPECT_TRUE(releaseMouse.isAccepted());

    // 4. Click horizontal track (jump to click position)
    core::Rect hTrack = scroll.horizontalTrackRect();
    float clickX = hTrack.origin.x + hTrack.size.width * 0.5f;
    WidgetMouseEvent clickHTrack{core::Point{clickX, 195.0f}, platform::MouseButton::Left,
                                 platform::KeyModifier::None};
    scroll.onMouseDown(clickHTrack);
    EXPECT_TRUE(clickHTrack.isAccepted());
    EXPECT_GT(scroll.scrollOffset().x, 100.0f);

    // 5. Mouse up releases horizontal drag
    scroll.onMouseUp(releaseMouse);
    EXPECT_TRUE(releaseMouse.isAccepted());
}

TEST(ScrollViewTest, HorizontalAndVerticalWheelScrolling) {
    ScrollView scroll;
    scroll.setScrollAxis(ScrollAxis::Both);
    scroll.style().width = layout::Length::points(200.0f);
    scroll.style().height = layout::Length::points(200.0f);

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(500.0f);
    content->style().height = layout::Length::points(500.0f);
    scroll.addChild(std::move(content));

    scroll.layoutNode().computeLayout(200.0f, 200.0f);

    // Vertical wheel delta
    WidgetMouseEvent vWheel{core::Point{50.0f, 50.0f}, platform::MouseButton::None,
                            platform::KeyModifier::None, core::Point{0.0f, -2.0f}};
    scroll.onMouseWheel(vWheel);
    EXPECT_TRUE(vWheel.isAccepted());
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 64.0f);
    EXPECT_FLOAT_EQ(scroll.scrollOffset().x, 0.0f);

    // Shift + vertical wheel routes to horizontal scrolling
    WidgetMouseEvent shiftWheel{core::Point{50.0f, 50.0f}, platform::MouseButton::None,
                                platform::KeyModifier::Shift, core::Point{0.0f, -1.0f}};
    scroll.onMouseWheel(shiftWheel);
    EXPECT_TRUE(shiftWheel.isAccepted());
    EXPECT_FLOAT_EQ(scroll.scrollOffset().x, 32.0f);

    // Direct horizontal wheel delta
    WidgetMouseEvent hWheel{core::Point{50.0f, 50.0f}, platform::MouseButton::None,
                            platform::KeyModifier::None, core::Point{-1.0f, 0.0f}};
    scroll.onMouseWheel(hWheel);
    EXPECT_TRUE(hWheel.isAccepted());
    EXPECT_FLOAT_EQ(scroll.scrollOffset().x, 64.0f);
}

TEST(ScrollViewTest, KeyboardNavigation) {
    ScrollView scroll;
    scroll.setScrollAxis(ScrollAxis::Both);
    scroll.style().width = layout::Length::points(200.0f);
    scroll.style().height = layout::Length::points(200.0f);

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(600.0f);
    content->style().height = layout::Length::points(600.0f);
    scroll.addChild(std::move(content));

    scroll.layoutNode().computeLayout(200.0f, 200.0f);

    // Down key: +40px
    WidgetKeyEvent downEv{platform::Key::Down, true, platform::KeyModifier::None};
    scroll.onKeyDown(downEv);
    EXPECT_TRUE(downEv.isAccepted());
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 40.0f);

    // Up key: -40px
    WidgetKeyEvent upEv{platform::Key::Up, true, platform::KeyModifier::None};
    scroll.onKeyDown(upEv);
    EXPECT_TRUE(upEv.isAccepted());
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 0.0f);

    // PageDown: 85% of 200px = 170px
    WidgetKeyEvent pageDownEv{platform::Key::PageDown, true, platform::KeyModifier::None};
    scroll.onKeyDown(pageDownEv);
    EXPECT_TRUE(pageDownEv.isAccepted());
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 170.0f);

    // PageUp: -170px
    WidgetKeyEvent pageUpEv{platform::Key::PageUp, true, platform::KeyModifier::None};
    scroll.onKeyDown(pageUpEv);
    EXPECT_TRUE(pageUpEv.isAccepted());
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 0.0f);

    // End key: jump to max Y (400px)
    WidgetKeyEvent endEv{platform::Key::End, true, platform::KeyModifier::None};
    scroll.onKeyDown(endEv);
    EXPECT_TRUE(endEv.isAccepted());
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 400.0f);

    // Home key: jump to 0
    WidgetKeyEvent homeEv{platform::Key::Home, true, platform::KeyModifier::None};
    scroll.onKeyDown(homeEv);
    EXPECT_TRUE(homeEv.isAccepted());
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 0.0f);

    // Right key: +40px X
    WidgetKeyEvent rightEv{platform::Key::Right, true, platform::KeyModifier::None};
    scroll.onKeyDown(rightEv);
    EXPECT_TRUE(rightEv.isAccepted());
    EXPECT_FLOAT_EQ(scroll.scrollOffset().x, 40.0f);

    // Left key: -40px X
    WidgetKeyEvent leftEv{platform::Key::Left, true, platform::KeyModifier::None};
    scroll.onKeyDown(leftEv);
    EXPECT_TRUE(leftEv.isAccepted());
    EXPECT_FLOAT_EQ(scroll.scrollOffset().x, 0.0f);
}

TEST(ScrollViewTest, SignalEmissionOnScroll) {
    ScrollView scroll;
    scroll.style().width = layout::Length::points(200.0f);
    scroll.style().height = layout::Length::points(200.0f);

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(400.0f);
    content->style().height = layout::Length::points(400.0f);
    scroll.addChild(std::move(content));

    scroll.layoutNode().computeLayout(200.0f, 200.0f);

    int signalCount = 0;
    core::Point lastOffset{-1.0f, -1.0f};

    scroll.onScrolled.connect([&](const core::Point& pt) {
        signalCount++;
        lastOffset = pt;
    });

    scroll.scrollTo(core::Point{0.0f, 50.0f});
    EXPECT_EQ(signalCount, 1);
    EXPECT_FLOAT_EQ(lastOffset.y, 50.0f);

    // Calling scrollTo with same coordinates does not re-emit
    scroll.scrollTo(core::Point{0.0f, 50.0f});
    EXPECT_EQ(signalCount, 1);

    // Clamping to max offset (200.0f)
    scroll.scrollTo(core::Point{0.0f, 500.0f});
    EXPECT_EQ(signalCount, 2);
    EXPECT_FLOAT_EQ(lastOffset.y, 200.0f);
}

TEST(ScrollViewTest, DynamicThemingAndOverlayRendering) {
    WidgetContext ctx;
    ctx.setTheme(theme::Theme::dark());

    auto scroll = std::make_unique<ScrollView>();
    scroll->setScrollAxis(ScrollAxis::Both);
    scroll->style().width = layout::Length::points(300.0f);
    scroll->style().height = layout::Length::points(200.0f);

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(600.0f);
    content->style().height = layout::Length::points(500.0f);
    scroll->addChild(std::move(content));

    ScrollView* scrollPtr = scroll.get();
    ctx.setRootWidget(std::move(scroll));
    ctx.handleResize(core::Size{300.0f, 200.0f});

    renderer::PaintContext paintCtx;
    scrollPtr->paintOverlay(paintCtx);
    size_t cmdCountDark = paintCtx.commands().size();
    EXPECT_GT(cmdCountDark, 0);

    // Switch to light theme and repaint overlay
    ctx.setTheme(theme::Theme::light());
    paintCtx.clear();
    scrollPtr->paintOverlay(paintCtx);
    size_t cmdCountLight = paintCtx.commands().size();
    EXPECT_GT(cmdCountLight, 0);
    EXPECT_EQ(cmdCountDark, cmdCountLight);
}
