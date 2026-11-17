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
#include "widgets/button.hpp"
#include "widgets/checkbox.hpp"
#include "widgets/container.hpp"
#include "widgets/label.hpp"
#include "widgets/scroll_view.hpp"
#include "widgets/slider.hpp"
#include "widgets/text_input.hpp"
#include "widgets/theme.hpp"
#include "widgets/widget_context.hpp"

#include <algorithm>
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
    devDesc.appName = "Northern Controls Demo";
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
    auto titleFont = fontMgr.defaultFont(13.5f * dpi).valueOr(nullptr);
    auto bodyFont = fontMgr.defaultFont(11.0f * dpi).valueOr(nullptr);

    // Build unslop declarative retained widget tree centered in the window
    WidgetContext uiContext;

    auto root = std::make_unique<Container>();
    root->style().flexDirection = FlexDirection::Column;
    root->style().justifyContent = JustifyContent::Center;
    root->style().alignItems = AlignItems::Center;
    root->style().width = Length::percent(100.0f);
    root->style().height = Length::percent(100.0f);

    // Centered workstation card using optical zinc tokens
    auto card = std::make_unique<Container>();
    card->style().flexDirection = FlexDirection::Column;
    card->style().width = Length::points(420.0f * dpi);
    card->style().padding = RectLength::all(Length::points(20.0f * dpi));
    card->style().rowGap = 14.0f * dpi;
    card->setBackgroundColor(theme::ColorTokens::SurfaceDefault);
    card->setBorderColor(theme::ColorTokens::BorderSubtle);
    card->setBorderWidth(1.0f * dpi);
    card->setCornerRadius(theme::Metrics::CornerRadiusCard * dpi);

    // 1. Header Label
    auto header = std::make_unique<Label>("Windowing", titleFont ? titleFont : bodyFont);
    header->setTextColor(theme::ColorTokens::TextPrimary);
    card->addChild(std::move(header));

    // 2. TextInput (32px standard row)
    auto textInput = std::make_unique<TextInput>("Search or enter command...", bodyFont);
    textInput->style().width = Length::percent(100.0f);
    textInput->style().height = Length::points(theme::Metrics::ControlHeightStandard * dpi);
    card->addChild(std::move(textInput));

    // 3. Slider (28px row)
    auto slider = std::make_unique<Slider>(0.0f, 100.0f, 40.0f);
    slider->style().width = Length::percent(100.0f);
    slider->style().height = Length::points(28.0f * dpi);
    card->addChild(std::move(slider));

    // 4. Checkbox (24px row)
    auto checkbox = std::make_unique<Checkbox>("Enable Hardware Acceleration", true, bodyFont);
    checkbox->style().width = Length::percent(100.0f);
    checkbox->style().height = Length::points(24.0f * dpi);
    card->addChild(std::move(checkbox));

    // 5. Button Actions Row (Primary + Secondary)
    auto btnRow = std::make_unique<Container>();
    btnRow->style().flexDirection = FlexDirection::Row;
    btnRow->style().justifyContent = JustifyContent::FlexEnd;
    btnRow->style().columnGap = 8.0f * dpi;
    btnRow->style().width = Length::percent(100.0f);
    btnRow->style().height = Length::points(theme::Metrics::ControlHeightStandard * dpi);

    auto cancelBtn = std::make_unique<Button>("Cancel", bodyFont, ButtonVariant::Secondary);
    cancelBtn->style().height = Length::points(theme::Metrics::ControlHeightStandard * dpi);

    auto applyBtn = std::make_unique<Button>("Apply", bodyFont, ButtonVariant::Primary);
    applyBtn->style().height = Length::points(theme::Metrics::ControlHeightStandard * dpi);

    btnRow->addChild(std::move(cancelBtn));
    btnRow->addChild(std::move(applyBtn));
    card->addChild(std::move(btnRow));

    // 6. ScrollView with quiet optical zinc list items
    auto scrollView = std::make_unique<ScrollView>();
    scrollView->style().width = Length::percent(100.0f);
    scrollView->style().height = Length::points(160.0f * dpi);
    scrollView->setBackgroundColor(theme::ColorTokens::SurfaceBase);

    for (int i = 1; i <= 6; ++i) {
        auto item = std::make_unique<Container>();
        item->style().width = Length::percent(100.0f);
        item->style().height = Length::points(28.0f * dpi);
        item->style().justifyContent = JustifyContent::Center;
        item->style().margin = RectLength{
            .left = Length::points(6.0f * dpi),
            .top = Length::points(2.0f * dpi),
            .right = Length::points(12.0f * dpi),
            .bottom = Length::points(2.0f * dpi),
        };
        item->style().padding = RectLength{
            .left = Length::points(10.0f * dpi),
            .top = Length::points(0.0f),
            .right = Length::points(10.0f * dpi),
            .bottom = Length::points(0.0f),
        };
        item->setBackgroundColor(theme::ColorTokens::SurfaceRaised);
        item->setBorderColor(theme::ColorTokens::BorderSubtle);
        item->setBorderWidth(1.0f * dpi);
        item->setCornerRadius(theme::Metrics::CornerRadiusControl * dpi);

        auto itemLabel = std::make_unique<Label>("Task item #" + std::to_string(i), bodyFont);
        itemLabel->style().height = Length::percent(100.0f);
        itemLabel->setTextColor(theme::ColorTokens::TextPrimary);
        item->addChild(std::move(itemLabel));

        scrollView->addChild(std::move(item));
    }
    card->addChild(std::move(scrollView));

    root->addChild(std::move(card));
    uiContext.setRootWidget(std::move(root));

    bool running = true;
    ClearColor clearColor{theme::ColorTokens::SurfaceCanvas.r, theme::ColorTokens::SurfaceCanvas.g,
                          theme::ColorTokens::SurfaceCanvas.b, 1.0f};

    window->onResize.connect([&swapchain](const ResizeEvent& ev) {
        if (ev.physicalSize.width > 0 && ev.physicalSize.height > 0) {
            auto res = swapchain->resize(ev.physicalSize);
            if (res.isErr()) {
                std::cerr << "[Resize Error] " << res.error().message << std::endl;
            }
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

    while (running) {
        app.pollEvents();

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
