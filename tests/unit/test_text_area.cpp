#include "platform/clipboard.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/controls/button.hpp"
#include "widgets/controls/text_area.hpp"

#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

namespace {

WidgetKeyEvent makeCharKey(uint32_t codepoint,
                           platform::KeyModifier mods = platform::KeyModifier::None) {
    return WidgetKeyEvent{platform::Key::Unknown, true, mods, codepoint};
}

WidgetKeyEvent makeSpecialKey(platform::Key key,
                              platform::KeyModifier mods = platform::KeyModifier::None) {
    return WidgetKeyEvent{key, true, mods, 0};
}

} // namespace

TEST(TextAreaTest, InitialStateDefaultsAndLifecycleDpiAdoption) {
    WidgetContext ctx;
    ctx.setDpiScale(1.5f);

    auto owned = std::make_unique<TextArea>("Enter notes...");
    TextArea* area = owned.get();
    EXPECT_TRUE(area->isFocusable());
    EXPECT_EQ(area->cursorShape(), platform::CursorShape::IBeam);
    EXPECT_EQ(area->wordWrap(), text::WordWrap::Word);
    EXPECT_FALSE(area->showLineNumbers());
    EXPECT_FALSE(area->acceptsTab());
    EXPECT_EQ(area->lineCount(), 1);
    EXPECT_EQ(area->cursorLine(), 0);
    EXPECT_EQ(area->cursorColumn(), 0);

    ctx.setRootWidget(std::move(owned));
    EXPECT_FLOAT_EQ(area->dpiScale(), 1.5f);
    ASSERT_NE(area->effectiveFont(), nullptr);

    area->setEnabled(false);
    EXPECT_EQ(area->cursorShape(), platform::CursorShape::Arrow);
    area->setEnabled(true);
    EXPECT_EQ(area->cursorShape(), platform::CursorShape::IBeam);

    ctx.setTheme(theme::Theme::light());
    EXPECT_EQ(area->effectiveBackgroundColor(), theme::ColorPalette::light().surfaceDefault);
}

TEST(TextAreaTest, MultiLineTypingEnterAndCursorNavigation) {
    TextArea area;
    area.layoutNode().setLayoutRect(core::Rect{{0.0f, 0.0f}, {320.0f, 160.0f}});

    for (char c : std::string("Hello")) {
        auto ev = makeCharKey(static_cast<uint32_t>(c));
        area.onKeyDown(ev);
        EXPECT_TRUE(ev.isAccepted());
    }
    auto enterEv = makeSpecialKey(platform::Key::Enter);
    area.onKeyDown(enterEv);
    EXPECT_TRUE(enterEv.isAccepted());

    for (char c : std::string("World")) {
        auto ev = makeCharKey(static_cast<uint32_t>(c));
        area.onKeyDown(ev);
    }

    EXPECT_EQ(area.text(), "Hello\nWorld");
    EXPECT_EQ(area.lineCount(), 2);
    EXPECT_EQ(area.cursorLine(), 1);
    EXPECT_EQ(area.cursorColumn(), 5);

    // Move Up -> stays near end of "Hello"
    auto upEv = makeSpecialKey(platform::Key::Up);
    area.onKeyDown(upEv);
    EXPECT_EQ(area.cursorLine(), 0);
    EXPECT_EQ(area.cursorIndex(), 5u);

    // Home -> start of line 0, End -> end of line 0
    auto homeEv = makeSpecialKey(platform::Key::Home);
    area.onKeyDown(homeEv);
    EXPECT_EQ(area.cursorIndex(), 0u);
    auto endEv = makeSpecialKey(platform::Key::End);
    area.onKeyDown(endEv);
    EXPECT_EQ(area.cursorIndex(), 5u);

    // Ctrl+End -> end of document, Ctrl+Home -> start of document
    auto ctrlEnd = makeSpecialKey(platform::Key::End, platform::KeyModifier::Control);
    area.onKeyDown(ctrlEnd);
    EXPECT_EQ(area.cursorIndex(), area.text().size());
    auto ctrlHome = makeSpecialKey(platform::Key::Home, platform::KeyModifier::Control);
    area.onKeyDown(ctrlHome);
    EXPECT_EQ(area.cursorIndex(), 0u);
}

TEST(TextAreaTest, WordWrapVsNoWrapAndScrolling) {
    TextArea area;
    area.layoutNode().setLayoutRect(core::Rect{{0.0f, 0.0f}, {160.0f, 64.0f}});
    area.setText(
        "This is a long line that should wrap across multiple visual lines inside a narrow "
        "TextArea viewport.");

    EXPECT_GT(area.maxScrollOffsetY(), 0.0f);
    EXPECT_FLOAT_EQ(area.maxScrollOffsetX(), 0.0f);

    WidgetMouseEvent wheelDown{core::Point{20.0f, 20.0f}, platform::MouseButton::None,
                               platform::KeyModifier::None, core::Point{0.0f, -1.0f}};
    area.onMouseWheel(wheelDown);
    EXPECT_TRUE(wheelDown.isAccepted());
    EXPECT_GT(area.scrollOffsetY(), 0.0f);

    area.setWordWrap(text::WordWrap::None);
    EXPECT_GT(area.maxScrollOffsetX(), 0.0f);

    WidgetMouseEvent shiftWheel{core::Point{20.0f, 20.0f}, platform::MouseButton::None,
                                platform::KeyModifier::Shift, core::Point{0.0f, -1.0f}};
    area.onMouseWheel(shiftWheel);
    EXPECT_TRUE(shiftWheel.isAccepted());
    EXPECT_GT(area.scrollOffsetX(), 0.0f);
}

