#pragma once

#include <functional>
#include <mutex>
#include <vector>

namespace northern::core {

class TaskQueue {
public:
    using Task = std::function<void()>;

    TaskQueue() = default;
    ~TaskQueue() = default;

    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;
    TaskQueue(TaskQueue&&) = delete;
    TaskQueue& operator=(TaskQueue&&) = delete;

    void push(Task task);

    /// Drains all tasks currently in the queue into @p out.
    /// Fast double-buffer swap minimizes lock hold time.
    void drain(std::vector<Task>& out);

    /// Executes all pending tasks on the calling thread.
    /// Tasks are popped into a local buffer before invocation so callbacks
    /// can re-entrantly push new tasks without deadlocking.
    size_t executeAll();

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] size_t size() const noexcept;
    void clear();

private:
    mutable std::mutex mutex_{};
    std::vector<Task> tasks_{};
};

} // namespace northern::core
