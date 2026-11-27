#include <gtest/gtest.h>

#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/controls/button.hpp"
#include "widgets/controls/label.hpp"
#include "widgets/dialogs/dialog.hpp"
#include "widgets/dialogs/message_box.hpp"

using namespace northern::widgets;
using namespace northern::core;
using namespace northern::platform;

TEST(DialogTest, DefaultsAndStandardButtons) {
    Dialog dialog("Confirm Action");
    dialog.setStandardButtons(DialogButton::Ok | DialogButton::Cancel);

    EXPECT_EQ(dialog.title(), "Confirm Action");
    EXPECT_EQ(dialog.result(), DialogResult::Rejected);
    EXPECT_NE(dialog.button(DialogButton::Ok), nullptr);
    EXPECT_NE(dialog.button(DialogButton::Cancel), nullptr);
    EXPECT_EQ(dialog.button(DialogButton::Yes), nullptr);
    EXPECT_EQ(dialog.button(DialogButton::Ok)->variant(), ButtonVariant::Primary);
    EXPECT_EQ(dialog.button(DialogButton::Cancel)->variant(), ButtonVariant::Secondary);
}

TEST(DialogTest, ShowAsModalCenteredOverlayAndTrapFocus) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    root->style().width = northern::layout::Length::percent(100.0f);
    root->style().height = northern::layout::Length::percent(100.0f);
    auto* bgBtn = root->addChild(std::make_unique<Button>("Background"));
    ctx.setRootWidget(std::move(root));
    ctx.handleResize(Size{1024.0f, 768.0f});

    int bgClicks = 0;
    bgBtn->onClicked.connect([&bgClicks]() { bgClicks++; });

    auto dlg = std::make_unique<Dialog>("Modal Title");
    dlg->setStandardButtons(DialogButton::Ok | DialogButton::Cancel);
    auto* okBtn = dlg->button(DialogButton::Ok);
    auto* cancelBtn = dlg->button(DialogButton::Cancel);

    int acceptedCount = 0;
    dlg->onAccepted.connect([&acceptedCount]() { acceptedCount++; });

    Dialog* dlgPtr = Dialog::show(ctx, std::move(dlg));
    ASSERT_NE(dlgPtr, nullptr);
    EXPECT_TRUE(ctx.hasOverlays());
    ASSERT_EQ(ctx.overlayStack().size(), 1u);
    EXPECT_TRUE(ctx.overlayStack().back().desc.isModal);
    EXPECT_FALSE(ctx.overlayStack().back().desc.dismissOnOutsideClick);

    // Centered in viewport
    Rect sRect = dlgPtr->screenRect();
    EXPECT_GT(sRect.size.width, 200.0f);
    EXPECT_GT(sRect.size.height, 80.0f);
    EXPECT_LT(sRect.size.height, 700.0f);
    EXPECT_NEAR(sRect.origin.x + sRect.size.width * 0.5f, 512.0f, 1.0f);
    EXPECT_NEAR(sRect.origin.y + sRect.size.height * 0.5f, 384.0f, 1.0f);

    // Outside click does not dismiss dialog and does not click background button
    Point bgCenter{bgBtn->screenRect().origin.x + 5.0f, bgBtn->screenRect().origin.y + 5.0f};
    ctx.handleMouseDown(
        MouseEvent{MouseEventType::ButtonDown, MouseButton::Left, bgCenter, bgCenter});
    ctx.handleMouseUp(MouseEvent{MouseEventType::ButtonUp, MouseButton::Left, bgCenter, bgCenter});
    EXPECT_TRUE(ctx.hasOverlays());
    EXPECT_EQ(bgClicks, 0);

    // Tab focus stays trapped inside the modal dialog controls
    ctx.focusNext(false);
    Widget* f1 = ctx.focusedWidget();
    ctx.focusNext(false);
    Widget* f2 = ctx.focusedWidget();
    ctx.focusNext(false);
    Widget* f3 = ctx.focusedWidget();
    ASSERT_NE(f1, nullptr);
    ASSERT_NE(f2, nullptr);
    ASSERT_NE(f3, nullptr);
    EXPECT_TRUE(f1->isDescendantOf(dlgPtr));
    EXPECT_TRUE(f2->isDescendantOf(dlgPtr));
    EXPECT_TRUE(f3->isDescendantOf(dlgPtr));
    EXPECT_TRUE(f2 == okBtn || f2 == cancelBtn || f3 == okBtn || f3 == cancelBtn);
    EXPECT_NE(f1, bgBtn);
    EXPECT_NE(f2, bgBtn);
    EXPECT_NE(f3, bgBtn);

    // Click OK button accepts and dismisses
    Rect okRect = okBtn->screenRect();
    Point okCenter{okRect.origin.x + okRect.size.width * 0.5f,
                   okRect.origin.y + okRect.size.height * 0.5f};
    ctx.handleMouseDown(
        MouseEvent{MouseEventType::ButtonDown, MouseButton::Left, okCenter, okCenter});
    ctx.handleMouseUp(MouseEvent{MouseEventType::ButtonUp, MouseButton::Left, okCenter, okCenter});

    EXPECT_EQ(acceptedCount, 1);
    EXPECT_FALSE(ctx.hasOverlays());
}

