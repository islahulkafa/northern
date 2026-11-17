#include "platform/clipboard.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <vector>
#include <windows.h>

namespace northern::platform {

core::Result<void, core::Error> Clipboard::setText(std::string_view text) {
    if (!OpenClipboard(nullptr)) {
        return core::Error(core::ErrorCode::PlatformError, "Failed to open Win32 clipboard");
    }

    if (!EmptyClipboard()) {
        CloseClipboard();
        return core::Error(core::ErrorCode::PlatformError, "Failed to empty Win32 clipboard");
    }

    if (text.empty()) {
        CloseClipboard();
        return core::Result<void, core::Error>::ok();
    }

    int wideLen =
        MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (wideLen <= 0) {
        CloseClipboard();
        return core::Error(core::ErrorCode::InvalidArgument,
                           "Failed to convert UTF-8 to UTF-16 for clipboard");
    }

    HGLOBAL hGlobal =
        GlobalAlloc(GMEM_MOVEABLE, static_cast<size_t>(wideLen + 1) * sizeof(wchar_t));
    if (!hGlobal) {
        CloseClipboard();
        return core::Error(core::ErrorCode::OutOfMemory,
                           "Failed to allocate memory for clipboard text");
    }

    wchar_t* dest = static_cast<wchar_t*>(GlobalLock(hGlobal));
    if (!dest) {
        GlobalFree(hGlobal);
        CloseClipboard();
        return core::Error(core::ErrorCode::PlatformError,
                           "Failed to lock global memory for clipboard text");
    }

    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), dest, wideLen);
    dest[wideLen] = L'\0';
    GlobalUnlock(hGlobal);

    if (!SetClipboardData(CF_UNICODETEXT, hGlobal)) {
        GlobalFree(hGlobal);
        CloseClipboard();
        return core::Error(core::ErrorCode::PlatformError, "Failed to set Win32 clipboard data");
    }

    CloseClipboard();
    return core::Result<void, core::Error>::ok();
}

core::Result<std::string, core::Error> Clipboard::getText() {
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        return std::string{};
    }

    if (!OpenClipboard(nullptr)) {
        return core::Error(core::ErrorCode::PlatformError, "Failed to open Win32 clipboard");
    }

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) {
        CloseClipboard();
        return std::string{};
    }

    const wchar_t* src = static_cast<const wchar_t*>(GlobalLock(hData));
    if (!src) {
        CloseClipboard();
        return core::Error(core::ErrorCode::PlatformError, "Failed to lock clipboard data");
    }

    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);
    if (utf8Len <= 1) {
        GlobalUnlock(hData);
        CloseClipboard();
        return std::string{};
    }

    std::string result(static_cast<size_t>(utf8Len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, src, -1, result.data(), utf8Len, nullptr, nullptr);

    GlobalUnlock(hData);
    CloseClipboard();
    return result;
}

bool Clipboard::hasText() noexcept {
    return IsClipboardFormatAvailable(CF_UNICODETEXT) != FALSE;
}

} // namespace northern::platform
