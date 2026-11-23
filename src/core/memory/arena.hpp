#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace northern::core {

class Arena {
public:
    struct Marker {
        size_t chunkIndex{0};
        size_t offset{0};
    };

    static constexpr size_t kDefaultChunkSize = 64 * 1024; // 64 KB

    explicit Arena(size_t defaultChunkSize = kDefaultChunkSize);
    ~Arena() = default;

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) noexcept = default;
    Arena& operator=(Arena&&) noexcept = default;

    [[nodiscard]] void* allocate(size_t size, size_t alignment = 16);

    template <typename T, typename... Args>
    [[nodiscard]] T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return ::new (mem) T(std::forward<Args>(args)...);
    }

    void reset() noexcept;

    [[nodiscard]] Marker getMarker() const noexcept;
    void rollback(const Marker& marker) noexcept;

    [[nodiscard]] size_t totalAllocated() const noexcept;
    [[nodiscard]] size_t totalCapacity() const noexcept;
    [[nodiscard]] size_t chunkCount() const noexcept { return chunks_.size(); }

private:
    struct Chunk {
        std::unique_ptr<uint8_t[]> data;
        size_t capacity{0};
        size_t used{0};
    };

    void grow(size_t minCapacity);

    size_t defaultChunkSize_{kDefaultChunkSize};
    std::vector<Chunk> chunks_{};
    size_t currentChunkIndex_{0};
};

class ScopedArena {
public:
    explicit ScopedArena(Arena& arena) noexcept
        : arena_(arena), marker_(arena.getMarker()) {}

    ~ScopedArena() noexcept {
        arena_.rollback(marker_);
    }

    ScopedArena(const ScopedArena&) = delete;
    ScopedArena& operator=(const ScopedArena&) = delete;
    ScopedArena(ScopedArena&&) = delete;
    ScopedArena& operator=(ScopedArena&&) = delete;

    [[nodiscard]] Arena& arena() noexcept { return arena_; }

private:
    Arena& arena_;
    Arena::Marker marker_;
};

template <typename T>
class ArenaAllocator {
public:
    using value_type = T;

    constexpr ArenaAllocator() noexcept = default;
    explicit constexpr ArenaAllocator(Arena& arena) noexcept : arena_(&arena) {}

    template <typename U>
    constexpr ArenaAllocator(const ArenaAllocator<U>& other) noexcept : arena_(other.arena_) {}

    [[nodiscard]] T* allocate(size_t n) {
        if (!arena_) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(arena_->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* /*p*/, size_t /*n*/) noexcept {}

    template <typename U>
    bool operator==(const ArenaAllocator<U>& other) const noexcept {
        return arena_ == other.arena_;
    }

    template <typename U>
    friend class ArenaAllocator;

private:
    Arena* arena_{nullptr};
};

} // namespace northern::core
