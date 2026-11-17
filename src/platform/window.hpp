#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "core/math/geometry.hpp"
#include "core/signal/signal.hpp"
#include "platform/events.hpp"
#include "platform/mouse_types.hpp"
#include "platform/native_handle.hpp"
#include "platform/window_desc.hpp"
#include <memory>
#include <string_view>

namespace northern::platform {

class Window {
public:
    static core::Result<std::unique_ptr<Window>, core::Error> create(const WindowDesc& desc = {});

    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept;
    Window& operator=(Window&&) noexcept;

    // Geometry and DPI
    [[nodiscard]] core::Size size() const noexcept;
    [[nodiscard]] core::SizeI physicalSize() const noexcept;
    [[nodiscard]] float dpiScale() const noexcept;
    [[nodiscard]] uint32_t dpi() const noexcept;
    void setSize(core::Size logicalSize);

    // Sizing constraints and resize state
    [[nodiscard]] core::Size minSize() const noexcept;
    void setMinSize(core::Size minSize);
    [[nodiscard]] core::Size maxSize() const noexcept;
    void setMaxSize(core::Size maxSize);
    [[nodiscard]] bool isResizing() const noexcept;

    // State
    [[nodiscard]] std::string_view title() const noexcept;
    void setTitle(std::string_view title);
    [[nodiscard]] bool isVisible() const noexcept;
    void setVisible(bool visible);
    void setCursorShape(CursorShape shape);
    void requestClose();

    // Native handle for GAL swapchain
    [[nodiscard]] NativeWindowHandle nativeHandle() const noexcept;

    // Event signals
    core::Signal<const MouseEvent&> onMouseEvent;
    core::Signal<const KeyEvent&> onKeyEvent;
    core::Signal<const ResizeEvent&> onResize;
    core::Signal<const DpiChangedEvent&> onDpiChanged;
    core::Signal<const FocusChangedEvent&> onFocusChanged;
    core::Signal<> onCloseRequested;

    class Impl;

private:
    explicit Window(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;
};

} // namespace northern::platform
