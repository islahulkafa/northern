#include "platform/win32/win32_window.hpp"
#include "platform/application.hpp"
#include "platform/win32/win32_key_map.hpp"

#include <algorithm>
#include <cmath>
#include <dwmapi.h>
#include <mutex>
#include <shellapi.h>
#include <vector>
#include <windowsx.h>

namespace northern::platform {

namespace {

std::wstring utf8ToWide(std::string_view utf8Str) {
    if (utf8Str.empty()) {
        return L"";
    }
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), static_cast<int>(utf8Str.size()),
                                  nullptr, 0);
    if (len <= 0) {
        return L"";
    }
    std::wstring result(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), static_cast<int>(utf8Str.size()), result.data(),
                        len);
    return result;
}

void registerWindowClassOnce() {
    static std::once_flag s_registered;
    std::call_once(s_registered, []() {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = &Window::Impl::s_wndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hIcon = LoadIconW(nullptr, reinterpret_cast<LPCWSTR>(IDI_APPLICATION));
        wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = L"NorthernWindowClass";
        wc.hIconSm = LoadIconW(nullptr, reinterpret_cast<LPCWSTR>(IDI_APPLICATION));

        RegisterClassExW(&wc);
    });
}

void adjustWindowRectForDpi(RECT* rect, DWORD style, DWORD exStyle, UINT dpi) {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        using AdjustWindowRectExForDpiFn = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        auto adjustFn = reinterpret_cast<AdjustWindowRectExForDpiFn>(
            GetProcAddress(user32, "AdjustWindowRectExForDpi"));
        if (adjustFn) {
            adjustFn(rect, style, FALSE, exStyle, dpi);
            return;
        }
    }
    AdjustWindowRectEx(rect, style, FALSE, exStyle);
}

} // namespace

const wchar_t* Window::Impl::windowClassName() noexcept {
    return L"NorthernWindowClass";
}

Window::Impl::Impl(Window* owner) noexcept : owner_(owner) {
    hCurrentCursor_ = win32CursorFromShape(cursorShape_);
}

Window::Impl::~Impl() {
    if (hwnd_) {
        SetWindowLongPtrW(hwnd_, GWLP_USERDATA, 0);
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

core::Result<std::unique_ptr<Window::Impl>, core::Error>
Window::Impl::create(Window* owner, const WindowDesc& desc) {
    registerWindowClassOnce();

    auto impl = std::make_unique<Window::Impl>(owner);
    impl->title_ = desc.title;
    impl->logicalSize_ = desc.size;
    impl->minSize_ = desc.minSize;
    impl->maxSize_ = desc.maxSize;
    impl->customTitleBar_ = desc.customTitleBar;
    impl->resizable_ = desc.resizable;

    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    DWORD exStyle = WS_EX_APPWINDOW;

    if (desc.decorated || desc.customTitleBar) {
        style |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        if (desc.resizable) {
            style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
        }
    } else {
        style |= WS_POPUP;
    }

    if (desc.visible) {
        style |= WS_VISIBLE;
    }

    impl->dpi_ = 96;
    impl->dpiScale_ = 1.0f;

    int initialWidth = static_cast<int>(desc.size.width);
    int initialHeight = static_cast<int>(desc.size.height);

    RECT rect{0, 0, initialWidth, initialHeight};
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);

    std::wstring wideTitle = utf8ToWide(desc.title);
    HWND parentHwnd = desc.parent.isValid() ? reinterpret_cast<HWND>(desc.parent.handle) : nullptr;

    HWND hwnd = CreateWindowExW(exStyle, windowClassName(), wideTitle.c_str(), style, CW_USEDEFAULT,
                                CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
                                parentHwnd, nullptr, hInstance, impl.get());

    if (!hwnd) {
        return core::Error(core::ErrorCode::PlatformError, "Failed to create Win32 window");
    }

    impl->hwnd_ = hwnd;
    impl->updateDpi();

    DragAcceptFiles(hwnd, TRUE);

    BOOL useDarkMode = Application::isDarkMode() ? TRUE : FALSE;
    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR = 20;
    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 = 19;
    if (FAILED(DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR, &useDarkMode,
                                     sizeof(useDarkMode)))) {
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &useDarkMode,
                              sizeof(useDarkMode));
    }

    if (impl->customTitleBar_) {
        MARGINS margins = {1, 1, 1, 1};
        DwmExtendFrameIntoClientArea(hwnd, &margins);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    RECT clientRect{};
    GetClientRect(hwnd, &clientRect);
    impl->physicalSize_ = {clientRect.right - clientRect.left, clientRect.bottom - clientRect.top};
    impl->logicalSize_ = {static_cast<float>(impl->physicalSize_.width) / impl->dpiScale_,
                          static_cast<float>(impl->physicalSize_.height) / impl->dpiScale_};

    if (desc.startMaximized && desc.visible) {
        ShowWindow(hwnd, SW_MAXIMIZE);
    }

    return impl;
}