TEST(TextAreaTest, SelectionClipboardDoubleTripleClickAndWordJumps) {
    TextArea area;
    area.layoutNode().setLayoutRect(core::Rect{{0.0f, 0.0f}, {320.0f, 160.0f}});
    area.setText("alpha beta gamma\nsecond line");

    // Ctrl+Left / Ctrl+Right word navigation
    area.setCursorIndex(0);
    auto ctrlRight = makeSpecialKey(platform::Key::Right, platform::KeyModifier::Control);
    area.onKeyDown(ctrlRight);
    EXPECT_GT(area.cursorIndex(), 0u);

    // Select all & clipboard roundtrip
    auto ctrlA = makeSpecialKey(platform::Key::A, platform::KeyModifier::Control);
    area.onKeyDown(ctrlA);
    EXPECT_TRUE(area.hasSelection());
    EXPECT_EQ(area.selectedText(), "alpha beta gamma\nsecond line");

    auto ctrlC = makeSpecialKey(platform::Key::C, platform::KeyModifier::Control);
    area.onKeyDown(ctrlC);
    EXPECT_EQ(platform::Clipboard::getText().valueOr(""), "alpha beta gamma\nsecond line");

    // Double-click word selection then triple-click line selection
    WidgetMouseEvent click1{core::Point{14.0f, 14.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    area.onMouseDown(click1);
    WidgetMouseEvent click1Up{core::Point{14.0f, 14.0f}, platform::MouseButton::Left,
                              platform::KeyModifier::None};
    area.onMouseUp(click1Up);

    area.onMouseDown(click1);
    area.onMouseUp(click1Up);
    EXPECT_TRUE(area.hasSelection());
    EXPECT_EQ(area.selectedText(), "alpha");

    area.onMouseDown(click1);
    area.onMouseUp(click1Up);
    EXPECT_EQ(area.selectedText(), "alpha beta gamma");
}

TEST(TextAreaTest, UndoRedoCoalescingAndReadOnlyMaxLength) {
    TextArea area;
    area.layoutNode().setLayoutRect(core::Rect{{0.0f, 0.0f}, {300.0f, 120.0f}});

    for (char c : std::string("abc")) {
        auto ev = makeCharKey(static_cast<uint32_t>(c));
        area.onKeyDown(ev);
    }
    EXPECT_EQ(area.text(), "abc");
    EXPECT_TRUE(area.canUndo());

    auto ctrlZ = makeSpecialKey(platform::Key::Z, platform::KeyModifier::Control);
    area.onKeyDown(ctrlZ);
    EXPECT_TRUE(area.text().empty());
    EXPECT_TRUE(area.canRedo());

    auto ctrlY = makeSpecialKey(platform::Key::Y, platform::KeyModifier::Control);
    area.onKeyDown(ctrlY);
    EXPECT_EQ(area.text(), "abc");

    // MaxLength & ReadOnly
    area.setMaxLength(4);
    auto dEv = makeCharKey('d');
    auto eEv = makeCharKey('e');
    area.onKeyDown(dEv);
    area.onKeyDown(eEv);
    EXPECT_EQ(area.text(), "abcd");

    area.setReadOnly(true);
    auto bsEv = makeSpecialKey(platform::Key::Backspace);
    area.onKeyDown(bsEv);
    EXPECT_EQ(area.text(), "abcd");
}

TEST(TextAreaTest, TabBehaviorLineNumbersAndPaintCommands) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    root->setFlexDirection(layout::FlexDirection::Column);

    auto* area = root->addChild(std::make_unique<TextArea>());
    area->style().width = layout::Length::points(280.0f);
    area->style().height = layout::Length::points(120.0f);
    auto* nextBtn = root->addChild(std::make_unique<Button>("Next"));

    ctx.setRootWidget(std::move(root));
    ctx.handleResize(core::Size{320.0f, 200.0f});

    // When acceptsTab == false, Tab moves focus to nextBtn.
    area->setFocus(FocusReason::Keyboard);
    platform::KeyEvent tabDown{platform::Key::Tab, true, false, 0, platform::KeyModifier::None};
    ctx.handleKeyEvent(tabDown);
    EXPECT_EQ(ctx.focusedWidget(), nextBtn);

    // When acceptsTab == true, Tab inserts tabWidth spaces and stays focused.
    area->setFocus(FocusReason::Keyboard);
    area->setAcceptsTab(true);
    area->setTabWidth(2);
    ctx.handleKeyEvent(tabDown);
    EXPECT_EQ(ctx.focusedWidget(), area);
    EXPECT_EQ(area->text(), "  ");

    area->setShowLineNumbers(true);
    area->setText("Line 1\nLine 2\nLine 3");
    renderer::PaintContext pctx;
    ctx.render(pctx, core::Size{320.0f, 200.0f});
    EXPECT_FALSE(pctx.commands().empty());
}
