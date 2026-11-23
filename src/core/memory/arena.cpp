#include "core/memory/arena.hpp"
#include <cassert>

namespace northern::core {

Arena::Arena(size_t defaultChunkSize)
    : defaultChunkSize_(std::max<size_t>(defaultChunkSize, 64)) {}

void* Arena::allocate(size_t size, size_t alignment) {
    if (size == 0) {
        return nullptr;
    }

    // Ensure alignment is at least sizeof(void*) and a power of two
    if (alignment < alignof(void*)) {
        alignment = alignof(void*);
    }

    if (chunks_.empty()) {
        grow(size + alignment);
    }

    while (true) {
        Chunk& chunk = chunks_[currentChunkIndex_];
        uintptr_t basePtr = reinterpret_cast<uintptr_t>(chunk.data.get() + chunk.used);
        uintptr_t alignedPtr = (basePtr + (alignment - 1)) & ~(alignment - 1);
        size_t padding = alignedPtr - basePtr;

        if (chunk.used + padding + size <= chunk.capacity) {
            chunk.used += padding + size;
            return reinterpret_cast<void*>(alignedPtr);
        }

        // Doesn't fit in current chunk; move to next existing chunk if available
        if (currentChunkIndex_ + 1 < chunks_.size()) {
            ++currentChunkIndex_;
            chunks_[currentChunkIndex_].used = 0;
        } else {
            // Need a new chunk
            grow(size + alignment);
            ++currentChunkIndex_;
        }
    }
}

void Arena::grow(size_t minCapacity) {
    size_t newCap = std::max(defaultChunkSize_, minCapacity);
    chunks_.push_back(Chunk{
        std::make_unique<uint8_t[]>(newCap),
        newCap,
        0
    });
}

void Arena::reset() noexcept {
    currentChunkIndex_ = 0;
    for (auto& chunk : chunks_) {
        chunk.used = 0;
    }
}

Arena::Marker Arena::getMarker() const noexcept {
    if (chunks_.empty()) {
        return Marker{0, 0};
    }
    return Marker{currentChunkIndex_, chunks_[currentChunkIndex_].used};
}

void Arena::rollback(const Marker& marker) noexcept {
    if (chunks_.empty()) {
        currentChunkIndex_ = 0;
        return;
    }

    if (marker.chunkIndex < chunks_.size()) {
        currentChunkIndex_ = marker.chunkIndex;
        chunks_[currentChunkIndex_].used = std::min(marker.offset, chunks_[currentChunkIndex_].capacity);
        for (size_t i = currentChunkIndex_ + 1; i < chunks_.size(); ++i) {
            chunks_[i].used = 0;
        }
    }
}

size_t Arena::totalAllocated() const noexcept {
    size_t total = 0;
    for (size_t i = 0; i <= currentChunkIndex_ && i < chunks_.size(); ++i) {
        total += chunks_[i].used;
    }
    return total;
}

size_t Arena::totalCapacity() const noexcept {
    size_t total = 0;
    for (const auto& chunk : chunks_) {
        total += chunk.capacity;
    }
    return total;
}

} // namespace northern::core
