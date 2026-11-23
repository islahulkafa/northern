#include "platform/window.hpp"
#include "platform/win32/win32_window.hpp"

namespace northern::platform {

Window::Window(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

Window::~Window() = default;

Window::Window(Window&&) noexcept = default;

Window& Window::operator=(Window&&) noexcept = default;

core::Result<std::unique_ptr<Window>, core::Error> Window::create(const WindowDesc& desc) {
    // Allocate outer window first so inner impl can reference it
    std::unique_ptr<Window> window(new Window(nullptr));

    auto implResult = Window::Impl::create(window.get(), desc);
    if (implResult.isErr()) {
        return implResult.error();
    }

    window->impl_ = std::move(implResult.value());
    return window;
}

core::Point Window::position() const noexcept {
    return impl_ ? impl_->position() : core::Point{0.0f, 0.0f};
}

void Window::setPosition(core::Point logicalPos) {
    if (impl_) {
        impl_->setPosition(logicalPos);
    }
}

void Window::centerOnScreen(std::optional<Display> display) {
    if (impl_) {
        impl_->centerOnScreen(display);
    }
}

core::Size Window::size() const noexcept {
    return impl_ ? impl_->size() : core::Size{0.0f, 0.0f};
}

core::SizeI Window::physicalSize() const noexcept {
    return impl_ ? impl_->physicalSize() : core::SizeI{0, 0};
}

float Window::dpiScale() const noexcept {
    return impl_ ? impl_->dpiScale() : 1.0f;
}

uint32_t Window::dpi() const noexcept {
    return impl_ ? impl_->dpi() : 96;
}

void Window::setSize(core::Size logicalSize) {
    if (impl_) {
        impl_->setSize(logicalSize);
    }
}

core::Size Window::minSize() const noexcept {
    return impl_ ? impl_->minSize() : core::Size{0.0f, 0.0f};
}

void Window::setMinSize(core::Size minSize) {
    if (impl_) {
        impl_->setMinSize(minSize);
    }
}

core::Size Window::maxSize() const noexcept {
    return impl_ ? impl_->maxSize() : core::Size{0.0f, 0.0f};
}

void Window::setMaxSize(core::Size maxSize) {
    if (impl_) {
        impl_->setMaxSize(maxSize);
    }
}

bool Window::isResizing() const noexcept {
    return impl_ ? impl_->isResizing() : false;
}

std::string_view Window::title() const noexcept {
    return impl_ ? impl_->title() : std::string_view{};
}

void Window::setTitle(std::string_view title) {
    if (impl_) {
        impl_->setTitle(title);
    }
}

bool Window::isVisible() const noexcept {
    return impl_ ? impl_->isVisible() : false;
}

void Window::setVisible(bool visible) {
    if (impl_) {
        impl_->setVisible(visible);
    }
}

void Window::setCursorShape(CursorShape shape) {
    if (impl_) {
        impl_->setCursorShape(shape);
    }
}

void Window::requestClose() {
    if (impl_) {
        impl_->requestClose();
    }
}

bool Window::isCustomTitleBar() const noexcept {
    return impl_ ? impl_->isCustomTitleBar() : false;
}

void Window::setCustomTitleBar(bool custom) {
    if (impl_) {
        impl_->setCustomTitleBar(custom);
    }
}

bool Window::isMaximized() const noexcept {
    return impl_ ? impl_->isMaximized() : false;
}

bool Window::isMinimized() const noexcept {
    return impl_ ? impl_->isMinimized() : false;
}

void Window::minimize() {
    if (impl_) {
        impl_->minimize();
    }
}

void Window::maximize() {
    if (impl_) {
        impl_->maximize();
    }
}

void Window::restore() {
    if (impl_) {
        impl_->restore();
    }
}

void Window::toggleMaximize() {
    if (impl_) {
        impl_->toggleMaximize();
    }
}

bool Window::isFullscreen() const noexcept {
    return impl_ ? impl_->isFullscreen() : false;
}

void Window::setFullscreen(bool fullscreen) {
    if (impl_) {
        impl_->setFullscreen(fullscreen);
    }
}

void Window::toggleFullscreen() {
    if (impl_) {
        impl_->toggleFullscreen();
    }
}

void Window::setHitTestCallback(HitTestCallback cb) {
    if (impl_) {
        impl_->setHitTestCallback(std::move(cb));
    }
}

NativeWindowHandle Window::nativeHandle() const noexcept {
    return impl_ ? impl_->nativeHandle() : NativeWindowHandle{0};
}

} // namespace northern::platform
