#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
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

    // Query global application instance
    [[nodiscard]] static Application* instance() noexcept;

    class Impl;

private:
    std::unique_ptr<Impl> impl_;
};

} // namespace northern::platform
