#include "platform/clipboard.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/icons.hpp"
#include "widgets/controls/text_input.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(TextInputTest, SizePresetsFontFallbackAndDynamicTheming) {
    WidgetContext ctx;
    ctx.setTheme(theme::Theme::dark());

    auto owned = std::make_unique<TextInput>("Search...", TextInputSize::Comfortable);
    TextInput* input = owned.get();
    ctx.setRootWidget(std::move(owned));

    ASSERT_NE(input->effectiveFont(), nullptr);
    float comfortablePt = input->effectiveFont()->pointSize();
    input->setSize(TextInputSize::Standard);
    float standardPt = input->effectiveFont()->pointSize();
    input->setSize(TextInputSize::Compact);
    float compactPt = input->effectiveFont()->pointSize();
    EXPECT_LT(compactPt, standardPt);
    EXPECT_LT(standardPt, comfortablePt);
    EXPECT_LT(comfortablePt, theme::typographyPointSize(theme::TypographyStyle::Body));
    input->setSize(TextInputSize::Comfortable);

    input->layoutNode().computeLayout(240.0f, 50.0f);
    EXPECT_FLOAT_EQ(input->layoutRect().size.height, theme::Metrics::ControlHeightComfortable);

    EXPECT_EQ(input->effectiveTextColor(), theme::ColorPalette::dark().textPrimary);
    EXPECT_EQ(input->effectivePlaceholderColor(), theme::ColorPalette::dark().textTertiary);

    ctx.setTheme(theme::Theme::light());
    EXPECT_EQ(input->effectiveTextColor(), theme::ColorPalette::light().textPrimary);
    EXPECT_EQ(input->effectiveBackgroundColor(), theme::ColorPalette::light().surfaceDefault);

    input->setEnabled(false);
    EXPECT_EQ(input->effectiveTextColor(), theme::ColorPalette::light().textDisabled);
    EXPECT_EQ(input->effectiveBorderColor(), theme::ColorPalette::light().borderSubtle);
}

TEST(TextInputTest, WordNavigationWordDeletionAndCut) {
    WidgetContext ctx;
    auto owned = std::make_unique<TextInput>("Enter words");
    TextInput* input = owned.get();
    ctx.setRootWidget(std::move(owned));
    input->setText("hello world test");
    input->setCursorIndex(input->text().size());
    input->setFocus();

    // Cursor starts at end (16); Ctrl+Left jumps to "test" (12)
    WidgetKeyEvent ctrlLeft{platform::Key::Left, true, platform::KeyModifier::Control};
    input->onKeyDown(ctrlLeft);
    EXPECT_EQ(input->cursorIndex(), 12u);

    // Shift+Ctrl+Left selects "world " backwards to index 6
    WidgetKeyEvent shiftCtrlLeft{platform::Key::Left, true,
                                 platform::KeyModifier::Control | platform::KeyModifier::Shift};
    input->onKeyDown(shiftCtrlLeft);
    EXPECT_EQ(input->cursorIndex(), 6u);
    EXPECT_EQ(input->selectedText(), "world ");

    // Ctrl+X cuts selection into clipboard and removes it from text
    WidgetKeyEvent ctrlX{platform::Key::X, true, platform::KeyModifier::Control};
    input->onKeyDown(ctrlX);
    EXPECT_EQ(input->text(), "hello test");
    auto clip = platform::Clipboard::getText();
    ASSERT_TRUE(clip.isOk());
    EXPECT_EQ(clip.value(), "world ");

    // Ctrl+Delete removes "test" ahead of cursor
    WidgetKeyEvent ctrlDel{platform::Key::Delete, true, platform::KeyModifier::Control};
    input->onKeyDown(ctrlDel);
    EXPECT_EQ(input->text(), "hello ");

    // Ctrl+Backspace removes "hello " back to 0
    WidgetKeyEvent ctrlBs{platform::Key::Backspace, true, platform::KeyModifier::Control};
    input->onKeyDown(ctrlBs);
    EXPECT_TRUE(input->text().empty());
}

