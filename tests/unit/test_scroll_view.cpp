#include "widgets/base/theme.hpp"
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

TEST(ScrollViewTest, QuietOverlayInactivityFadeAndVisibilityModes) {
    ScrollView scroll;
    scroll.style().width = layout::Length::points(200.0f);
    scroll.style().height = layout::Length::points(200.0f);

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(200.0f);
    content->style().height = layout::Length::points(600.0f);
    scroll.addChild(std::move(content));
    scroll.layoutNode().computeLayout(200.0f, 200.0f);

    EXPECT_EQ(scroll.scrollbarVisibility(), ScrollbarVisibility::Automatic);
    EXPECT_FLOAT_EQ(scroll.scrollbarOpacity(), 1.0f);
    EXPECT_TRUE(scroll.hasActiveAnimations());

    // Advance past 800ms inactivity threshold + 180ms fade-out
    scroll.updateAnimations(0.50f);
    EXPECT_FLOAT_EQ(scroll.scrollbarOpacity(), 1.0f);
    scroll.updateAnimations(0.35f);
    scroll.updateAnimations(0.25f);
    EXPECT_FLOAT_EQ(scroll.scrollbarOpacity(), 0.0f);
    EXPECT_FALSE(scroll.hasActiveAnimations());

    // Scrolling wakes scrollbar back up immediately over fade-in duration
    scroll.scrollBy(core::Point{0.0f, 40.0f});
    scroll.updateAnimations(0.15f);
    EXPECT_FLOAT_EQ(scroll.scrollbarOpacity(), 1.0f);

    // Visibility Never suppresses overlay rendering
    scroll.setScrollbarVisibility(ScrollbarVisibility::Never);
    EXPECT_FLOAT_EQ(scroll.scrollbarOpacity(), 0.0f);
    renderer::PaintContext paintCtx;
    scroll.paintOverlay(paintCtx);
    EXPECT_TRUE(paintCtx.commands().empty());

    // Visibility AlwaysVisible keeps scrollbars active
    scroll.setScrollbarVisibility(ScrollbarVisibility::AlwaysVisible);
    scroll.updateAnimations(2.0f);
    EXPECT_FLOAT_EQ(scroll.scrollbarOpacity(), 1.0f);
    scroll.paintOverlay(paintCtx);
    EXPECT_FALSE(paintCtx.commands().empty());
}

TEST(ScrollViewTest, DpiScalingAndHoverExpansion) {
    ScrollView scroll;
    scroll.setDpiScale(1.5f);
    EXPECT_FLOAT_EQ(scroll.dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(scroll.scrollbarThickness(), 9.0f);
    EXPECT_FLOAT_EQ(scroll.scrollbarPadding(), 4.5f);
    EXPECT_FLOAT_EQ(scroll.scrollbarHitThickness(), 21.0f);

    scroll.style().width = layout::Length::points(300.0f);
    scroll.style().height = layout::Length::points(200.0f);
    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(300.0f);
    content->style().height = layout::Length::points(800.0f);
    scroll.addChild(std::move(content));
    scroll.layoutNode().computeLayout(300.0f, 200.0f);

    WidgetMouseEvent wheelEv{core::Point{50.0f, 50.0f}, platform::MouseButton::None,
                             platform::KeyModifier::None, core::Point{0.0f, -1.0f}};
    scroll.onMouseWheel(wheelEv);
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 48.0f);

    // Hovering over vertical scrollbar hit zone triggers expand animation
    WidgetMouseEvent hoverEv{core::Point{295.0f, 40.0f}, platform::MouseButton::None,
                             platform::KeyModifier::None};
    scroll.onMouseMove(hoverEv);
    EXPECT_TRUE(scroll.hasActiveAnimations());
    scroll.updateAnimations(0.20f);
}

TEST(ScrollViewTest, SurfaceRoleTracksThemeSwitches) {
    WidgetContext ctx;
    ctx.setTheme(theme::Theme::dark());

    auto scroll = std::make_unique<ScrollView>();
    ScrollView* scrollPtr = scroll.get();
    scrollPtr->setSurface(SurfaceRole::Canvas);
    ctx.setRootWidget(std::move(scroll));

    EXPECT_EQ(scrollPtr->surface(), SurfaceRole::Canvas);
    EXPECT_EQ(scrollPtr->effectiveBackgroundColor(), theme::ColorPalette::dark().surfaceCanvas);

    ctx.setTheme(theme::Theme::light());
    EXPECT_EQ(scrollPtr->effectiveBackgroundColor(), theme::ColorPalette::light().surfaceCanvas);

    scrollPtr->setBackgroundColor(core::Color::red());
    EXPECT_EQ(scrollPtr->effectiveBackgroundColor(), core::Color::red());
}

