#include "platform/win32/win32_key_map.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace northern::platform {

Key win32VkToKey(uint32_t vkCode) noexcept {
    if (vkCode >= 'A' && vkCode <= 'Z') {
        return static_cast<Key>(static_cast<uint32_t>(Key::A) + (vkCode - 'A'));
    }
    if (vkCode >= '0' && vkCode <= '9') {
        return static_cast<Key>(static_cast<uint32_t>(Key::Num0) + (vkCode - '0'));
    }
    if (vkCode >= VK_NUMPAD0 && vkCode <= VK_NUMPAD9) {
        return static_cast<Key>(static_cast<uint32_t>(Key::Kp0) + (vkCode - VK_NUMPAD0));
    }
    if (vkCode >= VK_F1 && vkCode <= VK_F24) {
        return static_cast<Key>(static_cast<uint32_t>(Key::F1) + (vkCode - VK_F1));
    }

    switch (vkCode) {
    case VK_SPACE:
        return Key::Space;
    case VK_ESCAPE:
        return Key::Escape;
    case VK_RETURN:
        return Key::Enter;
    case VK_TAB:
        return Key::Tab;
    case VK_BACK:
        return Key::Backspace;
    case VK_INSERT:
        return Key::Insert;
    case VK_DELETE:
        return Key::Delete;
    case VK_RIGHT:
        return Key::Right;
    case VK_LEFT:
        return Key::Left;
    case VK_DOWN:
        return Key::Down;
    case VK_UP:
        return Key::Up;
    case VK_PRIOR:
        return Key::PageUp;
    case VK_NEXT:
        return Key::PageDown;
    case VK_HOME:
        return Key::Home;
    case VK_END:
        return Key::End;
    case VK_CAPITAL:
        return Key::CapsLock;
    case VK_SCROLL:
        return Key::ScrollLock;
    case VK_NUMLOCK:
        return Key::NumLock;
    case VK_SNAPSHOT:
        return Key::PrintScreen;
    case VK_PAUSE:
        return Key::Pause;

    case VK_DECIMAL:
        return Key::KpDecimal;
    case VK_DIVIDE:
        return Key::KpDivide;
    case VK_MULTIPLY:
        return Key::KpMultiply;
    case VK_SUBTRACT:
        return Key::KpSubtract;
    case VK_ADD:
        return Key::KpAdd;

    case VK_LSHIFT:
        return Key::LeftShift;
    case VK_RSHIFT:
        return Key::RightShift;
    case VK_LCONTROL:
        return Key::LeftControl;
    case VK_RCONTROL:
        return Key::RightControl;
    case VK_LMENU:
        return Key::LeftAlt;
    case VK_RMENU:
        return Key::RightAlt;
    case VK_LWIN:
        return Key::LeftSuper;
    case VK_RWIN:
        return Key::RightSuper;
    case VK_APPS:
        return Key::Menu;

    case VK_OEM_1:
        return Key::Semicolon;
    case VK_OEM_PLUS:
        return Key::Equal;
    case VK_OEM_COMMA:
        return Key::Comma;
    case VK_OEM_MINUS:
        return Key::Minus;
    case VK_OEM_PERIOD:
        return Key::Period;
    case VK_OEM_2:
        return Key::Slash;
    case VK_OEM_3:
        return Key::GraveAccent;
    case VK_OEM_4:
        return Key::LeftBracket;
    case VK_OEM_5:
        return Key::Backslash;
    case VK_OEM_6:
        return Key::RightBracket;
    case VK_OEM_7:
        return Key::Apostrophe;

    default:
        return Key::Unknown;
    }
}

KeyModifier queryWin32Modifiers() noexcept {
    KeyModifier modifiers = KeyModifier::None;

    if (GetKeyState(VK_SHIFT) & 0x8000) {
        modifiers |= KeyModifier::Shift;
    }
    if (GetKeyState(VK_CONTROL) & 0x8000) {
        modifiers |= KeyModifier::Control;
    }
    if (GetKeyState(VK_MENU) & 0x8000) {
        modifiers |= KeyModifier::Alt;
    }
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) {
        modifiers |= KeyModifier::Super;
    }
    if (GetKeyState(VK_CAPITAL) & 0x0001) {
        modifiers |= KeyModifier::CapsLock;
    }
    if (GetKeyState(VK_NUMLOCK) & 0x0001) {
        modifiers |= KeyModifier::NumLock;
    }

    return modifiers;
}

} // namespace northern::platform
