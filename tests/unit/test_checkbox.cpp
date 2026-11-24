#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/checkbox.hpp"

#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(CheckboxTest, InitialStateAndConstructors) {
    Checkbox cbDefault;
    EXPECT_EQ(cbDefault.checkState(), CheckState::Unchecked);
    EXPECT_FALSE(cbDefault.isChecked());
    EXPECT_FALSE(cbDefault.isIndeterminate());
    EXPECT_FALSE(cbDefault.isTristate());
    EXPECT_EQ(cbDefault.text(), "");

    Checkbox cbChecked("Checked Option", true);
    EXPECT_EQ(cbChecked.checkState(), CheckState::Checked);
    EXPECT_TRUE(cbChecked.isChecked());
    EXPECT_FALSE(cbChecked.isIndeterminate());
    EXPECT_EQ(cbChecked.text(), "Checked Option");

    Checkbox cbIndeterminate("Tri Option", CheckState::Indeterminate);
    EXPECT_EQ(cbIndeterminate.checkState(), CheckState::Indeterminate);
    EXPECT_FALSE(cbIndeterminate.isChecked());
    EXPECT_TRUE(cbIndeterminate.isIndeterminate());
}

TEST(CheckboxTest, BinaryToggleCycle) {
    Checkbox cb("Binary", false);
    EXPECT_FALSE(cb.isTristate());

    cb.toggle();
    EXPECT_EQ(cb.checkState(), CheckState::Checked);
    EXPECT_TRUE(cb.isChecked());

    cb.toggle();
    EXPECT_EQ(cb.checkState(), CheckState::Unchecked);
    EXPECT_FALSE(cb.isChecked());

    // If set to Indeterminate externally while tristate is false, toggle promotes to Checked
    cb.setCheckState(CheckState::Indeterminate);
    EXPECT_TRUE(cb.isIndeterminate());
    cb.toggle();
    EXPECT_EQ(cb.checkState(), CheckState::Checked);
}

TEST(CheckboxTest, TristateToggleCycle) {
    Checkbox cb("Tristate", false);
    cb.setTristate(true);
    EXPECT_TRUE(cb.isTristate());

    // Initial: Unchecked -> Checked
    cb.toggle();
    EXPECT_EQ(cb.checkState(), CheckState::Checked);
    EXPECT_TRUE(cb.isChecked());

    // Checked -> Indeterminate
    cb.toggle();
    EXPECT_EQ(cb.checkState(), CheckState::Indeterminate);
    EXPECT_TRUE(cb.isIndeterminate());

    // Indeterminate -> Unchecked
    cb.toggle();
    EXPECT_EQ(cb.checkState(), CheckState::Unchecked);
    EXPECT_FALSE(cb.isChecked());
    EXPECT_FALSE(cb.isIndeterminate());
}

TEST(CheckboxTest, SignalEmission) {
    Checkbox cb("Signals", false);
    cb.setTristate(true);

    CheckState lastToggledState = CheckState::Unchecked;
    int toggleCount = 0;
    cb.onToggled.connect([&](CheckState state) {
        lastToggledState = state;
        ++toggleCount;
    });

    bool lastCheckedBool = false;
    int checkedCount = 0;
    cb.onCheckedChanged.connect([&](bool c) {
        lastCheckedBool = c;
        ++checkedCount;
    });

    // Toggle 1: Unchecked -> Checked
    cb.toggle();
    EXPECT_EQ(lastToggledState, CheckState::Checked);
    EXPECT_TRUE(lastCheckedBool);
    EXPECT_EQ(toggleCount, 1);
    EXPECT_EQ(checkedCount, 1);

    // Toggle 2: Checked -> Indeterminate
    cb.toggle();
    EXPECT_EQ(lastToggledState, CheckState::Indeterminate);
    EXPECT_FALSE(lastCheckedBool);
    EXPECT_EQ(toggleCount, 2);
    EXPECT_EQ(checkedCount, 2);

    // Toggle 3: Indeterminate -> Unchecked
    cb.toggle();
    EXPECT_EQ(lastToggledState, CheckState::Unchecked);
    EXPECT_FALSE(lastCheckedBool);
    EXPECT_EQ(toggleCount, 3);
    EXPECT_EQ(checkedCount, 3);
}

