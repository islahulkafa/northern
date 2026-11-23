#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "core/math/geometry.hpp"
#include "platform/display.hpp"
#include "platform/events.hpp"
#include "platform/native_handle.hpp"
#include "platform/window.hpp"
#include "platform/window_desc.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <memory>
#include <optional>
#include <string>
#include <windows.h>

namespace northern::platform {

class Window::Impl {
public:
    explicit Impl(Window* owner) noexcept;
    ~Impl();

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    static core::Result<std::unique_ptr<Window::Impl>, core::Error> create(Window* owner,
                                                                           const WindowDesc& desc);

    [[nodiscard]] core::Point position() const noexcept;
    void setPosition(core::Point logicalPos);
    void centerOnScreen(std::optional<Display> display);

    [[nodiscard]] core::Size size() const noexcept { return logicalSize_; }
    [[nodiscard]] core::SizeI physicalSize() const noexcept { return physicalSize_; }
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }
    [[nodiscard]] uint32_t dpi() const noexcept { return dpi_; }
    void setSize(core::Size logicalSize);

    [[nodiscard]] core::Size minSize() const noexcept { return minSize_; }
    void setMinSize(core::Size minSize);
    [[nodiscard]] core::Size maxSize() const noexcept { return maxSize_; }
    void setMaxSize(core::Size maxSize);
    [[nodiscard]] bool isResizing() const noexcept { return isResizing_; }

    [[nodiscard]] std::string_view title() const noexcept { return title_; }
    void setTitle(std::string_view title);
    [[nodiscard]] bool isVisible() const noexcept;
    void setVisible(bool visible);
    void setCursorShape(CursorShape shape);
    void requestClose();

    [[nodiscard]] bool isCustomTitleBar() const noexcept { return customTitleBar_; }
    void setCustomTitleBar(bool custom);
    [[nodiscard]] bool isMaximized() const noexcept;
    [[nodiscard]] bool isMinimized() const noexcept;
    [[nodiscard]] bool isFullscreen() const noexcept { return isFullscreen_; }
    void minimize();
    void maximize();
    void restore();
    void toggleMaximize();
    void setFullscreen(bool fullscreen);
    void toggleFullscreen();

    void setHitTestCallback(HitTestCallback cb) { hitTestCallback_ = std::move(cb); }

    [[nodiscard]] NativeWindowHandle nativeHandle() const noexcept {
        return NativeWindowHandle{reinterpret_cast<uintptr_t>(hwnd_)};
    }

    static LRESULT CALLBACK s_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam);

    void updateDpi();
    void updateCursor();
    [[nodiscard]] static HCURSOR win32CursorFromShape(CursorShape shape) noexcept;
    [[nodiscard]] static const wchar_t* windowClassName() noexcept;

    Window* owner_{nullptr};
    HWND hwnd_{nullptr};
    std::string title_;
    core::Size logicalSize_{800.0f, 600.0f};
    core::SizeI physicalSize_{800, 600};
    core::Size minSize_{0.0f, 0.0f};
    core::Size maxSize_{0.0f, 0.0f};
    bool isResizing_{false};
    float dpiScale_{1.0f};
    uint32_t dpi_{96};
    bool mouseTracked_{false};
    CursorShape cursorShape_{CursorShape::Arrow};
    HCURSOR hCurrentCursor_{nullptr};
    bool customTitleBar_{false};
    bool resizable_{true};
    bool isFullscreen_{false};
    WINDOWPLACEMENT savedPlacement_{};
    LONG savedStyle_{0};
    HitTestCallback hitTestCallback_{};
};

} // namespace northern::platform
