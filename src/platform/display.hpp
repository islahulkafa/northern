#pragma once

#include "core/math/geometry.hpp"

#include <string>
#include <vector>

namespace northern::platform {

class Window;

struct Display {
    std::string name;
    core::Rect bounds{0.0f, 0.0f, 0.0f, 0.0f};
    core::Rect workArea{0.0f, 0.0f, 0.0f, 0.0f};
    float scale{1.0f};
    bool isPrimary{false};

    [[nodiscard]] static std::vector<Display> all();
    [[nodiscard]] static Display primary();
    [[nodiscard]] static Display forWindow(const Window& window);
};

} // namespace northern::platform
