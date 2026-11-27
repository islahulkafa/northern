#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/dropdown.hpp"
#include <gtest/gtest.h>
#include <variant>

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

TEST(DropdownTest, SizePresetsDpiAndFallbackFont) {
    Dropdown dd{{"Alpha", "Beta"}};

    // Default is Standard (32px)
    EXPECT_EQ(dd.size(), DropdownSize::Standard);
    EXPECT_FLOAT_EQ(dd.preferredHeight(), 32.0f);

    dd.setSize(DropdownSize::Compact);
    EXPECT_FLOAT_EQ(dd.preferredHeight(), 28.0f);

    dd.setSize(DropdownSize::Comfortable);
    EXPECT_FLOAT_EQ(dd.preferredHeight(), 36.0f);

    // DPI Scaling
    dd.setSize(DropdownSize::Standard);
    dd.setDpiScale(1.5f);
    EXPECT_FLOAT_EQ(dd.preferredHeight(), 48.0f);
    EXPECT_FLOAT_EQ(dd.dpiScale(), 1.5f);

    // Font fallbacks: even if no font was supplied, effectiveFont is non-null
    EXPECT_NE(dd.effectiveFont(), nullptr);
}

TEST(DropdownTest, RichDropdownItemsAndSeparators) {
    std::vector<DropdownItem> items;
    items.push_back(DropdownItem{.text = "Save"});
    items.push_back(DropdownItem::separator());
    items.push_back(DropdownItem{.text = "Export", .isEnabled = false});

    Dropdown dd{items};
    EXPECT_EQ(dd.itemCount(), 3);
    EXPECT_EQ(dd.itemText(0), "Save");
    EXPECT_FALSE(dd.isSeparator(0));
    EXPECT_TRUE(dd.isItemEnabled(0));

    EXPECT_TRUE(dd.isSeparator(1));
    EXPECT_FALSE(dd.isItemEnabled(1));

    EXPECT_EQ(dd.itemText(2), "Export");
    EXPECT_FALSE(dd.isItemEnabled(2));

    // Toggle enabled state
    dd.setItemEnabled(2, true);
    EXPECT_TRUE(dd.isItemEnabled(2));

    // Add another item and separator dynamically
    dd.addSeparator();
    EXPECT_EQ(dd.itemCount(), 4);
    EXPECT_TRUE(dd.isSeparator(3));

    dd.addItem("Quit");
    EXPECT_EQ(dd.itemCount(), 5);
    EXPECT_EQ(dd.itemText(4), "Quit");
}

