#include "core/concurrency/task_queue.hpp"
#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace northern::core;

TEST(TaskQueueTest, PushAndExecuteAll) {
    TaskQueue queue;
    EXPECT_TRUE(queue.empty());

    int counter = 0;
    queue.push([&counter]() { counter += 10; });
    queue.push([&counter]() { counter += 5; });

    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 2);

    size_t executed = queue.executeAll();
    EXPECT_EQ(executed, 2);
    EXPECT_EQ(counter, 15);
    EXPECT_TRUE(queue.empty());
}

TEST(TaskQueueTest, ReentrantPushDoesNotDeadlock) {
    TaskQueue queue;
    int counter = 0;

    queue.push([&]() {
        counter += 1;
        // Pushing from inside a task
        queue.push([&]() {
            counter += 10;
        });
    });

    size_t executed1 = queue.executeAll();
    EXPECT_EQ(executed1, 1);
    EXPECT_EQ(counter, 1);
    EXPECT_EQ(queue.size(), 1);

    size_t executed2 = queue.executeAll();
    EXPECT_EQ(executed2, 1);
    EXPECT_EQ(counter, 11);
    EXPECT_TRUE(queue.empty());
}

TEST(TaskQueueTest, MultiThreadedProducers) {
    TaskQueue queue;
    std::atomic<int> counter{0};
    constexpr int kNumThreads = 4;
    constexpr int kTasksPerThread = 50;

    std::vector<std::thread> producers;
    producers.reserve(kNumThreads);
    for (int t = 0; t < kNumThreads; ++t) {
        producers.emplace_back([&queue, &counter]() {
            for (int i = 0; i < kTasksPerThread; ++i) {
                queue.push([&counter]() {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }

    for (auto& th : producers) {
        th.join();
    }

    EXPECT_EQ(queue.size(), kNumThreads * kTasksPerThread);
    size_t executed = queue.executeAll();
    EXPECT_EQ(executed, kNumThreads * kTasksPerThread);
    EXPECT_EQ(counter.load(), kNumThreads * kTasksPerThread);
}