TEST(DialogTest, EscapeRejectsAndEnterAcceptsDefault) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    ctx.setRootWidget(std::move(root));
    ctx.handleResize(Size{800.0f, 600.0f});

    // Test Escape dismissal -> Rejected
    {
        auto dlg = std::make_unique<Dialog>("Escape Test");
        dlg->setStandardButtons(DialogButton::Ok | DialogButton::Cancel);
        int rejectedCount = 0;
        DialogResult lastRes = DialogResult::Accepted;
        dlg->onRejected.connect([&rejectedCount]() { rejectedCount++; });
        dlg->onFinished.connect([&lastRes](DialogResult r) { lastRes = r; });

        Dialog::show(ctx, std::move(dlg));
        EXPECT_TRUE(ctx.hasOverlays());

        ctx.handleKeyEvent(KeyEvent{Key::Escape, true, false, 0, KeyModifier::None});
        EXPECT_EQ(rejectedCount, 1);
        EXPECT_EQ(lastRes, DialogResult::Rejected);
        EXPECT_FALSE(ctx.hasOverlays());
    }

    // Test Enter default button activation -> Accepted
    {
        auto dlg = std::make_unique<Dialog>("Enter Test");
        dlg->setStandardButtons(DialogButton::Ok | DialogButton::Cancel);
        int acceptedCount = 0;
        dlg->onAccepted.connect([&acceptedCount]() { acceptedCount++; });

        Dialog::show(ctx, std::move(dlg));
        EXPECT_TRUE(ctx.hasOverlays());

        ctx.handleKeyEvent(KeyEvent{Key::Enter, true, false, 0, KeyModifier::None});
        EXPECT_EQ(acceptedCount, 1);
        EXPECT_FALSE(ctx.hasOverlays());
    }
}

TEST(DialogTest, CustomContentAndDpiScaling) {
    Dialog dialog("Custom Dialog");
    dialog.setDpiScale(1.5f);
    dialog.setContent(std::make_unique<Label>("Custom body content"));
    auto* customBtn = dialog.addButton("Proceed", ButtonVariant::Primary, DialogResult::Accepted);

    EXPECT_FLOAT_EQ(dialog.dpiScale(), 1.5f);
    EXPECT_NE(customBtn, nullptr);
    EXPECT_NE(dialog.contentContainer(), nullptr);
    EXPECT_EQ(dialog.contentContainer()->childCount(), 1u);
}

TEST(MessageBoxTest, FactoriesAndConfirmFlow) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    ctx.setRootWidget(std::move(root));
    ctx.handleResize(Size{1024.0f, 768.0f});

    auto* infoBox = MessageBox::info(ctx, "About", "Northern Desktop UI Framework");
    ASSERT_NE(infoBox, nullptr);
    EXPECT_EQ(infoBox->iconType(), MessageBoxIcon::None);
    EXPECT_FALSE(infoBox->isCloseButtonVisible());
    EXPECT_NE(infoBox->button(DialogButton::Ok), nullptr);
    EXPECT_TRUE(ctx.hasOverlays());
    infoBox->accept();
    EXPECT_FALSE(ctx.hasOverlays());

    auto* warnBox = MessageBox::warning(ctx, "Warning", "Check configuration");
    ASSERT_NE(warnBox, nullptr);
    EXPECT_EQ(warnBox->iconType(), MessageBoxIcon::Warning);
    warnBox->accept();

    auto* errBox = MessageBox::error(ctx, "Error", "Failed to load asset");
    ASSERT_NE(errBox, nullptr);
    EXPECT_EQ(errBox->iconType(), MessageBoxIcon::Error);
    errBox->accept();

    DialogResult confirmResult = DialogResult::Rejected;
    auto* confirmBox = MessageBox::confirm(ctx, "Exit Application", "Close all open views?",
                                           [&confirmResult](DialogResult r) { confirmResult = r; });
    ASSERT_NE(confirmBox, nullptr);
    EXPECT_EQ(confirmBox->iconType(), MessageBoxIcon::None);
    ASSERT_NE(confirmBox->button(DialogButton::Yes), nullptr);
    ASSERT_NE(confirmBox->button(DialogButton::No), nullptr);

    confirmBox->button(DialogButton::Yes)->onClicked.emit();
    EXPECT_EQ(confirmResult, DialogResult::Accepted);
    EXPECT_FALSE(ctx.hasOverlays());

    // Modal scrim fade-in registers active animation and settles to 0.55f
    MessageBox::info(ctx, "Fade Test", "Scrim fades in.");
    EXPECT_TRUE(ctx.hasActiveAnimations());
    ctx.advanceAnimations(0.20f);
    EXPECT_FALSE(ctx.hasActiveAnimations());
}

