#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/button.hpp"
#include "widgets/dialogs/title_bar.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(TitleBarTest, DefaultsAndDimensions) {
    TitleBar bar("Test App");
    EXPECT_EQ(bar.title(), "Test App");
    EXPECT_FLOAT_EQ(bar.style().height.value, TitleBar::kDefaultHeight);
    EXPECT_FLOAT_EQ(bar.style().width.value, 100.0f); // percent

    bar.setTitle("Updated Title");
    EXPECT_EQ(bar.title(), "Updated Title");
}

TEST(TitleBarTest, HitTestingCaptionAndButtons) {
    TitleBar bar("HitTest Window");
    bar.layoutNode().setLayoutRect(core::Rect{core::Point{0.0f, 0.0f}, core::Size{800.0f, 36.0f}});

    // Center caption area
    EXPECT_EQ(bar.hitTestPoint(core::Point{400.0f, 18.0f}), platform::HitTestResult::Caption);

    // Left title area
    EXPECT_EQ(bar.hitTestPoint(core::Point{50.0f, 18.0f}), platform::HitTestResult::Caption);

    // Close button: width 800 - 44 = 756..800
    EXPECT_EQ(bar.hitTestPoint(core::Point{780.0f, 18.0f}), platform::HitTestResult::Client);

    // Maximize button: 756 - 44 = 712..756
    EXPECT_EQ(bar.hitTestPoint(core::Point{730.0f, 18.0f}), platform::HitTestResult::Client);

    // Minimize button: 712 - 44 = 668..712
    EXPECT_EQ(bar.hitTestPoint(core::Point{690.0f, 18.0f}), platform::HitTestResult::Client);

    // Outside bounds
    EXPECT_EQ(bar.hitTestPoint(core::Point{400.0f, 50.0f}), platform::HitTestResult::Client);
}

TEST(TitleBarTest, ClickSignals) {
    TitleBar bar("Signals");
    bar.layoutNode().setLayoutRect(core::Rect{core::Point{0.0f, 0.0f}, core::Size{800.0f, 36.0f}});

    bool minClicked = false;
    bool maxClicked = false;
    bool closeClicked = false;

    bar.onMinimizeClicked.connect([&]() { minClicked = true; });
    bar.onMaximizeClicked.connect([&]() { maxClicked = true; });
    bar.onCloseClicked.connect([&]() { closeClicked = true; });

    // Click close button
    WidgetMouseEvent downClose{core::Point{780.0f, 18.0f}, platform::MouseButton::Left,
                               platform::KeyModifier::None};
    bar.onMouseDown(downClose);
    EXPECT_TRUE(downClose.isAccepted());

    WidgetMouseEvent upClose{core::Point{780.0f, 18.0f}, platform::MouseButton::Left,
                             platform::KeyModifier::None};
    bar.onMouseUp(upClose);
    EXPECT_TRUE(upClose.isAccepted());
    EXPECT_TRUE(closeClicked);

    // Click minimize button
    WidgetMouseEvent downMin{core::Point{690.0f, 18.0f}, platform::MouseButton::Left,
                             platform::KeyModifier::None};
    bar.onMouseDown(downMin);
    WidgetMouseEvent upMin{core::Point{690.0f, 18.0f}, platform::MouseButton::Left,
                           platform::KeyModifier::None};
    bar.onMouseUp(upMin);
    EXPECT_TRUE(minClicked);

    // Click maximize button
    WidgetMouseEvent downMax{core::Point{730.0f, 18.0f}, platform::MouseButton::Left,
                             platform::KeyModifier::None};
    bar.onMouseDown(downMax);
    WidgetMouseEvent upMax{core::Point{730.0f, 18.0f}, platform::MouseButton::Left,
                           platform::KeyModifier::None};
    bar.onMouseUp(upMax);
    EXPECT_TRUE(maxClicked);
}

TEST(TitleBarTest, HoverAndLeave) {
    TitleBar bar("Hover Test");
    bar.layoutNode().setLayoutRect(core::Rect{core::Point{0.0f, 0.0f}, core::Size{800.0f, 36.0f}});

    WidgetMouseEvent moveClose{core::Point{780.0f, 18.0f}, platform::MouseButton::None,
                               platform::KeyModifier::None};
    bar.onMouseMove(moveClose);

    WidgetMouseEvent moveAway{core::Point{50.0f, 18.0f}, platform::MouseButton::None,
                              platform::KeyModifier::None};
    bar.onMouseMove(moveAway);

    bar.onMouseLeave();
}

TEST(TitleBarTest, FallbackFontEllipsisThemeAndDpi) {
    WidgetContext ctx;
    ctx.setTheme(theme::Theme::dark());

    auto owned = std::make_unique<TitleBar>(
        "Extremely Long Application Window Title That Should Truncate Cleanly");
    TitleBar* bar = owned.get();
    ctx.setRootWidget(std::move(owned));

    ASSERT_NE(bar->effectiveFont(), nullptr);
    EXPECT_FLOAT_EQ(bar->style().height.value, TitleBar::kDefaultHeight);

    bar->setSize(TitleBarSize::Compact);
    EXPECT_FLOAT_EQ(bar->style().height.value, TitleBar::kCompactHeight);
    bar->setSize(TitleBarSize::Comfortable);
    EXPECT_FLOAT_EQ(bar->style().height.value, TitleBar::kComfortableHeight);

    bar->setDpiScale(1.5f);
    EXPECT_FLOAT_EQ(bar->style().height.value, TitleBar::kComfortableHeight * 1.5f);

    bar->setDpiScale(1.0f);
    bar->setSize(TitleBarSize::Standard);

    renderer::PaintContext pctx;
    ctx.render(pctx, core::Size{280.0f, 36.0f});

    bool drewTitleGlyphs = false;
    for (const auto& cmd : pctx.commands()) {
        if (std::holds_alternative<renderer::DrawGlyphsCommand>(cmd)) {
            drewTitleGlyphs = true;
        }
    }
    EXPECT_TRUE(drewTitleGlyphs);

    EXPECT_EQ(bar->effectiveBackgroundColor(), theme::ColorPalette::dark().surfaceDefault);
    EXPECT_EQ(bar->effectiveTitleColor(), theme::ColorPalette::dark().textPrimary);

    ctx.setTheme(theme::Theme::light());
    EXPECT_EQ(bar->effectiveBackgroundColor(), theme::ColorPalette::light().surfaceDefault);
    EXPECT_EQ(bar->effectiveTitleColor(), theme::ColorPalette::light().textPrimary);
}

