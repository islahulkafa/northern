#include <gtest/gtest.h>

#include "renderer/paint_context.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/menu_bar.hpp"

using namespace northern;
using namespace northern::widgets;

TEST(MenuBarTest, CreationAndMenuAddition) {
    MenuBar bar;
    EXPECT_EQ(bar.menuCount(), 0u);

    auto* fileMenu = bar.addMenu("File");
    ASSERT_NE(fileMenu, nullptr);
    EXPECT_EQ(bar.menuCount(), 1u);
    EXPECT_EQ(bar.menuTitle(0), "File");
    EXPECT_EQ(bar.menuAt(0), fileMenu);

    auto* editMenu = bar.addMenu("Edit");
    ASSERT_NE(editMenu, nullptr);
    EXPECT_EQ(bar.menuCount(), 2u);
    EXPECT_EQ(bar.menuTitle(1), "Edit");
    EXPECT_EQ(bar.menuAt(1), editMenu);

    EXPECT_FLOAT_EQ(bar.preferredHeight(), 28.0f);

    bar.setDpiScale(1.5f);
    EXPECT_FLOAT_EQ(bar.preferredHeight(), 42.0f);
    EXPECT_FLOAT_EQ(bar.dpiScale(), 1.5f);
}

TEST(MenuBarTest, MouseClickAndToggleMenu) {
    WidgetContext ctx;
    auto bar = std::make_unique<MenuBar>();
    MenuBar* barPtr = bar.get();
    ctx.setRootWidget(std::move(bar));
    ctx.handleResize(core::Size{800.0f, 600.0f});

    auto* fileMenu = barPtr->addMenu("File");
    fileMenu->addItem("New", []() {});
    fileMenu->addItem("Open", []() {});

    auto* editMenu = barPtr->addMenu("Edit");
    editMenu->addItem("Undo", []() {});

    EXPECT_EQ(barPtr->openMenuIndex(), -1);
    EXPECT_FALSE(ctx.hasOverlays());

    // Click on "File" (index 0, roughly x=15, y=10)
    WidgetMouseEvent clickFile{core::Point{15.0f, 10.0f}, platform::MouseButton::Left,
                               platform::KeyModifier::None};
    barPtr->onMouseDown(clickFile);

    EXPECT_EQ(barPtr->openMenuIndex(), 0);
    EXPECT_TRUE(ctx.hasOverlays());
    EXPECT_EQ(ctx.topOverlay(), fileMenu);

    // Click on "File" again to toggle close
    barPtr->onMouseDown(clickFile);
    EXPECT_EQ(barPtr->openMenuIndex(), -1);
}

TEST(MenuBarTest, MouseHoverSmoothSwitching) {
    WidgetContext ctx;
    auto bar = std::make_unique<MenuBar>();
    MenuBar* barPtr = bar.get();
    ctx.setRootWidget(std::move(bar));
    ctx.handleResize(core::Size{800.0f, 600.0f});

    auto* fileMenu = barPtr->addMenu("File");
    fileMenu->addItem("New", []() {});

    auto* editMenu = barPtr->addMenu("Edit");
    editMenu->addItem("Undo", []() {});

    // Open File menu directly
    barPtr->openMenu(0);
    EXPECT_EQ(barPtr->openMenuIndex(), 0);
    EXPECT_EQ(ctx.topOverlay(), fileMenu);

    // Moving mouse over Edit menu (roughly x=60, y=10)
    WidgetMouseEvent moveEdit{core::Point{60.0f, 10.0f}, platform::MouseButton::None,
                              platform::KeyModifier::None};
    barPtr->onMouseMove(moveEdit);

    EXPECT_EQ(barPtr->openMenuIndex(), 1);
    EXPECT_EQ(ctx.topOverlay(), editMenu);
}

