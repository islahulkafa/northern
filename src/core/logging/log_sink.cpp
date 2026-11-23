#include "core/logging/log_sink.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace northern::core {

namespace {

std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto duration = tp.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;

    std::time_t timeT = seconds.count();
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &timeT);
#else
    localtime_r(&timeT, &tmBuf);
#endif

    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                  tmBuf.tm_year + 1900, tmBuf.tm_mon + 1, tmBuf.tm_mday,
                  tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec,
                  static_cast<int>(ms.count()));
    return std::string(buffer);
}

std::string_view extractBasename(std::string_view path) {
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string_view::npos) {
        return path.substr(lastSlash + 1);
    }
    return path;
}

} // namespace

ConsoleSink::ConsoleSink(bool useColor) : useColor_(useColor) {}

void ConsoleSink::setColorEnabled(bool enabled) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    useColor_ = enabled;
}

bool ConsoleSink::isColorEnabled() const noexcept {
    return useColor_;
}

void ConsoleSink::log(const LogMessage& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostream& out = (message.level >= LogLevel::Error) ? std::cerr : std::cout;

    std::string timeStr = formatTimestamp(message.timestamp);
    std::string_view levelStr = logLevelToString(message.level);
    std::string_view filename = extractBasename(message.file);

    if (useColor_) {
        std::string_view color = logLevelAnsiColor(message.level);
        out << "\033[90m[" << timeStr << "]\033[0m "
            << color << "[" << levelStr << "]\033[0m "
            << "\033[90m[" << filename << ":" << message.line << "]\033[0m "
            << message.message << "\n";
    } else {
        out << "[" << timeStr << "] "
            << "[" << levelStr << "] "
            << "[" << filename << ":" << message.line << "] "
            << message.message << "\n";
    }
}

void ConsoleSink::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout.flush();
    std::cerr.flush();
}

FileSink::FileSink(const std::string& filePath) {
    stream_.open(filePath, std::ios::out | std::ios::app);
}

FileSink::~FileSink() {
    flush();
    if (stream_.is_open()) {
        stream_.close();
    }
}

bool FileSink::isOpen() const noexcept {
    return stream_.is_open();
}

void FileSink::log(const LogMessage& message) {
    if (!stream_.is_open()) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    std::string timeStr = formatTimestamp(message.timestamp);
    std::string_view levelStr = logLevelToString(message.level);
    std::string_view filename = extractBasename(message.file);

    stream_ << "[" << timeStr << "] "
            << "[" << levelStr << "] "
            << "[" << filename << ":" << message.line << "] "
            << message.message << "\n";

    if (message.level >= LogLevel::Warn) {
        stream_.flush();
    }
}

void FileSink::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stream_.is_open()) {
        stream_.flush();
    }
}

} // namespace northern::core
