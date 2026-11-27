#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/slider.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(SliderTest, SizePresetsDpiAndStepSnapping) {
    Slider s(SliderSize::Thick, SliderRole::Success, 0.0f, 100.0f, 33.0f);
    EXPECT_FLOAT_EQ(s.trackHeight(), 6.0f);
    EXPECT_FLOAT_EQ(s.thumbRadius(), 9.0f);
    EXPECT_EQ(s.role(), SliderRole::Success);

    s.setStep(10.0f);
    EXPECT_FLOAT_EQ(s.value(), 30.0f);
    s.setValue(38.0f);
    EXPECT_FLOAT_EQ(s.value(), 40.0f);

    s.setDpiScale(2.0f);
    EXPECT_FLOAT_EQ(s.trackHeight(), 12.0f);
    EXPECT_FLOAT_EQ(s.thumbRadius(), 18.0f);
}

TEST(SliderTest, DynamicThemingRolesAndDisabledGuards) {
    WidgetContext ctx;
    ctx.setTheme(theme::Theme::dark());

    auto owned =
        std::make_unique<Slider>(SliderSize::Standard, SliderRole::Warning, 0.0f, 10.0f, 5.0f);
    Slider* s = owned.get();
    ctx.setRootWidget(std::move(owned));

    EXPECT_EQ(s->effectiveActiveTrackColor(), theme::ColorPalette::dark().warning);
    EXPECT_EQ(s->effectiveInactiveTrackColor(), theme::ColorPalette::dark().surfaceRaised);

    ctx.setTheme(theme::Theme::light());
    EXPECT_EQ(s->effectiveActiveTrackColor(), theme::ColorPalette::light().warning);
    EXPECT_EQ(s->effectiveInactiveTrackColor(), theme::ColorPalette::light().borderDefault);

    s->setEnabled(false);
    EXPECT_EQ(s->effectiveActiveTrackColor(), theme::ColorPalette::light().textDisabled);

    // Disabled blocks key, wheel, and pointer inputs
    WidgetKeyEvent rightEv{platform::Key::Right, true, platform::KeyModifier::None};
    s->onKeyDown(rightEv);
    EXPECT_FALSE(rightEv.isAccepted());
    EXPECT_FLOAT_EQ(s->value(), 5.0f);

    WidgetMouseEvent wheelEv{core::Point{10.0f, 10.0f}, platform::MouseButton::None,
                             platform::KeyModifier::None, core::Point{0.0f, 1.0f}};
    s->onMouseWheel(wheelEv);
    EXPECT_FALSE(wheelEv.isAccepted());
    EXPECT_FLOAT_EQ(s->value(), 5.0f);

    s->setEnabled(true);
    core::Color custom = core::Color::fromRgba8(10, 200, 150);
    s->setActiveTrackColor(custom);
    EXPECT_EQ(s->effectiveActiveTrackColor(), custom);
    s->clearColorOverrides();
    EXPECT_EQ(s->effectiveActiveTrackColor(), theme::ColorPalette::light().warning);
}

