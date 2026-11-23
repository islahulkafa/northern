#include "graphics/command_buffer.hpp"
#include "graphics/device.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/types.hpp"
#include "platform/application.hpp"
#include "platform/events.hpp"
#include "platform/key_codes.hpp"
#include "platform/window.hpp"
#include "platform/window_desc.hpp"
#include "renderer/batch_renderer.hpp"
#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/containers/tab_view.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace northern;
using namespace northern::platform;
using namespace northern::graphics;
using namespace northern::renderer;
using namespace northern::layout;
using namespace northern::widgets;

int main() {
    Application app;

    WindowDesc desc;
    desc.title = "Windowing";
    desc.size = {1024.0f, 768.0f};
    desc.minSize = {320.0f, 240.0f};
    desc.resizable = true;
    desc.visible = true;

    auto windowRes = Window::create(desc);
    if (windowRes.isErr()) {
        std::cerr << "Failed to create window: " << windowRes.error().message << std::endl;
        return 1;
    }
    auto window = std::move(windowRes.value());

    DeviceDesc devDesc;
    devDesc.appName = "Windowing";
    devDesc.enableValidation = true;

    auto devRes = Device::create(devDesc);
    if (devRes.isErr()) {
        std::cerr << "Failed to initialize Vulkan device: " << devRes.error().message << std::endl;
        return 1;
    }
    auto device = std::move(devRes.value());

    SwapchainDesc scDesc;
    scDesc.windowHandle = window->nativeHandle();
    scDesc.size = window->physicalSize();
    scDesc.format = Format::B8G8R8A8_Unorm;
    scDesc.vsync = true;

    auto scRes = device->createSwapchain(scDesc);
    if (scRes.isErr()) {
        std::cerr << "Failed to create swapchain: " << scRes.error().message << std::endl;
        return 1;
    }
    auto swapchain = std::move(scRes.value());

    std::vector<std::unique_ptr<CommandBuffer>> cmdBuffers;
    for (uint32_t i = 0; i < swapchain->maxFramesInFlight(); ++i) {
        auto cmdRes = device->createCommandBuffer();
        if (cmdRes.isErr()) {
            std::cerr << "Failed to create command buffer: " << cmdRes.error().message << std::endl;
            return 1;
        }
        cmdBuffers.push_back(std::move(cmdRes.value()));
    }

    auto rendRes = BatchRenderer::create(*device, Format::B8G8R8A8_Unorm);
    if (rendRes.isErr()) {
        std::cerr << "Failed to create batch renderer: " << rendRes.error().message << std::endl;
        return 1;
    }
    auto batchRenderer = std::move(rendRes.value());

    float dpi = window->dpiScale();
    auto& fontMgr = text::FontManager::instance();
    auto bodyFont = fontMgr.defaultFont(12.0f * dpi).valueOr(nullptr);

    WidgetContext uiContext;
    uiContext.setTheme(app.isDarkMode() ? theme::Theme::dark() : theme::Theme::light());
    uiContext.onCursorShapeChanged.connect(
        [&window](CursorShape shape) { window->setCursorShape(shape); });

    auto root = std::make_unique<Container>();
    root->setSurface(SurfaceRole::Canvas);
    root->style().width = Length::percent(100.0f);
    root->style().height = Length::percent(100.0f);

    auto tabView = std::make_unique<TabView>(bodyFont);
    tabView->setTabBarStyle(TabBarStyle::EditorCard);
    tabView->setReorderable(true);
    tabView->setAddButtonVisible(true);

    auto makePage = [](SurfaceRole surface) {
        auto page = std::make_unique<Container>();
        page->setSurface(surface);
        page->style().width = Length::percent(100.0f);
        page->style().height = Length::percent(100.0f);
        return page;
    };

    tabView->addTab("main.cpp", IconId::File, makePage(SurfaceRole::Canvas), true);
    tabView->addTab("tab_view.hpp", IconId::File, makePage(SurfaceRole::Default), true);
    tabView->addTab("Settings", IconId::Settings, makePage(SurfaceRole::Raised), true);
    tabView->setTabModified(0, true);

    int nextTabNum = 1;
    tabView->onTabAddRequested.connect([tv = tabView.get(), &nextTabNum, &makePage]() {
        int idx = tv->addTab("Untitled " + std::to_string(nextTabNum++), IconId::File,
                             makePage(SurfaceRole::Canvas), true);
        tv->setCurrentIndex(idx);
    });

    root->addChild(std::move(tabView));
    uiContext.setRootWidget(std::move(root));

    bool running = true;
    ClearColor clearColor{uiContext.theme().colors().surfaceCanvas.r,
                          uiContext.theme().colors().surfaceCanvas.g,
                          uiContext.theme().colors().surfaceCanvas.b, 1.0f};

    uiContext.onThemeChanged.connect([&clearColor](const theme::Theme& t) {
        clearColor = {t.colors().surfaceCanvas.r, t.colors().surfaceCanvas.g,
                      t.colors().surfaceCanvas.b, 1.0f};
    });

    window->onResize.connect([&swapchain, &uiContext](const ResizeEvent& ev) {
        if (ev.physicalSize.width > 0 && ev.physicalSize.height > 0) {
            auto res = swapchain->resize(ev.physicalSize);
            if (res.isErr()) {
                std::cerr << "[Resize Error] " << res.error().message << std::endl;
            }
            uiContext.handleResize(ev.size);
        }
    });

    window->onMouseEvent.connect([&uiContext](const MouseEvent& ev) {
        MouseEvent physEv = ev;
        physEv.position = ev.physicalPosition;
        if (ev.type == MouseEventType::Move) {
            uiContext.handleMouseMove(physEv);
        } else if (ev.type == MouseEventType::ButtonDown) {
            uiContext.handleMouseDown(physEv);
        } else if (ev.type == MouseEventType::ButtonUp) {
            uiContext.handleMouseUp(physEv);
        } else if (ev.type == MouseEventType::Wheel) {
            uiContext.handleMouseWheel(physEv);
        } else if (ev.type == MouseEventType::Leave) {
            uiContext.handleMouseLeave();
        }
    });

    window->onKeyEvent.connect([&](const KeyEvent& ev) {
        if (ev.isDown && ev.key == Key::Escape) {
            running = false;
            app.quit(0);
            return;
        }
        uiContext.handleKeyEvent(ev);
    });

    window->onCloseRequested.connect([&]() {
        running = false;
        app.quit(0);
    });

    window->onThemeChanged.connect([&uiContext](const ThemeChangedEvent& ev) {
        uiContext.setTheme(ev.isDarkMode ? theme::Theme::dark() : theme::Theme::light());
    });

    auto lastFrameTime = std::chrono::steady_clock::now();

    while (running) {
        if (!uiContext.hasActiveAnimations() && !uiContext.isPaintDirty()) {
            app.waitEvents(16);
            lastFrameTime = std::chrono::steady_clock::now();
        } else {
            app.pollEvents();
        }

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastFrameTime).count();
        dt = std::min(dt, 0.033f);
        lastFrameTime = now;
        uiContext.advanceAnimations(dt);

        if (swapchain->size().width <= 0 || swapchain->size().height <= 0) {
            continue;
        }

        auto acquireRes = swapchain->acquireNextImage();
        if (acquireRes.isErr()) {
            continue;
        }
        uint32_t imageIndex = acquireRes.value();
        uint32_t frameIndex = swapchain->currentFrameIndex();
        auto& cmdBuffer = cmdBuffers[frameIndex];

        cmdBuffer->begin();

        RenderingDesc renderDesc{};
        renderDesc.renderArea = {{0, 0}, swapchain->size()};
        renderDesc.colorAttachment.target = swapchain->currentTexture();
        renderDesc.colorAttachment.clear = true;
        renderDesc.colorAttachment.clearColor = clearColor;

        cmdBuffer->beginRendering(renderDesc);

        float winW = static_cast<float>(swapchain->size().width);
        float winH = static_cast<float>(swapchain->size().height);

        PaintContext ctx;
        uiContext.render(ctx, {winW, winH});

        batchRenderer->begin({winW, winH}, frameIndex);
        batchRenderer->render(ctx, *cmdBuffer);
        batchRenderer->end();

        cmdBuffer->endRendering();
        cmdBuffer->end();

        auto submitRes = device->submit(cmdBuffer.get(), swapchain.get());
        if (submitRes.isOk()) {
            auto presentRes = swapchain->present(imageIndex);
            if (presentRes.isErr()) {
                std::cerr << "[Present Error] " << presentRes.error().message << std::endl;
            }
        }

        device->advanceFrame();
    }

    device->waitIdle();
    return 0;
}
