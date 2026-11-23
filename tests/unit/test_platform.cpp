#include "platform/application.hpp"
#include "platform/clipboard.hpp"
#include "platform/display.hpp"
#include "platform/events.hpp"
#include "platform/key_codes.hpp"
#include "platform/mouse_types.hpp"
#include "platform/win32/win32_key_map.hpp"
#include "platform/window.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::platform;

TEST(PlatformTest, KeyModifiersBitwiseOperations) {
    KeyModifier mods = KeyModifier::Shift | KeyModifier::Control;
    EXPECT_TRUE(hasModifier(mods, KeyModifier::Shift));
    EXPECT_TRUE(hasModifier(mods, KeyModifier::Control));
    EXPECT_FALSE(hasModifier(mods, KeyModifier::Alt));

    mods |= KeyModifier::Alt;
    EXPECT_TRUE(hasModifier(mods, KeyModifier::Alt));

    mods &= ~KeyModifier::Shift;
    EXPECT_FALSE(hasModifier(mods, KeyModifier::Shift));
    EXPECT_TRUE(hasModifier(mods, KeyModifier::Control));
    EXPECT_TRUE(hasModifier(mods, KeyModifier::Alt));
}

TEST(PlatformTest, Win32KeyMapping) {
    EXPECT_EQ(win32VkToKey('A'), Key::A);
    EXPECT_EQ(win32VkToKey('Z'), Key::Z);
    EXPECT_EQ(win32VkToKey('0'), Key::Num0);
    EXPECT_EQ(win32VkToKey('9'), Key::Num9);
    EXPECT_EQ(win32VkToKey(0x1B), Key::Escape); // VK_ESCAPE
    EXPECT_EQ(win32VkToKey(0x0D), Key::Enter);  // VK_RETURN
    EXPECT_EQ(win32VkToKey(0x20), Key::Space);  // VK_SPACE
    EXPECT_EQ(win32VkToKey(0x70), Key::F1);     // VK_F1
    EXPECT_EQ(win32VkToKey(0x25), Key::Left);   // VK_LEFT
    EXPECT_EQ(win32VkToKey(0x27), Key::Right);  // VK_RIGHT
}

TEST(PlatformTest, EventStructDefaults) {
    MouseEvent mouseEv{};
    EXPECT_EQ(mouseEv.type, MouseEventType::Move);
    EXPECT_EQ(mouseEv.button, MouseButton::None);
    EXPECT_FLOAT_EQ(mouseEv.position.x, 0.0f);
    EXPECT_FLOAT_EQ(mouseEv.position.y, 0.0f);

    KeyEvent keyEv{};
    EXPECT_EQ(keyEv.key, Key::Unknown);
    EXPECT_FALSE(keyEv.isDown);
    EXPECT_FALSE(keyEv.isRepeat);
    EXPECT_EQ(keyEv.textCodepoint, 0U);

    ResizeEvent resizeEv{{800.0f, 600.0f}, {1600, 1200}};
    EXPECT_FLOAT_EQ(resizeEv.size.width, 800.0f);
    EXPECT_FLOAT_EQ(resizeEv.size.height, 600.0f);
    EXPECT_EQ(resizeEv.physicalSize.width, 1600);
    EXPECT_EQ(resizeEv.physicalSize.height, 1200);

    DpiChangedEvent dpiEv{2.0f, 192, {0.0f, 0.0f, 800.0f, 600.0f}};
    EXPECT_FLOAT_EQ(dpiEv.dpiScale, 2.0f);
    EXPECT_EQ(dpiEv.dpi, 192U);
}

TEST(PlatformTest, HiddenWindowLifecycle) {
    Application app;

    WindowDesc desc;
    desc.title = "TestHiddenWindow";
    desc.size = {640.0f, 480.0f};
    desc.visible = false;

    auto winResult = Window::create(desc);
    ASSERT_TRUE(winResult.isOk());

    auto window = std::move(winResult.value());
    ASSERT_NE(window, nullptr);

    EXPECT_FALSE(window->isVisible());
    EXPECT_EQ(window->title(), "TestHiddenWindow");
    EXPECT_GT(window->dpi(), 0U);
    EXPECT_GE(window->dpiScale(), 1.0f);
    EXPECT_TRUE(window->nativeHandle().isValid());

    // Title mutation
    window->setTitle("UpdatedTitle");
    EXPECT_EQ(window->title(), "UpdatedTitle");

    // Sizing query and constraints
    EXPECT_GT(window->size().width, 0.0f);
    EXPECT_GT(window->size().height, 0.0f);
    EXPECT_GT(window->physicalSize().width, 0);
    EXPECT_GT(window->physicalSize().height, 0);

    window->setMinSize({300.0f, 200.0f});
    EXPECT_FLOAT_EQ(window->minSize().width, 300.0f);
    EXPECT_FLOAT_EQ(window->minSize().height, 200.0f);

    window->setMaxSize({1200.0f, 900.0f});
    EXPECT_FLOAT_EQ(window->maxSize().width, 1200.0f);
    EXPECT_FLOAT_EQ(window->maxSize().height, 900.0f);

    EXPECT_FALSE(window->isResizing());

    // Signal connection test
    bool resizeNotified = false;
    auto scoped =
        window->onResize.connect([&resizeNotified](const ResizeEvent&) { resizeNotified = true; });
    EXPECT_TRUE(scoped.isConnected());

    // Poll events non-blockingly
    app.pollEvents();

    // Wait events with timeout
    app.waitEvents(10);
}

