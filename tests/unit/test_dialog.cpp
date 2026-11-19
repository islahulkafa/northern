#include "widgets/dialog.hpp"
#include "widgets/message_box.hpp"
#include "widgets/widget_context.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(DialogTest, StructureAndProperties) {
    Dialog dlg("Confirm Action");
    EXPECT_EQ(dlg.title(), "Confirm Action");

    dlg.setMessage("Are you sure you want to proceed?");
    EXPECT_EQ(dlg.message(), "Are you sure you want to proceed?");

    dlg.setTitle("New Title");
    EXPECT_EQ(dlg.title(), "New Title");
}

TEST(DialogTest, OpenAndSignals) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    ctx.setRootWidget(std::move(root));

    EXPECT_FALSE(ctx.hasOverlays());

    auto* dlg = new Dialog("Modal Test");
    bool accepted = false;
    bool rejected = false;
    DialogResult resultState = DialogResult::Custom;

    dlg->onAccepted.connect([&]() { accepted = true; });
    dlg->onRejected.connect([&]() { rejected = true; });
    dlg->onFinished.connect([&](DialogResult r) { resultState = r; });

    dlg->open(ctx);

    EXPECT_TRUE(ctx.hasOverlays());
    EXPECT_TRUE(ctx.overlayStack().back().desc.isModal);

    // Accept dialog
    dlg->accept();
    EXPECT_TRUE(accepted);
    EXPECT_FALSE(rejected);
    EXPECT_EQ(resultState, DialogResult::Accepted);
    EXPECT_FALSE(ctx.hasOverlays());
}

TEST(DialogTest, KeyNavigationEscapeAndEnter) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    ctx.setRootWidget(std::move(root));

    auto* dlg = new Dialog("Key Test");
    bool rejected = false;
    dlg->onRejected.connect([&]() { rejected = true; });

    dlg->open(ctx);
    EXPECT_TRUE(ctx.hasOverlays());

    WidgetKeyEvent escEv{platform::Key::Escape, true, platform::KeyModifier::None};
    dlg->onKeyDown(escEv);
    EXPECT_TRUE(escEv.isAccepted());
    EXPECT_TRUE(rejected);
    EXPECT_FALSE(ctx.hasOverlays());
}

TEST(DialogTest, MessageBoxHelpers) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    ctx.setRootWidget(std::move(root));

    bool dismissed = false;
    MessageBox::showInformation(ctx, "Notice", "Information message", nullptr,
                                [&]() { dismissed = true; });

    EXPECT_TRUE(ctx.hasOverlays());
    auto* topDlg = dynamic_cast<Dialog*>(ctx.topOverlay());
    ASSERT_NE(topDlg, nullptr);
    EXPECT_EQ(topDlg->title(), "Notice");
    EXPECT_EQ(topDlg->message(), "Information message");

    topDlg->accept();
    EXPECT_TRUE(dismissed);
    EXPECT_FALSE(ctx.hasOverlays());
}
