#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/divider.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(DividerTest, HorizontalDefaultLayoutAndThickness) {
    Divider div;
    EXPECT_EQ(div.orientation(), DividerOrientation::Horizontal);
    EXPECT_FLOAT_EQ(div.thickness(), 1.0f);
    EXPECT_TRUE(div.label().empty());
    EXPECT_FALSE(div.isFocusable());

    EXPECT_TRUE(div.style().width.isPercent());
    EXPECT_FLOAT_EQ(div.style().width.value, 100.0f);
    EXPECT_TRUE(div.style().height.isPoints());
    EXPECT_FLOAT_EQ(div.style().height.value, 1.0f);

    div.setThickness(2.0f);
    EXPECT_FLOAT_EQ(div.thickness(), 2.0f);
    EXPECT_FLOAT_EQ(div.style().height.value, 2.0f);
}

TEST(DividerTest, VerticalOrientationLayout) {
    Divider div(DividerOrientation::Vertical);
    EXPECT_EQ(div.orientation(), DividerOrientation::Vertical);
    EXPECT_FLOAT_EQ(div.thickness(), 1.0f);

    EXPECT_TRUE(div.style().height.isPercent());
    EXPECT_FLOAT_EQ(div.style().height.value, 100.0f);
    EXPECT_TRUE(div.style().width.isPoints());
    EXPECT_FLOAT_EQ(div.style().width.value, 1.0f);

    div.setThickness(3.0f);
    EXPECT_FLOAT_EQ(div.thickness(), 3.0f);
    EXPECT_FLOAT_EQ(div.style().width.value, 3.0f);

    div.setOrientation(DividerOrientation::Horizontal);
    EXPECT_EQ(div.orientation(), DividerOrientation::Horizontal);
    EXPECT_FLOAT_EQ(div.style().height.value, 3.0f);
}

TEST(DividerTest, LabeledDividerProperties) {
    Divider div("SECTION");
    EXPECT_EQ(div.label(), "SECTION");
    EXPECT_FLOAT_EQ(div.gap(), 8.0f);

    div.setLabel("NEW SECTION");
    EXPECT_EQ(div.label(), "NEW SECTION");

    div.setGap(12.0f);
    EXPECT_FLOAT_EQ(div.gap(), 12.0f);

    div.setColor(core::Color{1.0f, 0.0f, 0.0f, 1.0f});
    EXPECT_EQ(div.color(), (core::Color{1.0f, 0.0f, 0.0f, 1.0f}));

    div.setTextColor(core::Color{0.0f, 1.0f, 0.0f, 1.0f});
    EXPECT_EQ(div.textColor(), (core::Color{0.0f, 1.0f, 0.0f, 1.0f}));
}

TEST(DividerTest, ThemingAndVariants) {
    WidgetContext ctx;
    auto divUnique = std::make_unique<Divider>();
    Divider* div = divUnique.get();
    ctx.setRootWidget(std::move(divUnique));

    const auto& pal = ctx.theme().colors();
    EXPECT_EQ(div->variant(), DividerVariant::Subtle);
    EXPECT_EQ(div->effectiveColor(), pal.borderSubtle);
    EXPECT_EQ(div->effectiveTextColor(), pal.textTertiary);

    div->setVariant(DividerVariant::Default);
    EXPECT_EQ(div->variant(), DividerVariant::Default);
    EXPECT_EQ(div->effectiveColor(), pal.borderDefault);

    div->setVariant(DividerVariant::Strong);
    EXPECT_EQ(div->variant(), DividerVariant::Strong);
    EXPECT_EQ(div->effectiveColor(), pal.borderStrong);

    // Overrides take precedence
    core::Color customColor{0.2f, 0.4f, 0.6f, 1.0f};
    div->setColor(customColor);
    EXPECT_EQ(div->effectiveColor(), customColor);
    EXPECT_EQ(div->color(), customColor);
}

TEST(DividerTest, InsetsAndTrimming) {
    Divider div(DividerOrientation::Horizontal);
    EXPECT_FLOAT_EQ(div.insetStart(), 0.0f);
    EXPECT_FLOAT_EQ(div.insetEnd(), 0.0f);

    div.setInset(16.0f);
    EXPECT_FLOAT_EQ(div.insetStart(), 16.0f);
    EXPECT_FLOAT_EQ(div.insetEnd(), 16.0f);

    div.setInset(8.0f, 24.0f);
    EXPECT_FLOAT_EQ(div.insetStart(), 8.0f);
    EXPECT_FLOAT_EQ(div.insetEnd(), 24.0f);

    // Negative insets clamp to 0
    div.setInset(-5.0f, -10.0f);
    EXPECT_FLOAT_EQ(div.insetStart(), 0.0f);
    EXPECT_FLOAT_EQ(div.insetEnd(), 0.0f);
}