TEST(TextInputTest, ReadOnlyMaxLengthPasswordModeAndFocusPlaceholder) {
    WidgetContext ctx;
    auto owned = std::make_unique<TextInput>("Password");
    TextInput* input = owned.get();
    input->setPasswordMode(true);
    input->setMaxLength(5);
    input->style().width = layout::Length::points(220.0f);
    ctx.setRootWidget(std::move(owned));
    input->setFocus();

    // Empty focused TextInput still paints its placeholder
    renderer::PaintContext pctx;
    ctx.render(pctx, {220.0f, 32.0f});
    bool drewGlyphs = false;
    for (const auto& cmd : pctx.commands()) {
        if (std::holds_alternative<renderer::DrawGlyphsCommand>(cmd)) {
            drewGlyphs = true;
        }
    }
    EXPECT_TRUE(drewGlyphs);

    // Typing respects maxLength (5 codepoints)
    input->setText("123456789");
    EXPECT_EQ(input->text(), "12345");

    // Password mode blocks clipboard copy/cut
    (void)platform::Clipboard::setText("safe");
    input->selectAll();
    WidgetKeyEvent ctrlC{platform::Key::C, true, platform::KeyModifier::Control};
    input->onKeyDown(ctrlC);
    EXPECT_EQ(platform::Clipboard::getText().value(), "safe");

    // Read-only blocks mutations while keeping selection working
    input->setReadOnly(true);
    WidgetKeyEvent typeA{platform::Key::A, true, platform::KeyModifier::None, 'A'};
    input->onKeyDown(typeA);
    EXPECT_EQ(input->text(), "12345");

    // Pointer click focuses and positions cursor accurately using cached clusterPenX_
    input->setPasswordMode(false);
    input->setReadOnly(false);
    input->setMaxLength(0);
    input->setText("ABCDEFGHIJ");
    core::Rect sRect = input->screenRect();
    platform::MouseEvent clickRight{};
    clickRight.type = platform::MouseEventType::ButtonDown;
    clickRight.button = platform::MouseButton::Left;
    clickRight.position = {sRect.origin.x + 200.0f, sRect.origin.y + 16.0f};
    ctx.handleMouseDown(clickRight);
    EXPECT_EQ(input->cursorIndex(), 10u);
    EXPECT_FALSE(input->hasKeyboardFocus());
}

TEST(TextInputTest, LeadingIconClearButtonAndBorderFade) {
    WidgetContext ctx;
    auto owned = std::make_unique<TextInput>("Search...");
    TextInput* input = owned.get();
    input->setLeadingIcon(IconId::Search);
    input->setClearButtonEnabled(true);
    input->style().width = layout::Length::points(240.0f);
    ctx.setRootWidget(std::move(owned));
    ctx.handleResize({240.0f, 32.0f});

    EXPECT_NE(input->leadingIcon(), nullptr);
    EXPECT_TRUE(input->isClearButtonEnabled());

    input->setText("northern");
    EXPECT_EQ(input->text(), "northern");

    input->onMouseEnter();
    EXPECT_TRUE(input->hasActiveAnimations());
    input->updateAnimations(0.20f);
    EXPECT_FALSE(input->hasActiveAnimations());

    // Clicking clear button near trailing edge resets text
    WidgetMouseEvent clearClick{core::Point{224.0f, 16.0f}, platform::MouseButton::Left,
                                platform::KeyModifier::None};
    input->onMouseDown(clearClick);
    EXPECT_TRUE(input->text().empty());

    // Focus then blur via WidgetContext; hovering afterwards must not flash focus ring
    ctx.setFocusedWidget(input, FocusReason::Pointer);
    ctx.advanceAnimations(0.20f);
    ctx.clearFocus();
    ctx.advanceAnimations(0.20f);
    EXPECT_FALSE(input->hasActiveAnimations());

    input->onMouseEnter();
    renderer::PaintContext pctx;
    input->paint(pctx);
    int boxCount = 0;
    for (const auto& cmd : pctx.commands()) {
        if (std::holds_alternative<renderer::DrawBoxCommand>(cmd)) {
            ++boxCount;
        }
    }
    // Only the input box itself (no outer focus ring)
    EXPECT_EQ(boxCount, 1);
}
