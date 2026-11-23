#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "core/signal/signal.hpp"
#include "platform/events.hpp"
#include <memory>

namespace northern::platform {

class Application {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) noexcept;
    Application& operator=(Application&&) noexcept;

    // Run the platform main event loop until quit is requested
    int run();

    // Request the event loop to terminate
    void quit(int exitCode = 0);

    // Pump pending window and input messages without blocking
    void pollEvents();

    // Wait for incoming window messages with optional timeout (in milliseconds, 0 = infinite wait)
    void waitEvents(uint32_t timeoutMs = 0);

    // Query global application instance
    [[nodiscard]] static Application* instance() noexcept;

    // System theme detection
    [[nodiscard]] static bool isDarkMode() noexcept;

    // Global application signals
    core::Signal<const ThemeChangedEvent&> onThemeChanged;

    class Impl;

private:
    std::unique_ptr<Impl> impl_;
};

} // namespace northern::platform
