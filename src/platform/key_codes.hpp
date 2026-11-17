#pragma once

#include <cstdint>
#include <type_traits>

namespace northern::platform {

enum class Key : uint32_t {
    Unknown = 0,

    // Printable keys
    Space,
    Apostrophe,
    Comma,
    Minus,
    Period,
    Slash,
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    Semicolon,
    Equal,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    LeftBracket,
    Backslash,
    RightBracket,
    GraveAccent,

    // Function and control keys
    Escape,
    Enter,
    Tab,
    Backspace,
    Insert,
    Delete,
    Right,
    Left,
    Down,
    Up,
    PageUp,
    PageDown,
    Home,
    End,
    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,

    // Function keys F1 - F24
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,

    // Keypad keys
    Kp0,
    Kp1,
    Kp2,
    Kp3,
    Kp4,
    Kp5,
    Kp6,
    Kp7,
    Kp8,
    Kp9,
    KpDecimal,
    KpDivide,
    KpMultiply,
    KpSubtract,
    KpAdd,
    KpEnter,
    KpEqual,

    // Modifiers
    LeftShift,
    LeftControl,
    LeftAlt,
    LeftSuper,
    RightShift,
    RightControl,
    RightAlt,
    RightSuper,
    Menu
};

enum class KeyModifier : uint32_t {
    None = 0,
    Shift = 1 << 0,
    Control = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3,
    CapsLock = 1 << 4,
    NumLock = 1 << 5
};

constexpr KeyModifier operator|(KeyModifier lhs, KeyModifier rhs) noexcept {
    using Underlying = std::underlying_type_t<KeyModifier>;
    return static_cast<KeyModifier>(static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs));
}

constexpr KeyModifier operator&(KeyModifier lhs, KeyModifier rhs) noexcept {
    using Underlying = std::underlying_type_t<KeyModifier>;
    return static_cast<KeyModifier>(static_cast<Underlying>(lhs) & static_cast<Underlying>(rhs));
}

constexpr KeyModifier operator^(KeyModifier lhs, KeyModifier rhs) noexcept {
    using Underlying = std::underlying_type_t<KeyModifier>;
    return static_cast<KeyModifier>(static_cast<Underlying>(lhs) ^ static_cast<Underlying>(rhs));
}

constexpr KeyModifier operator~(KeyModifier modifier) noexcept {
    using Underlying = std::underlying_type_t<KeyModifier>;
    return static_cast<KeyModifier>(~static_cast<Underlying>(modifier));
}

constexpr KeyModifier& operator|=(KeyModifier& lhs, KeyModifier rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

constexpr KeyModifier& operator&=(KeyModifier& lhs, KeyModifier rhs) noexcept {
    lhs = lhs & rhs;
    return lhs;
}

constexpr bool hasModifier(KeyModifier mask, KeyModifier modifier) noexcept {
    return (mask & modifier) == modifier;
}

} // namespace northern::platform
