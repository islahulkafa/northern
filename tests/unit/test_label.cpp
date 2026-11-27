#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/controls/label.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(LabelTest, AutomaticDefaultFontFallback) {
    Label label("Automatic Font");
    ASSERT_NE(label.effectiveFont(), nullptr);

    label.layoutNode().computeLayout(200.0f, 50.0f);
    EXPECT_GT(label.layoutRect().size.width, 0.0f);
    EXPECT_GT(label.layoutRect().size.height, 0.0f);

    renderer::PaintContext ctx;
    label.paint(ctx);
    EXPECT_FALSE(ctx.commands().empty());
}

TEST(LabelTest, TypographyStylesScaleHierarchy) {
    Label display("Heading", theme::TypographyStyle::Display);
    Label title("Title", theme::TypographyStyle::Title);
    Label body("Body", theme::TypographyStyle::Body);
    Label caption("Caption", theme::TypographyStyle::Caption);

    ASSERT_NE(display.effectiveFont(), nullptr);
    ASSERT_NE(title.effectiveFont(), nullptr);
    ASSERT_NE(body.effectiveFont(), nullptr);
    ASSERT_NE(caption.effectiveFont(), nullptr);

    EXPECT_GT(display.effectiveFont()->pointSize(), title.effectiveFont()->pointSize());
    EXPECT_GT(title.effectiveFont()->pointSize(), body.effectiveFont()->pointSize());
    EXPECT_GT(body.effectiveFont()->pointSize(), caption.effectiveFont()->pointSize());

    display.layoutNode().computeLayout(400.0f, 200.0f);
    caption.layoutNode().computeLayout(400.0f, 200.0f);
    EXPECT_GT(display.layoutRect().size.height, caption.layoutRect().size.height);
}

TEST(LabelTest, DynamicTextRoleThemingAndDisabledState) {
    WidgetContext uiCtx;
    uiCtx.setTheme(theme::Theme::dark());

    auto labelPtr =
        std::make_unique<Label>("Status", theme::TypographyStyle::Body, theme::TextRole::Primary);
    Label* label = labelPtr.get();
    uiCtx.setRootWidget(std::move(labelPtr));

    EXPECT_EQ(label->effectiveColor(), theme::ColorPalette::dark().textPrimary);

    label->setRole(theme::TextRole::Accent);
    EXPECT_EQ(label->effectiveColor(), theme::ColorPalette::dark().accentDefault);

    uiCtx.setTheme(theme::Theme::light());
    label->setRole(theme::TextRole::Primary);
    EXPECT_EQ(label->effectiveColor(), theme::ColorPalette::light().textPrimary);

    label->setEnabled(false);
    EXPECT_EQ(label->effectiveColor(), theme::ColorPalette::light().textDisabled);
    label->setEnabled(true);

    core::Color customCol = core::Color::fromRgba8(255, 100, 50);
    label->setTextColor(customCol);
    EXPECT_EQ(label->effectiveColor(), customCol);

    label->clearColorOverride();
    EXPECT_EQ(label->effectiveColor(), theme::ColorPalette::light().textPrimary);
}

TEST(LabelTest, WordWrapOverflowEllipsisAndMaxLines) {
    Label label("The quick brown fox jumps over the lazy dog across multiple lines",
                theme::TypographyStyle::Body);
    label.setWordWrap(text::WordWrap::None);
    label.setOverflow(text::TextOverflow::Ellipsis);
    label.layoutNode().computeLayout(90.0f, 40.0f);

    renderer::PaintContext ctx;
    label.paint(ctx);
    EXPECT_TRUE(label.isTruncated());

    label.setWordWrap(text::WordWrap::Word);
    label.setMaxLines(2);
    label.layoutNode().computeLayout(90.0f, 200.0f);
    ctx.clear();
    label.paint(ctx);
    EXPECT_TRUE(label.isTruncated());
    EXPECT_LE(label.layoutRect().size.height, label.effectiveFont()->lineHeight() * 2.1f);
}