void Window::Impl::updateDpi() {
    if (!hwnd_) {
        return;
    }
    UINT dpi = GetDpiForWindow(hwnd_);
    if (dpi == 0) {
        HDC hdc = GetDC(hwnd_);
        dpi = static_cast<UINT>(GetDeviceCaps(hdc, LOGPIXELSX));
        ReleaseDC(hwnd_, hdc);
    }
    if (dpi == 0) {
        dpi = 96;
    }
    dpi_ = dpi;
    dpiScale_ = static_cast<float>(dpi) / 96.0f;
}

core::Point Window::Impl::position() const noexcept {
    if (!hwnd_ || !IsWindow(hwnd_)) {
        return core::Point{0.0f, 0.0f};
    }
    RECT rc{};
    GetWindowRect(hwnd_, &rc);
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    return core::Point{static_cast<float>(rc.left) / scale, static_cast<float>(rc.top) / scale};
}

void Window::Impl::setPosition(core::Point logicalPos) {
    if (!hwnd_ || !IsWindow(hwnd_)) {
        return;
    }
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    int physX = static_cast<int>(std::round(logicalPos.x * scale));
    int physY = static_cast<int>(std::round(logicalPos.y * scale));
    SetWindowPos(hwnd_, nullptr, physX, physY, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

void Window::Impl::centerOnScreen(std::optional<Display> display) {
    if (!hwnd_ || !IsWindow(hwnd_)) {
        return;
    }
    Display target = display.value_or(owner_ ? Display::forWindow(*owner_) : Display::primary());
    core::Rect workArea = target.workArea;

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float currentW = logicalSize_.width;
    float currentH = logicalSize_.height;

    float centeredX = workArea.origin.x + std::max(0.0f, (workArea.size.width - currentW) * 0.5f);
    float centeredY = workArea.origin.y + std::max(0.0f, (workArea.size.height - currentH) * 0.5f);

    int physX = static_cast<int>(std::round(centeredX * scale));
    int physY = static_cast<int>(std::round(centeredY * scale));
    SetWindowPos(hwnd_, nullptr, physX, physY, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

void Window::Impl::setSize(core::Size logicalSize) {
    if (!hwnd_) {
        return;
    }
    logicalSize_ = logicalSize;
    int physW = static_cast<int>(logicalSize.width * dpiScale_);
    int physH = static_cast<int>(logicalSize.height * dpiScale_);

    RECT rect{0, 0, physW, physH};
    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd_, GWL_STYLE));
    DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd_, GWL_EXSTYLE));
    adjustWindowRectForDpi(&rect, style, exStyle, dpi_);

    SetWindowPos(hwnd_, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::Impl::setMinSize(core::Size minSize) {
    minSize_ = minSize;
    if (hwnd_) {
        bool needAdjust = false;
        core::Size clamped = logicalSize_;
        if (minSize_.width > 0.0f && clamped.width < minSize_.width) {
            clamped.width = minSize_.width;
            needAdjust = true;
        }
        if (minSize_.height > 0.0f && clamped.height < minSize_.height) {
            clamped.height = minSize_.height;
            needAdjust = true;
        }
        if (needAdjust) {
            setSize(clamped);
        }
    }
}

void Window::Impl::setMaxSize(core::Size maxSize) {
    maxSize_ = maxSize;
    if (hwnd_) {
        bool needAdjust = false;
        core::Size clamped = logicalSize_;
        if (maxSize_.width > 0.0f && clamped.width > maxSize_.width) {
            clamped.width = maxSize_.width;
            needAdjust = true;
        }
        if (maxSize_.height > 0.0f && clamped.height > maxSize_.height) {
            clamped.height = maxSize_.height;
            needAdjust = true;
        }
        if (needAdjust) {
            setSize(clamped);
        }
    }
}

void Window::Impl::setTitle(std::string_view title) {
    title_ = title;
    if (hwnd_) {
        std::wstring wideTitle = utf8ToWide(title);
        SetWindowTextW(hwnd_, wideTitle.c_str());
    }
}

bool Window::Impl::isVisible() const noexcept {
    if (!hwnd_) {
        return false;
    }
    return IsWindowVisible(hwnd_) != FALSE;
}

void Window::Impl::setVisible(bool visible) {
    if (!hwnd_) {
        return;
    }
    ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE);
}

void Window::Impl::setCursorShape(CursorShape shape) {
    cursorShape_ = shape;
    hCurrentCursor_ = win32CursorFromShape(shape);
    updateCursor();
}

void Window::Impl::updateCursor() {
    if (hCurrentCursor_) {
        SetCursor(hCurrentCursor_);
    }
}

HCURSOR Window::Impl::win32CursorFromShape(CursorShape shape) noexcept {
    auto loadWin32Cursor = [](auto idc) noexcept -> HCURSOR {
        return LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(idc));
    };

    switch (shape) {
    case CursorShape::Arrow:
        return loadWin32Cursor(IDC_ARROW);
    case CursorShape::IBeam:
        return loadWin32Cursor(IDC_IBEAM);
    case CursorShape::PointingHand:
        return loadWin32Cursor(IDC_HAND);
    case CursorShape::ResizeEW:
        return loadWin32Cursor(IDC_SIZEWE);
    case CursorShape::ResizeNS:
        return loadWin32Cursor(IDC_SIZENS);
    case CursorShape::ResizeNWSE:
        return loadWin32Cursor(IDC_SIZENWSE);
    case CursorShape::ResizeNESW:
        return loadWin32Cursor(IDC_SIZENESW);
    case CursorShape::Crosshair:
        return loadWin32Cursor(IDC_CROSS);
    case CursorShape::Wait:
        return loadWin32Cursor(IDC_WAIT);
    case CursorShape::Forbidden:
        return loadWin32Cursor(IDC_NO);
    default:
        return loadWin32Cursor(IDC_ARROW);
    }
}

