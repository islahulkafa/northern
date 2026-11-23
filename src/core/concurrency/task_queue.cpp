#include "core/concurrency/task_queue.hpp"

namespace northern::core {

void TaskQueue::push(Task task) {
    if (!task) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push_back(std::move(task));
}

void TaskQueue::drain(std::vector<Task>& out) {
    std::vector<Task> localBuffer;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        localBuffer.swap(tasks_);
    }
    out.insert(out.end(),
               std::make_move_iterator(localBuffer.begin()),
               std::make_move_iterator(localBuffer.end()));
}

size_t TaskQueue::executeAll() {
    std::vector<Task> localTasks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        localTasks.swap(tasks_);
    }

    for (auto& task : localTasks) {
        if (task) {
            task();
        }
    }
    return localTasks.size();
}

bool TaskQueue::empty() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.empty();
}

size_t TaskQueue::size() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

void TaskQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.clear();
}

} // namespace northern::core
