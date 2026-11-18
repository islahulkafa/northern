#include "widgets/container.hpp"
#include "widgets/overlay.hpp"
#include "widgets/widget_context.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(OverlayTest, ComputeOverlayPositionBottomAndClamp) {
    core::Size viewport{800.0f, 600.0f};
    core::Size overlaySize{200.0f, 150.0f};

    OverlayDesc desc{};
    desc.anchorRect = core::Rect{core::Point{100.0f, 100.0f}, core::Size{120.0f, 32.0f}};
    desc.preferredPlacement = OverlayPlacement::Bottom;
    desc.autoFlip = true;
    desc.gap = 4.0f;

    core::Point pos = computeOverlayPosition(viewport, overlaySize, desc);
    EXPECT_FLOAT_EQ(pos.x, 100.0f);
    EXPECT_FLOAT_EQ(pos.y, 136.0f); // 100 + 32 + 4

    // Horizontal clamping: anchor near right edge
    desc.anchorRect.origin.x = 700.0f;
    pos = computeOverlayPosition(viewport, overlaySize, desc);
    EXPECT_FLOAT_EQ(pos.x, 600.0f); // clamped to 800 - 200
}

TEST(OverlayTest, AutoFlipToTopNearBottomBoundary) {
    core::Size viewport{800.0f, 600.0f};
    core::Size overlaySize{200.0f, 150.0f};

    OverlayDesc desc{};
    desc.anchorRect = core::Rect{core::Point{100.0f, 500.0f}, core::Size{120.0f, 32.0f}};
    desc.preferredPlacement = OverlayPlacement::Bottom;
    desc.autoFlip = true;
    desc.gap = 4.0f;

    // 500 + 32 + 4 + 150 = 686 > 600 -> Should flip to top!
    core::Point pos = computeOverlayPosition(viewport, overlaySize, desc);
    EXPECT_FLOAT_EQ(pos.x, 100.0f);
    EXPECT_FLOAT_EQ(pos.y, 500.0f - 150.0f - 4.0f); // 346.0f
}

TEST(OverlayTest, ShowAndDismissTopOverlay) {
    WidgetContext ctx;
    ctx.handleResize(core::Size{800.0f, 600.0f});

    EXPECT_FALSE(ctx.hasOverlays());
    EXPECT_EQ(ctx.topOverlay(), nullptr);

    bool dismissed = false;
    OverlayDesc desc{};
    desc.anchorRect = core::Rect{core::Point{50.0f, 50.0f}, core::Size{100.0f, 30.0f}};
    desc.onDismiss = [&]() { dismissed = true; };

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(150.0f);
    content->style().height = layout::Length::points(100.0f);
    Widget* raw = content.get();

    Widget* shown = ctx.showOverlay(std::move(content), desc);
    EXPECT_EQ(shown, raw);
    EXPECT_TRUE(ctx.hasOverlays());
    EXPECT_EQ(ctx.topOverlay(), raw);

    ctx.dismissTopOverlay();
    EXPECT_FALSE(ctx.hasOverlays());
    EXPECT_EQ(ctx.topOverlay(), nullptr);
    EXPECT_TRUE(dismissed);
}

TEST(OverlayTest, LightDismissOnOutsideClick) {
    WidgetContext ctx;
    ctx.handleResize(core::Size{800.0f, 600.0f});

    bool dismissed = false;
    OverlayDesc desc{};
    desc.anchorRect = core::Rect{core::Point{100.0f, 100.0f}, core::Size{100.0f, 30.0f}};
    desc.dismissOnOutsideClick = true;
    desc.onDismiss = [&]() { dismissed = true; };

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(100.0f);
    content->style().height = layout::Length::points(80.0f);
    ctx.showOverlay(std::move(content), desc);

    // Overlay is placed at (100, 134), size (100, 80)
    // Click outside at (300, 300)
    platform::MouseEvent evOutside{};
    evOutside.button = platform::MouseButton::Left;
    evOutside.position = core::Point{300.0f, 300.0f};
    ctx.handleMouseDown(evOutside);

    EXPECT_FALSE(ctx.hasOverlays());
    EXPECT_TRUE(dismissed);
}

TEST(OverlayTest, EscapeKeyDismissal) {
    WidgetContext ctx;
    ctx.handleResize(core::Size{800.0f, 600.0f});

    bool dismissed = false;
    OverlayDesc desc{};
    desc.anchorRect = core::Rect{core::Point{100.0f, 100.0f}, core::Size{100.0f, 30.0f}};
    desc.dismissOnEscape = true;
    desc.onDismiss = [&]() { dismissed = true; };

    auto content = std::make_unique<Container>();
    content->style().width = layout::Length::points(100.0f);
    content->style().height = layout::Length::points(80.0f);
    ctx.showOverlay(std::move(content), desc);

    platform::KeyEvent escEv{};
    escEv.isDown = true;
    escEv.key = platform::Key::Escape;
    ctx.handleKeyEvent(escEv);

    EXPECT_FALSE(ctx.hasOverlays());
    EXPECT_TRUE(dismissed);
}

TEST(OverlayTest, DismissOverlaysForOwnerCleanup) {
    WidgetContext ctx;
    ctx.handleResize(core::Size{800.0f, 600.0f});

    auto owner = std::make_unique<Container>();
    Container* ownerPtr = owner.get();
    ctx.setRootWidget(std::move(owner));

    bool dismissed = false;
    OverlayDesc desc{};
    desc.anchorRect = core::Rect{core::Point{50.0f, 50.0f}, core::Size{50.0f, 20.0f}};
    desc.onDismiss = [&]() { dismissed = true; };

    auto content = std::make_unique<Container>();
    ctx.showOverlay(std::move(content), desc, ownerPtr);
    EXPECT_TRUE(ctx.hasOverlays());

    // Unregistering or clearing owner dismisses associated overlays
    ctx.dismissOverlaysForOwner(ownerPtr);
    EXPECT_FALSE(ctx.hasOverlays());
    EXPECT_TRUE(dismissed);
}
