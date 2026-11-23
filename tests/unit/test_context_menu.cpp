#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/containers/context_menu.hpp"
#include "text/font_manager.hpp"

#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

namespace {

platform::MouseEvent makeMouseEvent(platform::MouseEventType type, platform::MouseButton button,
                                    core::Point pos) {
    platform::MouseEvent ev{};
    ev.type = type;
    ev.button = button;
    ev.position = pos;
    return ev;
}

platform::KeyEvent makeKeyEvent(platform::Key key, bool isDown) {
    platform::KeyEvent ev{};
    ev.key = key;
    ev.isDown = isDown;
    return ev;
}

} // namespace

TEST(ContextMenuTest, BuildItemsSeparatorsAndPreferredSize) {
    ContextMenu menu;
    menu.addItem("Copy", nullptr, "Ctrl+C");
    menu.addSeparator();
    menu.addItem("Paste", nullptr, "Ctrl+V", false);

    EXPECT_EQ(menu.items().size(), 3u);
    EXPECT_FALSE(menu.items()[0].isSeparator);
    EXPECT_TRUE(menu.items()[1].isSeparator);
    EXPECT_FALSE(menu.items()[2].enabled);

    core::Size pref = menu.preferredSize();
    EXPECT_GE(pref.width, 160.0f);
    EXPECT_GT(pref.height, 56.0f);
}

TEST(ContextMenuTest, RightClickReleaseTriggersBuilderAndOutsideReleaseCancels) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    root->style().width = layout::Length::points(200.0f);
    root->style().height = layout::Length::points(200.0f);

    int buildCount = 0;
    root->setContextMenuBuilder([&]() {
        ++buildCount;
        auto m = std::make_unique<ContextMenu>();
        m->addItem("Action", nullptr);
        return m;
    });

    ctx.setRootWidget(std::move(root));
    ctx.handleResize(core::Size{400.0f, 400.0f});

    // Right press inside, release outside -> should NOT open
    ctx.handleMouseDown(makeMouseEvent(platform::MouseEventType::ButtonDown,
                                       platform::MouseButton::Right, {50.0f, 50.0f}));
    ctx.handleMouseUp(makeMouseEvent(platform::MouseEventType::ButtonUp,
                                     platform::MouseButton::Right, {350.0f, 350.0f}));
    EXPECT_EQ(buildCount, 0);
    EXPECT_FALSE(ctx.hasOverlays());

    // Right press inside, release inside -> opens ContextMenu
    ctx.handleMouseDown(makeMouseEvent(platform::MouseEventType::ButtonDown,
                                       platform::MouseButton::Right, {50.0f, 50.0f}));
    ctx.handleMouseUp(makeMouseEvent(platform::MouseEventType::ButtonUp,
                                     platform::MouseButton::Right, {50.0f, 50.0f}));
    EXPECT_EQ(buildCount, 1);
    EXPECT_TRUE(ctx.hasOverlays());
}

TEST(ContextMenuTest, ClickItemExecutesAndDismissesDisabledDoesNot) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    ctx.setRootWidget(std::move(root));
    ctx.handleResize(core::Size{500.0f, 400.0f});

    int triggered = 0;
    auto menu = std::make_unique<ContextMenu>();
    menu->addItem("Disabled Action", [&]() { triggered = 99; }, "", false);
    menu->addSeparator();
    menu->addItem("Run Action", [&]() { triggered = 1; }, "F5", true);

    ContextMenu* rawMenu = ContextMenu::showAt(ctx, std::move(menu), core::Point{40.0f, 40.0f});
    ASSERT_NE(rawMenu, nullptr);
    EXPECT_TRUE(ctx.hasOverlays());

    core::Rect menuRect = rawMenu->screenRect();
    core::Size pref = rawMenu->preferredSize();
    EXPECT_FLOAT_EQ(menuRect.size.width, pref.width);
    EXPECT_FLOAT_EQ(menuRect.size.height, pref.height);
    EXPECT_LT(menuRect.size.width, 500.0f);
    EXPECT_LT(menuRect.size.height, 400.0f);

    // Click disabled first item (y ~ origin.y + 4 + 14)
    core::Point disabledPos{menuRect.origin.x + 20.0f, menuRect.origin.y + 18.0f};
    ctx.handleMouseDown(makeMouseEvent(platform::MouseEventType::ButtonDown,
                                       platform::MouseButton::Left, disabledPos));
    ctx.handleMouseUp(makeMouseEvent(platform::MouseEventType::ButtonUp,
                                     platform::MouseButton::Left, disabledPos));
    EXPECT_EQ(triggered, 0);
    EXPECT_TRUE(ctx.hasOverlays());

    // Click enabled third item (y ~ origin.y + 4 + 28 + 9 + 14 = +55)
    core::Point enabledPos{menuRect.origin.x + 20.0f, menuRect.origin.y + 55.0f};
    ctx.handleMouseDown(makeMouseEvent(platform::MouseEventType::ButtonDown,
                                       platform::MouseButton::Left, enabledPos));
    ctx.handleMouseUp(makeMouseEvent(platform::MouseEventType::ButtonUp,
                                     platform::MouseButton::Left, enabledPos));
    EXPECT_EQ(triggered, 1);
    EXPECT_FALSE(ctx.hasOverlays());
}

