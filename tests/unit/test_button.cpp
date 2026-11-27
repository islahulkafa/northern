#include "text/font_manager.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/button.hpp"
#include "widgets/controls/icons.hpp"
#include <gtest/gtest.h>
#include <variant>

namespace northern::widgets {

class ButtonModernizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        uiContext_ = std::make_unique<WidgetContext>();
        uiContext_->setTheme(theme::Theme::dark());
    }

    std::unique_ptr<WidgetContext> uiContext_;
};

TEST_F(ButtonModernizationTest, ConstructorsAndDefaults) {
    Button b1("Save");
    EXPECT_EQ(b1.text(), "Save");
    EXPECT_EQ(b1.variant(), ButtonVariant::Secondary);
    EXPECT_EQ(b1.size(), ButtonSize::Standard);
    EXPECT_EQ(b1.icon(), nullptr);
    EXPECT_FALSE(b1.isLoading());

    Button b2("Download", IconId::Download, ButtonVariant::Primary, ButtonSize::Comfortable);
    EXPECT_EQ(b2.text(), "Download");
    EXPECT_NE(b2.icon(), nullptr);
    EXPECT_EQ(b2.variant(), ButtonVariant::Primary);
    EXPECT_EQ(b2.size(), ButtonSize::Comfortable);

    Button b3(IconId::Settings, ButtonVariant::Ghost, ButtonSize::Compact);
    EXPECT_TRUE(b3.text().empty());
    EXPECT_NE(b3.icon(), nullptr);
    EXPECT_EQ(b3.variant(), ButtonVariant::Ghost);
    EXPECT_EQ(b3.size(), ButtonSize::Compact);
}

TEST_F(ButtonModernizationTest, VariantsAndColorTokens) {
    auto bPtr = std::make_unique<Button>("Action");
    auto* b = bPtr.get();
    uiContext_->setRootWidget(std::move(bPtr));

    b->setVariant(ButtonVariant::Primary);
    EXPECT_EQ(b->effectiveIdleBackgroundColor(), uiContext_->theme().colors().accentDefault);
    EXPECT_EQ(b->effectiveIdleTextColor(), core::Color::white());

    b->setVariant(ButtonVariant::Secondary);
    EXPECT_EQ(b->effectiveIdleBackgroundColor(), uiContext_->theme().colors().surfaceRaised);
    EXPECT_EQ(b->effectiveIdleBorderColor(), uiContext_->theme().colors().borderSubtle);
    EXPECT_EQ(b->effectiveIdleTextColor(), uiContext_->theme().colors().textPrimary);

    b->setVariant(ButtonVariant::Outline);
    EXPECT_EQ(b->effectiveIdleBackgroundColor(), core::Color::transparent());
    EXPECT_EQ(b->effectiveIdleBorderColor(), uiContext_->theme().colors().borderDefault);
    EXPECT_EQ(b->effectiveIdleTextColor(), uiContext_->theme().colors().textPrimary);

    b->setVariant(ButtonVariant::Ghost);
    EXPECT_EQ(b->effectiveIdleBackgroundColor(), core::Color::transparent());
    EXPECT_EQ(b->effectiveIdleBorderColor(), core::Color::transparent());
    EXPECT_EQ(b->effectiveIdleTextColor(), uiContext_->theme().colors().textSecondary);

    b->setVariant(ButtonVariant::Danger);
    EXPECT_EQ(b->effectiveIdleBackgroundColor(), uiContext_->theme().colors().danger);
    EXPECT_EQ(b->effectiveIdleTextColor(), core::Color::white());
}

TEST_F(ButtonModernizationTest, SizePresetsAndDimensions) {
    Button b("Test", nullptr, ButtonVariant::Secondary, ButtonSize::Compact);
    EXPECT_EQ(b.size(), ButtonSize::Compact);
    EXPECT_FLOAT_EQ(b.style().height.value, theme::Metrics::ControlHeightCompact);

    b.setSize(ButtonSize::Standard);
    EXPECT_EQ(b.size(), ButtonSize::Standard);
    EXPECT_FLOAT_EQ(b.style().height.value, theme::Metrics::ControlHeightStandard);

    b.setSize(ButtonSize::Comfortable);
    EXPECT_EQ(b.size(), ButtonSize::Comfortable);
    EXPECT_FLOAT_EQ(b.style().height.value, theme::Metrics::ControlHeightComfortable);
}

