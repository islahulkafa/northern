#pragma once

#include "core/math/geometry.hpp"
#include <string>

namespace northern::platform {

struct WindowDesc {
    std::string title{"Northern"};
    core::Size size{800.0f, 600.0f};
    core::Size minSize{0.0f, 0.0f};
    core::Size maxSize{0.0f, 0.0f};
    bool resizable{true};
    bool decorated{true};
    bool visible{true};
    bool startMaximized{false};
};

} // namespace northern::platform
