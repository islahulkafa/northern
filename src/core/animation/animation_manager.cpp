#include "core/animation/animation_manager.hpp"

#include <algorithm>

namespace northern::core::animation {

uint32_t AnimationManager::addAnimation(AnimationCallback callback) {
    if (!callback) {
        return 0;
    }
    uint32_t id = nextId_++;
    entries_.push_back(Entry{id, std::move(callback), false});
    return id;
}

void AnimationManager::removeAnimation(uint32_t id) noexcept {
    for (auto& entry : entries_) {
        if (entry.id == id) {
            entry.markedForRemoval = true;
            break;
        }
    }
    if (!isIterating_) {
        entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                      [](const Entry& e) { return e.markedForRemoval; }),
                       entries_.end());
    }
}

void AnimationManager::advance(float dt) {
    if (dt <= 0.0f || entries_.empty()) {
        return;
    }

    isIterating_ = true;
    size_t count = entries_.size();
    for (size_t i = 0; i < count; ++i) {
        if (!entries_[i].markedForRemoval && entries_[i].callback) {
            bool keepRunning = entries_[i].callback(dt);
            if (!keepRunning) {
                entries_[i].markedForRemoval = true;
            }
        }
    }
    isIterating_ = false;

    entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                  [](const Entry& e) { return e.markedForRemoval; }),
                   entries_.end());
}

bool AnimationManager::hasActiveAnimations() const noexcept {
    return std::any_of(entries_.begin(), entries_.end(),
                       [](const Entry& e) { return !e.markedForRemoval; });
}

size_t AnimationManager::activeAnimationCount() const noexcept {
    return static_cast<size_t>(std::count_if(entries_.begin(), entries_.end(),
                                             [](const Entry& e) { return !e.markedForRemoval; }));
}

void AnimationManager::clear() noexcept {
    if (isIterating_) {
        for (auto& e : entries_) {
            e.markedForRemoval = true;
        }
    } else {
        entries_.clear();
    }
}

} // namespace northern::core::animation
