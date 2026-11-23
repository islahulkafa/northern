#pragma once

#include <cstdint>
#include <string_view>

namespace northern::core {

enum class LogLevel : uint8_t {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
    Off
};

[[nodiscard]] constexpr std::string_view logLevelToString(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        case LogLevel::Off:   return "OFF";
    }
    return "UNKNOWN";
}

[[nodiscard]] constexpr std::string_view logLevelAnsiColor(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace: return "\033[90m"; // Gray
        case LogLevel::Debug: return "\033[36m"; // Cyan
        case LogLevel::Info:  return "\033[32m"; // Green
        case LogLevel::Warn:  return "\033[33m"; // Yellow
        case LogLevel::Error: return "\033[31m"; // Red
        case LogLevel::Fatal: return "\033[35m"; // Magenta
        case LogLevel::Off:   return "";
    }
    return "";
}

inline constexpr std::string_view kAnsiReset = "\033[0m";

} // namespace northern::core