void Window::Impl::requestClose() {
    if (hwnd_) {
        PostMessageW(hwnd_, WM_CLOSE, 0, 0);
    }
}

void Window::Impl::setCustomTitleBar(bool custom) {
    if (customTitleBar_ == custom) {
        return;
    }
    customTitleBar_ = custom;
    if (hwnd_) {
        MARGINS margins = customTitleBar_ ? MARGINS{1, 1, 1, 1} : MARGINS{0, 0, 0, 0};
        DwmExtendFrameIntoClientArea(hwnd_, &margins);
        SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

bool Window::Impl::isMaximized() const noexcept {
    return hwnd_ ? (IsZoomed(hwnd_) != FALSE) : false;
}

bool Window::Impl::isMinimized() const noexcept {
    return hwnd_ ? (IsIconic(hwnd_) != FALSE) : false;
}

void Window::Impl::minimize() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_MINIMIZE);
    }
}

void Window::Impl::maximize() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_MAXIMIZE);
    }
}

void Window::Impl::restore() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_RESTORE);
    }
}

void Window::Impl::toggleMaximize() {
    if (isMaximized()) {
        restore();
    } else {
        maximize();
    }
}

void Window::Impl::setFullscreen(bool fullscreen) {
    if (isFullscreen_ == fullscreen || !hwnd_ || !IsWindow(hwnd_)) {
        return;
    }

    if (fullscreen) {
        savedPlacement_.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hwnd_, &savedPlacement_);
        savedStyle_ = static_cast<LONG>(GetWindowLongPtrW(hwnd_, GWL_STYLE));

        HMONITOR hMon = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi{};
        mi.cbSize = sizeof(MONITORINFO);
        if (GetMonitorInfoW(hMon, &mi)) {
            LONG newStyle = savedStyle_ & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX |
                                            WS_MAXIMIZEBOX | WS_SYSMENU);
            SetWindowLongPtrW(hwnd_, GWL_STYLE, newStyle);

            int x = mi.rcMonitor.left;
            int y = mi.rcMonitor.top;
            int w = mi.rcMonitor.right - mi.rcMonitor.left;
            int h = mi.rcMonitor.bottom - mi.rcMonitor.top;

            SetWindowPos(hwnd_, HWND_TOP, x, y, w, h, SWP_FRAMECHANGED | SWP_NOACTIVATE);
            isFullscreen_ = true;
        }
    } else {
        SetWindowLongPtrW(hwnd_, GWL_STYLE, savedStyle_);
        SetWindowPlacement(hwnd_, &savedPlacement_);
        SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
        isFullscreen_ = false;
    }
}

