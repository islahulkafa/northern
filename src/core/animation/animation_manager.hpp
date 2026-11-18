#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace northern::core::animation {

class AnimationManager {
public:
    using AnimationCallback = std::function<bool(float dt)>;

    explicit AnimationManager() = default;
    ~AnimationManager() = default;

    AnimationManager(const AnimationManager&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;
    AnimationManager(AnimationManager&&) noexcept = default;
    AnimationManager& operator=(AnimationManager&&) noexcept = default;

    uint32_t addAnimation(AnimationCallback callback);
    void removeAnimation(uint32_t id) noexcept;
    void advance(float dt);

    [[nodiscard]] bool hasActiveAnimations() const noexcept;
    [[nodiscard]] size_t activeAnimationCount() const noexcept;
    void clear() noexcept;

private:
    struct Entry {
        uint32_t id{0};
        AnimationCallback callback{};
        bool markedForRemoval{false};
    };

    std::vector<Entry> entries_{};
    uint32_t nextId_{1};
    bool isIterating_{false};
};

} // namespace northern::core::animation
