#include "core/logging/logger.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <vector>

using namespace northern::core;

namespace {
class MockLogSink : public ILogSink {
public:
    void log(const LogMessage& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.push_back(message);
    }

    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        flushCount_++;
    }

    std::vector<LogMessage> getMessages() {
        std::lock_guard<std::mutex> lock(mutex_);
        return messages_;
    }

    int flushCount() {
        std::lock_guard<std::mutex> lock(mutex_);
        return flushCount_;
    }

private:
    std::mutex mutex_{};
    std::vector<LogMessage> messages_{};
    int flushCount_{0};
};
} // namespace

TEST(LoggingTest, LogLevelFilteringAndDelivery) {
    Logger logger;
    logger.clearSinks();
    auto mockSink = std::make_shared<MockLogSink>();
    logger.addSink(mockSink);

    logger.setLevel(LogLevel::Warn);

    logger.log(LogLevel::Debug, __FILE__, __LINE__, __func__, "Ignored debug");
    logger.log(LogLevel::Info, __FILE__, __LINE__, __func__, "Ignored info");
    logger.log(LogLevel::Warn, __FILE__, __LINE__, __func__, "Captured warn");
    logger.log(LogLevel::Error, __FILE__, __LINE__, __func__, "Captured error");

    auto msgs = mockSink->getMessages();
    ASSERT_EQ(msgs.size(), 2);
    EXPECT_EQ(msgs[0].level, LogLevel::Warn);
    EXPECT_EQ(msgs[0].message, "Captured warn");
    EXPECT_EQ(msgs[1].level, LogLevel::Error);
    EXPECT_EQ(msgs[1].message, "Captured error");
}

TEST(LoggingTest, FileSinkWritesToFile) {
    std::string testLogPath = "build/test_output.log";
    if (std::filesystem::exists(testLogPath)) {
        std::filesystem::remove(testLogPath);
    }

    {
        FileSink fileSink(testLogPath);
        ASSERT_TRUE(fileSink.isOpen());

        LogMessage msg{
            LogLevel::Info,
            std::chrono::system_clock::now(),
            "File sink verification message",
            "test_logging.cpp",
            42,
            "TestFunc"
        };
        fileSink.log(msg);
        fileSink.flush();
    }

    ASSERT_TRUE(std::filesystem::exists(testLogPath));
    {
        std::ifstream in(testLogPath);
        std::string content;
        std::getline(in, content);
        EXPECT_NE(content.find("File sink verification message"), std::string::npos);
        EXPECT_NE(content.find("[INFO]"), std::string::npos);
    }

    std::filesystem::remove(testLogPath);
}

TEST(LoggingTest, GlobalMacrosExecuteCleanly) {
    auto mockSink = std::make_shared<MockLogSink>();
    Logger::instance().setSinks({mockSink});
    Logger::instance().setLevel(LogLevel::Trace);

    NORTHERN_LOG_TRACE("Trace macro test");
    NORTHERN_LOG_DEBUG("Debug macro test");
    NORTHERN_LOG_INFO("Info macro test");
    NORTHERN_LOG_WARN("Warn macro test");
    NORTHERN_LOG_ERROR("Error macro test");
    NORTHERN_LOG_FATAL("Fatal macro test");

    auto msgs = mockSink->getMessages();
    EXPECT_EQ(msgs.size(), 6);

    // Reset default sinks and level
    Logger::instance().setSinks({std::make_shared<ConsoleSink>(true)});
    Logger::instance().setLevel(LogLevel::Info);
}
