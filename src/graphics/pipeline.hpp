#pragma once

#include "graphics/gpu_handle.hpp"
#include "graphics/types.hpp"
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace northern::graphics {

struct VertexAttribute {
    uint32_t location{0};
    uint32_t binding{0};
    Format format{Format::R32G32_Sfloat};
    uint32_t offset{0};
};

struct VertexBinding {
    uint32_t binding{0};
    uint32_t stride{0};
};

struct VertexLayout {
    std::vector<VertexBinding> bindings;
    std::vector<VertexAttribute> attributes;
};

struct BlendState {
    bool blendEnable{true};
    BlendFactor srcColorFactor{BlendFactor::SrcAlpha};
    BlendFactor dstColorFactor{BlendFactor::OneMinusSrcAlpha};
    BlendOp colorBlendOp{BlendOp::Add};
    BlendFactor srcAlphaFactor{BlendFactor::One};
    BlendFactor dstAlphaFactor{BlendFactor::OneMinusSrcAlpha};
    BlendOp alphaBlendOp{BlendOp::Add};
    ColorWriteMask writeMask{ColorWriteMask::All};
};

struct ShaderBytecode {
    std::span<const uint32_t> spirv;
    std::string_view entryPoint{"main"};
};

struct PipelineDesc {
    ShaderBytecode vertexShader;
    ShaderBytecode fragmentShader;
    VertexLayout vertexLayout;
    PrimitiveTopology topology{PrimitiveTopology::TriangleList};
    BlendState blendState;
    Format colorAttachmentFormat{Format::B8G8R8A8_Unorm};
    uint32_t pushConstantSize{0};
    bool hasTexture{false};
    std::string_view debugName{};
};

} // namespace northern::graphics