TEST(MenuBarTest, KeyboardNavigation) {
    WidgetContext ctx;
    auto bar = std::make_unique<MenuBar>();
    MenuBar* barPtr = bar.get();
    ctx.setRootWidget(std::move(bar));
    ctx.handleResize(core::Size{800.0f, 600.0f});

    barPtr->addMenu("File");
    barPtr->addMenu("Edit");
    barPtr->addMenu("View");

    // Right arrow highlights first menu
    WidgetKeyEvent rightEv{platform::Key::Right, true, platform::KeyModifier::None};
    barPtr->onKeyDown(rightEv);
    EXPECT_EQ(barPtr->highlightedIndex(), 0);

    // Right arrow again moves to Edit (index 1)
    barPtr->onKeyDown(rightEv);
    EXPECT_EQ(barPtr->highlightedIndex(), 1);

    // Left arrow moves back to File (index 0)
    WidgetKeyEvent leftEv{platform::Key::Left, true, platform::KeyModifier::None};
    barPtr->onKeyDown(leftEv);
    EXPECT_EQ(barPtr->highlightedIndex(), 0);

    // Left arrow wraps around to View (index 2)
    barPtr->onKeyDown(leftEv);
    EXPECT_EQ(barPtr->highlightedIndex(), 2);

    // Down arrow opens highlighted menu
    WidgetKeyEvent downEv{platform::Key::Down, true, platform::KeyModifier::None};
    barPtr->onKeyDown(downEv);
    EXPECT_EQ(barPtr->openMenuIndex(), 2);
    EXPECT_TRUE(ctx.hasOverlays());

    // Escape closes open menu
    WidgetKeyEvent escEv{platform::Key::Escape, true, platform::KeyModifier::None};
    barPtr->onKeyDown(escEv);
    EXPECT_EQ(barPtr->openMenuIndex(), -1);
}

TEST(MenuBarTest, GlobalAcceleratorParsingAndDispatch) {
    WidgetContext ctx;

    // Test parseShortcut
    auto s1 = WidgetContext::parseShortcut("Ctrl+S");
    ASSERT_TRUE(s1.has_value());
    EXPECT_EQ(s1->key, platform::Key::S);
    EXPECT_EQ(s1->modifiers, platform::KeyModifier::Control);

    auto s2 = WidgetContext::parseShortcut("Ctrl+Shift+P");
    ASSERT_TRUE(s2.has_value());
    EXPECT_EQ(s2->key, platform::Key::P);
    EXPECT_EQ(s2->modifiers, platform::KeyModifier::Control | platform::KeyModifier::Shift);

    auto s3 = WidgetContext::parseShortcut("Alt+F4");
    ASSERT_TRUE(s3.has_value());
    EXPECT_EQ(s3->key, platform::Key::F4);
    EXPECT_EQ(s3->modifiers, platform::KeyModifier::Alt);

    auto s4 = WidgetContext::parseShortcut("Delete");
    ASSERT_TRUE(s4.has_value());
    EXPECT_EQ(s4->key, platform::Key::Delete);
    EXPECT_EQ(s4->modifiers, platform::KeyModifier::None);

    // Test registration and dispatch
    bool triggered = false;
    ctx.registerAccelerator("Ctrl+S", [&triggered]() { triggered = true; });

    platform::KeyEvent evDown{platform::Key::S, true, false, 0, platform::KeyModifier::Control};
    ctx.handleKeyEvent(evDown);
    EXPECT_TRUE(triggered);

    // Unregister
    triggered = false;
    ctx.unregisterAccelerator("Ctrl+S");
    ctx.handleKeyEvent(evDown);
    EXPECT_FALSE(triggered);
}

TEST(MenuBarTest, MenuBarAcceleratorRegistration) {
    WidgetContext ctx;
    auto bar = std::make_unique<MenuBar>();
    MenuBar* barPtr = bar.get();

    bool saveTriggered = false;
    bool undoTriggered = false;

    auto* fileMenu = barPtr->addMenu("File");
    fileMenu->addItem("Save", [&saveTriggered]() { saveTriggered = true; }, "Ctrl+S");

    auto* editMenu = barPtr->addMenu("Edit");
    editMenu->addItem("Undo", [&undoTriggered]() { undoTriggered = true; }, "Ctrl+Z");

    barPtr->registerAccelerators(ctx);

    platform::KeyEvent ctrlS{platform::Key::S, true, false, 0, platform::KeyModifier::Control};
    ctx.handleKeyEvent(ctrlS);
    EXPECT_TRUE(saveTriggered);
    EXPECT_FALSE(undoTriggered);

    platform::KeyEvent ctrlZ{platform::Key::Z, true, false, 0, platform::KeyModifier::Control};
    ctx.handleKeyEvent(ctrlZ);
    EXPECT_TRUE(undoTriggered);
}

