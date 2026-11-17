#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "graphics/device.hpp"
#include "renderer/vertex_2d.hpp"
#include <cstdint>
#include <memory>

namespace northern::renderer {

struct RingAllocation {
    uint32_t firstVertex{0};
    uint32_t firstIndex{0};
    Vertex2D* vertexData{nullptr};
    uint32_t* indexData{nullptr};
};

class DynamicRingBuffer {
public:
    static core::Result<std::unique_ptr<DynamicRingBuffer>, core::Error>
    create(graphics::Device& device, uint32_t vertexCapacity, uint32_t indexCapacity,
           uint32_t frameSlots = 3);

    ~DynamicRingBuffer();

    DynamicRingBuffer(const DynamicRingBuffer&) = delete;
    DynamicRingBuffer& operator=(const DynamicRingBuffer&) = delete;
    DynamicRingBuffer(DynamicRingBuffer&&) noexcept = default;
    DynamicRingBuffer& operator=(DynamicRingBuffer&&) noexcept = default;

    void resetFrame(uint32_t frameSlot);

    [[nodiscard]] core::Result<RingAllocation, core::Error> allocate(uint32_t vertexCount,
                                                                     uint32_t indexCount);

    [[nodiscard]] graphics::BufferHandle vertexBuffer() const noexcept { return vertexBuffer_; }
    [[nodiscard]] graphics::BufferHandle indexBuffer() const noexcept { return indexBuffer_; }
    [[nodiscard]] uint32_t frameSlots() const noexcept { return frameSlots_; }

private:
    DynamicRingBuffer(graphics::Device& device, graphics::BufferHandle vBuf,
                      graphics::BufferHandle iBuf, Vertex2D* vMapped, uint32_t* iMapped,
                      uint32_t vertexCap, uint32_t indexCap, uint32_t frameSlots) noexcept;

    graphics::Device* device_{nullptr};
    graphics::BufferHandle vertexBuffer_{graphics::BufferHandle::invalid()};
    graphics::BufferHandle indexBuffer_{graphics::BufferHandle::invalid()};
    Vertex2D* vertexMapped_{nullptr};
    uint32_t* indexMapped_{nullptr};

    uint32_t frameSlots_{1};
    uint32_t slotVertexCapacity_{0};
    uint32_t slotIndexCapacity_{0};

    uint32_t currentFrameSlot_{0};
    uint32_t currentVertexHead_{0};
    uint32_t currentIndexHead_{0};
};

} // namespace northern::renderer
