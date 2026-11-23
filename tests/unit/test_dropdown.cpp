#include "widgets/base/widget_context.hpp"
#include "widgets/containers/dropdown.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(DropdownTest, InitialStateAndItemOperations) {
    Dropdown dd{{"First", "Second", "Third"}};

    EXPECT_EQ(dd.itemCount(), 3);
    EXPECT_EQ(dd.selectedIndex(), -1);
    EXPECT_EQ(dd.selectedText(), "");
    EXPECT_FALSE(dd.isOpen());
    EXPECT_TRUE(dd.isFocusable());

    dd.addItem("Fourth");
    EXPECT_EQ(dd.itemCount(), 4);

    dd.setPlaceholder("Custom...");
    EXPECT_EQ(dd.placeholder(), "Custom...");

    dd.clearItems();
    EXPECT_EQ(dd.itemCount(), 0);
    EXPECT_EQ(dd.selectedIndex(), -1);
}

TEST(DropdownTest, SelectionAndSignalEmission) {
    Dropdown dd{{"Apple", "Banana", "Cherry"}};

    int lastIndex = -1;
    std::string lastText = "";
    int signalCount = 0;

    dd.onSelectionChanged.connect([&](int idx, const std::string& text) {
        lastIndex = idx;
        lastText = text;
        signalCount++;
    });

    dd.setSelectedIndex(1);
    EXPECT_EQ(dd.selectedIndex(), 1);
    EXPECT_EQ(dd.selectedText(), "Banana");
    EXPECT_EQ(lastIndex, 1);
    EXPECT_EQ(lastText, "Banana");
    EXPECT_EQ(signalCount, 1);

    // Setting same index should not re-emit
    dd.setSelectedIndex(1);
    EXPECT_EQ(signalCount, 1);

    // Clamping invalid indices
    dd.setSelectedIndex(100);
    EXPECT_EQ(dd.selectedIndex(), 2);
    EXPECT_EQ(dd.selectedText(), "Cherry");

    dd.setSelectedIndex(-5);
    EXPECT_EQ(dd.selectedIndex(), -1);
    EXPECT_EQ(dd.selectedText(), "");
}

TEST(DropdownTest, OpenCloseInstant) {
    WidgetContext ctx;
    auto dd = std::make_unique<Dropdown>(std::vector<std::string>{"Alpha", "Beta"});
    Dropdown* ddPtr = dd.get();
    ctx.setRootWidget(std::move(dd));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    EXPECT_FALSE(ddPtr->isOpen());
    EXPECT_FALSE(ctx.hasOverlays());
    EXPECT_FLOAT_EQ(ddPtr->chevronAnimationProgress(), 0.0f);
    EXPECT_FLOAT_EQ(ddPtr->hoverAnimationProgress(), 0.0f);
    EXPECT_FLOAT_EQ(ddPtr->popupAnimationProgress(), 0.0f);
    EXPECT_FALSE(ddPtr->hasActiveAnimations());

    // Hover animation
    ddPtr->onMouseEnter();
    EXPECT_TRUE(ddPtr->hasActiveAnimations());
    ctx.advanceAnimations(0.2f);
    EXPECT_FLOAT_EQ(ddPtr->hoverAnimationProgress(), 1.0f);
    EXPECT_FALSE(ddPtr->hasActiveAnimations());

    ddPtr->onMouseLeave();
    EXPECT_TRUE(ddPtr->hasActiveAnimations());
    ctx.advanceAnimations(0.2f);
    EXPECT_FLOAT_EQ(ddPtr->hoverAnimationProgress(), 0.0f);
    EXPECT_FALSE(ddPtr->hasActiveAnimations());

    // Open dropdown: starts chevron rotation and popup fade/slide animation
    ddPtr->open();
    EXPECT_TRUE(ddPtr->isOpen());
    EXPECT_TRUE(ctx.hasOverlays());
    EXPECT_TRUE(ddPtr->hasActiveAnimations());
    EXPECT_FLOAT_EQ(ddPtr->popupAnimationProgress(), 0.0f);

    ctx.advanceAnimations(0.25f);
    EXPECT_FALSE(ddPtr->hasActiveAnimations());
    EXPECT_FLOAT_EQ(ddPtr->chevronAnimationProgress(), 1.0f);
    EXPECT_FLOAT_EQ(ddPtr->popupAnimationProgress(), 1.0f);

    // Close dropdown: immediately dismisses overlay and reverses chevron
    ddPtr->close();
    EXPECT_FALSE(ddPtr->isOpen());
    EXPECT_FALSE(ctx.hasOverlays());
    EXPECT_TRUE(ddPtr->hasActiveAnimations());

    ctx.advanceAnimations(0.25f);
    EXPECT_FALSE(ddPtr->hasActiveAnimations());
    EXPECT_FLOAT_EQ(ddPtr->chevronAnimationProgress(), 0.0f);
}

