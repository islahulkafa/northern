#include "platform/application.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <objbase.h>
#include <windows.h>

namespace northern::platform {

namespace {
Application* s_instance = nullptr;
}

class Application::Impl {
public:
    Impl() {
        // Enable Per-Monitor V2 DPI Awareness for crisp high-DPI scaling
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (user32) {
            using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
            auto setDpiContext = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
                GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
            if (setDpiContext) {
                setDpiContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            } else {
                SetProcessDPIAware();
            }
        }

        // Initialize COM for clipboard and shell integration
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    }

    ~Impl() { CoUninitialize(); }

    int run() {
        MSG msg{};
        while (!quitRequested_) {
            BOOL ret = GetMessageW(&msg, nullptr, 0, 0);
            if (ret == 0) {
                // WM_QUIT received
                exitCode_ = static_cast<int>(msg.wParam);
                break;
            }
            if (ret == -1) {
                // Error in message pump
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return exitCode_;
    }

    void quit(int exitCode) {
        quitRequested_ = true;
        exitCode_ = exitCode;
        PostQuitMessage(exitCode);
    }

    void pollEvents() {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                quitRequested_ = true;
                exitCode_ = static_cast<int>(msg.wParam);
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void waitEvents(uint32_t timeoutMs) {
        DWORD waitTime = (timeoutMs == 0) ? INFINITE : static_cast<DWORD>(timeoutMs);
        MsgWaitForMultipleObjectsEx(0, nullptr, waitTime, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        pollEvents();
    }

private:
    bool quitRequested_{false};
    int exitCode_{0};
};

Application::Application() : impl_(std::make_unique<Impl>()) {
    s_instance = this;
}

Application::~Application() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

Application::Application(Application&&) noexcept = default;

Application& Application::operator=(Application&&) noexcept = default;

int Application::run() {
    return impl_ ? impl_->run() : -1;
}

void Application::quit(int exitCode) {
    if (impl_) {
        impl_->quit(exitCode);
    }
}

void Application::pollEvents() {
    if (impl_) {
        impl_->pollEvents();
    }
}

void Application::waitEvents(uint32_t timeoutMs) {
    if (impl_) {
        impl_->waitEvents(timeoutMs);
    }
}

Application* Application::instance() noexcept {
    return s_instance;
}

bool Application::isDarkMode() noexcept {
    HKEY hKey{};
    LSTATUS status = RegOpenKeyExW(
        HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0,
        KEY_READ, &hKey);
    if (status == ERROR_SUCCESS) {
        DWORD appsUseLightTheme = 1;
        DWORD dataSize = sizeof(DWORD);
        status = RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr,
                                  reinterpret_cast<LPBYTE>(&appsUseLightTheme), &dataSize);
        RegCloseKey(hKey);
        if (status == ERROR_SUCCESS) {
            return appsUseLightTheme == 0;
        }
    }
    return true; // Default to dark mode per Northern design specifications
}

} // namespace northern::platform
