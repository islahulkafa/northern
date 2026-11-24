#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/switch.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(SwitchTest, InitialStateAndDefaults) {
    Switch sw;
    EXPECT_FALSE(sw.isChecked());
    EXPECT_TRUE(sw.isFocusable());
    EXPECT_TRUE(sw.text().empty());
}

TEST(SwitchTest, SetCheckedAndToggle) {
    Switch sw;
    int toggleCount = 0;
    bool lastState = false;

    sw.onToggled.connect([&](bool checked) {
        ++toggleCount;
        lastState = checked;
    });

    sw.setChecked(true);
    EXPECT_TRUE(sw.isChecked());
    EXPECT_EQ(toggleCount, 1);
    EXPECT_TRUE(lastState);

    // No redundant emission if value doesn't change
    sw.setChecked(true);
    EXPECT_EQ(toggleCount, 1);

    sw.toggle();
    EXPECT_FALSE(sw.isChecked());
    EXPECT_EQ(toggleCount, 2);
    EXPECT_FALSE(lastState);
}

TEST(SwitchTest, MouseInteraction) {
    Switch sw;
    bool toggled = false;
    sw.onToggled.connect([&](bool c) { toggled = c; });

    WidgetMouseEvent downEv{core::Point{10.0f, 10.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    sw.onMouseDown(downEv);
    EXPECT_TRUE(downEv.isAccepted());
    EXPECT_FALSE(sw.isChecked()); // Toggles on mouse up

    WidgetMouseEvent upEv{core::Point{10.0f, 10.0f}, platform::MouseButton::Left,
                          platform::KeyModifier::None};
    sw.onMouseUp(upEv);
    EXPECT_TRUE(upEv.isAccepted());
    EXPECT_TRUE(sw.isChecked());
    EXPECT_TRUE(toggled);
}

TEST(SwitchTest, KeyboardActivation) {
    Switch sw;
    int toggleCount = 0;
    sw.onToggled.connect([&](bool) { ++toggleCount; });

    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    sw.onKeyDown(spaceEv);
    EXPECT_TRUE(spaceEv.isAccepted());
    EXPECT_TRUE(sw.isChecked());
    EXPECT_EQ(toggleCount, 1);

    WidgetKeyEvent enterEv{platform::Key::Enter, true, platform::KeyModifier::None};
    sw.onKeyDown(enterEv);
    EXPECT_TRUE(enterEv.isAccepted());
    EXPECT_FALSE(sw.isChecked());
    EXPECT_EQ(toggleCount, 2);
}

TEST(SwitchTest, SizePresetsFontFallbackAndDynamicTheming) {
    WidgetContext ctx;
    ctx.setTheme(theme::Theme::dark());

    auto owned =
        std::make_unique<Switch>("Auto Sync", SwitchSize::Large, SwitchRole::Success, true);
    Switch* sw = owned.get();
    ctx.setRootWidget(std::move(owned));

    EXPECT_FLOAT_EQ(sw->trackWidth(), 44.0f);
    EXPECT_FLOAT_EQ(sw->trackHeight(), 24.0f);
    ASSERT_NE(sw->effectiveFont(), nullptr);

    sw->layoutNode().computeLayout(240.0f, 40.0f);
    EXPECT_GT(sw->layoutRect().size.width, 50.0f);

    EXPECT_EQ(sw->effectiveTrackColor(), theme::ColorPalette::dark().success);
    EXPECT_EQ(sw->effectiveThumbColor(), core::Color::white());
    EXPECT_EQ(sw->effectiveTextColor(), theme::ColorPalette::dark().textPrimary);

    ctx.setTheme(theme::Theme::light());
    EXPECT_EQ(sw->effectiveTrackColor(), theme::ColorPalette::light().success);
    EXPECT_EQ(sw->effectiveThumbColor(), core::Color::white());
    EXPECT_EQ(sw->effectiveTextColor(), theme::ColorPalette::light().textPrimary);

    sw->setEnabled(false);
    EXPECT_EQ(sw->effectiveTextColor(), theme::ColorPalette::light().textDisabled);
    EXPECT_EQ(sw->effectiveThumbColor(), theme::ColorPalette::light().textDisabled);
}

TEST(SwitchTest, LeadingPlacementDragOffCancelAndDisabledGuards) {
    WidgetContext ctx;
    auto owned =
        std::make_unique<Switch>("Notifications", SwitchSize::Standard, SwitchRole::Accent, false);
    Switch* sw = owned.get();
    sw->setLabelPlacement(SwitchLabelPlacement::Leading);
    sw->style().width = layout::Length::points(220.0f);
    ctx.setRootWidget(std::move(owned));

    renderer::PaintContext pctx;
    ctx.render(pctx, {220.0f, 40.0f});

    // In Leading placement, text sits to the left of the switch track
    core::Point textOrigin{};
    core::Point trackOrigin{};
    for (const auto& cmd : pctx.commands()) {
        if (std::holds_alternative<renderer::DrawGlyphsCommand>(cmd)) {
            textOrigin = std::get<renderer::DrawGlyphsCommand>(cmd).origin;
        } else if (std::holds_alternative<renderer::DrawBoxCommand>(cmd)) {
            const auto& box = std::get<renderer::DrawBoxCommand>(cmd);
            if (box.rect.size.width > 30.0f) {
                trackOrigin = box.rect.origin;
            }
        }
    }
    EXPECT_LT(textOrigin.x, trackOrigin.x);

    // Drag-off mouse release cancels toggle and pointer click does not show keyboard focus ring
    WidgetMouseEvent downEv{core::Point{10.0f, 10.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    sw->onMouseDown(downEv);
    EXPECT_FALSE(sw->hasKeyboardFocus());
    EXPECT_TRUE(sw->hasActiveAnimations());

    WidgetMouseEvent outsideUp{core::Point{500.0f, 500.0f}, platform::MouseButton::Left,
                               platform::KeyModifier::None};
    sw->onMouseUp(outsideUp);
    EXPECT_FALSE(sw->isChecked());

    // Disabled switch ignores keyboard and mouse toggles
    sw->setEnabled(false);
    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    sw->onKeyDown(spaceEv);
    EXPECT_FALSE(spaceEv.isAccepted());
    EXPECT_FALSE(sw->isChecked());
}