TEST(DropdownTest, KeyboardNavigationHighlightAndConfirm) {
    WidgetContext ctx;
    auto dd = std::make_unique<Dropdown>(std::vector<std::string>{"Item 0", "Item 1", "Item 2"});
    Dropdown* ddPtr = dd.get();
    ctx.setRootWidget(std::move(dd));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    // Space toggles open
    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(spaceEv);
    EXPECT_TRUE(ddPtr->isOpen());
    EXPECT_TRUE(spaceEv.isAccepted());
    EXPECT_EQ(ddPtr->selectedIndex(), -1);
    EXPECT_EQ(ddPtr->highlightedIndex(), 0);

    // Down key moves highlight in popup, does NOT immediately change selectedIndex
    WidgetKeyEvent downEv{platform::Key::Down, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(downEv);
    EXPECT_EQ(ddPtr->highlightedIndex(), 1);
    EXPECT_EQ(ddPtr->selectedIndex(), -1);

    ddPtr->onKeyDown(downEv);
    EXPECT_EQ(ddPtr->highlightedIndex(), 2);
    EXPECT_EQ(ddPtr->selectedIndex(), -1);

    // Up key moves highlight back
    WidgetKeyEvent upEv{platform::Key::Up, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(upEv);
    EXPECT_EQ(ddPtr->highlightedIndex(), 1);
    EXPECT_EQ(ddPtr->selectedIndex(), -1);

    // Enter confirms highlight and closes popup
    WidgetKeyEvent enterEv{platform::Key::Enter, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(enterEv);
    EXPECT_FALSE(ddPtr->isOpen());
    EXPECT_EQ(ddPtr->selectedIndex(), 1);
    EXPECT_EQ(ddPtr->selectedText(), "Item 1");
}

TEST(DropdownTest, DynamicThemingAndCaching) {
    WidgetContext ctx;
    ctx.setTheme(theme::Theme::dark());
    auto dd = std::make_unique<Dropdown>(std::vector<std::string>{"First", "Second"}, 0);
    Dropdown* ddPtr = dd.get();
    ctx.setRootWidget(std::move(dd));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    renderer::PaintContext paintCtx;
    ddPtr->paint(paintCtx);
    size_t cmdCountDark = paintCtx.commands().size();
    EXPECT_GT(cmdCountDark, 0);

    // Switch theme to light and repaint
    ctx.setTheme(theme::Theme::light());
    paintCtx.clear();
    ddPtr->paint(paintCtx);
    size_t cmdCountLight = paintCtx.commands().size();
    EXPECT_EQ(cmdCountDark, cmdCountLight);
}

TEST(DropdownTest, SelectAndCloseHelper) {
    Dropdown dd{{"One", "Two", "Three"}};
    dd.open();
    EXPECT_TRUE(dd.isOpen());

    dd.selectAndClose(2);
    EXPECT_FALSE(dd.isOpen());
    EXPECT_EQ(dd.selectedIndex(), 2);
    EXPECT_EQ(dd.selectedText(), "Three");
}

TEST(DropdownTest, EscapeDismissWithoutChanging) {
    WidgetContext ctx;
    auto dd = std::make_unique<Dropdown>(std::vector<std::string>{"Alpha", "Beta", "Gamma"}, 0);
    Dropdown* ddPtr = dd.get();
    ctx.setRootWidget(std::move(dd));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    ddPtr->open();
    EXPECT_TRUE(ddPtr->isOpen());

    // Highlight moves to Beta
    WidgetKeyEvent downEv{platform::Key::Down, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(downEv);
    EXPECT_EQ(ddPtr->highlightedIndex(), 1);
    EXPECT_EQ(ddPtr->selectedIndex(), 0);

    // Escape cancels
    WidgetKeyEvent escEv{platform::Key::Escape, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(escEv);
    EXPECT_FALSE(ddPtr->isOpen());
    EXPECT_EQ(ddPtr->selectedIndex(), 0);
    EXPECT_EQ(ddPtr->selectedText(), "Alpha");
}

TEST(DropdownTest, OverflowListScrollbarAndCheckmark) {
    WidgetContext ctx;
    std::vector<std::string> manyItems;
    for (int i = 0; i < 15; ++i) {
        manyItems.push_back("Item " + std::to_string(i));
    }
    auto dd = std::make_unique<Dropdown>(manyItems, 2);
    Dropdown* ddPtr = dd.get();
    ctx.setRootWidget(std::move(dd));
    ctx.handleResize(core::Size{500.0f, 400.0f});

    ddPtr->open();
    EXPECT_TRUE(ddPtr->isOpen());
    EXPECT_NE(ddPtr->activePopup(), nullptr);

    // Layout popup and paint
    renderer::PaintContext paintCtx;
    ctx.render(paintCtx, core::Size{500.0f, 400.0f});

    // Check that paint commands recorded drawing
    EXPECT_GT(paintCtx.commands().size(), 0);

    // Navigate to bottom item (Item 14) and verify auto-scroll updates highlightedIndex
    WidgetKeyEvent endEv{platform::Key::End, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(endEv);
    EXPECT_EQ(ddPtr->highlightedIndex(), 14);
    EXPECT_EQ(ddPtr->selectedIndex(), 2); // Still unchanged

    // Confirm selection
    WidgetKeyEvent enterEv{platform::Key::Enter, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(enterEv);
    EXPECT_FALSE(ddPtr->isOpen());
    EXPECT_EQ(ddPtr->selectedIndex(), 14);
    EXPECT_EQ(ddPtr->selectedText(), "Item 14");
}

TEST(DropdownTest, ScrollbarDragAndWheelInteraction) {
    WidgetContext ctx;
    std::vector<std::string> manyItems;
    for (int i = 0; i < 20; ++i) {
        manyItems.push_back("Option " + std::to_string(i));
    }
    auto dd = std::make_unique<Dropdown>(manyItems, 0);
    Dropdown* ddPtr = dd.get();
    ctx.setRootWidget(std::move(dd));
    ctx.handleResize(core::Size{600.0f, 500.0f});

    ddPtr->open();
    EXPECT_TRUE(ddPtr->isOpen());

    renderer::PaintContext paintCtx;
    ctx.render(paintCtx, core::Size{600.0f, 500.0f});

    Widget* popupWidget = ddPtr->activePopup();
    ASSERT_NE(popupWidget, nullptr);

    // 1. Mouse wheel test: wheel delta {0, -1} scrolls down
    WidgetMouseEvent wheelDown{core::Point{50.0f, 50.0f}, platform::MouseButton::None,
                               platform::KeyModifier::None, core::Point{0.0f, -1.0f}};
    popupWidget->onMouseWheel(wheelDown);
    EXPECT_TRUE(wheelDown.isAccepted());

    // 2. Mouse down on scrollbar column (right edge): clicking track or thumb
    float popupW = popupWidget->layoutRect().size.width;
    WidgetMouseEvent clickScrollbar{core::Point{popupW - 6.0f, 100.0f}, platform::MouseButton::Left,
                                    platform::KeyModifier::None};
    popupWidget->onMouseDown(clickScrollbar);
    EXPECT_TRUE(clickScrollbar.isAccepted());

    // Dragging down
    WidgetMouseEvent dragMove{core::Point{popupW - 6.0f, 140.0f}, platform::MouseButton::Left,
                              platform::KeyModifier::None};
    popupWidget->onMouseMove(dragMove);
    EXPECT_TRUE(dragMove.isAccepted());

    // Mouse up on scrollbar: should NOT select item or close popup
    WidgetMouseEvent releaseScrollbar{core::Point{popupW - 6.0f, 140.0f},
                                      platform::MouseButton::Left, platform::KeyModifier::None};
    popupWidget->onMouseUp(releaseScrollbar);
    EXPECT_TRUE(releaseScrollbar.isAccepted());
    EXPECT_TRUE(ddPtr->isOpen());
    EXPECT_EQ(ddPtr->selectedIndex(), 0); // Not changed by scrollbar drag
}