TEST(LabelTest, VerticalAlignmentPositioningAndLayoutCaching) {
    Label label("Short");
    label.style().width = layout::Length::points(300.0f);
    label.style().height = layout::Length::points(60.0f);
    label.layoutNode().computeLayout(300.0f, 60.0f);

    renderer::PaintContext ctxTop;
    label.setVerticalAlignment(VerticalAlignment::Top);
    label.paint(ctxTop);

    renderer::PaintContext ctxBottom;
    label.setVerticalAlignment(VerticalAlignment::Bottom);
    label.paint(ctxBottom);

    ASSERT_EQ(ctxTop.commands().size(), 1u);
    ASSERT_EQ(ctxBottom.commands().size(), 1u);
    const auto& cmdTop = std::get<renderer::DrawGlyphsCommand>(ctxTop.commands()[0]);
    const auto& cmdBottom = std::get<renderer::DrawGlyphsCommand>(ctxBottom.commands()[0]);
    EXPECT_LT(cmdTop.origin.y, cmdBottom.origin.y);

    // Wider resize for short unwrapped left-aligned text keeps layout fast-path consistent
    label.style().width = layout::Length::autoLength();
    label.style().height = layout::Length::autoLength();
    label.markLayoutDirty();
    label.layoutNode().computeLayout(300.0f, 60.0f);
    float initialW = label.layoutRect().size.width;
    label.layoutNode().computeLayout(500.0f, 60.0f);
    EXPECT_FLOAT_EQ(label.layoutRect().size.width, initialW);
}

TEST(LabelTest, ColumnContainerMaxWidthWrappingDoesNotOverlapSibling) {
    Container col;
    col.setFlexDirection(layout::FlexDirection::Column);
    col.setGap(12.0f);
    col.style().width = layout::Length::points(800.0f);
    col.style().height = layout::Length::points(400.0f);

    auto wrapped = std::make_unique<Label>(
        "Labels resolve system fonts automatically and support multi-line word wrapping.",
        theme::TypographyStyle::Body);
    wrapped->style().maxWidth = layout::Length::points(220.0f);
    Label* wrappedPtr = col.addChild(std::move(wrapped));

    auto sibling = std::make_unique<Label>("Next Row", theme::TypographyStyle::Monospace);
    Label* siblingPtr = col.addChild(std::move(sibling));

    col.layoutNode().computeLayout(800.0f, 400.0f);

    EXPECT_GT(wrappedPtr->layoutRect().size.height,
              wrappedPtr->effectiveFont()->lineHeight() * 1.5f);
    EXPECT_GE(siblingPtr->layoutRect().origin.y,
              wrappedPtr->layoutRect().origin.y + wrappedPtr->layoutRect().size.height + 11.5f);
}

TEST(LabelTest, ContextDpiAdoptionAndExplicitOverride) {
    Label unscaled("Body");
    ASSERT_NE(unscaled.effectiveFont(), nullptr);
    float basePt = unscaled.effectiveFont()->pointSize();

    WidgetContext ctx;
    ctx.setDpiScale(1.5f);
    auto lblUnique = std::make_unique<Label>("Scaled");
    Label* lbl = lblUnique.get();
    ctx.setRootWidget(std::move(lblUnique));

    EXPECT_FLOAT_EQ(lbl->dpiScale(), 1.5f);
    ASSERT_NE(lbl->effectiveFont(), nullptr);
    EXPECT_FLOAT_EQ(lbl->effectiveFont()->pointSize(), basePt * 1.5f);

    WidgetContext ctx2;
    ctx2.setDpiScale(1.5f);
    auto explicitUnique = std::make_unique<Label>("Explicit");
    Label* explicitLbl = explicitUnique.get();
    explicitLbl->setDpiScale(2.0f);
    ctx2.setRootWidget(std::move(explicitUnique));
    EXPECT_FLOAT_EQ(explicitLbl->dpiScale(), 2.0f);
}

TEST(LabelTest, ThemeEnabledAndFixedSizeTextPaintDirty) {
    WidgetContext ctx;
    auto lblUnique = std::make_unique<Label>("Initial");
    Label* lbl = lblUnique.get();
    ctx.setRootWidget(std::move(lblUnique));

    lbl->clearPaintDirty();
    ctx.setTheme(theme::Theme::light());
    EXPECT_TRUE(lbl->isPaintDirty());

    lbl->clearPaintDirty();
    lbl->setEnabled(false);
    EXPECT_TRUE(lbl->isPaintDirty());

    lbl->clearPaintDirty();
    lbl->setText("Updated");
    EXPECT_TRUE(lbl->isPaintDirty());

    lbl->clearPaintDirty();
    lbl->setTextStyle(theme::TypographyStyle::Title);
    EXPECT_TRUE(lbl->isPaintDirty());
}