TEST_F(ButtonModernizationTest, DpiScaling) {
    Button b("Scalable", IconId::Folder, ButtonVariant::Secondary, ButtonSize::Standard);
    b.setDpiScale(1.5f);
    EXPECT_FLOAT_EQ(b.dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(b.style().height.value, theme::Metrics::ControlHeightStandard * 1.5f);
    EXPECT_FLOAT_EQ(b.iconSize(), 16.0f * 1.5f);
    EXPECT_FLOAT_EQ(b.iconGap(), 6.0f * 1.5f);
}

TEST_F(ButtonModernizationTest, LoadingStateSuppressesClicks) {
    auto bPtr = std::make_unique<Button>("Submit");
    auto* b = bPtr.get();
    b->layoutNode().setLayoutRect(core::Rect{{0.0f, 0.0f}, {100.0f, 32.0f}});
    uiContext_->setRootWidget(std::move(bPtr));

    int clicks = 0;
    b->onClicked.connect([&clicks]() { ++clicks; });

    // Normal click fires signal
    WidgetMouseEvent pressEv(core::Point{50.0f, 16.0f}, platform::MouseButton::Left,
                             platform::KeyModifier::None);
    b->onMouseDown(pressEv);
    WidgetMouseEvent releaseEv(core::Point{50.0f, 16.0f}, platform::MouseButton::Left,
                               platform::KeyModifier::None);
    b->onMouseUp(releaseEv);
    EXPECT_EQ(clicks, 1);

    // Enter loading state
    b->setLoading(true);
    EXPECT_TRUE(b->isLoading());
    EXPECT_TRUE(b->hasActiveAnimations());

    // Clicks during loading are suppressed
    b->onMouseDown(pressEv);
    b->onMouseUp(releaseEv);
    EXPECT_EQ(clicks, 1);

    // Keyboard activate during loading is suppressed
    WidgetKeyEvent keyEv(platform::Key::Enter, true, platform::KeyModifier::None);
    b->onKeyDown(keyEv);
    EXPECT_EQ(clicks, 1);

    // Disable loading
    b->setLoading(false);
    EXPECT_FALSE(b->isLoading());

    // Clicks resume
    b->onMouseDown(pressEv);
    b->onMouseUp(releaseEv);
    EXPECT_EQ(clicks, 2);
}

TEST_F(ButtonModernizationTest, AutomaticDefaultFontFallback) {
    Button compactBtn("Save", nullptr, ButtonVariant::Secondary, ButtonSize::Compact);
    auto compactFont = compactBtn.effectiveFont();
    ASSERT_NE(compactFont, nullptr);
    EXPECT_FLOAT_EQ(compactFont->pixelSize(), 12.5f);

    Button standardBtn("Save", nullptr, ButtonVariant::Secondary, ButtonSize::Standard);
    auto standardFont = standardBtn.effectiveFont();
    ASSERT_NE(standardFont, nullptr);
    EXPECT_FLOAT_EQ(standardFont->pixelSize(), 14.0f);

    Button comfortableBtn("Save", nullptr, ButtonVariant::Secondary, ButtonSize::Comfortable);
    auto comfortableFont = comfortableBtn.effectiveFont();
    ASSERT_NE(comfortableFont, nullptr);
    EXPECT_FLOAT_EQ(comfortableFont->pixelSize(), 14.5f);
}

TEST_F(ButtonModernizationTest, KeyboardAndDisabledHandling) {
    Button b("Press Me");
    int clicks = 0;
    b.onClicked.connect([&clicks]() { ++clicks; });

    WidgetKeyEvent spaceEv(platform::Key::Space, true, platform::KeyModifier::None);
    b.onKeyDown(spaceEv);
    EXPECT_EQ(clicks, 1);

    b.setEnabled(false);
    b.onKeyDown(spaceEv);
    EXPECT_EQ(clicks, 1);
    EXPECT_EQ(b.state(), ButtonState::Disabled);
}

TEST_F(ButtonModernizationTest, SpringPressScale) {
    auto bPtr = std::make_unique<Button>("Press", nullptr, ButtonVariant::Primary);
    auto* b = bPtr.get();
    b->layoutNode().setLayoutRect(core::Rect{{0.0f, 0.0f}, {120.0f, 32.0f}});
    uiContext_->setRootWidget(std::move(bPtr));

    WidgetMouseEvent pressEv(core::Point{20.0f, 16.0f}, platform::MouseButton::Left,
                             platform::KeyModifier::None);
    b->onMouseDown(pressEv);
    EXPECT_TRUE(b->hasActiveAnimations());
    for (int i = 0; i < 60; ++i) {
        b->updateAnimations(1.0f / 60.0f);
    }
    EXPECT_FALSE(b->hasActiveAnimations());

    WidgetMouseEvent releaseEv(core::Point{20.0f, 16.0f}, platform::MouseButton::Left,
                               platform::KeyModifier::None);
    b->onMouseUp(releaseEv);
    EXPECT_TRUE(b->hasActiveAnimations());
    for (int i = 0; i < 60; ++i) {
        b->updateAnimations(1.0f / 60.0f);
    }
    EXPECT_FALSE(b->hasActiveAnimations());
}

TEST_F(ButtonModernizationTest, HoverColorAnimationMarksPaintDirty) {
    auto bPtr = std::make_unique<Button>("Hover", nullptr, ButtonVariant::Primary);
    auto* b = bPtr.get();
    uiContext_->setRootWidget(std::move(bPtr));
    uiContext_->handleResize(core::Size{120.0f, 32.0f});

    b->onMouseEnter();
    EXPECT_TRUE(b->hasActiveAnimations());
    b->clearPaintDirty();
    b->updateAnimations(0.03f);
    EXPECT_TRUE(b->isPaintDirty());
}

TEST_F(ButtonModernizationTest, ContextDpiAdoptionAndThemeChangeRefresh) {
    uiContext_->setDpiScale(1.5f);
    auto bPtr = std::make_unique<Button>("DPI");
    auto* b = bPtr.get();
    uiContext_->setRootWidget(std::move(bPtr));
    EXPECT_FLOAT_EQ(b->dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(b->style().height.value, theme::Metrics::ControlHeightStandard * 1.5f);

    // Explicit setDpiScale is preserved on context attachment
    WidgetContext customCtx;
    customCtx.setDpiScale(1.5f);
    auto customPtr = std::make_unique<Button>("CustomDPI");
    auto* customBtn = customPtr.get();
    customBtn->setDpiScale(2.0f);
    customCtx.setRootWidget(std::move(customPtr));
    EXPECT_FLOAT_EQ(customBtn->dpiScale(), 2.0f);

    // Theme change updates painted background color immediately even when not hovered
    uiContext_->setTheme(theme::Theme::light());
    uiContext_->handleResize(core::Size{120.0f, 48.0f});
    renderer::PaintContext paintCtx;
    b->paint(paintCtx);
    bool foundBox = false;
    core::Color paintedBg{};
    for (const auto& cmd : paintCtx.commands()) {
        if (std::holds_alternative<renderer::DrawBoxCommand>(cmd)) {
            paintedBg = std::get<renderer::DrawBoxCommand>(cmd).fillColor;
            foundBox = true;
            break;
        }
    }
    ASSERT_TRUE(foundBox);
    EXPECT_EQ(paintedBg, uiContext_->theme().colors().surfaceRaised);
}

TEST_F(ButtonModernizationTest, MouseGrabAndResetOnDisableLoadingFocusLost) {
    auto bPtr = std::make_unique<Button>("Action");
    auto* b = bPtr.get();
    uiContext_->setRootWidget(std::move(bPtr));
    uiContext_->handleResize(core::Size{120.0f, 32.0f});

    int clicks = 0;
    b->onClicked.connect([&clicks]() { ++clicks; });

    // Press acquires mouse grab; release outside does not click and releases grab
    WidgetMouseEvent pressIn(core::Point{40.0f, 16.0f}, platform::MouseButton::Left,
                             platform::KeyModifier::None);
    b->onMouseDown(pressIn);
    EXPECT_EQ(uiContext_->mouseGrabWidget(), b);
    EXPECT_EQ(b->state(), ButtonState::Pressed);

    WidgetMouseEvent releaseOut(core::Point{200.0f, 16.0f}, platform::MouseButton::Left,
                                platform::KeyModifier::None);
    b->onMouseUp(releaseOut);
    EXPECT_EQ(uiContext_->mouseGrabWidget(), nullptr);
    EXPECT_EQ(clicks, 0);
    EXPECT_EQ(b->state(), ButtonState::Idle);

    // Disabling mid-press releases grab, resets cursor to Arrow, and enters Disabled state
    b->onMouseDown(pressIn);
    EXPECT_EQ(uiContext_->mouseGrabWidget(), b);
    b->setEnabled(false);
    EXPECT_EQ(uiContext_->mouseGrabWidget(), nullptr);
    EXPECT_EQ(b->cursorShape(), platform::CursorShape::Arrow);
    EXPECT_EQ(b->state(), ButtonState::Disabled);

    b->setEnabled(true);
    EXPECT_EQ(b->cursorShape(), platform::CursorShape::PointingHand);

    // Loading mid-press releases grab, resets press state, and sets Arrow cursor
    b->onMouseDown(pressIn);
    EXPECT_EQ(uiContext_->mouseGrabWidget(), b);
    b->setLoading(true);
    EXPECT_EQ(uiContext_->mouseGrabWidget(), nullptr);
    EXPECT_EQ(b->cursorShape(), platform::CursorShape::Arrow);
    EXPECT_EQ(b->state(), ButtonState::Idle);

    b->setLoading(false);
    EXPECT_EQ(b->cursorShape(), platform::CursorShape::PointingHand);

    // Focus lost mid-press releases grab and resets press state
    b->onMouseDown(pressIn);
    EXPECT_EQ(uiContext_->mouseGrabWidget(), b);
    b->onFocusLost();
    EXPECT_EQ(uiContext_->mouseGrabWidget(), nullptr);
    EXPECT_EQ(b->state(), ButtonState::Idle);
}

} // namespace northern::widgets
