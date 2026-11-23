#include "widgets/containers/container.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/label.hpp"

#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(ContainerTest, DefaultState) {
    Container c;
    EXPECT_EQ(c.surface(), SurfaceRole::None);
    EXPECT_EQ(c.borderRole(), BorderRole::None);
    EXPECT_EQ(c.effectiveBackgroundColor(), core::Color::transparent());
    EXPECT_EQ(c.effectiveBorderColor(), core::Color::transparent());
    EXPECT_FLOAT_EQ(c.effectiveBorderWidth(), 0.0f);
    EXPECT_FALSE(c.clipsChildren());
    EXPECT_FALSE(c.isInteractive());
    EXPECT_FALSE(c.isFocusable());
}

TEST(ContainerTest, SemanticSurfaceRoles) {
    WidgetContext ctx;
    auto c = std::make_unique<Container>();
    Container* cPtr = c.get();
    ctx.setRootWidget(std::move(c));

    cPtr->setSurface(SurfaceRole::Canvas);
    EXPECT_EQ(cPtr->effectiveBackgroundColor(), theme::ColorTokens::SurfaceCanvas);

    cPtr->setSurface(SurfaceRole::Default);
    EXPECT_EQ(cPtr->effectiveBackgroundColor(), theme::ColorTokens::SurfaceDefault);

    cPtr->setSurface(SurfaceRole::Raised);
    EXPECT_EQ(cPtr->effectiveBackgroundColor(), theme::ColorTokens::SurfaceRaised);

    cPtr->setSurface(SurfaceRole::Overlay);
    EXPECT_EQ(cPtr->effectiveBackgroundColor(), theme::ColorTokens::SurfaceOverlay);

    cPtr->setBorderRole(BorderRole::Subtle);
    EXPECT_EQ(cPtr->effectiveBorderColor(), theme::ColorTokens::BorderSubtle);
    EXPECT_FLOAT_EQ(cPtr->effectiveBorderWidth(), theme::Metrics::BorderWidthHairline);

    cPtr->setBorderRole(BorderRole::Default);
    EXPECT_EQ(cPtr->effectiveBorderColor(), theme::ColorTokens::BorderDefault);
    EXPECT_FLOAT_EQ(cPtr->effectiveBorderWidth(), theme::Metrics::BorderWidthHairline);

    // Switch to Light Theme
    ctx.setTheme(theme::Theme::light());
    cPtr->setSurface(SurfaceRole::Default);
    EXPECT_EQ(cPtr->effectiveBackgroundColor(), theme::ColorPalette::light().surfaceDefault);
    EXPECT_EQ(cPtr->effectiveBorderColor(), theme::ColorPalette::light().borderDefault);
}

TEST(ContainerTest, CustomColorOverrides) {
    Container c;
    c.setSurface(SurfaceRole::Raised);
    c.setBorderRole(BorderRole::Subtle);

    core::Color customBg{0.2f, 0.4f, 0.6f, 1.0f};
    c.setBackgroundColor(customBg);
    EXPECT_EQ(c.effectiveBackgroundColor(), customBg);

    core::Color customBorder{0.8f, 0.1f, 0.1f, 1.0f};
    c.setBorderColor(customBorder);
    EXPECT_EQ(c.effectiveBorderColor(), customBorder);

    c.setBorderWidth(3.0f);
    EXPECT_FLOAT_EQ(c.effectiveBorderWidth(), 3.0f);

    c.setCornerRadius(8.0f);
    EXPECT_FLOAT_EQ(c.cornerRadii()[0], 8.0f);
    EXPECT_FLOAT_EQ(c.cornerRadii()[1], 8.0f);
    EXPECT_FLOAT_EQ(c.cornerRadii()[2], 8.0f);
    EXPECT_FLOAT_EQ(c.cornerRadii()[3], 8.0f);

    c.setCornerRadii(2.0f, 4.0f, 6.0f, 8.0f);
    EXPECT_FLOAT_EQ(c.cornerRadii()[0], 2.0f);
    EXPECT_FLOAT_EQ(c.cornerRadii()[1], 4.0f);
    EXPECT_FLOAT_EQ(c.cornerRadii()[2], 6.0f);
    EXPECT_FLOAT_EQ(c.cornerRadii()[3], 8.0f);
}

TEST(ContainerTest, ChildClipping) {
    Container c;
    EXPECT_FALSE(c.clipsChildren());

    c.setClipsChildren(true);
    EXPECT_TRUE(c.clipsChildren());

    c.setClipsChildren(false);
    EXPECT_FALSE(c.clipsChildren());
}

