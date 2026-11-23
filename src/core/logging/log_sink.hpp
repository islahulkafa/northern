#pragma once

#include "core/logging/log_message.hpp"
#include <fstream>
#include <memory>
#include <mutex>
#include <string>

namespace northern::core {

class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void log(const LogMessage& message) = 0;
    virtual void flush() = 0;
};

class ConsoleSink : public ILogSink {
public:
    explicit ConsoleSink(bool useColor = true);
    ~ConsoleSink() override = default;

    void log(const LogMessage& message) override;
    void flush() override;

    void setColorEnabled(bool enabled) noexcept;
    [[nodiscard]] bool isColorEnabled() const noexcept;

private:
    std::mutex mutex_{};
    bool useColor_{true};
};

class FileSink : public ILogSink {
public:
    explicit FileSink(const std::string& filePath);
    ~FileSink() override;

    void log(const LogMessage& message) override;
    void flush() override;

    [[nodiscard]] bool isOpen() const noexcept;

private:
    std::mutex mutex_{};
    std::ofstream stream_{};
};

} // namespace northern::core