TEST(CheckboxTest, ThemeDerivedColorsAndOverrides) {
    WidgetContext ctx;
    auto cbUnique = std::make_unique<Checkbox>("Themed", false);
    Checkbox* cb = cbUnique.get();
    ctx.setRootWidget(std::move(cbUnique));

    // Default theme colors
    const auto& pal = ctx.theme().colors();
    EXPECT_EQ(cb->effectiveCheckedColor(), pal.accentDefault);
    EXPECT_EQ(cb->effectiveUncheckedColor(), core::Color::transparent());
    EXPECT_EQ(cb->effectiveBorderColor(false), pal.borderDefault);
    EXPECT_EQ(cb->effectiveBorderColor(true), pal.borderStrong);
    EXPECT_EQ(cb->effectiveCheckmarkColor(), core::Color::white());
    EXPECT_EQ(cb->effectiveTextColor(), pal.textPrimary);

    // Overrides
    core::Color customChecked{1.0f, 0.0f, 0.0f, 1.0f};
    cb->setCheckedColor(customChecked);
    EXPECT_EQ(cb->effectiveCheckedColor(), customChecked);

    core::Color customBorder{0.0f, 1.0f, 0.0f, 1.0f};
    cb->setBorderColor(customBorder);
    EXPECT_EQ(cb->effectiveBorderColor(false), customBorder);

    core::Color customCheckmark{0.0f, 0.0f, 1.0f, 1.0f};
    cb->setCheckmarkColor(customCheckmark);
    EXPECT_EQ(cb->effectiveCheckmarkColor(), customCheckmark);

    core::Color customText{0.5f, 0.5f, 0.5f, 1.0f};
    cb->setTextColor(customText);
    EXPECT_EQ(cb->effectiveTextColor(), customText);
}

TEST(CheckboxTest, HoverAndAnimationLifecycle) {
    Checkbox cb("Hover Test", false);
    EXPECT_FALSE(cb.hasActiveAnimations());

    cb.onMouseEnter();
    EXPECT_TRUE(cb.hasActiveAnimations());

    // Advance animation
    cb.updateAnimations(0.2f);
    EXPECT_FALSE(cb.hasActiveAnimations());

    cb.onMouseLeave();
    EXPECT_TRUE(cb.hasActiveAnimations());
    cb.updateAnimations(0.2f);
    EXPECT_FALSE(cb.hasActiveAnimations());
}

TEST(CheckboxTest, CheckAnimationLifecycle) {
    Checkbox cb("Check Anim", false);
    EXPECT_FALSE(cb.hasActiveAnimations());

    cb.setChecked(true);
    EXPECT_TRUE(cb.hasActiveAnimations());

    cb.updateAnimations(0.2f);
    EXPECT_FALSE(cb.hasActiveAnimations());
    EXPECT_TRUE(cb.isChecked());
}

TEST(CheckboxTest, KeyboardActivation) {
    Checkbox cb("Key Nav", false);
    EXPECT_TRUE(cb.isFocusable());

    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    cb.onKeyDown(spaceEv);
    EXPECT_TRUE(cb.isChecked());

    WidgetKeyEvent enterEv{platform::Key::Enter, true, platform::KeyModifier::None};
    cb.onKeyDown(enterEv);
    EXPECT_FALSE(cb.isChecked());
}

TEST(CheckboxTest, DisabledBehavior) {
    Checkbox cb("Disabled", false);
    cb.setEnabled(false);
    EXPECT_FALSE(cb.isEnabled());

    cb.toggle();
    EXPECT_FALSE(cb.isChecked());

    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    cb.onKeyDown(spaceEv);
    EXPECT_FALSE(cb.isChecked());

    WidgetMouseEvent downEv{core::Point{5.0f, 5.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    cb.onMouseDown(downEv);
    EXPECT_FALSE(downEv.isAccepted());
    EXPECT_FALSE(cb.isChecked());

    cb.onMouseEnter();
    EXPECT_FALSE(cb.hasActiveAnimations());
}

TEST(CheckboxTest, PaintGeneratesDrawCommands) {
    WidgetContext ctx;
    auto cbUnique = std::make_unique<Checkbox>("Paint Test", true);
    Checkbox* cb = cbUnique.get();
    ctx.setRootWidget(std::move(cbUnique));
    ctx.handleResize(core::Size{200.0f, 40.0f});

    renderer::PaintContext paintCtx;
    cb->paint(paintCtx);

    // Box and checkmark lines must be emitted
    EXPECT_FALSE(paintCtx.commands().empty());
}