TEST(ContextMenuTest, KeyboardNavigationSkipsSeparatorsAndDisabledItems) {
    WidgetContext ctx;
    ctx.setRootWidget(std::make_unique<Container>());
    ctx.handleResize(core::Size{500.0f, 400.0f});

    int picked = -1;
    auto menu = std::make_unique<ContextMenu>();
    menu->addItem("First", [&]() { picked = 0; });
    menu->addSeparator();
    menu->addItem("Disabled", [&]() { picked = 2; }, "", false);
    menu->addItem("Last", [&]() { picked = 3; });

    ContextMenu* rawMenu = ContextMenu::showAt(ctx, std::move(menu), core::Point{20.0f, 20.0f});
    ASSERT_NE(rawMenu, nullptr);

    ctx.handleKeyEvent(makeKeyEvent(platform::Key::Down, true));
    EXPECT_EQ(rawMenu->highlightedIndex(), 0);

    // Next Down should skip separator (1) and disabled (2) straight to Last (3)
    ctx.handleKeyEvent(makeKeyEvent(platform::Key::Down, true));
    EXPECT_EQ(rawMenu->highlightedIndex(), 3);

    ctx.handleKeyEvent(makeKeyEvent(platform::Key::Enter, true));
    EXPECT_EQ(picked, 3);
    EXPECT_FALSE(ctx.hasOverlays());
}

TEST(ContextMenuTest, RichItemTypesAndGutter) {
    ContextMenu plainMenu;
    plainMenu.addItem("A Very Long Label That Easily Exceeds Minimum Menu Width", nullptr);
    EXPECT_FALSE(plainMenu.hasLeadingGutter());
    float plainWidth = plainMenu.preferredSize().width;

    ContextMenu richMenu;
    richMenu.addItem(IconId::Search, "A Very Long Label That Easily Exceeds Minimum Menu Width", nullptr);
    richMenu.addCheckItem("Show Grid", true, nullptr);
    richMenu.addRadioItem("Option A", false, nullptr);
    richMenu.addDestructiveItem("Delete", nullptr);

    EXPECT_TRUE(richMenu.hasLeadingGutter());
    EXPECT_EQ(richMenu.items().size(), 4u);
    EXPECT_EQ(richMenu.items()[0].type, MenuItemType::Standard);
    EXPECT_NE(richMenu.items()[0].icon, nullptr);
    EXPECT_EQ(richMenu.items()[1].type, MenuItemType::Checkbox);
    EXPECT_TRUE(richMenu.items()[1].checked);
    EXPECT_EQ(richMenu.items()[2].type, MenuItemType::Radio);
    EXPECT_FALSE(richMenu.items()[2].checked);
    EXPECT_TRUE(richMenu.items()[3].isDestructive);

    // Reserved gutter widens preferred width
    EXPECT_GE(richMenu.preferredSize().width, plainWidth + 24.0f);
}

TEST(ContextMenuTest, CheckboxToggleAndRadioActivation) {
    WidgetContext ctx;
    ctx.setRootWidget(std::make_unique<Container>());
    ctx.handleResize(core::Size{500.0f, 400.0f});

    bool toggledVal = false;
    int radioSelected = 0;

    auto menu = std::make_unique<ContextMenu>();
    menu->addCheckItem("Toggle Feature", false, [&](bool val) { toggledVal = val; });
    menu->addRadioItem("Radio Mode", false, [&]() { radioSelected = 1; });

    ContextMenu* rawMenu = ContextMenu::showAt(ctx, std::move(menu), core::Point{30.0f, 30.0f});
    ASSERT_NE(rawMenu, nullptr);

    // Activate checkbox (first item)
    core::Rect r = rawMenu->screenRect();
    core::Point checkPos{r.origin.x + 20.0f, r.origin.y + 14.0f};
    ctx.handleMouseDown(makeMouseEvent(platform::MouseEventType::ButtonDown,
                                       platform::MouseButton::Left, checkPos));
    ctx.handleMouseUp(makeMouseEvent(platform::MouseEventType::ButtonUp,
                                     platform::MouseButton::Left, checkPos));

    EXPECT_TRUE(toggledVal);
    EXPECT_FALSE(ctx.hasOverlays());

    // Open another menu to test radio
    auto radioMenu = std::make_unique<ContextMenu>();
    radioMenu->addRadioItem("Radio Mode", false, [&]() { radioSelected = 1; });
    ContextMenu* rawRadioMenu =
        ContextMenu::showAt(ctx, std::move(radioMenu), core::Point{30.0f, 30.0f});
    ASSERT_NE(rawRadioMenu, nullptr);

    core::Rect r2 = rawRadioMenu->screenRect();
    core::Point radioPos{r2.origin.x + 20.0f, r2.origin.y + 14.0f};
    ctx.handleMouseDown(makeMouseEvent(platform::MouseEventType::ButtonDown,
                                       platform::MouseButton::Left, radioPos));
    ctx.handleMouseUp(makeMouseEvent(platform::MouseEventType::ButtonUp,
                                     platform::MouseButton::Left, radioPos));

    EXPECT_EQ(radioSelected, 1);
    EXPECT_FALSE(ctx.hasOverlays());
}

