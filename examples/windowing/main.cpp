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
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/containers/menu_bar.hpp"
#include "widgets/containers/tab_view.hpp"
#include "widgets/controls/icons.hpp"
#include "widgets/dialogs/title_bar.hpp"
#include "widgets/views/item_model.hpp"
#include "widgets/views/list_view.hpp"
#include "widgets/views/table_view.hpp"
#include "widgets/views/tree_view.hpp"

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
    desc.customTitleBar = true;
    desc.visible = true;

    auto windowRes = Window::create(desc);
    if (windowRes.isErr()) {
        std::cerr << "Failed to create window: " << windowRes.error().message << std::endl;
        return 1;
    }
    auto window = std::move(windowRes.value());
    window->centerOnScreen();

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

    WidgetContext uiContext;
    uiContext.setTheme(app.isDarkMode() ? theme::Theme::dark() : theme::Theme::light());
    uiContext.setTooltipDpiScale(dpi);
    uiContext.onCursorShapeChanged.connect(
        [&window](CursorShape shape) { window->setCursorShape(shape); });

    auto root = std::make_unique<Container>();
    root->setSurface(SurfaceRole::Canvas);
    root->style().width = Length::percent(100.0f);
    root->style().height = Length::percent(100.0f);
    root->setFlexDirection(FlexDirection::Column);
    root->setJustifyContent(layout::JustifyContent::FlexStart);
    root->setAlignItems(layout::AlignItems::Stretch);

    bool running = true;

    auto titleBar =
        std::make_unique<TitleBar>("Windowing", nullptr, window.get(), TitleBarSize::Standard);
    titleBar->setDpiScale(dpi);
    TitleBar* titleBarPtr = static_cast<TitleBar*>(root->addChild(std::move(titleBar)));

    window->setHitTestCallback([titleBarPtr, dpi](core::Point logicalPos) {
        return titleBarPtr->hitTestPoint(core::Point{logicalPos.x * dpi, logicalPos.y * dpi});
    });

    // Top-level MenuBar
    auto menuBar = std::make_unique<MenuBar>();
    menuBar->setDpiScale(dpi);

    auto* fileMenu = menuBar->addMenu("File");
    fileMenu->addItem(
        IconId::Folder, "New File", []() { std::cout << "File -> New File\n"; }, "Ctrl+N");
    fileMenu->addItem(
        IconId::Folder, "Open File...", []() { std::cout << "File -> Open File\n"; }, "Ctrl+O");
    fileMenu->addItem(IconId::Save, "Save", []() { std::cout << "File -> Save\n"; }, "Ctrl+S");
    fileMenu->addSeparator();
    fileMenu->addItem(IconId::Close, "Exit", [&running]() { running = false; }, "Alt+F4");

    auto* editMenu = menuBar->addMenu("Edit");
    editMenu->addItem(IconId::Refresh, "Undo", []() { std::cout << "Edit -> Undo\n"; }, "Ctrl+Z");
    editMenu->addItem(IconId::Refresh, "Redo", []() { std::cout << "Edit -> Redo\n"; }, "Ctrl+Y");
    editMenu->addSeparator();
    editMenu->addItem(IconId::Edit, "Cut", []() { std::cout << "Edit -> Cut\n"; }, "Ctrl+X");
    editMenu->addItem(IconId::Copy, "Copy", []() { std::cout << "Edit -> Copy\n"; }, "Ctrl+C");
    editMenu->addItem(IconId::Folder, "Paste", []() { std::cout << "Edit -> Paste\n"; }, "Ctrl+V");

    auto* viewMenu = menuBar->addMenu("View");
    viewMenu->addCheckItem("Status Bar", true, [](bool checked) {
        std::cout << "View -> Status Bar: " << (checked ? "ON" : "OFF") << "\n";
    });
    viewMenu->addSubmenu(IconId::Search, "Zoom", []() {
        auto sub = std::make_unique<ContextMenu>();
        sub->addItem("Zoom In (125%)", []() { std::cout << "Zoom In\n"; }, "Ctrl++");
        sub->addItem("Zoom Out (75%)", []() { std::cout << "Zoom Out\n"; }, "Ctrl+-");
        sub->addItem("Reset Zoom (100%)", []() { std::cout << "Zoom 100%\n"; }, "Ctrl+0");
        return sub;
    });
    viewMenu->addSeparator();
    viewMenu->addItem(
        IconId::Settings, "Toggle Dark Mode",
        [&uiContext]() {
            bool dark = uiContext.theme().isDark();
            uiContext.setTheme(dark ? theme::Theme::light() : theme::Theme::dark());
        },
        "Ctrl+D");

    auto* helpMenu = menuBar->addMenu("Help");
    helpMenu->addItem(
        IconId::HelpCircle, "Documentation", []() { std::cout << "Help -> Docs\n"; }, "F1");
    helpMenu->addItem(IconId::DialogInfo, "About Northern",
                      []() { std::cout << "Northern UI Framework v1.0\n"; });

    menuBar->registerAccelerators(uiContext);
    root->addChild(std::move(menuBar));

    // View models for the tab views
    StandardTableModel tableModel(
        std::vector<std::string>{"Service", "Status", "Memory", "Latency"});
    tableModel.appendRow({"Vulkan Graphics Pipeline", "Active", "128.4 MB", "1.2 ms"});
    tableModel.appendRow({"Text Shaping (HarfBuzz)", "Active", "24.6 MB", "0.4 ms"});
    tableModel.appendRow({"Display List Compositor", "Running", "16.2 MB", "0.8 ms"});
    tableModel.appendRow({"Worker ThreadPool", "Idle", "8.1 MB", "0.0 ms"});
    tableModel.appendRow({"TaskQueue Dispatcher", "Active", "4.0 MB", "0.1 ms"});
    tableModel.appendRow({"Asset Cache & Staging", "Idle", "64.0 MB", "0.0 ms"});
    tableModel.appendRow({"Win32 Platform Loop", "Active", "12.8 MB", "0.3 ms"});
    tableModel.appendRow({"Arena Memory Allocator", "Active", "32.0 MB", "0.0 ms"});

    for (int r = 0; r < tableModel.rowCount(); ++r) {
        std::string status = tableModel.data(tableModel.index(r, 1));
        tableModel.setData(tableModel.index(r, 1), status, ItemDataRole::BadgeText);
    }

    auto tableView = std::make_unique<TableView>(&tableModel);
    tableView->setDpiScale(dpi);
    tableView->setAlternatingRowColors(true);
    tableView->setShowGridLines(true);
    tableView->horizontalHeader()->setSectionWidth(0, 240.0f * dpi);
    tableView->horizontalHeader()->setSectionWidth(1, 130.0f * dpi);
    tableView->horizontalHeader()->setSectionWidth(2, 130.0f * dpi);
    tableView->horizontalHeader()->setSectionWidth(3, 130.0f * dpi);

    StandardTreeModel treeModel(std::vector<std::string>{"Project Structure", "Type", "Size"});
    auto* rootTree = treeModel.rootItem();

    auto srcDir =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"src", "Directory", ""});
    auto coreDir = srcDir->appendChild(
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"core", "Directory", ""}));
    coreDir->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"arena.hpp", "Header", "8.4 KB"}));
    coreDir->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"thread_pool.hpp", "Header", "12.1 KB"}));

    auto widgetsDir = srcDir->appendChild(
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"widgets", "Directory", ""}));
    widgetsDir->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"table_view.cpp", "Source", "24.5 KB"}));
    widgetsDir->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"tree_view.cpp", "Source", "22.3 KB"}));
    widgetsDir->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"list_view.cpp", "Source", "18.7 KB"}));

    rootTree->appendChild(std::move(srcDir));

    auto docsDir =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"docs", "Directory", ""});
    docsDir->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"design-languages.md", "Markdown", "14.2 KB"}));
    rootTree->appendChild(std::move(docsDir));

    auto treeView = std::make_unique<TreeView>(&treeModel);
    treeView->setDpiScale(dpi);
    treeView->expandAll();
    treeView->header()->setSectionWidth(0, 280.0f * dpi);
    treeView->header()->setSectionWidth(1, 140.0f * dpi);
    treeView->header()->setSectionWidth(2, 140.0f * dpi);

    StringListModel listModel;
    listModel.append("Hardware Graphics Acceleration (Vulkan 1.3)");
    listModel.append("Retained Display List Compositor");
    listModel.append("Bidirectional Zero-Heap Virtualized Scrolling");
    listModel.append("HarfBuzz Text Shaping & FreeType Rasterization");
    listModel.append("Optical Zinc Elevation Palette System");
    listModel.append("Keyboard Accelerators & Cascading Menus");
    listModel.append("Parent-Owned Retained Widget Lifecycle");
    listModel.append("Asynchronous Timeline Semaphore Resource Staging");

    auto listView = std::make_unique<ListView>(&listModel);
    listView->setDpiScale(dpi);

    // TabView container switching between views
    auto tabView = std::make_unique<TabView>();
    tabView->setDpiScale(dpi);
    tabView->setTabBarStyle(TabBarStyle::Underline);
    tabView->tabBar()->setTabHeight(34.0f * dpi);
    tabView->style().width = Length::percent(100.0f);
    tabView->style().flexGrow = 1.0f;
    tabView->style().flexShrink = 1.0f;
    tabView->style().flexBasis = layout::Length::points(0.0f);

    tabView->addTab("Table View", IconId::Grid, std::move(tableView));
    tabView->addTab("Tree View", IconId::Folder, std::move(treeView));
    tabView->addTab("List View", IconId::List, std::move(listView));

    root->addChild(std::move(tabView));

    uiContext.setRootWidget(std::move(root));
    uiContext.handleResize(window->size());

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