TEST(DropdownTest, DisabledItemsAndSeparatorNavigation) {
    WidgetContext ctx;
    std::vector<DropdownItem> items;
    items.push_back(DropdownItem{.text = "Item 0"});
    items.push_back(DropdownItem{.text = "Item 1", .isEnabled = false});
    items.push_back(DropdownItem::separator());
    items.push_back(DropdownItem{.text = "Item 3"});
    items.push_back(DropdownItem{.text = "Item 4", .isEnabled = false});

    auto dd = std::make_unique<Dropdown>(items, 0);
    Dropdown* ddPtr = dd.get();
    ctx.setRootWidget(std::move(dd));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    ddPtr->open();
    EXPECT_TRUE(ddPtr->isOpen());
    EXPECT_EQ(ddPtr->highlightedIndex(), 0);

    // Press Down -> should skip index 1 (disabled) and index 2 (separator) -> land on index 3
    WidgetKeyEvent downEv{platform::Key::Down, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(downEv);
    EXPECT_EQ(ddPtr->highlightedIndex(), 3);

    // Another Down -> cannot go to index 4 (disabled), stays on 3
    ddPtr->onKeyDown(downEv);
    EXPECT_EQ(ddPtr->highlightedIndex(), 3);

    // Press Up -> skips index 2 and index 1 -> lands on 0
    WidgetKeyEvent upEv{platform::Key::Up, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(upEv);
    EXPECT_EQ(ddPtr->highlightedIndex(), 0);

    // Enter confirms 0
    WidgetKeyEvent enterEv{platform::Key::Enter, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(enterEv);
    EXPECT_FALSE(ddPtr->isOpen());
    EXPECT_EQ(ddPtr->selectedIndex(), 0);
}

TEST(DropdownTest, TypeAheadPrefixSearch) {
    WidgetContext ctx;
    std::vector<DropdownItem> items;
    items.push_back(DropdownItem{.text = "Apple"});
    items.push_back(DropdownItem{.text = "Apricot"});
    items.push_back(DropdownItem{.text = "Banana"});
    items.push_back(DropdownItem{.text = "Blueberry", .isEnabled = false});
    items.push_back(DropdownItem{.text = "Cherry"});

    auto dd = std::make_unique<Dropdown>(items);
    Dropdown* ddPtr = dd.get();
    ctx.setRootWidget(std::move(dd));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    ddPtr->open();
    EXPECT_TRUE(ddPtr->isOpen());

    // Press 'C' -> jumps to "Cherry" (index 4)
    WidgetKeyEvent keyC{platform::Key::C, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(keyC);
    EXPECT_EQ(ddPtr->highlightedIndex(), 4);

    // Press 'B' -> jumps to "Banana" (index 2), skips "Blueberry" (disabled)
    WidgetKeyEvent keyB{platform::Key::B, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(keyB);
    EXPECT_EQ(ddPtr->highlightedIndex(), 2);

    // Confirm selection
    WidgetKeyEvent enterEv{platform::Key::Enter, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(enterEv);
    EXPECT_FALSE(ddPtr->isOpen());
    EXPECT_EQ(ddPtr->selectedIndex(), 2);
    EXPECT_EQ(ddPtr->selectedText(), "Banana");
}

TEST(DropdownTest, PopupDimensionsAndSeparators) {
    WidgetContext ctx;
    std::vector<DropdownItem> items;
    items.push_back(DropdownItem{.text = "Item 0"});
    items.push_back(DropdownItem::separator());
    items.push_back(DropdownItem{.text = "Item 1"});

    auto dd = std::make_unique<Dropdown>(items, 0, nullptr, DropdownSize::Standard);
    Dropdown* ddPtr = dd.get();
    ctx.setRootWidget(std::move(dd));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    ddPtr->open();
    EXPECT_TRUE(ddPtr->isOpen());

    Widget* popup = ddPtr->activePopup();
    ASSERT_NE(popup, nullptr);

    // Two 28px items (56px) + 6px separator + 8px vertical padding = 70px height
    EXPECT_FLOAT_EQ(popup->style().height.value, 70.0f);

    renderer::PaintContext paintCtx;
    ctx.render(paintCtx, core::Size{400.0f, 300.0f});
    EXPECT_GT(paintCtx.commands().size(), 0);
}

TEST(DropdownTest, DisabledIgnoresInteractionAndClosesOpenPopup) {
    WidgetContext ctx;
    auto dd = std::make_unique<Dropdown>(std::vector<std::string>{"One", "Two"});
    Dropdown* ddPtr = dd.get();
    ctx.setRootWidget(std::move(dd));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    ddPtr->onMouseEnter();
    ctx.advanceAnimations(0.2f);
    EXPECT_FLOAT_EQ(ddPtr->hoverAnimationProgress(), 1.0f);

    ddPtr->open();
    EXPECT_TRUE(ddPtr->isOpen());
    EXPECT_TRUE(ctx.hasOverlays());

    ddPtr->setEnabled(false);
    EXPECT_FALSE(ddPtr->isOpen());
    EXPECT_FALSE(ctx.hasOverlays());

    // Disabled dropdown must ignore open, mouse, and keyboard interaction
    ddPtr->open();
    EXPECT_FALSE(ddPtr->isOpen());

    WidgetMouseEvent downEv{core::Point{20.0f, 16.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    ddPtr->onMouseDown(downEv);
    EXPECT_FALSE(downEv.isAccepted());
    EXPECT_FALSE(ddPtr->isOpen());

    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    ddPtr->onKeyDown(spaceEv);
    EXPECT_FALSE(spaceEv.isAccepted());
    EXPECT_FALSE(ddPtr->isOpen());
}

TEST(DropdownTest, ContextAttachmentSyncsDpiAndThemeMarksDirty) {
    WidgetContext ctx;
    ctx.setDpiScale(1.5f);

    auto dd = std::make_unique<Dropdown>(std::vector<std::string>{"A", "B"}, 0);
    Dropdown* ddPtr = dd.get();
    ctx.setRootWidget(std::move(dd));

    EXPECT_FLOAT_EQ(ddPtr->dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(ddPtr->preferredHeight(), theme::Metrics::ControlHeightStandard * 1.5f);

    renderer::PaintContext pctx;
    ctx.render(pctx, core::Size{400.0f, 300.0f});
    EXPECT_FALSE(ddPtr->isPaintDirty());

    ctx.setTheme(theme::Theme::light());
    EXPECT_TRUE(ddPtr->isPaintDirty());
}

TEST(DropdownTest, PopupConcentricRadiiHoverPressAndMismatchCancel) {
    WidgetContext ctx;
    auto dd = std::make_unique<Dropdown>(std::vector<std::string>{"Alpha", "Beta", "Gamma"}, 0);
    Dropdown* ddPtr = dd.get();
    ctx.setRootWidget(std::move(dd));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    ddPtr->open();
    Widget* popup = ddPtr->activePopup();
    ASSERT_NE(popup, nullptr);

    renderer::PaintContext initPaint;
    ctx.render(initPaint, core::Size{400.0f, 300.0f});
    ctx.advanceAnimations(0.25f);

    // Move mouse over row 1 ("Beta", y ~ 4 + 28 + 14 = 46) -> fades hover pill in
    WidgetMouseEvent moveRow1{core::Point{20.0f, 46.0f}, platform::MouseButton::None,
                              platform::KeyModifier::None};
    popup->onMouseMove(moveRow1);
    EXPECT_TRUE(popup->hasActiveAnimations());
    ctx.advanceAnimations(0.15f);
    EXPECT_FALSE(popup->hasActiveAnimations());

    renderer::PaintContext hoverPaint;
    popup->paint(hoverPaint);

    // Outer popup card has 10px CornerRadiusOverlay; row 1 pill has 6px concentric radius & 0.55
    // alpha
    ASSERT_FALSE(hoverPaint.commands().empty());
    ASSERT_TRUE(std::holds_alternative<renderer::DrawBoxCommand>(hoverPaint.commands()[0]));
    const auto& cardBox = std::get<renderer::DrawBoxCommand>(hoverPaint.commands()[0]);
    EXPECT_FLOAT_EQ(cardBox.cornerRadii[0], theme::Metrics::CornerRadiusOverlay);

    bool foundHoverPill = false;
    for (size_t i = 1; i < hoverPaint.commands().size(); ++i) {
        if (std::holds_alternative<renderer::DrawBoxCommand>(hoverPaint.commands()[i])) {
            const auto& box = std::get<renderer::DrawBoxCommand>(hoverPaint.commands()[i]);
            if (box.borderWidth == 0.0f && std::abs(box.fillColor.a - 0.55f) < 0.03f) {
                EXPECT_FLOAT_EQ(box.cornerRadii[0], 6.0f);
                foundHoverPill = true;
            }
        }
    }
    EXPECT_TRUE(foundHoverPill);

    // Press row 1 ("Beta"), release on row 2 ("Gamma") -> mismatch cancels selection
    WidgetMouseEvent pressRow1{core::Point{20.0f, 46.0f}, platform::MouseButton::Left,
                               platform::KeyModifier::None};
    popup->onMouseDown(pressRow1);
    WidgetMouseEvent releaseRow2{core::Point{20.0f, 74.0f}, platform::MouseButton::Left,
                                 platform::KeyModifier::None};
    popup->onMouseUp(releaseRow2);
    EXPECT_EQ(ddPtr->selectedIndex(), 0);
}