TEST(ScrollViewTest, ContentShrinkClampsStaleScrollOffsetOnPaint) {
    ScrollView scroll;
    scroll.style().width = layout::Length::points(200.0f);
    scroll.style().height = layout::Length::points(200.0f);

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(200.0f);
    content->style().height = layout::Length::points(600.0f);
    Container* contentPtr = content.get();
    scroll.addChild(std::move(content));
    scroll.layoutNode().computeLayout(200.0f, 200.0f);

    scroll.scrollTo(core::Point{0.0f, 200.0f});
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 200.0f);

    int clampedSignals = 0;
    core::Point lastOffset{};
    scroll.onScrolled.connect([&](const core::Point& off) {
        ++clampedSignals;
        lastOffset = off;
    });

    contentPtr->style().height = layout::Length::points(300.0f);
    contentPtr->markLayoutDirty();
    scroll.layoutNode().computeLayout(200.0f, 200.0f);

    renderer::PaintContext pctx;
    scroll.paint(pctx);
    EXPECT_FLOAT_EQ(scroll.scrollOffset().y, 100.0f);
    EXPECT_EQ(clampedSignals, 1);
    EXPECT_FLOAT_EQ(lastOffset.y, 100.0f);
}

TEST(ScrollViewTest, DisabledScrollViewReleasesGrabAndIgnoresInput) {
    WidgetContext ctx;
    auto scroll = std::make_unique<ScrollView>();
    ScrollView* scrollPtr = scroll.get();
    scrollPtr->style().width = layout::Length::points(200.0f);
    scrollPtr->style().height = layout::Length::points(200.0f);

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(200.0f);
    content->style().height = layout::Length::points(600.0f);
    scrollPtr->addChild(std::move(content));

    ctx.setRootWidget(std::move(scroll));
    ctx.handleResize(core::Size{200.0f, 200.0f});

    // Grab vertical scrollbar
    platform::MouseEvent downEv{};
    downEv.type = platform::MouseEventType::ButtonDown;
    downEv.button = platform::MouseButton::Left;
    downEv.physicalPosition = {195.0f, 15.0f};
    downEv.position = downEv.physicalPosition;
    ctx.handleMouseDown(downEv);
    EXPECT_EQ(ctx.mouseGrabWidget(), scrollPtr);

    scrollPtr->setEnabled(false);
    EXPECT_EQ(ctx.mouseGrabWidget(), nullptr);

    float offsetBefore = scrollPtr->scrollOffset().y;
    WidgetMouseEvent wheelEv{core::Point{50.0f, 50.0f}, platform::MouseButton::None,
                             platform::KeyModifier::None, core::Point{0.0f, -2.0f}};
    scrollPtr->onMouseWheel(wheelEv);
    EXPECT_FALSE(wheelEv.isAccepted());
    EXPECT_FLOAT_EQ(scrollPtr->scrollOffset().y, offsetBefore);

    WidgetKeyEvent downKey{platform::Key::Down, true, platform::KeyModifier::None};
    scrollPtr->onKeyDown(downKey);
    EXPECT_FALSE(downKey.isAccepted());
    EXPECT_FLOAT_EQ(scrollPtr->scrollOffset().y, offsetBefore);
}

TEST(ScrollViewTest, ContextAttachmentAdoptsDpiScale) {
    WidgetContext ctx;
    ctx.setDpiScale(1.5f);

    auto scroll = std::make_unique<ScrollView>();
    ScrollView* scrollPtr = scroll.get();
    ctx.setRootWidget(std::move(scroll));

    EXPECT_FLOAT_EQ(scrollPtr->dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(scrollPtr->scrollbarThickness(), 9.0f);
    EXPECT_FLOAT_EQ(scrollPtr->scrollbarPadding(), 4.5f);
    EXPECT_FLOAT_EQ(scrollPtr->scrollbarHitThickness(), 21.0f);
}
