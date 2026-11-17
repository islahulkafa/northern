#pragma once

#include <cstdint>

namespace northern::core {

struct NativeWindowHandle {
    uintptr_t handle{0};

    [[nodiscard]] constexpr bool isValid() const noexcept { return handle != 0; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return isValid(); }
};

} // namespace northern::core
