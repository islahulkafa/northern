#include "widgets/container.hpp"
#include "widgets/context_menu.hpp"
#include "widgets/widget_context.hpp"
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