TEST(ContainerTest, FluentLayoutConvenience) {
    Container c;
    c.style().width = layout::Length::points(100.0f);
    c.style().height = layout::Length::points(100.0f);
    c.layoutNode().computeLayout(100.0f, 100.0f);
    c.clearLayoutDirty();
    EXPECT_FALSE(c.isLayoutDirty());

    c.setFlexDirection(layout::FlexDirection::Column)
        ->setJustifyContent(layout::JustifyContent::Center)
        ->setAlignItems(layout::AlignItems::FlexEnd)
        ->setFlexWrap(layout::FlexWrap::Wrap)
        ->setGap(12.0f)
        ->setPadding(16.0f);

    EXPECT_TRUE(c.isLayoutDirty());
    EXPECT_EQ(c.style().flexDirection, layout::FlexDirection::Column);
    EXPECT_EQ(c.style().justifyContent, layout::JustifyContent::Center);
    EXPECT_EQ(c.style().alignItems, layout::AlignItems::FlexEnd);
    EXPECT_EQ(c.style().flexWrap, layout::FlexWrap::Wrap);
    EXPECT_FLOAT_EQ(c.style().rowGap, 12.0f);
    EXPECT_FLOAT_EQ(c.style().columnGap, 12.0f);
    EXPECT_FLOAT_EQ(c.style().padding.left.value, 16.0f);
    EXPECT_FLOAT_EQ(c.style().padding.top.value, 16.0f);
    EXPECT_FLOAT_EQ(c.style().padding.right.value, 16.0f);
    EXPECT_FLOAT_EQ(c.style().padding.bottom.value, 16.0f);

    c.clearLayoutDirty();
    c.setPadding(8.0f, 24.0f);
    EXPECT_TRUE(c.isLayoutDirty());
    EXPECT_FLOAT_EQ(c.style().padding.top.value, 8.0f);
    EXPECT_FLOAT_EQ(c.style().padding.bottom.value, 8.0f);
    EXPECT_FLOAT_EQ(c.style().padding.left.value, 24.0f);
    EXPECT_FLOAT_EQ(c.style().padding.right.value, 24.0f);

    c.setRowGap(10.0f)->setColumnGap(20.0f);
    EXPECT_FLOAT_EQ(c.style().rowGap, 10.0f);
    EXPECT_FLOAT_EQ(c.style().columnGap, 20.0f);
}

TEST(ContainerTest, InteractiveCardStates) {
    WidgetContext ctx;
    auto c = std::make_unique<Container>();
    Container* cPtr = c.get();
    ctx.setRootWidget(std::move(c));

    cPtr->setSurface(SurfaceRole::Default);
    cPtr->setBorderRole(BorderRole::Subtle);
    cPtr->setInteractive(true);

    EXPECT_TRUE(cPtr->isInteractive());
    EXPECT_TRUE(cPtr->isFocusable());
    EXPECT_EQ(cPtr->cursorShape(), platform::CursorShape::PointingHand);

    int clickCount = 0;
    cPtr->onClicked.connect([&clickCount]() { ++clickCount; });

    // Pointer events
    cPtr->style().width = layout::Length::points(100.0f);
    cPtr->style().height = layout::Length::points(100.0f);
    cPtr->layoutNode().computeLayout(100.0f, 100.0f);

    cPtr->onMouseEnter();
    EXPECT_TRUE(cPtr->hasActiveAnimations());
    cPtr->updateAnimations(0.15f);

    WidgetMouseEvent pressEv{core::Point{50.0f, 50.0f},
                             platform::MouseButton::Left,
                             platform::KeyModifier::None};
    cPtr->onMouseDown(pressEv);
    EXPECT_TRUE(cPtr->hasActiveAnimations());
    cPtr->updateAnimations(0.15f);

    WidgetMouseEvent releaseEv{core::Point{50.0f, 50.0f},
                              platform::MouseButton::Left,
                              platform::KeyModifier::None};
    cPtr->onMouseUp(releaseEv);
    EXPECT_EQ(clickCount, 1);

    // Keyboard activation
    WidgetKeyEvent enterEv{platform::Key::Enter, true, platform::KeyModifier::None};
    cPtr->onKeyDown(enterEv);
    EXPECT_EQ(clickCount, 2);

    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    cPtr->onKeyDown(spaceEv);
    EXPECT_EQ(clickCount, 3);
}

TEST(ContainerTest, ChildTextHoverAndCursorShape) {
    WidgetContext ctx;
    auto root = std::make_unique<Container>();
    root->style().width = layout::Length::points(400.0f);
    root->style().height = layout::Length::points(300.0f);

    auto card = std::make_unique<Container>();
    card->setSurface(SurfaceRole::Default);
    card->setInteractive(true);
    card->style().width = layout::Length::points(200.0f);
    card->style().height = layout::Length::points(100.0f);

    auto label = std::make_unique<Label>("Card Label");
    label->style().width = layout::Length::points(100.0f);
    label->style().height = layout::Length::points(30.0f);
    Label* labelPtr = card->addChild(std::move(label));

    Container* cardPtr = root->addChild(std::move(card));
    ctx.setRootWidget(std::move(root));
    ctx.handleResize(core::Size{400.0f, 300.0f});

    int clickCount = 0;
    cardPtr->onClicked.connect([&clickCount]() { ++clickCount; });

    core::Rect labelRect = labelPtr->screenRect();
    EXPECT_GT(labelRect.size.width, 0.0f);
    EXPECT_GT(labelRect.size.height, 0.0f);

    // Hover over text label inside card
    platform::MouseEvent moveEv{};
    moveEv.type = platform::MouseEventType::Move;
    moveEv.position = core::Point{labelRect.origin.x + 5.0f, labelRect.origin.y + 5.0f};
    ctx.handleMouseMove(moveEv);

    // Context cursor must inherit PointingHand from interactive ancestor
    EXPECT_EQ(ctx.currentCursorShape(), platform::CursorShape::PointingHand);

    // Card container must be in active hover animation
    EXPECT_TRUE(cardPtr->hasActiveAnimations());

    // Click on text label inside card
    platform::MouseEvent downEv{};
    downEv.type = platform::MouseEventType::ButtonDown;
    downEv.button = platform::MouseButton::Left;
    downEv.position = moveEv.position;
    ctx.handleMouseDown(downEv);

    platform::MouseEvent upEv{};
    upEv.type = platform::MouseEventType::ButtonUp;
    upEv.button = platform::MouseButton::Left;
    upEv.position = moveEv.position;
    ctx.handleMouseUp(upEv);

    EXPECT_EQ(clickCount, 1);
}
