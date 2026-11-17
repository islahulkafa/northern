#pragma once

#include "core/memory/handle.hpp"
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace northern::core {

template <typename T, typename Tag>
class HandlePool {
public:
    using HandleType = Handle<Tag>;
    using ValueType = T;

    HandlePool() = default;

    template <typename... Args>
    HandleType allocate(Args&&... args) {
        uint32_t slotIndex = 0;
        if (freeHead_ != kInvalidIndex) {
            slotIndex = freeHead_;
            freeHead_ = slots_[slotIndex].nextFree;
        } else {
            slotIndex = static_cast<uint32_t>(slots_.size());
            slots_.emplace_back();
            slots_.back().generation = 1;
        }

        Slot& slot = slots_[slotIndex];
        slot.value.emplace(std::forward<Args>(args)...);
        slot.nextFree = kInvalidIndex;
        ++activeCount_;

        return HandleType{slotIndex, slot.generation};
    }

    bool release(HandleType handle) {
        if (!isValid(handle)) {
            return false;
        }

        Slot& slot = slots_[handle.index];
        slot.value.reset();

        // Advance generation to invalidate existing handles
        slot.generation++;
        if (slot.generation == 0) {
            slot.generation = 1;
        }

        slot.nextFree = freeHead_;
        freeHead_ = handle.index;
        --activeCount_;

        return true;
    }

    [[nodiscard]] bool isValid(HandleType handle) const noexcept {
        if (!handle.isValid() || handle.index >= slots_.size()) {
            return false;
        }
        const Slot& slot = slots_[handle.index];
        return slot.generation == handle.generation && slot.value.has_value();
    }

    [[nodiscard]] T* get(HandleType handle) noexcept {
        if (!isValid(handle)) {
            return nullptr;
        }
        return &(*slots_[handle.index].value);
    }

    [[nodiscard]] const T* get(HandleType handle) const noexcept {
        if (!isValid(handle)) {
            return nullptr;
        }
        return &(*slots_[handle.index].value);
    }

    [[nodiscard]] size_t activeCount() const noexcept { return activeCount_; }

    [[nodiscard]] size_t capacity() const noexcept { return slots_.size(); }

    void clear() {
        slots_.clear();
        freeHead_ = kInvalidIndex;
        activeCount_ = 0;
    }

private:
    static constexpr uint32_t kInvalidIndex = 0xFFFFFFFF;

    struct Slot {
        uint32_t generation{1};
        uint32_t nextFree{kInvalidIndex};
        std::optional<T> value;
    };

    std::vector<Slot> slots_;
    uint32_t freeHead_{kInvalidIndex};
    size_t activeCount_{0};
};

} // namespace northern::core
