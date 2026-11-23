#include "platform/display.hpp"
#include "platform/window.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace northern::platform {

namespace {

float getDpiScaleForMonitor(HMONITOR hMonitor) {
    HMODULE shcore = LoadLibraryW(L"shcore.dll");
    if (shcore) {
        using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
        auto getDpiFn =
            reinterpret_cast<GetDpiForMonitorFn>(GetProcAddress(shcore, "GetDpiForMonitor"));
        if (getDpiFn) {
            UINT dpiX = 96;
            UINT dpiY = 96;
            if (SUCCEEDED(getDpiFn(hMonitor, 0 /*MDT_EFFECTIVE_DPI*/, &dpiX, &dpiY))) {
                FreeLibrary(shcore);
                return static_cast<float>(dpiX) / 96.0f;
            }
        }
        FreeLibrary(shcore);
    }

    HDC hdc = GetDC(nullptr);
    if (hdc) {
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
        if (dpi > 0) {
            return static_cast<float>(dpi) / 96.0f;
        }
    }
    return 1.0f;
}

Display displayFromHMonitor(HMONITOR hMonitor) {
    MONITORINFOEXW mi{};
    mi.cbSize = sizeof(MONITORINFOEXW);
    if (!GetMonitorInfoW(hMonitor, &mi)) {
        return Display{};
    }

    float scale = getDpiScaleForMonitor(hMonitor);
    if (scale <= 0.0f) {
        scale = 1.0f;
    }

    Display d;
    int nameLen = WideCharToMultiByte(CP_UTF8, 0, mi.szDevice, -1, nullptr, 0, nullptr, nullptr);
    if (nameLen > 1) {
        d.name.resize(static_cast<size_t>(nameLen - 1));
        WideCharToMultiByte(CP_UTF8, 0, mi.szDevice, -1, d.name.data(), nameLen, nullptr, nullptr);
    } else {
        d.name = "Default";
    }

    d.scale = scale;
    d.isPrimary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;

    float left = static_cast<float>(mi.rcMonitor.left) / scale;
    float top = static_cast<float>(mi.rcMonitor.top) / scale;
    float width = static_cast<float>(mi.rcMonitor.right - mi.rcMonitor.left) / scale;
    float height = static_cast<float>(mi.rcMonitor.bottom - mi.rcMonitor.top) / scale;
    d.bounds = core::Rect{{left, top}, {width, height}};

    float workLeft = static_cast<float>(mi.rcWork.left) / scale;
    float workTop = static_cast<float>(mi.rcWork.top) / scale;
    float workWidth = static_cast<float>(mi.rcWork.right - mi.rcWork.left) / scale;
    float workHeight = static_cast<float>(mi.rcWork.bottom - mi.rcWork.top) / scale;
    d.workArea = core::Rect{{workLeft, workTop}, {workWidth, workHeight}};

    return d;
}

BOOL CALLBACK monitorEnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM lParam) {
    auto* list = reinterpret_cast<std::vector<Display>*>(lParam);
    list->push_back(displayFromHMonitor(hMonitor));
    return TRUE;
}

} // namespace

std::vector<Display> Display::all() {
    std::vector<Display> displays;
    EnumDisplayMonitors(nullptr, nullptr, monitorEnumProc, reinterpret_cast<LPARAM>(&displays));
    if (displays.empty()) {
        displays.push_back(primary());
    }
    return displays;
}

Display Display::primary() {
    POINT ptZero{0, 0};
    HMONITOR hMon = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
    if (hMon) {
        return displayFromHMonitor(hMon);
    }
    Display d;
    d.name = "Primary";
    d.bounds = core::Rect{{0.0f, 0.0f}, {1920.0f, 1080.0f}};
    d.workArea = d.bounds;
    d.scale = 1.0f;
    d.isPrimary = true;
    return d;
}

Display Display::forWindow(const Window& window) {
    HWND hwnd = reinterpret_cast<HWND>(window.nativeHandle().handle);
    if (hwnd && IsWindow(hwnd)) {
        HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        if (hMon) {
            return displayFromHMonitor(hMon);
        }
    }
    return primary();
}

} // namespace northern::platform
