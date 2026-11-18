#include "widgets/dropdown.hpp"
#include "widgets/widget_context.hpp"
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

TEST(DropdownTest, OpenCloseAndAnimation) {
    WidgetContext ctx;
    auto dd = std::make_unique<Dropdown>(std::vector<std::string>{"Alpha", "Beta"});
    Dropdown* ddPtr = dd.get();
    ctx.setRootWidget(std::move(dd));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    EXPECT_FALSE(ddPtr->isOpen());
    EXPECT_FALSE(ctx.hasOverlays());

    ddPtr->open();
    EXPECT_TRUE(ddPtr->isOpen());
    EXPECT_TRUE(ctx.hasOverlays());
    EXPECT_TRUE(ddPtr->hasActiveAnimations());

    // Advance animation
    ctx.advanceAnimations(0.20f);
    EXPECT_FALSE(ddPtr->hasActiveAnimations());

    ddPtr->close();
    EXPECT_FALSE(ddPtr->isOpen());
    EXPECT_FALSE(ctx.hasOverlays());
}

TEST(DropdownTest, KeyboardNavigation) {
    Dropdown dd{{"Item 0", "Item 1", "Item 2"}};

    // Space toggles open
    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    dd.onKeyDown(spaceEv);
    EXPECT_TRUE(dd.isOpen());
    EXPECT_TRUE(spaceEv.isAccepted());

    // Down key selects next item
    WidgetKeyEvent downEv{platform::Key::Down, true, platform::KeyModifier::None};
    dd.onKeyDown(downEv);
    EXPECT_EQ(dd.selectedIndex(), 0);

    dd.onKeyDown(downEv);
    EXPECT_EQ(dd.selectedIndex(), 1);

    // Up key selects previous item
    WidgetKeyEvent upEv{platform::Key::Up, true, platform::KeyModifier::None};
    dd.onKeyDown(upEv);
    EXPECT_EQ(dd.selectedIndex(), 0);
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