TEST(MenuBarTest, SubmenuBoundaryFlipping) {
    // Tests that overlay positioning for submenus flips leftward when colliding with the right edge
    core::Size viewport{800.0f, 600.0f};
    core::Size submenuSize{180.0f, 200.0f};

    OverlayDesc desc{};
    // Anchor placed near right edge (x=700, width=80 -> right edge is 780)
    desc.anchorRect = core::Rect{core::Point{700.0f, 100.0f}, core::Size{80.0f, 28.0f}};
    desc.preferredPlacement = OverlayPlacement::Right;
    desc.autoFlip = true;
    desc.gap = -2.0f;

    core::Point pos = computeOverlayPosition(viewport, submenuSize, desc);
    // 780 + 180 > 800, so it should flip leftwards
    // leftX = 700 - 180 - (-2.0) = 522
    EXPECT_FLOAT_EQ(pos.x, 522.0f);
}

TEST(MenuBarTest, CascadingSubmenuKeyboardNavigation) {
    WidgetContext ctx;
    auto menu = std::make_unique<ContextMenu>();
    ContextMenu* menuPtr = menu.get();

    bool subActionTriggered = false;
    menuPtr->addItem("Top Item", []() {});
    menuPtr->addSubmenu("More Options", [&subActionTriggered]() {
        auto sub = std::make_unique<ContextMenu>();
        sub->addItem("Sub Item", [&subActionTriggered]() { subActionTriggered = true; });
        return sub;
    });

    ctx.setRootWidget(std::make_unique<Widget>());
    ctx.handleResize(core::Size{800.0f, 600.0f});

    ContextMenu::showAt(ctx, std::move(menu), core::Point{100.0f, 100.0f});
    EXPECT_TRUE(ctx.hasOverlays());

    // Highlight submenu item (index 1)
    WidgetKeyEvent downEv{platform::Key::Down, true, platform::KeyModifier::None};
    menuPtr->onKeyDown(downEv); // index 0
    menuPtr->onKeyDown(downEv); // index 1
    EXPECT_EQ(menuPtr->highlightedIndex(), 1);

    // Right arrow opens submenu
    WidgetKeyEvent rightEv{platform::Key::Right, true, platform::KeyModifier::None};
    menuPtr->onKeyDown(rightEv);
    ASSERT_NE(menuPtr->activeSubmenu(), nullptr);

    // Submenu is focused
    ContextMenu* subMenu = menuPtr->activeSubmenu();
    EXPECT_EQ(ctx.focusedWidget(), subMenu);

    // Left arrow closes submenu and returns focus to parent
    WidgetKeyEvent leftEv{platform::Key::Left, true, platform::KeyModifier::None};
    subMenu->onKeyDown(leftEv);
    EXPECT_EQ(menuPtr->activeSubmenu(), nullptr);
    EXPECT_EQ(ctx.focusedWidget(), menuPtr);
}

TEST(MenuBarTest, RenderingSmokeTest) {
    WidgetContext ctx;
    auto bar = std::make_unique<MenuBar>();
    MenuBar* barPtr = bar.get();
    ctx.setRootWidget(std::move(bar));
    ctx.handleResize(core::Size{800.0f, 600.0f});

    barPtr->addMenu("File");
    barPtr->addMenu("Edit");
    barPtr->openMenu(0);

    renderer::PaintContext paintCtx;
    ctx.render(paintCtx, core::Size{800.0f, 600.0f});
    EXPECT_GT(paintCtx.commands().size(), 0u);
}