TEST(TitleBarTest, SlotsActionWidgetsSafetyAndButtonVisibility) {
    WidgetContext ctx;
    auto owned = std::make_unique<TitleBar>("Workspace");
    TitleBar* bar = owned.get();

    auto centerBtn = std::make_unique<Button>("Search");
    centerBtn->style().width = layout::Length::points(120.0f);
    centerBtn->style().height = layout::Length::points(24.0f);
    Button* centerPtr = centerBtn.get();
    bar->setCenterWidget(std::move(centerBtn));

    auto act1 = std::make_unique<Button>("A");
    act1->style().width = layout::Length::points(32.0f);
    act1->style().height = layout::Length::points(24.0f);
    Button* act1Ptr = act1.get();
    bar->addActionWidget(std::move(act1));

    auto act2 = std::make_unique<Button>("B");
    act2->style().width = layout::Length::points(32.0f);
    act2->style().height = layout::Length::points(24.0f);
    Button* act2Ptr = act2.get();
    bar->addActionWidget(std::move(act2));

    ctx.setRootWidget(std::move(owned));
    renderer::PaintContext pctx;
    ctx.render(pctx, core::Size{800.0f, 36.0f});

    core::Rect cRect = centerPtr->screenRect();
    float centerMidX = cRect.origin.x + cRect.size.width * 0.5f;
    EXPECT_NEAR(centerMidX, 400.0f, 4.0f);

    core::Rect a2Rect = act2Ptr->screenRect();
    float captionLeft = 800.0f - 3.0f * TitleBar::kCaptionButtonWidth;
    EXPECT_LE(a2Rect.origin.x + a2Rect.size.width, captionLeft);

    EXPECT_EQ(bar->hitTestPoint({centerMidX, 18.0f}), platform::HitTestResult::Client);
    EXPECT_EQ(bar->hitTestPoint({a2Rect.origin.x + 8.0f, 18.0f}), platform::HitTestResult::Client);
    EXPECT_EQ(bar->hitTestPoint({45.0f, 18.0f}), platform::HitTestResult::Caption);

    bar->removeActionWidget(act1Ptr);
    EXPECT_EQ(bar->actionWidgets().size(), 1u);
    bar->clearActionWidgets();
    EXPECT_TRUE(bar->actionWidgets().empty());

    // Hide maximize and minimize; only Close stays at right edge
    bar->setMaximizeVisible(false);
    bar->setMinimizeVisible(false);
    EXPECT_EQ(bar->hitTestPoint({780.0f, 18.0f}), platform::HitTestResult::Client);
    EXPECT_EQ(bar->hitTestPoint({730.0f, 18.0f}), platform::HitTestResult::Caption);
}

TEST(TitleBarTest, DoubleClickCaptionMaximizeAndTooltips) {
    WidgetContext ctx;
    auto owned = std::make_unique<TitleBar>("Interactive");
    TitleBar* bar = owned.get();

    int maxCount = 0;
    bar->onMaximizeClicked.connect([&]() { ++maxCount; });

    ctx.setRootWidget(std::move(owned));
    renderer::PaintContext pctx;
    ctx.render(pctx, core::Size{800.0f, 36.0f});

    WidgetMouseEvent moveMin{core::Point{690.0f, 18.0f}, platform::MouseButton::None,
                             platform::KeyModifier::None};
    bar->onMouseMove(moveMin);
    EXPECT_EQ(bar->tooltipText(), "Minimize");
    EXPECT_TRUE(bar->hasActiveAnimations());

    WidgetMouseEvent moveMax{core::Point{730.0f, 18.0f}, platform::MouseButton::None,
                             platform::KeyModifier::None};
    bar->onMouseMove(moveMax);
    EXPECT_EQ(bar->tooltipText(), "Maximize");

    WidgetMouseEvent moveClose{core::Point{780.0f, 18.0f}, platform::MouseButton::None,
                               platform::KeyModifier::None};
    bar->onMouseMove(moveClose);
    EXPECT_EQ(bar->tooltipText(), "Close");

    WidgetMouseEvent moveCaption{core::Point{200.0f, 18.0f}, platform::MouseButton::None,
                                 platform::KeyModifier::None};
    bar->onMouseMove(moveCaption);
    EXPECT_TRUE(bar->tooltipText().empty());

    WidgetMouseEvent click1{core::Point{200.0f, 18.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    WidgetMouseEvent click2{core::Point{200.0f, 18.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    bar->onMouseDown(click1);
    EXPECT_EQ(maxCount, 0);
    bar->onMouseDown(click2);
    EXPECT_EQ(maxCount, 1);
}