TEST(ContextMenuTest, TextLayoutMemoizationAndTheming) {
    WidgetContext ctx;
    ctx.setTheme(theme::Theme::dark());
    ctx.setRootWidget(std::make_unique<Container>());
    ctx.handleResize(core::Size{500.0f, 400.0f});

    auto font = text::FontManager::instance().defaultFont(12.0f).valueOr(nullptr);
    auto menu = std::make_unique<ContextMenu>(font);
    menu->addItem("Cut", nullptr, "Ctrl+X");
    menu->addItem("Copy", nullptr, "Ctrl+C");

    ContextMenu* rawMenu = ContextMenu::showAt(ctx, std::move(menu), core::Point{50.0f, 50.0f});
    ASSERT_NE(rawMenu, nullptr);

    // Initial preferredSize populates cached layouts
    EXPECT_TRUE(rawMenu->items()[0].cachedLabelLayout.has_value());
    EXPECT_TRUE(rawMenu->items()[0].cachedShortcutLayout.has_value());

    const void* ptr1 = &(*rawMenu->items()[0].cachedLabelLayout);

    // Paint pass
    renderer::PaintContext paintCtx;
    rawMenu->paint(paintCtx);

    const void* ptr2 = &(*rawMenu->items()[0].cachedLabelLayout);
    EXPECT_EQ(ptr1, ptr2); // Reused same TextLayout instance!

    // Switch theme and verify paint succeeds cleanly
    ctx.setTheme(theme::Theme::light());
    paintCtx.clear();
    rawMenu->paint(paintCtx);
    EXPECT_FALSE(paintCtx.commands().empty());
}

TEST(ContextMenuTest, InstantOpeningWithoutAnimation) {
    WidgetContext ctx;
    ctx.setRootWidget(std::make_unique<Container>());
    ctx.handleResize(core::Size{500.0f, 400.0f});

    auto menu = std::make_unique<ContextMenu>();
    menu->addItem("Item 1", nullptr);
    menu->addItem("Item 2", nullptr);

    ContextMenu* rawMenu = ContextMenu::showAt(ctx, std::move(menu), core::Point{50.0f, 50.0f});
    ASSERT_NE(rawMenu, nullptr);

    // Context menu opens instantly with zero animation delay
    EXPECT_FALSE(rawMenu->hasActiveAnimations());

    // Paint immediately draws complete commands
    renderer::PaintContext paintCtx;
    rawMenu->paint(paintCtx);
    EXPECT_FALSE(paintCtx.commands().empty());
}

TEST(ContextMenuTest, SubmenuItemDefinitionAndChevron) {
    ContextMenu menu;
    menu.addSubmenu("Zoom", []() {
        auto sub = std::make_unique<ContextMenu>();
        sub->addItem("100%", nullptr);
        return sub;
    });

    ASSERT_EQ(menu.items().size(), 1u);
    EXPECT_EQ(menu.items()[0].type, MenuItemType::Submenu);
    EXPECT_NE(menu.items()[0].submenuBuilder, nullptr);

    // Submenu item instantiates properly via builder
    auto spawned = menu.items()[0].submenuBuilder();
    ASSERT_NE(spawned, nullptr);
    EXPECT_EQ(spawned->items().size(), 1u);

    // Preferred size width includes chevron reservation
    float w = menu.preferredSize().width;
    EXPECT_GE(w, 160.0f);

    // Paint pass emits draw commands including chevron lines
    menu.layoutNode().setLayoutRect(core::Rect{{0.0f, 0.0f}, menu.preferredSize()});
    renderer::PaintContext paintCtx;
    menu.paint(paintCtx);
    EXPECT_FALSE(paintCtx.commands().empty());
}

