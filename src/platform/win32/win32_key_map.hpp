#pragma once

#include "platform/key_codes.hpp"
#include <cstdint>

namespace northern::platform {

[[nodiscard]] Key win32VkToKey(uint32_t vkCode) noexcept;

[[nodiscard]] KeyModifier queryWin32Modifiers() noexcept;

} // namespace northern::platform
