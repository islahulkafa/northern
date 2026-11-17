#pragma once

#include <cstdint>

namespace northern::platform {

enum class MouseButton : uint8_t { None = 0, Left, Right, Middle, X1, X2 };

enum class MouseEventType : uint8_t { ButtonDown, ButtonUp, Move, Wheel, Enter, Leave };

enum class CursorShape : uint8_t {
    Arrow,
    IBeam,
    PointingHand,
    ResizeEW,
    ResizeNS,
    ResizeNWSE,
    ResizeNESW,
    Crosshair,
    Wait,
    Forbidden
};

} // namespace northern::platform