TEST(ContextMenuTest, SubmenuOpenOnHoverAndRetract) {
    WidgetContext ctx;
    ctx.setRootWidget(std::make_unique<Container>());
    ctx.handleResize(core::Size{800.0f, 600.0f});

    auto menu = std::make_unique<ContextMenu>();
    menu->addItem("Item 1", nullptr);
    menu->addSubmenu("Submenu", []() {
        auto sub = std::make_unique<ContextMenu>();
        sub->addItem("Sub Item A", nullptr);
        sub->addItem("Sub Item B", nullptr);
        return sub;
    });
    menu->addItem("Item 3", nullptr);

    ContextMenu* rawMenu = ContextMenu::showAt(ctx, std::move(menu), core::Point{50.0f, 50.0f});
    ASSERT_NE(rawMenu, nullptr);
    EXPECT_EQ(ctx.overlayStack().size(), 1u);

    // Hover submenu item (second item: local y ~ 4 + 28 + 14 = 46.0f)
    WidgetMouseEvent hoverSubEv{core::Point{60.0f, 46.0f}, platform::MouseButton::None,
                               platform::KeyModifier::None};
    rawMenu->onMouseMove(hoverSubEv);
    EXPECT_EQ(ctx.overlayStack().size(), 1u); // Not opened immediately (debounce)

    // Advance 0.08s (less than 0.15s debounce)
    rawMenu->updateAnimations(0.08f);
    EXPECT_EQ(ctx.overlayStack().size(), 1u);

    // Advance another 0.08s (total 0.16s >= 0.15s) -> Child submenu opens
    rawMenu->updateAnimations(0.08f);
    EXPECT_EQ(ctx.overlayStack().size(), 2u);

    // Hovering a different item (third item: local y ~ 4 + 28 + 28 + 14 = 74.0f) retracts child submenu
    WidgetMouseEvent hoverOtherEv{core::Point{60.0f, 74.0f}, platform::MouseButton::None,
                                 platform::KeyModifier::None};
    rawMenu->onMouseMove(hoverOtherEv);
    EXPECT_EQ(ctx.overlayStack().size(), 1u);
}

TEST(ContextMenuTest, SubmenuKeyboardNavigationAndActionDismissAll) {
    WidgetContext ctx;
    ctx.setRootWidget(std::make_unique<Container>());
    ctx.handleResize(core::Size{800.0f, 600.0f});

    int executedAction = 0;
    auto menu = std::make_unique<ContextMenu>();
    menu->addItem("First", nullptr);
    menu->addSubmenu("Cascading", [&]() {
        auto sub = std::make_unique<ContextMenu>();
        sub->addItem("Child Action 1", [&]() { executedAction = 1; });
        sub->addItem("Child Action 2", [&]() { executedAction = 2; });
        return sub;
    });
    menu->addItem("Last", nullptr);

    ContextMenu* rawMenu = ContextMenu::showAt(ctx, std::move(menu), core::Point{50.0f, 50.0f});
    ASSERT_NE(rawMenu, nullptr);

    // Down to First (0), then Down to Cascading (1)
    ctx.handleKeyEvent(makeKeyEvent(platform::Key::Down, true));
    ctx.handleKeyEvent(makeKeyEvent(platform::Key::Down, true));
    EXPECT_EQ(rawMenu->highlightedIndex(), 1);

    // Press Right Arrow on submenu item: should open child and focus it
    ctx.handleKeyEvent(makeKeyEvent(platform::Key::Right, true));
    EXPECT_EQ(ctx.overlayStack().size(), 2u);
    ContextMenu* childMenu = rawMenu->activeSubmenu();
    ASSERT_NE(childMenu, nullptr);
    EXPECT_EQ(ctx.focusedWidget(), childMenu);
    EXPECT_EQ(childMenu->highlightedIndex(), 0);

    // Press Left Arrow in child: should close child and refocus parent at row 1
    ctx.handleKeyEvent(makeKeyEvent(platform::Key::Left, true));
    EXPECT_EQ(ctx.overlayStack().size(), 1u);
    EXPECT_EQ(ctx.focusedWidget(), rawMenu);
    EXPECT_EQ(rawMenu->highlightedIndex(), 1);

    // Press Right Arrow again: opens child again
    ctx.handleKeyEvent(makeKeyEvent(platform::Key::Right, true));
    EXPECT_EQ(ctx.overlayStack().size(), 2u);

    // Navigate Down to Child Action 2 (index 1) and press Enter
    ctx.handleKeyEvent(makeKeyEvent(platform::Key::Down, true));
    ctx.handleKeyEvent(makeKeyEvent(platform::Key::Enter, true));

    EXPECT_EQ(executedAction, 2);
    EXPECT_FALSE(ctx.hasOverlays());
}
