#pragma once

#include "graphics/gpu_handle.hpp"
#include "graphics/types.hpp"
#include <cstdint>
#include <string_view>

namespace northern::graphics {

struct SamplerDesc {
    Filter minFilter{Filter::Linear};
    Filter magFilter{Filter::Linear};
    AddressMode addressModeU{AddressMode::ClampToEdge};
    AddressMode addressModeV{AddressMode::ClampToEdge};
};

struct TextureDesc {
    uint32_t width{0};
    uint32_t height{0};
    Format format{Format::R8G8B8A8_Unorm};
    TextureUsage usage{TextureUsage::Sampled};
    uint32_t mipLevels{1};
    SamplerDesc sampler{};
    std::string_view debugName{};
};

} // namespace northern::graphics
