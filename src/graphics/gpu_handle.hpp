#pragma once

#include "core/memory/handle.hpp"

namespace northern::graphics {

struct BufferTag {};
struct TextureTag {};
struct PipelineTag {};
struct ShaderTag {};

using BufferHandle = core::Handle<BufferTag>;
using TextureHandle = core::Handle<TextureTag>;
using PipelineHandle = core::Handle<PipelineTag>;
using ShaderHandle = core::Handle<ShaderTag>;

} // namespace northern::graphics
