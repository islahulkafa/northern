#pragma once

#include "graphics/gpu_handle.hpp"
#include "graphics/types.hpp"
#include <cstdint>
#include <string_view>

namespace northern::graphics {

struct BufferDesc {
    uint64_t size{0};
    BufferUsage usage{BufferUsage::None};
    MemoryUsage memoryUsage{MemoryUsage::GpuOnly};
    std::string_view debugName{};
};

} // namespace northern::graphics