TEST(SliderTest, ValueCommittedWheelPageKeysAndFocusReason) {
    WidgetContext ctx;
    auto owned = std::make_unique<Slider>(0.0f, 100.0f, 20.0f);
    Slider* s = owned.get();
    s->setStep(5.0f);
    s->style().width = layout::Length::points(240.0f);
    ctx.setRootWidget(std::move(owned));
    s->layoutNode().computeLayout(240.0f, 30.0f);

    float lastCommitted = -1.0f;
    int commitCount = 0;
    s->onValueCommitted.connect([&](float v) {
        lastCommitted = v;
        ++commitCount;
    });

    // Pointer click focuses with Pointer reason (no keyboard focus ring)
    WidgetMouseEvent downEv{core::Point{120.0f, 15.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    s->onMouseDown(downEv);
    EXPECT_FALSE(s->hasKeyboardFocus());
    EXPECT_EQ(commitCount, 0);

    WidgetMouseEvent upEv{core::Point{120.0f, 15.0f}, platform::MouseButton::Left,
                          platform::KeyModifier::None};
    s->onMouseUp(upEv);
    EXPECT_EQ(commitCount, 1);

    // PageUp jumps by 5 * step (25.0f) and commits
    float beforePage = s->value();
    WidgetKeyEvent pageUp{platform::Key::PageUp, true, platform::KeyModifier::None};
    s->onKeyDown(pageUp);
    EXPECT_TRUE(pageUp.isAccepted());
    EXPECT_FLOAT_EQ(s->value(), std::min(100.0f, beforePage + 25.0f));
    EXPECT_EQ(commitCount, 2);

    // Mouse wheel down steps down by step_ and commits
    float beforeWheel = s->value();
    WidgetMouseEvent wheelDown{core::Point{50.0f, 15.0f}, platform::MouseButton::None,
                               platform::KeyModifier::None, core::Point{0.0f, -1.0f}};
    s->onMouseWheel(wheelDown);
    EXPECT_TRUE(wheelDown.isAccepted());
    EXPECT_FLOAT_EQ(s->value(), beforeWheel - 5.0f);
    EXPECT_EQ(commitCount, 3);
    EXPECT_FLOAT_EQ(lastCommitted, s->value());
}

TEST(SliderTest, InlineValueReadoutAndStepTicksEmission) {
    Slider s(0.0f, 100.0f, 50.0f);
    s.setStep(25.0f);
    s.setShowValue(true);
    s.setShowTicks(true);
    s.style().width = layout::Length::points(280.0f);
    s.layoutNode().computeLayout(280.0f, 30.0f);

    renderer::PaintContext pctx;
    s.paint(pctx);

    int glyphsCount = 0;
    int boxCount = 0;
    for (const auto& cmd : pctx.commands()) {
        if (std::holds_alternative<renderer::DrawGlyphsCommand>(cmd)) {
            ++glyphsCount;
        } else if (std::holds_alternative<renderer::DrawBoxCommand>(cmd)) {
            ++boxCount;
        }
    }
    EXPECT_EQ(glyphsCount, 1);
    // Inactive track + active track + 5 step ticks (0, 25, 50, 75, 100) + thumb = 8 boxes
    EXPECT_EQ(boxCount, 8);
}

TEST(SliderTest, WhiteThumbAndSpringHoverScale) {
    WidgetContext ctx;
    auto owned = std::make_unique<Slider>(0.0f, 100.0f, 50.0f);
    Slider* s = owned.get();
    ctx.setRootWidget(std::move(owned));

    EXPECT_EQ(s->effectiveThumbColor(), core::Color::white());
    s->onMouseEnter();
    EXPECT_TRUE(s->hasActiveAnimations());
    for (int i = 0; i < 60; ++i) {
        s->updateAnimations(1.0f / 60.0f);
    }
    EXPECT_FALSE(s->hasActiveAnimations());
}

TEST(SliderTest, MouseGrabDragCommitAndCleanupOnDisableBlur) {
    WidgetContext ctx;
    auto owned = std::make_unique<Slider>(0.0f, 100.0f, 20.0f);
    Slider* s = owned.get();
    s->style().width = layout::Length::points(240.0f);
    ctx.setRootWidget(std::move(owned));
    s->layoutNode().computeLayout(240.0f, 30.0f);

    int commits = 0;
    s->onValueCommitted.connect([&](float) { ++commits; });

    WidgetMouseEvent pressIn{core::Point{50.0f, 12.0f}, platform::MouseButton::Left,
                             platform::KeyModifier::None};
    s->onMouseDown(pressIn);
    EXPECT_EQ(ctx.mouseGrabWidget(), s);

    // Dragging outside vertical bounds still updates value while grabbed
    WidgetMouseEvent dragOut{core::Point{230.0f, 150.0f}, platform::MouseButton::Left,
                             platform::KeyModifier::None};
    s->onMouseMove(dragOut);
    EXPECT_GT(s->value(), 80.0f);

    WidgetMouseEvent releaseOut{core::Point{230.0f, 150.0f}, platform::MouseButton::Left,
                                platform::KeyModifier::None};
    s->onMouseUp(releaseOut);
    EXPECT_EQ(ctx.mouseGrabWidget(), nullptr);
    EXPECT_EQ(commits, 1);

    // Disabling mid-drag releases grab and switches cursor to Arrow
    s->onMouseDown(pressIn);
    EXPECT_EQ(ctx.mouseGrabWidget(), s);
    s->setEnabled(false);
    EXPECT_EQ(ctx.mouseGrabWidget(), nullptr);
    EXPECT_EQ(s->cursorShape(), platform::CursorShape::Arrow);

    s->setEnabled(true);
    EXPECT_EQ(s->cursorShape(), platform::CursorShape::PointingHand);

    // Focus lost mid-drag releases grab
    s->onMouseDown(pressIn);
    EXPECT_EQ(ctx.mouseGrabWidget(), s);
    s->onFocusLost();
    EXPECT_EQ(ctx.mouseGrabWidget(), nullptr);
}

TEST(SliderTest, ContextDpiAdoptionCustomOverrideAndThemeRefresh) {
    WidgetContext ctx;
    ctx.setDpiScale(1.5f);
    auto owned =
        std::make_unique<Slider>(SliderSize::Standard, SliderRole::Accent, 0.0f, 100.0f, 50.0f);
    Slider* s = owned.get();
    ctx.setRootWidget(std::move(owned));

    EXPECT_FLOAT_EQ(s->dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(s->trackHeight(), 6.0f);
    EXPECT_FLOAT_EQ(s->thumbRadius(), 10.5f);
    ASSERT_NE(s->effectiveFont(), nullptr);
    EXPECT_FLOAT_EQ(s->effectiveFont()->pixelSize(), 18.0f);

    // Explicit trackHeight/thumbRadius overrides survive DPI changes
    Slider custom(0.0f, 100.0f, 50.0f);
    custom.setTrackHeight(10.0f);
    custom.setThumbRadius(12.0f);
    custom.setDpiScale(2.0f);
    EXPECT_FLOAT_EQ(custom.trackHeight(), 10.0f);
    EXPECT_FLOAT_EQ(custom.thumbRadius(), 12.0f);

    // Explicit setDpiScale survives context attachment
    WidgetContext ctx2;
    ctx2.setDpiScale(1.5f);
    auto explicitOwned = std::make_unique<Slider>(0.0f, 100.0f, 50.0f);
    Slider* explicitSlider = explicitOwned.get();
    explicitSlider->setDpiScale(2.0f);
    ctx2.setRootWidget(std::move(explicitOwned));
    EXPECT_FLOAT_EQ(explicitSlider->dpiScale(), 2.0f);

    // Idle updateAnimations does not mark paint dirty
    s->clearPaintDirty();
    s->updateAnimations(0.016f);
    EXPECT_FALSE(s->isPaintDirty());

    // Disabled inactive track mutes alpha; theme change marks paint dirty
    float enabledInactiveAlpha = s->effectiveInactiveTrackColor().a;
    s->setEnabled(false);
    EXPECT_LT(s->effectiveInactiveTrackColor().a, enabledInactiveAlpha);

    s->setEnabled(true);
    s->clearPaintDirty();
    ctx.setTheme(theme::Theme::light());
    EXPECT_TRUE(s->isPaintDirty());
}
