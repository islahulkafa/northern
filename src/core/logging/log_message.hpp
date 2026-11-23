#pragma once

#include "core/logging/log_level.hpp"
#include <chrono>
#include <string>
#include <string_view>

namespace northern::core {

struct LogMessage {
    LogLevel level{LogLevel::Info};
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
    std::string message{};
    std::string_view file{};
    uint32_t line{0};
    std::string_view function{};
};

} // namespace northern::core
