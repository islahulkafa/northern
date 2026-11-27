#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/progress_bar.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(ProgressBarTest, InitialStateAndDefaults) {
    ProgressBar bar;
    EXPECT_FLOAT_EQ(bar.progress(), 0.0f);
    EXPECT_FLOAT_EQ(bar.minimum(), 0.0f);
    EXPECT_FLOAT_EQ(bar.maximum(), 1.0f);
    EXPECT_FLOAT_EQ(bar.value(), 0.0f);
    EXPECT_FALSE(bar.isIndeterminate());
    EXPECT_FLOAT_EQ(bar.barHeight(), 4.0f);
    EXPECT_EQ(bar.role(), ProgressRole::Accent);
    EXPECT_FALSE(bar.showValue());
}

TEST(ProgressBarTest, ProgressClampingAndRangeMapping) {
    ProgressBar bar;
    bar.setProgress(0.5f);
    EXPECT_FLOAT_EQ(bar.progress(), 0.5f);

    bar.setProgress(-0.25f);
    EXPECT_FLOAT_EQ(bar.progress(), 0.0f);

    bar.setProgress(1.5f);
    EXPECT_FLOAT_EQ(bar.progress(), 1.0f);

    bar.setRange(10.0f, 50.0f);
    bar.setValue(30.0f);
    EXPECT_FLOAT_EQ(bar.progress(), 0.5f);
    EXPECT_FLOAT_EQ(bar.value(), 30.0f);

    bar.setValue(5.0f);
    EXPECT_FLOAT_EQ(bar.progress(), 0.0f);

    bar.setValue(100.0f);
    EXPECT_FLOAT_EQ(bar.progress(), 1.0f);
}

TEST(ProgressBarTest, SignalsEmitProgressAndCompletion) {
    ProgressBar bar;
    float lastProgress = -1.0f;
    int progressCount = 0;
    int completeCount = 0;

    bar.onProgressChanged.connect([&](float p) {
        lastProgress = p;
        ++progressCount;
    });
    bar.onCompleted.connect([&]() { ++completeCount; });

    bar.setProgress(0.4f);
    EXPECT_EQ(progressCount, 1);
    EXPECT_FLOAT_EQ(lastProgress, 0.4f);
    EXPECT_EQ(completeCount, 0);

    bar.setProgress(1.0f);
    EXPECT_EQ(progressCount, 2);
    EXPECT_FLOAT_EQ(lastProgress, 1.0f);
    EXPECT_EQ(completeCount, 1);

    // Repeating same progress does not re-emit completion
    bar.setProgress(1.0f);
    EXPECT_EQ(completeCount, 1);
}

TEST(ProgressBarTest, SizePresetsAndDynamicTheming) {
    ProgressBar bar(ProgressSize::Thick, ProgressRole::Success, 0.5f);
    EXPECT_FLOAT_EQ(bar.barHeight(), 8.0f);
    EXPECT_EQ(bar.role(), ProgressRole::Success);

    WidgetContext ctx;
    ctx.setTheme(theme::Theme::dark());
    auto owned =
        std::make_unique<ProgressBar>(ProgressSize::Standard, ProgressRole::Warning, 0.25f);
    ProgressBar* barPtr = owned.get();
    ctx.setRootWidget(std::move(owned));

    EXPECT_FLOAT_EQ(barPtr->barHeight(), 6.0f);
    EXPECT_EQ(barPtr->effectiveFillColor(), theme::ColorPalette::dark().warning);
    EXPECT_EQ(barPtr->effectiveTrackColor(), theme::ColorPalette::dark().surfaceRaised);

    ctx.setTheme(theme::Theme::light());
    EXPECT_EQ(barPtr->effectiveFillColor(), theme::ColorPalette::light().warning);
    EXPECT_EQ(barPtr->effectiveTrackColor(), theme::ColorPalette::light().borderDefault);

    barPtr->setEnabled(false);
    EXPECT_EQ(barPtr->effectiveFillColor(), theme::ColorPalette::light().textDisabled);
    barPtr->setEnabled(true);

    core::Color custom = core::Color::fromRgba8(10, 200, 150);
    barPtr->setFillColor(custom);
    EXPECT_EQ(barPtr->effectiveFillColor(), custom);

    barPtr->clearColorOverrides();
    EXPECT_EQ(barPtr->effectiveFillColor(), theme::ColorPalette::light().warning);
}

