#pragma once

#include "core/error/error.hpp"
#include "graphics/types.hpp"

#ifdef _WIN32
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <string_view>
#include <vulkan/vulkan.h>

namespace northern::graphics {

[[nodiscard]] constexpr std::string_view vkResultToString(VkResult result) noexcept {
    switch (result) {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_EVENT_SET:
        return "VK_EVENT_SET";
    case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    default:
        return "VK_UNKNOWN_ERROR";
    }
}

[[nodiscard]] inline core::Error makeVkError(VkResult result, std::string_view message) {
    return core::Error(core::ErrorCode::GraphicsError,
                       std::string(message) + " (" + std::string(vkResultToString(result)) + ")");
}

[[nodiscard]] constexpr VkFormat toVkFormat(Format format) noexcept {
    switch (format) {
    case Format::R8_Unorm:
        return VK_FORMAT_R8_UNORM;
    case Format::R8G8B8A8_Unorm:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case Format::B8G8R8A8_Unorm:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case Format::R8G8B8A8_Srgb:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case Format::B8G8R8A8_Srgb:
        return VK_FORMAT_B8G8R8A8_SRGB;
    case Format::R16G16_Sfloat:
        return VK_FORMAT_R16G16_SFLOAT;
    case Format::R32_Sfloat:
        return VK_FORMAT_R32_SFLOAT;
    case Format::R32G32_Sfloat:
        return VK_FORMAT_R32G32_SFLOAT;
    case Format::R32G32B32_Sfloat:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case Format::R32G32B32A32_Sfloat:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case Format::D24_Unorm_S8_Uint:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case Format::D32_Sfloat:
        return VK_FORMAT_D32_SFLOAT;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

[[nodiscard]] constexpr Format fromVkFormat(VkFormat format) noexcept {
    switch (format) {
    case VK_FORMAT_R8_UNORM:
        return Format::R8_Unorm;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return Format::R8G8B8A8_Unorm;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return Format::B8G8R8A8_Unorm;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return Format::R8G8B8A8_Srgb;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return Format::B8G8R8A8_Srgb;
    case VK_FORMAT_R16G16_SFLOAT:
        return Format::R16G16_Sfloat;
    case VK_FORMAT_R32_SFLOAT:
        return Format::R32_Sfloat;
    case VK_FORMAT_R32G32_SFLOAT:
        return Format::R32G32_Sfloat;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return Format::R32G32B32_Sfloat;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return Format::R32G32B32A32_Sfloat;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return Format::D24_Unorm_S8_Uint;
    case VK_FORMAT_D32_SFLOAT:
        return Format::D32_Sfloat;
    default:
        return Format::Undefined;
    }
}

[[nodiscard]] constexpr VkPrimitiveTopology toVkTopology(PrimitiveTopology topology) noexcept {
    switch (topology) {
    case PrimitiveTopology::TriangleList:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case PrimitiveTopology::TriangleStrip:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case PrimitiveTopology::LineList:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case PrimitiveTopology::LineStrip:
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case PrimitiveTopology::PointList:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    default:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

[[nodiscard]] constexpr VkBlendFactor toVkBlendFactor(BlendFactor factor) noexcept {
    switch (factor) {
    case BlendFactor::Zero:
        return VK_BLEND_FACTOR_ZERO;
    case BlendFactor::One:
        return VK_BLEND_FACTOR_ONE;
    case BlendFactor::SrcColor:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case BlendFactor::OneMinusSrcColor:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case BlendFactor::DstColor:
        return VK_BLEND_FACTOR_DST_COLOR;
    case BlendFactor::OneMinusDstColor:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case BlendFactor::SrcAlpha:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case BlendFactor::OneMinusSrcAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case BlendFactor::DstAlpha:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case BlendFactor::OneMinusDstAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    default:
        return VK_BLEND_FACTOR_ONE;
    }
}

[[nodiscard]] constexpr VkBlendOp toVkBlendOp(BlendOp op) noexcept {
    switch (op) {
    case BlendOp::Add:
        return VK_BLEND_OP_ADD;
    case BlendOp::Subtract:
        return VK_BLEND_OP_SUBTRACT;
    case BlendOp::ReverseSubtract:
        return VK_BLEND_OP_REVERSE_SUBTRACT;
    case BlendOp::Min:
        return VK_BLEND_OP_MIN;
    case BlendOp::Max:
        return VK_BLEND_OP_MAX;
    default:
        return VK_BLEND_OP_ADD;
    }
}

[[nodiscard]] constexpr VkFilter toVkFilter(Filter filter) noexcept {
    switch (filter) {
    case Filter::Nearest:
        return VK_FILTER_NEAREST;
    case Filter::Linear:
        return VK_FILTER_LINEAR;
    default:
        return VK_FILTER_LINEAR;
    }
}

[[nodiscard]] constexpr VkSamplerAddressMode toVkAddressMode(AddressMode mode) noexcept {
    switch (mode) {
    case AddressMode::Repeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case AddressMode::MirroredRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case AddressMode::ClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case AddressMode::ClampToBorder:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    default:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }
}

} // namespace northern::graphics