TEST(PlatformTest, ClipboardTextRoundtrip) {
    const std::string testPayload = "Northern UI Platform Test String";

    auto setResult = Clipboard::setText(testPayload);
    ASSERT_TRUE(setResult.isOk());

    EXPECT_TRUE(Clipboard::hasText());

    auto getResult = Clipboard::getText();
    ASSERT_TRUE(getResult.isOk());
    EXPECT_EQ(getResult.value(), testPayload);
}

TEST(PlatformTest, DisplayEnumerationAndPrimary) {
    auto displays = Display::all();
    EXPECT_FALSE(displays.empty());

    Display primary = Display::primary();
    EXPECT_TRUE(primary.isPrimary);
    EXPECT_GT(primary.bounds.size.width, 0.0f);
    EXPECT_GT(primary.bounds.size.height, 0.0f);
    EXPECT_GT(primary.workArea.size.width, 0.0f);
    EXPECT_GT(primary.workArea.size.height, 0.0f);
    EXPECT_GE(primary.scale, 1.0f);
    EXPECT_FALSE(primary.name.empty());
}

TEST(PlatformTest, DisplayForWindow) {
    Application app;
    WindowDesc desc;
    desc.title = "TestDisplayWindow";
    desc.visible = false;

    auto winResult = Window::create(desc);
    ASSERT_TRUE(winResult.isOk());
    auto window = std::move(winResult.value());

    Display d = Display::forWindow(*window);
    EXPECT_GT(d.bounds.size.width, 0.0f);
    EXPECT_GT(d.bounds.size.height, 0.0f);
    EXPECT_GE(d.scale, 1.0f);
}

TEST(PlatformTest, WindowPositionAndSetPosition) {
    Application app;
    WindowDesc desc;
    desc.title = "TestPosWindow";
    desc.size = {400.0f, 300.0f};
    desc.visible = false;

    auto winResult = Window::create(desc);
    ASSERT_TRUE(winResult.isOk());
    auto window = std::move(winResult.value());

    core::Point initialPos = window->position();
    (void)initialPos;

    core::Point targetPos{120.0f, 140.0f};
    window->setPosition(targetPos);
    core::Point newPos = window->position();
    EXPECT_NEAR(newPos.x, targetPos.x, 5.0f);
    EXPECT_NEAR(newPos.y, targetPos.y, 5.0f);
}

TEST(PlatformTest, WindowCenterOnScreen) {
    Application app;
    WindowDesc desc;
    desc.title = "TestCenterWindow";
    desc.size = {400.0f, 300.0f};
    desc.visible = false;

    auto winResult = Window::create(desc);
    ASSERT_TRUE(winResult.isOk());
    auto window = std::move(winResult.value());

    Display primary = Display::primary();
    window->centerOnScreen(primary);

    core::Point pos = window->position();
    EXPECT_GE(pos.x, primary.workArea.origin.x - 5.0f);
    EXPECT_GE(pos.y, primary.workArea.origin.y - 5.0f);
}

TEST(PlatformTest, WindowFullscreenToggle) {
    Application app;
    WindowDesc desc;
    desc.title = "TestFullscreenWindow";
    desc.size = {640.0f, 480.0f};
    desc.visible = false;

    auto winResult = Window::create(desc);
    ASSERT_TRUE(winResult.isOk());
    auto window = std::move(winResult.value());

    EXPECT_FALSE(window->isFullscreen());

    window->setFullscreen(true);
    EXPECT_TRUE(window->isFullscreen());

    window->setFullscreen(false);
    EXPECT_FALSE(window->isFullscreen());

    window->toggleFullscreen();
    EXPECT_TRUE(window->isFullscreen());

    window->toggleFullscreen();
    EXPECT_FALSE(window->isFullscreen());
}

TEST(PlatformTest, FileDropAndThemeEvents) {
    Application app;
    WindowDesc desc;
    desc.title = "TestEventWindow";
    desc.visible = false;

    auto winResult = Window::create(desc);
    ASSERT_TRUE(winResult.isOk());
    auto window = std::move(winResult.value());

    // FileDropEvent signal verification
    bool dropReceived = false;
    std::string receivedPath;
    auto dropConn = window->onFileDrop.connect([&](const FileDropEvent& ev) {
        dropReceived = true;
        if (!ev.paths.empty()) {
            receivedPath = ev.paths[0];
        }
    });

    FileDropEvent testDrop{{"C:/test/file.png"}, {50.0f, 60.0f}};
    window->onFileDrop.emit(testDrop);
    EXPECT_TRUE(dropReceived);
    EXPECT_EQ(receivedPath, "C:/test/file.png");

    // Theme detection and ThemeChangedEvent signal
    bool isDark = Application::isDarkMode();
    (void)isDark; // Query succeeds without exception

    bool themeChanged = false;
    auto themeConn =
        window->onThemeChanged.connect([&](const ThemeChangedEvent&) { themeChanged = true; });
    window->onThemeChanged.emit(ThemeChangedEvent{true});
    EXPECT_TRUE(themeChanged);
}