TEST(ProgressBarTest, IndeterminateAnimationZeroScissorAndReadout) {
    WidgetContext ctx;
    auto owned = std::make_unique<ProgressBar>(0.0f, true);
    ProgressBar* bar = owned.get();
    ctx.setRootWidget(std::move(owned));

    bar->style().width = layout::Length::points(240.0f);
    bar->layoutNode().computeLayout(240.0f, 30.0f);

    float initialOffset = bar->indeterminateOffset();
    bar->advanceAnimation(0.5f);
    EXPECT_NE(bar->indeterminateOffset(), initialOffset);

    bar->advanceAnimation(5.0f);
    EXPECT_GE(bar->indeterminateOffset(), 0.0f);
    EXPECT_LT(bar->indeterminateOffset(), 1.0f);

    // Paint indeterminate pulse mid-cycle and verify CPU-clipped rendering emits zero PushClip
    renderer::PaintContext pctx;
    bar->paint(pctx);
    EXPECT_EQ(pctx.commands().size(), 2u);
    for (const auto& cmd : pctx.commands()) {
        EXPECT_FALSE(std::holds_alternative<renderer::PushClipCommand>(cmd));
        EXPECT_FALSE(std::holds_alternative<renderer::PopClipCommand>(cmd));
    }

    // Verify determinate inline readout emits DrawGlyphsCommand
    bar->setIndeterminate(false);
    bar->setProgress(0.75f);
    bar->updateAnimations(1.0f);
    bar->setShowValue(true);
    bar->layoutNode().computeLayout(240.0f, 30.0f);

    pctx.clear();
    bar->paint(pctx);
    bool hasGlyphs = false;
    for (const auto& cmd : pctx.commands()) {
        if (std::holds_alternative<renderer::DrawGlyphsCommand>(cmd)) {
            hasGlyphs = true;
        }
    }
    EXPECT_TRUE(hasGlyphs);
}

TEST(ProgressBarTest, ContextDpiAdoptionAndCustomBarHeightPreservation) {
    WidgetContext ctx;
    ctx.setDpiScale(1.5f);
    auto owned = std::make_unique<ProgressBar>(ProgressSize::Standard, ProgressRole::Accent, 0.5f);
    ProgressBar* bar = owned.get();
    ctx.setRootWidget(std::move(owned));

    EXPECT_FLOAT_EQ(bar->dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(bar->barHeight(), 9.0f);
    ASSERT_NE(bar->effectiveFont(), nullptr);
    EXPECT_FLOAT_EQ(bar->effectiveFont()->pixelSize(), 18.0f);

    // Explicit barHeight survives DPI changes
    ProgressBar custom(ProgressSize::Thin);
    custom.setBarHeight(10.0f);
    custom.setDpiScale(2.0f);
    EXPECT_FLOAT_EQ(custom.barHeight(), 10.0f);

    // Explicit setDpiScale survives context attachment
    WidgetContext ctx2;
    ctx2.setDpiScale(1.5f);
    auto explicitOwned = std::make_unique<ProgressBar>(ProgressSize::Thin);
    ProgressBar* explicitBar = explicitOwned.get();
    explicitBar->setDpiScale(2.0f);
    ctx2.setRootWidget(std::move(explicitOwned));
    EXPECT_FLOAT_EQ(explicitBar->dpiScale(), 2.0f);
}

TEST(ProgressBarTest, DisabledFreezesAnimationAndReEnablesCleanly) {
    WidgetContext ctx;
    auto owned = std::make_unique<ProgressBar>(0.0f, true);
    ProgressBar* bar = owned.get();
    ctx.setRootWidget(std::move(owned));

    EXPECT_TRUE(bar->hasActiveAnimations());
    const float enabledTrackAlpha = bar->effectiveTrackColor().a;
    const float startOffset = bar->indeterminateOffset();

    bar->setEnabled(false);
    EXPECT_FALSE(bar->hasActiveAnimations());
    EXPECT_LT(bar->effectiveTrackColor().a, enabledTrackAlpha);

    bar->advanceAnimation(0.5f);
    EXPECT_FLOAT_EQ(bar->indeterminateOffset(), startOffset);

    bar->setEnabled(true);
    EXPECT_TRUE(bar->hasActiveAnimations());

    // Idle determinate bar does not mark paint dirty in updateAnimations
    bar->setIndeterminate(false);
    bar->clearPaintDirty();
    bar->updateAnimations(0.016f);
    EXPECT_FALSE(bar->isPaintDirty());

    // Theme change marks paint dirty
    bar->clearPaintDirty();
    ctx.setTheme(theme::Theme::light());
    EXPECT_TRUE(bar->isPaintDirty());
}
