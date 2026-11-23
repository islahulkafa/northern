#pragma once

#include "core/math/geometry.hpp"
#include "platform/key_codes.hpp"
#include "platform/mouse_types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace northern::platform {

struct MouseEvent {
    MouseEventType type{MouseEventType::Move};
    MouseButton button{MouseButton::None};
    core::Point position{0.0f, 0.0f};
    core::Point physicalPosition{0.0f, 0.0f};
    core::Point wheelDelta{0.0f, 0.0f};
    KeyModifier modifiers{KeyModifier::None};
};

struct KeyEvent {
    Key key{Key::Unknown};
    bool isDown{false};
    bool isRepeat{false};
    uint32_t textCodepoint{0};
    KeyModifier modifiers{KeyModifier::None};
};

struct ResizeEvent {
    core::Size size{0.0f, 0.0f};
    core::SizeI physicalSize{0, 0};
    bool isLive{false};
};

struct DpiChangedEvent {
    float dpiScale{1.0f};
    uint32_t dpi{96};
    core::Rect suggestedWindowRect{0.0f, 0.0f, 0.0f, 0.0f};
};

struct FocusChangedEvent {
    bool hasFocus{false};
};

struct FileDropEvent {
    std::vector<std::string> paths{};
    core::Point position{0.0f, 0.0f};
};

struct ThemeChangedEvent {
    bool isDarkMode{true};
};

} // namespace northern::platform