void Window::Impl::toggleFullscreen() {
    setFullscreen(!isFullscreen_);
}

LRESULT CALLBACK Window::Impl::s_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window::Impl* impl = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        impl = static_cast<Window::Impl*>(cs->lpCreateParams);
        if (impl) {
            impl->hwnd_ = hwnd;
        }
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(impl));
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    } else {
        impl = reinterpret_cast<Window::Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (impl) {
        return impl->wndProc(msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT Window::Impl::wndProc(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_NCCALCSIZE: {
        if (wParam && customTitleBar_) {
            if (IsZoomed(hwnd_)) {
                auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                int borderThickness =
                    GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                params->rgrc[0].top += borderThickness;
                params->rgrc[0].left += borderThickness;
                params->rgrc[0].right -= borderThickness;
                params->rgrc[0].bottom -= borderThickness;
            }
            return 0;
        }
        break;
    }

    case WM_NCHITTEST: {
        if (customTitleBar_) {
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            RECT winRect;
            GetWindowRect(hwnd_, &winRect);

            if (resizable_ && !IsZoomed(hwnd_)) {
                int border = static_cast<int>(6.0f * (dpiScale_ > 0.0f ? dpiScale_ : 1.0f));
                bool onLeft = (pt.x >= winRect.left && pt.x < winRect.left + border);
                bool onRight = (pt.x < winRect.right && pt.x >= winRect.right - border);
                bool onTop = (pt.y >= winRect.top && pt.y < winRect.top + border);
                bool onBottom = (pt.y < winRect.bottom && pt.y >= winRect.bottom - border);

                if (onTop && onLeft)
                    return HTTOPLEFT;
                if (onTop && onRight)
                    return HTTOPRIGHT;
                if (onBottom && onLeft)
                    return HTBOTTOMLEFT;
                if (onBottom && onRight)
                    return HTBOTTOMRIGHT;
                if (onLeft)
                    return HTLEFT;
                if (onRight)
                    return HTRIGHT;
                if (onTop)
                    return HTTOP;
                if (onBottom)
                    return HTBOTTOM;
            }

            if (hitTestCallback_) {
                core::Point logicalPos{
                    static_cast<float>(pt.x - winRect.left) / (dpiScale_ > 0.0f ? dpiScale_ : 1.0f),
                    static_cast<float>(pt.y - winRect.top) / (dpiScale_ > 0.0f ? dpiScale_ : 1.0f)};
                HitTestResult res = hitTestCallback_(logicalPos);
                switch (res) {
                case HitTestResult::Caption:
                    return HTCAPTION;
                case HitTestResult::MinimizeBtn:
                    return HTMINBUTTON;
                case HitTestResult::MaximizeBtn:
                    return HTMAXBUTTON;
                case HitTestResult::CloseBtn:
                    return HTCLOSE;
                case HitTestResult::Client:
                    return HTCLIENT;
                default:
                    break;
                }
            }
            return HTCLIENT;
        }
        break;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd_, &ps);
        FillRect(hdc, &ps.rcPaint, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        EndPaint(hwnd_, &ps);
        return 0;
    }

    case WM_GETMINMAXINFO: {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        if (mmi && hwnd_) {
            DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd_, GWL_STYLE));
            DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd_, GWL_EXSTYLE));

            if (minSize_.width > 0.0f || minSize_.height > 0.0f) {
                int minW = static_cast<int>(minSize_.width * dpiScale_);
                int minH = static_cast<int>(minSize_.height * dpiScale_);
                RECT minRect{0, 0, minW, minH};
                adjustWindowRectForDpi(&minRect, style, exStyle, dpi_);
                mmi->ptMinTrackSize.x = minRect.right - minRect.left;
                mmi->ptMinTrackSize.y = minRect.bottom - minRect.top;
            }

            if (maxSize_.width > 0.0f || maxSize_.height > 0.0f) {
                int maxW = static_cast<int>(maxSize_.width * dpiScale_);
                int maxH = static_cast<int>(maxSize_.height * dpiScale_);
                RECT maxRect{0, 0, maxW, maxH};
                adjustWindowRectForDpi(&maxRect, style, exStyle, dpi_);
                mmi->ptMaxSize.x = maxRect.right - maxRect.left;
                mmi->ptMaxSize.y = maxRect.bottom - maxRect.top;
                mmi->ptMaxTrackSize.x = maxRect.right - maxRect.left;
                mmi->ptMaxTrackSize.y = maxRect.bottom - maxRect.top;
            }
        }
        return 0;
    }

    case WM_ENTERSIZEMOVE: {
        isResizing_ = true;
        return 0;
    }

    case WM_EXITSIZEMOVE: {
        isResizing_ = false;
        InvalidateRect(hwnd_, nullptr, TRUE);
        UpdateWindow(hwnd_);
        if (owner_) {
            owner_->onResize.emit(ResizeEvent{logicalSize_, physicalSize_, false});
        }
        return 0;
    }

    case WM_SIZING: {
        auto* rect = reinterpret_cast<RECT*>(lParam);
        if (rect && hwnd_) {
            DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd_, GWL_STYLE));
            DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd_, GWL_EXSTYLE));
            RECT borderRect{0, 0, 0, 0};
            adjustWindowRectForDpi(&borderRect, style, exStyle, dpi_);

            int clientW = (rect->right - rect->left) - (borderRect.right - borderRect.left);
            int clientH = (rect->bottom - rect->top) - (borderRect.bottom - borderRect.top);
            if (clientW > 0 && clientH > 0) {
                physicalSize_ = {clientW, clientH};
                logicalSize_ = {static_cast<float>(clientW) / (dpiScale_ > 0.0f ? dpiScale_ : 1.0f),
                                static_cast<float>(clientH) /
                                    (dpiScale_ > 0.0f ? dpiScale_ : 1.0f)};
                InvalidateRect(hwnd_, nullptr, TRUE);
                UpdateWindow(hwnd_);
                if (owner_) {
                    owner_->onResize.emit(ResizeEvent{logicalSize_, physicalSize_, true});
                }
            }
        }
        return TRUE;
    }

    case WM_SIZE: {
        if (wParam != SIZE_MINIMIZED) {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            physicalSize_ = {width, height};
            logicalSize_ = {static_cast<float>(width) / (dpiScale_ > 0.0f ? dpiScale_ : 1.0f),
                            static_cast<float>(height) / (dpiScale_ > 0.0f ? dpiScale_ : 1.0f)};

            InvalidateRect(hwnd_, nullptr, TRUE);
            UpdateWindow(hwnd_);

            if (owner_) {
                owner_->onResize.emit(ResizeEvent{logicalSize_, physicalSize_, isResizing_});
            }
        }
        return 0;
    }

    case WM_DPICHANGED: {
        dpi_ = LOWORD(wParam);
        dpiScale_ = static_cast<float>(dpi_) / 96.0f;

        auto* suggestedRect = reinterpret_cast<RECT*>(lParam);
        if (suggestedRect) {
            SetWindowPos(hwnd_, nullptr, suggestedRect->left, suggestedRect->top,
                         suggestedRect->right - suggestedRect->left,
                         suggestedRect->bottom - suggestedRect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }

        if (owner_) {
            core::Rect suggested{static_cast<float>(suggestedRect->left),
                                 static_cast<float>(suggestedRect->top),
                                 static_cast<float>(suggestedRect->right - suggestedRect->left),
                                 static_cast<float>(suggestedRect->bottom - suggestedRect->top)};
            owner_->onDpiChanged.emit(DpiChangedEvent{dpiScale_, dpi_, suggested});
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int physX = GET_X_LPARAM(lParam);
        int physY = GET_Y_LPARAM(lParam);

        if (!mouseTracked_) {
            TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd_;
            TrackMouseEvent(&tme);
            mouseTracked_ = true;

            if (owner_) {
                MouseEvent enterEv{};
                enterEv.type = MouseEventType::Enter;
                enterEv.position = {static_cast<float>(physX) / dpiScale_,
                                    static_cast<float>(physY) / dpiScale_};
                enterEv.physicalPosition = {static_cast<float>(physX), static_cast<float>(physY)};
                enterEv.modifiers = queryWin32Modifiers();
                owner_->onMouseEvent.emit(enterEv);
            }
        }

        if (owner_) {
            MouseEvent ev{};
            ev.type = MouseEventType::Move;
            ev.position = {static_cast<float>(physX) / dpiScale_,
                           static_cast<float>(physY) / dpiScale_};
            ev.physicalPosition = {static_cast<float>(physX), static_cast<float>(physY)};
            ev.modifiers = queryWin32Modifiers();
            owner_->onMouseEvent.emit(ev);
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        mouseTracked_ = false;
        if (owner_) {
            MouseEvent ev{};
            ev.type = MouseEventType::Leave;
            ev.modifiers = queryWin32Modifiers();
            owner_->onMouseEvent.emit(ev);
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP: {
        int physX = GET_X_LPARAM(lParam);
        int physY = GET_Y_LPARAM(lParam);

        MouseEvent ev{};
        ev.position = {static_cast<float>(physX) / dpiScale_,
                       static_cast<float>(physY) / dpiScale_};
        ev.physicalPosition = {static_cast<float>(physX), static_cast<float>(physY)};
        ev.modifiers = queryWin32Modifiers();

        bool isDown = (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN ||
                       msg == WM_XBUTTONDOWN);
        ev.type = isDown ? MouseEventType::ButtonDown : MouseEventType::ButtonUp;

        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP) {
            ev.button = MouseButton::Left;
        } else if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP) {
            ev.button = MouseButton::Right;
        } else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP) {
            ev.button = MouseButton::Middle;
        } else {
            ev.button = (HIWORD(wParam) == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;
        }

        if (isDown) {
            SetCapture(hwnd_);
        } else {
            ReleaseCapture();
        }

        if (owner_) {
            owner_->onMouseEvent.emit(ev);
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(hwnd_, &pt);

        MouseEvent ev{};
        ev.type = MouseEventType::Wheel;
        ev.position = {static_cast<float>(pt.x) / dpiScale_, static_cast<float>(pt.y) / dpiScale_};
        ev.physicalPosition = {static_cast<float>(pt.x), static_cast<float>(pt.y)};
        ev.wheelDelta = {0.0f, static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) /
                                   static_cast<float>(WHEEL_DELTA)};
        ev.modifiers = queryWin32Modifiers();

        if (owner_) {
            owner_->onMouseEvent.emit(ev);
        }
        return 0;
    }

    case WM_MOUSEHWHEEL: {
        POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(hwnd_, &pt);

        MouseEvent ev{};
        ev.type = MouseEventType::Wheel;
        ev.position = {static_cast<float>(pt.x) / dpiScale_, static_cast<float>(pt.y) / dpiScale_};
        ev.physicalPosition = {static_cast<float>(pt.x), static_cast<float>(pt.y)};
        ev.wheelDelta = {static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) /
                             static_cast<float>(WHEEL_DELTA),
                         0.0f};
        ev.modifiers = queryWin32Modifiers();

        if (owner_) {
            owner_->onMouseEvent.emit(ev);
        }
        return 0;
    }

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        bool isDown = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
        bool isRepeat = isDown && ((lParam & 0x40000000) != 0);

        KeyEvent ev{};
        ev.key = win32VkToKey(static_cast<uint32_t>(wParam));
        ev.isDown = isDown;
        ev.isRepeat = isRepeat;
        ev.modifiers = queryWin32Modifiers();

        if (owner_) {
            owner_->onKeyEvent.emit(ev);
        }
        if (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) {
            return DefWindowProcW(hwnd_, msg, wParam, lParam);
        }
        return 0;
    }

    case WM_CHAR: {
        if (wParam >= 32 || wParam == 9 || wParam == 10 || wParam == 13) {
            KeyEvent ev{};
            ev.key = Key::Unknown;
            ev.isDown = true;
            ev.textCodepoint = static_cast<uint32_t>(wParam);
            ev.modifiers = queryWin32Modifiers();

            if (owner_) {
                owner_->onKeyEvent.emit(ev);
            }
        }
        return 0;
    }

    case WM_SETFOCUS:
    case WM_KILLFOCUS: {
        if (owner_) {
            owner_->onFocusChanged.emit(FocusChangedEvent{msg == WM_SETFOCUS});
        }
        return 0;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            updateCursor();
            return TRUE;
        }
        break;
    }

    case WM_DROPFILES: {
        HDROP hDrop = reinterpret_cast<HDROP>(wParam);
        if (hDrop) {
            UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
            std::vector<std::string> droppedPaths;
            droppedPaths.reserve(count);

            for (UINT i = 0; i < count; ++i) {
                UINT len = DragQueryFileW(hDrop, i, nullptr, 0);
                if (len > 0) {
                    std::wstring widePath(static_cast<size_t>(len + 1), L'\0');
                    DragQueryFileW(hDrop, i, widePath.data(), len + 1);
                    widePath.resize(len);

                    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, widePath.data(), -1, nullptr, 0,
                                                      nullptr, nullptr);
                    if (utf8Len > 1) {
                        std::string u8Path(static_cast<size_t>(utf8Len - 1), '\0');
                        WideCharToMultiByte(CP_UTF8, 0, widePath.data(), -1, u8Path.data(), utf8Len,
                                            nullptr, nullptr);
                        droppedPaths.push_back(std::move(u8Path));
                    }
                }
            }

            POINT pt{};
            DragQueryPoint(hDrop, &pt);
            DragFinish(hDrop);

            float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
            core::Point dropPos{static_cast<float>(pt.x) / scale, static_cast<float>(pt.y) / scale};

            if (owner_) {
                owner_->onFileDrop.emit(FileDropEvent{std::move(droppedPaths), dropPos});
            }
        }
        return 0;
    }

    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED: {
        BOOL useDarkMode = Application::isDarkMode() ? TRUE : FALSE;
        const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR = 20;
        const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 = 19;
        if (FAILED(DwmSetWindowAttribute(hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR, &useDarkMode,
                                         sizeof(useDarkMode)))) {
            DwmSetWindowAttribute(hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &useDarkMode,
                                  sizeof(useDarkMode));
        }

        ThemeChangedEvent ev{useDarkMode == TRUE};
        if (owner_) {
            owner_->onThemeChanged.emit(ev);
        }
        if (Application::instance()) {
            Application::instance()->onThemeChanged.emit(ev);
        }
        break;
    }

    case WM_CLOSE: {
        if (owner_) {
            owner_->onCloseRequested.emit();
        }
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
        return 0;
    }

    case WM_DESTROY: {
        return 0;
    }

    default:
        break;
    }

    return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

} // namespace northern::platform
