#pragma once

#include <cstdint>

namespace northern::graphics::shaders {

inline constexpr uint32_t kQuadVertSpv[] =
#include "quad_vert_spv.hpp"
    ;

inline constexpr uint32_t kQuadFragSpv[] =
#include "quad_frag_spv.hpp"
    ;

} // namespace northern::graphics::shaders
