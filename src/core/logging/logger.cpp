#include "core/logging/logger.hpp"

namespace northern::core {

Logger& Logger::instance() noexcept {
    static Logger sInstance;
    return sInstance;
}

Logger::Logger() {
    // Default to ConsoleSink
    sinks_.push_back(std::make_shared<ConsoleSink>(true));
}

void Logger::addSink(std::shared_ptr<ILogSink> sink) {
    if (!sink) {
        return;
    }
    std::lock_guard<std::mutex> lock(sinkMutex_);
    sinks_.push_back(std::move(sink));
}

void Logger::clearSinks() {
    std::lock_guard<std::mutex> lock(sinkMutex_);
    sinks_.clear();
}

void Logger::setSinks(std::vector<std::shared_ptr<ILogSink>> sinks) {
    std::lock_guard<std::mutex> lock(sinkMutex_);
    sinks_ = std::move(sinks);
}

void Logger::log(LogLevel level, std::string_view file, uint32_t line,
                 std::string_view func, std::string message) {
    if (!shouldLog(level)) {
        return;
    }

    LogMessage msg{
        level,
        std::chrono::system_clock::now(),
        std::move(message),
        file,
        line,
        func
    };

    std::lock_guard<std::mutex> lock(sinkMutex_);
    for (const auto& sink : sinks_) {
        if (sink) {
            sink->log(msg);
        }
    }
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(sinkMutex_);
    for (const auto& sink : sinks_) {
        if (sink) {
            sink->flush();
        }
    }
}

} // namespace northern::core