TEST(DividerTest, LabelAlignmentAndOffset) {
    Divider div("ALIGNMENT");
    EXPECT_EQ(div.labelAlignment(), DividerAlignment::Center);
    EXPECT_FLOAT_EQ(div.labelOffset(), 24.0f);

    div.setLabelAlignment(DividerAlignment::Start);
    EXPECT_EQ(div.labelAlignment(), DividerAlignment::Start);

    div.setLabelOffset(32.0f);
    EXPECT_FLOAT_EQ(div.labelOffset(), 32.0f);

    div.setLabelAlignment(DividerAlignment::End);
    EXPECT_EQ(div.labelAlignment(), DividerAlignment::End);
}

TEST(DividerTest, PaintExecutionGeneratesDrawCommands) {
    WidgetContext ctx;
    auto divUnique = std::make_unique<Divider>("PAINT TEST");
    Divider* div = divUnique.get();
    div->setInset(8.0f);
    ctx.setRootWidget(std::move(divUnique));
    ctx.handleResize(core::Size{300.0f, 24.0f});

    renderer::PaintContext paintCtx;
    div->paint(paintCtx);

    EXPECT_FALSE(paintCtx.commands().empty());

    // Test Vertical Divider Paint
    div->setOrientation(DividerOrientation::Vertical);
    renderer::PaintContext vPaintCtx;
    div->paint(vPaintCtx);
    EXPECT_FALSE(vPaintCtx.commands().empty());
}

TEST(DividerTest, ContextDpiAdoptionAndCustomOverridePreservation) {
    WidgetContext ctx;
    ctx.setDpiScale(2.0f);
    auto divUnique = std::make_unique<Divider>("SECTION");
    Divider* div = divUnique.get();
    ctx.setRootWidget(std::move(divUnique));

    EXPECT_FLOAT_EQ(div->dpiScale(), 2.0f);
    EXPECT_FLOAT_EQ(div->thickness(), 2.0f);
    EXPECT_FLOAT_EQ(div->gap(), 16.0f);
    EXPECT_FLOAT_EQ(div->labelOffset(), 48.0f);
    ASSERT_NE(div->effectiveFont(), nullptr);
    EXPECT_FLOAT_EQ(div->effectiveFont()->pointSize(), 22.0f);

    // Explicit metric overrides survive DPI changes
    Divider custom("CUSTOM");
    custom.setThickness(3.0f);
    custom.setGap(10.0f);
    custom.setLabelOffset(30.0f);
    custom.setDpiScale(1.5f);
    EXPECT_FLOAT_EQ(custom.thickness(), 3.0f);
    EXPECT_FLOAT_EQ(custom.gap(), 10.0f);
    EXPECT_FLOAT_EQ(custom.labelOffset(), 30.0f);
    ASSERT_NE(custom.effectiveFont(), nullptr);
    EXPECT_FLOAT_EQ(custom.effectiveFont()->pointSize(), 16.5f);

    // Explicit setDpiScale is preserved on context attachment
    WidgetContext ctx2;
    ctx2.setDpiScale(2.0f);
    auto explicitUnique = std::make_unique<Divider>();
    Divider* explicitDiv = explicitUnique.get();
    explicitDiv->setDpiScale(1.5f);
    ctx2.setRootWidget(std::move(explicitUnique));
    EXPECT_FLOAT_EQ(explicitDiv->dpiScale(), 1.5f);
}

TEST(DividerTest, DisabledVisualsAndThemeChangeInvalidation) {
    WidgetContext ctx;
    auto divUnique = std::make_unique<Divider>("Section");
    Divider* div = divUnique.get();
    ctx.setRootWidget(std::move(divUnique));

    const core::Color enabledLine = div->effectiveColor();
    div->setEnabled(false);
    EXPECT_EQ(div->effectiveTextColor(), ctx.theme().colors().textDisabled);
    EXPECT_LT(div->effectiveColor().a, enabledLine.a);

    div->setEnabled(true);
    div->clearPaintDirty();
    ctx.setTheme(theme::Theme::light());
    EXPECT_TRUE(div->isPaintDirty());
    EXPECT_EQ(div->effectiveColor(), ctx.theme().colors().borderSubtle);
}
