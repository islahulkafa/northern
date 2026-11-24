#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/icon.hpp"
#include "widgets/controls/icons.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace northern;
using namespace northern::widgets;

TEST(IconTest, IconCatalogCoverage) {
    // Verify all IconIds in the expanded catalog return valid 24x24 SVG documents
    for (size_t i = 0; i < kTotalIconCount; ++i) {
        IconId id = static_cast<IconId>(i);
        std::string_view svg = getSvgString(id);
        EXPECT_FALSE(svg.empty());
        EXPECT_NE(svg.find("<svg"), std::string_view::npos);

        auto doc = getSvgDocument(id);
        ASSERT_NE(doc, nullptr);
        EXPECT_TRUE(doc->isValid());
        EXPECT_FLOAT_EQ(doc->size().width, 24.0f);
        EXPECT_FLOAT_EQ(doc->size().height, 24.0f);
    }
}

TEST(IconTest, IconWidgetProperties) {
    Icon icon(IconId::Search, 20.0f, core::Color::fromRgba8(255, 0, 0));
    EXPECT_EQ(icon.iconId(), IconId::Search);
    EXPECT_FLOAT_EQ(icon.iconSize(), 20.0f);
    EXPECT_EQ(icon.color(), core::Color::fromRgba8(255, 0, 0));

    icon.setIcon(IconId::Folder);
    EXPECT_EQ(icon.iconId(), IconId::Folder);

    icon.setIconSize(32.0f);
    EXPECT_FLOAT_EQ(icon.iconSize(), 32.0f);
    EXPECT_FLOAT_EQ(icon.style().width.value, 32.0f);
    EXPECT_FLOAT_EQ(icon.style().height.value, 32.0f);

    icon.setColor(core::Color::white());
    EXPECT_EQ(icon.color(), core::Color::white());
}

TEST(IconTest, IconThemingAndRoles) {
    WidgetContext ctx;
    auto iconUnique = std::make_unique<Icon>(IconId::Settings);
    Icon* icon = iconUnique.get();
    ctx.setRootWidget(std::move(iconUnique));

    const auto& pal = ctx.theme().colors();
    EXPECT_EQ(icon->role(), IconRole::Primary);
    EXPECT_EQ(icon->effectiveColor(), pal.textPrimary);

    icon->setRole(IconRole::Secondary);
    EXPECT_EQ(icon->effectiveColor(), pal.textSecondary);

    icon->setRole(IconRole::Accent);
    EXPECT_EQ(icon->effectiveColor(), pal.accentDefault);

    icon->setRole(IconRole::Danger);
    EXPECT_EQ(icon->effectiveColor(), pal.danger);

    // Explicit override overrides role
    icon->setColor(core::Color{1.0f, 0.0f, 1.0f, 1.0f});
    EXPECT_EQ(icon->effectiveColor(), (core::Color{1.0f, 0.0f, 1.0f, 1.0f}));
}

TEST(IconTest, IconSizePresets) {
    EXPECT_FLOAT_EQ(toPoints(IconSize::Small), 12.0f);
    EXPECT_FLOAT_EQ(toPoints(IconSize::Standard), 16.0f);
    EXPECT_FLOAT_EQ(toPoints(IconSize::Medium), 20.0f);
    EXPECT_FLOAT_EQ(toPoints(IconSize::Large), 24.0f);

    Icon icon(IconId::Bell, IconSize::Large, IconRole::Accent);
    EXPECT_FLOAT_EQ(icon.iconSize(), 24.0f);
    EXPECT_EQ(icon.role(), IconRole::Accent);

    icon.setIconSize(IconSize::Small);
    EXPECT_FLOAT_EQ(icon.iconSize(), 12.0f);
}

TEST(IconTest, IconInteractiveModeAndEvents) {
    Icon icon(IconId::Edit, IconSize::Standard);
    EXPECT_FALSE(icon.isInteractive());
    EXPECT_FALSE(icon.isFocusable());

    icon.setInteractive(true);
    EXPECT_TRUE(icon.isInteractive());
    EXPECT_TRUE(icon.isFocusable());
    EXPECT_EQ(icon.cursorShape(), platform::CursorShape::PointingHand);

    int clickCount = 0;
    icon.onClicked.connect([&]() { ++clickCount; });

    // Hover animation
    icon.onMouseEnter();
    EXPECT_TRUE(icon.hasActiveAnimations());
    icon.updateAnimations(0.2f);
    EXPECT_FALSE(icon.hasActiveAnimations());

    // Mouse click trigger
    WidgetMouseEvent downEv{core::Point{8.0f, 8.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    icon.onMouseDown(downEv);
    EXPECT_TRUE(downEv.isAccepted());

    WidgetMouseEvent upEv{core::Point{8.0f, 8.0f}, platform::MouseButton::Left,
                          platform::KeyModifier::None};
    icon.onMouseUp(upEv);
    EXPECT_TRUE(upEv.isAccepted());
    EXPECT_EQ(clickCount, 1);

    // Keyboard Space key trigger
    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    icon.onKeyDown(spaceEv);
    EXPECT_TRUE(spaceEv.isAccepted());
    EXPECT_EQ(clickCount, 2);

    icon.onMouseLeave();
    EXPECT_TRUE(icon.hasActiveAnimations());
    icon.updateAnimations(0.2f);
    EXPECT_FALSE(icon.hasActiveAnimations());
}

TEST(IconTest, IconConcurrentCacheAccess) {
    // Multi-threaded stress test ensuring atomic fast-path caching is lock-free and thread-safe
    constexpr int kThreadCount = 8;
    constexpr int kIterations = 100;
    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    for (int t = 0; t < kThreadCount; ++t) {
        threads.emplace_back([]() {
            for (int i = 0; i < kIterations; ++i) {
                for (size_t id = 0; id < kTotalIconCount; ++id) {
                    auto doc = getSvgDocument(static_cast<IconId>(id));
                    EXPECT_NE(doc, nullptr);
                    EXPECT_TRUE(doc->isValid());
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}
