#pragma once

#include "core/logging/log_level.hpp"
#include "core/logging/log_sink.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace northern::core {

class Logger {
public:
    static Logger& instance() noexcept;

    Logger();
    ~Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void setLevel(LogLevel level) noexcept {
        level_.store(level, std::memory_order_relaxed);
    }

    [[nodiscard]] LogLevel level() const noexcept {
        return level_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] bool shouldLog(LogLevel level) const noexcept {
        return level >= level_.load(std::memory_order_relaxed) && level != LogLevel::Off;
    }

    void addSink(std::shared_ptr<ILogSink> sink);
    void clearSinks();
    void setSinks(std::vector<std::shared_ptr<ILogSink>> sinks);

    void log(LogLevel level, std::string_view file, uint32_t line,
             std::string_view func, std::string message);

    void flush();

private:
    std::atomic<LogLevel> level_{LogLevel::Info};
    std::mutex sinkMutex_{};
    std::vector<std::shared_ptr<ILogSink>> sinks_{};
};

} // namespace northern::core

#define NORTHERN_LOG_TRACE(msg)                                                                     \
    do {                                                                                            \
        if (::northern::core::Logger::instance().shouldLog(::northern::core::LogLevel::Trace)) {   \
            ::northern::core::Logger::instance().log(                                               \
                ::northern::core::LogLevel::Trace, __FILE__,                                        \
                static_cast<uint32_t>(__LINE__), __func__, (msg));                                  \
        }                                                                                           \
    } while (false)

#define NORTHERN_LOG_DEBUG(msg)                                                                     \
    do {                                                                                            \
        if (::northern::core::Logger::instance().shouldLog(::northern::core::LogLevel::Debug)) {   \
            ::northern::core::Logger::instance().log(                                               \
                ::northern::core::LogLevel::Debug, __FILE__,                                        \
                static_cast<uint32_t>(__LINE__), __func__, (msg));                                  \
        }                                                                                           \
    } while (false)

#define NORTHERN_LOG_INFO(msg)                                                                      \
    do {                                                                                            \
        if (::northern::core::Logger::instance().shouldLog(::northern::core::LogLevel::Info)) {    \
            ::northern::core::Logger::instance().log(                                               \
                ::northern::core::LogLevel::Info, __FILE__,                                         \
                static_cast<uint32_t>(__LINE__), __func__, (msg));                                  \
        }                                                                                           \
    } while (false)

#define NORTHERN_LOG_WARN(msg)                                                                      \
    do {                                                                                            \
        if (::northern::core::Logger::instance().shouldLog(::northern::core::LogLevel::Warn)) {    \
            ::northern::core::Logger::instance().log(                                               \
                ::northern::core::LogLevel::Warn, __FILE__,                                         \
                static_cast<uint32_t>(__LINE__), __func__, (msg));                                  \
        }                                                                                           \
    } while (false)

#define NORTHERN_LOG_ERROR(msg)                                                                     \
    do {                                                                                            \
        if (::northern::core::Logger::instance().shouldLog(::northern::core::LogLevel::Error)) {   \
            ::northern::core::Logger::instance().log(                                               \
                ::northern::core::LogLevel::Error, __FILE__,                                        \
                static_cast<uint32_t>(__LINE__), __func__, (msg));                                  \
        }                                                                                           \
    } while (false)

#define NORTHERN_LOG_FATAL(msg)                                                                     \
    do {                                                                                            \
        if (::northern::core::Logger::instance().shouldLog(::northern::core::LogLevel::Fatal)) {   \
            ::northern::core::Logger::instance().log(                                               \
                ::northern::core::LogLevel::Fatal, __FILE__,                                        \
                static_cast<uint32_t>(__LINE__), __func__, (msg));                                  \
        }                                                                                           \
    } while (false)
