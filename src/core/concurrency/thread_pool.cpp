#include "core/concurrency/thread_pool.hpp"
#include <algorithm>

namespace northern::core {

ThreadPool::ThreadPool(size_t workerCount) {
    if (workerCount == 0) {
        workerCount = std::max(1u, std::thread::hardware_concurrency());
    }

    workers_.reserve(workerCount);
    for (size_t i = 0; i < workerCount; ++i) {
        workers_.emplace_back([this](std::stop_token st) { workerLoop(st); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    cv_.notify_all();
    for (auto& worker : workers_) {
        worker.request_stop();
    }
}

void ThreadPool::workerLoop(std::stop_token stopToken) {
    while (!stopToken.stop_requested()) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this, &stopToken]() {
                return stopping_ || !tasks_.empty() || stopToken.stop_requested();
            });

            if ((stopping_ || stopToken.stop_requested()) && tasks_.empty()) {
                return;
            }

            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
                ++activeWorkers_;
            }
        }

        if (task) {
            task();
            {
                std::lock_guard<std::mutex> lock(mutex_);
                --activeWorkers_;
                if (tasks_.empty() && activeWorkers_ == 0) {
                    idleCv_.notify_all();
                }
            }
        }
    }
}

size_t ThreadPool::pendingTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

void ThreadPool::waitIdle() {
    std::unique_lock<std::mutex> lock(mutex_);
    idleCv_.wait(lock, [this]() {
        return tasks_.empty() && activeWorkers_ == 0;
    });
}

} // namespace northern::core
