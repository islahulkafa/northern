#include "renderer/dynamic_ring_buffer.hpp"

namespace northern::renderer {

DynamicRingBuffer::DynamicRingBuffer(graphics::Device& device, graphics::BufferHandle vBuf,
                                     graphics::BufferHandle iBuf, Vertex2D* vMapped,
                                     uint32_t* iMapped, uint32_t vertexCap, uint32_t indexCap,
                                     uint32_t frameSlots) noexcept
    : device_(&device), vertexBuffer_(vBuf), indexBuffer_(iBuf), vertexMapped_(vMapped),
      indexMapped_(iMapped), frameSlots_(frameSlots > 0 ? frameSlots : 1),
      slotVertexCapacity_(vertexCap / (frameSlots > 0 ? frameSlots : 1)),
      slotIndexCapacity_(indexCap / (frameSlots > 0 ? frameSlots : 1)) {
    resetFrame(0);
}

DynamicRingBuffer::~DynamicRingBuffer() {
    if (device_) {
        if (vertexBuffer_.isValid()) {
            device_->destroyBuffer(vertexBuffer_);
            vertexBuffer_ = graphics::BufferHandle::invalid();
        }
        if (indexBuffer_.isValid()) {
            device_->destroyBuffer(indexBuffer_);
            indexBuffer_ = graphics::BufferHandle::invalid();
        }
    }
}

core::Result<std::unique_ptr<DynamicRingBuffer>, core::Error>
DynamicRingBuffer::create(graphics::Device& device, uint32_t vertexCapacity, uint32_t indexCapacity,
                          uint32_t frameSlots) {
    if (vertexCapacity == 0 || indexCapacity == 0) {
        return core::Error(core::ErrorCode::InvalidArgument,
                           "Ring buffer capacity must be greater than 0");
    }

    uint32_t slots = frameSlots > 0 ? frameSlots : 1;

    graphics::BufferDesc vDesc{};
    vDesc.size = static_cast<uint64_t>(vertexCapacity) * sizeof(Vertex2D);
    vDesc.usage = graphics::BufferUsage::Vertex;
    vDesc.memoryUsage = graphics::MemoryUsage::CpuToGpu;
    vDesc.debugName = "2D Batch Renderer Vertex Ring Buffer";

    auto vBufRes = device.createBuffer(vDesc);
    if (vBufRes.isErr()) {
        return vBufRes.error();
    }
    graphics::BufferHandle vBuf = vBufRes.value();

    graphics::BufferDesc iDesc{};
    iDesc.size = static_cast<uint64_t>(indexCapacity) * sizeof(uint32_t);
    iDesc.usage = graphics::BufferUsage::Index;
    iDesc.memoryUsage = graphics::MemoryUsage::CpuToGpu;
    iDesc.debugName = "2D Batch Renderer Index Ring Buffer";

    auto iBufRes = device.createBuffer(iDesc);
    if (iBufRes.isErr()) {
        device.destroyBuffer(vBuf);
        return iBufRes.error();
    }
    graphics::BufferHandle iBuf = iBufRes.value();

    void* vMapped = device.mapBuffer(vBuf);
    void* iMapped = device.mapBuffer(iBuf);
    if (!vMapped || !iMapped) {
        device.destroyBuffer(vBuf);
        device.destroyBuffer(iBuf);
        return core::Error(core::ErrorCode::GraphicsError,
                           "Failed to map persistently coherent GPU buffer memory");
    }

    return std::unique_ptr<DynamicRingBuffer>(new DynamicRingBuffer(
        device, vBuf, iBuf, static_cast<Vertex2D*>(vMapped), static_cast<uint32_t*>(iMapped),
        vertexCapacity, indexCapacity, slots));
}

void DynamicRingBuffer::resetFrame(uint32_t frameSlot) {
    currentFrameSlot_ = frameSlot % frameSlots_;
    currentVertexHead_ = currentFrameSlot_ * slotVertexCapacity_;
    currentIndexHead_ = currentFrameSlot_ * slotIndexCapacity_;
}

core::Result<RingAllocation, core::Error> DynamicRingBuffer::allocate(uint32_t vertexCount,
                                                                      uint32_t indexCount) {
    uint32_t slotEndVertex = (currentFrameSlot_ + 1) * slotVertexCapacity_;
    uint32_t slotEndIndex = (currentFrameSlot_ + 1) * slotIndexCapacity_;

    if (currentVertexHead_ + vertexCount > slotEndVertex) {
        return core::Error(core::ErrorCode::OutOfMemory,
                           "Vertex ring buffer capacity exceeded for frame slot");
    }
    if (currentIndexHead_ + indexCount > slotEndIndex) {
        return core::Error(core::ErrorCode::OutOfMemory,
                           "Index ring buffer capacity exceeded for frame slot");
    }

    RingAllocation alloc{};
    alloc.firstVertex = currentVertexHead_;
    alloc.firstIndex = currentIndexHead_;
    alloc.vertexData = vertexMapped_ + currentVertexHead_;
    alloc.indexData = indexMapped_ + currentIndexHead_;

    currentVertexHead_ += vertexCount;
    currentIndexHead_ += indexCount;

    return alloc;
}

} // namespace northern::renderer
