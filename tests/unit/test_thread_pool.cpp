#include "core/concurrency/thread_pool.hpp"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <vector>

using namespace northern::core;

TEST(ThreadPoolTest, EnqueueAndFutureResolution) {
    ThreadPool pool(2);
    EXPECT_EQ(pool.workerCount(), 2);

    auto f1 = pool.enqueue([]() { return 42; });
    auto f2 = pool.enqueue([](int a, int b) { return a + b; }, 10, 20);

    EXPECT_EQ(f1.get(), 42);
    EXPECT_EQ(f2.get(), 30);
}

TEST(ThreadPoolTest, ConcurrentTaskThroughput) {
    ThreadPool pool(4);
    constexpr int kTaskCount = 100;
    std::atomic<int> counter{0};

    std::vector<std::future<void>> futures;
    futures.reserve(kTaskCount);

    for (int i = 0; i < kTaskCount; ++i) {
        futures.push_back(pool.enqueue([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    EXPECT_EQ(counter.load(), kTaskCount);
}

TEST(ThreadPoolTest, WaitIdleBlocksUntilFinished) {
    ThreadPool pool(2);
    std::atomic<bool> flag{false};

    pool.enqueue([&flag]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        flag.store(true);
    });

    pool.waitIdle();
    EXPECT_TRUE(flag.load());
    EXPECT_EQ(pool.pendingTasks(), 0);
}

TEST(ThreadPoolTest, TaskExceptionPropagatesToFuture) {
    ThreadPool pool(1);

    auto f = pool.enqueue([]() -> int {
        throw std::runtime_error("Worker exception");
    });

    EXPECT_THROW(f.get(), std::runtime_error);
}
