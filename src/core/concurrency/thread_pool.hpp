#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace northern::core {

class ThreadPool {
public:
    explicit ThreadPool(size_t workerCount = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    template <typename F, typename... Args>
        requires std::invocable<F, Args...>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [func = std::forward<F>(f),
             ... capturedArgs = std::forward<Args>(args)]() mutable -> ReturnType {
                return std::invoke(std::move(func), std::move(capturedArgs)...);
            });

        std::future<ReturnType> future = task->get_future();

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) {
                throw std::runtime_error("Cannot enqueue task to a stopping ThreadPool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }

        cv_.notify_one();
        return future;
    }

    [[nodiscard]] size_t workerCount() const noexcept { return workers_.size(); }
    [[nodiscard]] size_t pendingTasks() const;
    void waitIdle();

private:
    void workerLoop(std::stop_token stopToken);

    std::vector<std::jthread> workers_{};
    std::queue<std::function<void()>> tasks_{};
    mutable std::mutex mutex_{};
    std::condition_variable cv_{};
    std::condition_variable idleCv_{};
    size_t activeWorkers_{0};
    bool stopping_{false};
};

} // namespace northern::core