TEST(DialogTest, ContextDpiAdoptionKeyUpGuardAndOutsideClickOptions) {
    WidgetContext ctx;
    ctx.setDpiScale(1.5f);
    auto root = std::make_unique<Container>();
    root->style().width = northern::layout::Length::percent(100.0f);
    root->style().height = northern::layout::Length::percent(100.0f);
    ctx.setRootWidget(std::move(root));
    ctx.handleResize(Size{1024.0f, 768.0f});

    auto dlg = std::make_unique<Dialog>("Options");
    dlg->setStandardButtons(DialogButton::Ok | DialogButton::Cancel);
    auto* cancelBtn = dlg->button(DialogButton::Cancel);
    dlg->setDefaultButton(cancelBtn);
    dlg->setDismissOnOutsideClick(true);
    EXPECT_EQ(dlg->defaultButton(), cancelBtn);
    EXPECT_TRUE(dlg->dismissOnOutsideClick());

    Dialog* dlgPtr = Dialog::show(ctx, std::move(dlg));
    ASSERT_NE(dlgPtr, nullptr);
    EXPECT_FLOAT_EQ(dlgPtr->dpiScale(), 1.5f);
    EXPECT_TRUE(ctx.overlayStack().back().desc.dismissOnOutsideClick);

    // Key-up events must not trigger Escape or Enter actions.
    ctx.handleKeyEvent(KeyEvent{Key::Escape, false, false, 0, KeyModifier::None});
    EXPECT_TRUE(ctx.hasOverlays());
    ctx.handleKeyEvent(KeyEvent{Key::Enter, false, false, 0, KeyModifier::None});
    EXPECT_TRUE(ctx.hasOverlays());

    // Enter activates custom defaultButton (Cancel -> Rejected).
    int rejectedCount = 0;
    dlgPtr->onRejected.connect([&rejectedCount]() { rejectedCount++; });
    ctx.handleKeyEvent(KeyEvent{Key::Enter, true, false, 0, KeyModifier::None});
    EXPECT_EQ(rejectedCount, 1);
    EXPECT_FALSE(ctx.hasOverlays());

    // MessageBox::info without explicit dpiScale adopts ctx.dpiScale().
    auto* infoBox = MessageBox::info(ctx, "Scaled", "Adopts context DPI scale");
    ASSERT_NE(infoBox, nullptr);
    EXPECT_FLOAT_EQ(infoBox->dpiScale(), 1.5f);
    ctx.setTheme(northern::theme::Theme::light());
    infoBox->accept();
    EXPECT_FALSE(ctx.hasOverlays());
}

TEST(MessageBoxTest, SetMessageSetIconTypeAndFontSync) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    ctx.setRootWidget(std::move(root));
    ctx.handleResize(Size{800.0f, 600.0f});

    MessageBox box("Notice", "Initial body", MessageBoxIcon::None, DialogButton::Ok);
    EXPECT_EQ(box.iconType(), MessageBoxIcon::None);

    box.setMessage("Updated body");
    EXPECT_EQ(box.message(), "Updated body");

    box.setIconType(MessageBoxIcon::Warning);
    EXPECT_EQ(box.iconType(), MessageBoxIcon::Warning);

    box.setIconType(MessageBoxIcon::None);
    EXPECT_EQ(box.iconType(), MessageBoxIcon::None);

    box.setFont(nullptr);
    box.open(ctx);
    EXPECT_TRUE(ctx.hasOverlays());
    box.accept();
    EXPECT_FALSE(ctx.hasOverlays());
}
