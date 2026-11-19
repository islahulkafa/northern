#include "graphics/command_buffer.hpp"
#include "graphics/device.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/types.hpp"
#include "platform/application.hpp"
#include "platform/events.hpp"
#include "platform/file_dialog.hpp"
#include "platform/key_codes.hpp"
#include "platform/window.hpp"
#include "platform/window_desc.hpp"
#include "renderer/batch_renderer.hpp"
#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "widgets/accordion.hpp"
#include "widgets/button.hpp"
#include "widgets/checkbox.hpp"
#include "widgets/container.hpp"
#include "widgets/dialog.hpp"
#include "widgets/divider.hpp"
#include "widgets/header_view.hpp"
#include "widgets/icon.hpp"
#include "widgets/icons.hpp"
#include "widgets/item_delegate.hpp"
#include "widgets/item_model.hpp"
#include "widgets/item_selection_model.hpp"
#include "widgets/label.hpp"
#include "widgets/list_view.hpp"
#include "widgets/message_box.hpp"
#include "widgets/radio_button.hpp"
#include "widgets/radio_group.hpp"
#include "widgets/scroll_view.hpp"
#include "widgets/slider.hpp"
#include "widgets/split_view.hpp"
#include "widgets/switch.hpp"
#include "widgets/tab_view.hpp"
#include "widgets/table_view.hpp"
#include "widgets/text_input.hpp"
#include "widgets/theme.hpp"
#include "widgets/title_bar.hpp"
#include "widgets/tree_view.hpp"
#include "widgets/widget_context.hpp"

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
    desc.size = {1280.0f, 820.0f};
    desc.minSize = {720.0f, 480.0f};
    desc.resizable = true;
    desc.visible = true;
    desc.customTitleBar = true;

    auto windowRes = Window::create(desc);
    if (windowRes.isErr()) {
        std::cerr << "Failed to create window: " << windowRes.error().message << std::endl;
        return 1;
    }
    auto window = std::move(windowRes.value());

    DeviceDesc devDesc;
    devDesc.appName = "Northern Workbench";
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
    auto titleFont = fontMgr.defaultFont(13.0f * dpi).valueOr(nullptr);
    auto bodyFont = fontMgr.defaultFont(11.0f * dpi).valueOr(nullptr);
    auto captionFont = fontMgr.defaultFont(10.0f * dpi).valueOr(nullptr);

    WidgetContext uiContext;
    uiContext.setTooltipFont(captionFont ? captionFont : bodyFont);
    uiContext.onCursorShapeChanged.connect(
        [&window](CursorShape shape) { window->setCursorShape(shape); });

    auto root = std::make_unique<Container>();
    root->style().flexDirection = FlexDirection::Column;
    root->style().width = Length::percent(100.0f);
    root->style().height = Length::percent(100.0f);
    root->setBackgroundColor(theme::ColorTokens::SurfaceCanvas);

    auto titleBar = std::make_unique<TitleBar>("Northern Workbench", bodyFont, window.get());
    titleBar->style().height = Length::points(36.0f * dpi);

    auto searchBox = std::make_unique<TextInput>("Filter collections...", bodyFont);
    searchBox->style().width = Length::points(240.0f * dpi);
    searchBox->style().height = Length::points(24.0f * dpi);
    searchBox->setTooltip("Filter visible items", "Ctrl+F");
    titleBar->setCenterWidget(std::move(searchBox));

    root->addChild(std::move(titleBar));

    auto splitView = std::make_unique<SplitView>(SplitOrientation::Horizontal, 0.25f);
    splitView->style().flexGrow = 1.0f;
    splitView->style().width = Length::percent(100.0f);
    splitView->setMinPaneSize(180.0f * dpi);
    splitView->setSashThickness(5.0f * dpi);

    auto sidebar = std::make_unique<Container>();
    sidebar->style().flexDirection = FlexDirection::Column;
    sidebar->style().width = Length::percent(100.0f);
    sidebar->style().height = Length::percent(100.0f);
    sidebar->setBackgroundColor(theme::ColorTokens::SurfaceDefault);

    auto sidebarScroll = std::make_unique<ScrollView>();
    sidebarScroll->style().flexGrow = 1.0f;
    sidebarScroll->style().width = Length::percent(100.0f);
    sidebarScroll->style().padding = RectLength::all(Length::points(10.0f * dpi));

    auto accordion = std::make_unique<Accordion>(false);
    accordion->style().rowGap = 6.0f * dpi;

    auto sec1 = std::make_unique<CollapsibleSection>("Collection Options", true, bodyFont);
    auto sec1Content = std::make_unique<Container>();
    sec1Content->style().flexDirection = FlexDirection::Column;
    sec1Content->style().padding = RectLength::all(Length::points(10.0f * dpi));
    sec1Content->style().rowGap = 10.0f * dpi;

    auto cbZebra = std::make_unique<Checkbox>("Alternating Rows", true, bodyFont);
    auto cbGrid = std::make_unique<Checkbox>("Show Grid Lines", true, bodyFont);
    auto swStretch = std::make_unique<Switch>("Stretch Last Column", true, bodyFont);
    Checkbox* cbZebraPtr = sec1Content->addChild(std::move(cbZebra));
    Checkbox* cbGridPtr = sec1Content->addChild(std::move(cbGrid));
    Switch* swStretchPtr = sec1Content->addChild(std::move(swStretch));

    sec1->setContent(std::move(sec1Content));
    accordion->addSection(std::move(sec1));

    auto sec2 = std::make_unique<CollapsibleSection>("Virtualization Specs", true, bodyFont);
    auto sec2Content = std::make_unique<Container>();
    sec2Content->style().flexDirection = FlexDirection::Column;
    sec2Content->style().padding = RectLength::all(Length::points(10.0f * dpi));
    sec2Content->style().rowGap = 8.0f * dpi;

    auto lblRows =
        std::make_unique<Label>("Total Items: 10,000+", captionFont ? captionFont : bodyFont);
    lblRows->setTextColor(theme::ColorTokens::TextSecondary);
    sec2Content->addChild(std::move(lblRows));

    auto lblComplexity = std::make_unique<Label>("Visible Range: O(1) Slicing",
                                                 captionFont ? captionFont : bodyFont);
    lblComplexity->setTextColor(theme::ColorTokens::TextTertiary);
    sec2Content->addChild(std::move(lblComplexity));

    auto lblAlloc = std::make_unique<Label>("Memory: Zero Widget Overhead",
                                            captionFont ? captionFont : bodyFont);
    lblAlloc->setTextColor(theme::ColorTokens::TextTertiary);
    sec2Content->addChild(std::move(lblAlloc));

    sec2->setContent(std::move(sec2Content));
    accordion->addSection(std::move(sec2));

    auto sec3 = std::make_unique<CollapsibleSection>("Shell & Dialogs", true, bodyFont);
    auto sec3Content = std::make_unique<Container>();
    sec3Content->style().flexDirection = FlexDirection::Column;
    sec3Content->style().padding = RectLength::all(Length::points(10.0f * dpi));
    sec3Content->style().rowGap = 8.0f * dpi;

    auto btnConfirm = std::make_unique<Button>("Show Confirm Dialog", bodyFont);
    btnConfirm->onClicked.connect([&uiContext, bodyFont]() {
        MessageBox::showConfirm(
            uiContext, "Unsaved Changes",
            "Are you sure you want to discard your unsaved workbench modifications?", bodyFont,
            [&uiContext, bodyFont](bool confirmed) {
                if (confirmed) {
                    MessageBox::showInformation(uiContext, "Action Completed",
                                                "Modifications were successfully discarded.",
                                                bodyFont);
                }
            });
    });
    sec3Content->addChild(std::move(btnConfirm));

    auto btnFile = std::make_unique<Button>("Open File Dialog...", bodyFont);
    btnFile->onClicked.connect([&window, &uiContext, bodyFont]() {
        FileDialogDesc desc;
        desc.title = "Select Source File";
        desc.filters = {{"Supported Files (*.cpp;*.hpp;*.json)", "*.cpp;*.hpp;*.json"},
                        {"All Files (*.*)", "*.*"}};
        auto res = FileDialog::openFile(window.get(), desc);
        if (res.isOk() && !res.value().empty()) {
            MessageBox::showInformation(uiContext, "File Selected", "Selected:\n" + res.value(),
                                        bodyFont);
        }
    });
    sec3Content->addChild(std::move(btnFile));

    sec3->setContent(std::move(sec3Content));
    accordion->addSection(std::move(sec3));

    sidebarScroll->addChild(std::move(accordion));
    sidebar->addChild(std::move(sidebarScroll));

    auto tabView = std::make_unique<TabView>(bodyFont);
    tabView->style().flexGrow = 1.0f;
    tabView->style().width = Length::percent(100.0f);
    tabView->style().height = Length::percent(100.0f);

    auto tableModel = std::make_shared<StandardTableModel>(
        std::vector<std::string>{"PID", "Subsystem / Module", "Status", "CPU", "Memory"},
        std::vector<std::vector<std::string>>{
            {"1001", "Vulkan Graphics GAL", "Active", "4.8%", "124 MB"},
            {"1002", "BatchRenderer 2D Pipeline", "Running", "1.2%", "48 MB"},
            {"1003", "HarfBuzz Text Shaper", "Idle", "0.1%", "32 MB"},
            {"1004", "FreeType Glyph Atlas", "Cached", "0.0%", "16 MB"},
            {"1005", "FlexSolver Engine", "Solved", "0.4%", "8 MB"},
            {"1006", "Win32 Message Loop", "Polling", "0.2%", "12 MB"},
            {"1007", "Worker Thread Pool", "4 Workers", "2.1%", "24 MB"},
            {"1008", "Progressive Compositor", "Standby", "0.0%", "18 MB"},
            {"1009", "GPU Handle Recycler", "Healthy", "0.0%", "4 MB"},
            {"1010", "Animation Manager", "Ticking", "0.5%", "6 MB"},
            {"1011", "Overlay Layer Stack", "Ready", "0.0%", "2 MB"},
            {"1012", "DevTools Telemetry Sink", "Sampling", "0.3%", "14 MB"},
        });

    auto tableView = std::make_unique<TableView>(tableModel.get(), bodyFont);
    tableView->setRowHeight(28.0f * dpi);
    tableView->horizontalHeader()->setSectionWidth(0, 70.0f * dpi);
    tableView->horizontalHeader()->setSectionWidth(1, 240.0f * dpi);
    tableView->horizontalHeader()->setSectionWidth(2, 110.0f * dpi);
    tableView->horizontalHeader()->setSectionWidth(3, 80.0f * dpi);
    tableView->horizontalHeader()->setSectionWidth(4, 110.0f * dpi);
    tableView->horizontalHeader()->setStretchLastSection(true);

    TableView* tableViewPtr = tableView.get();
    cbZebraPtr->onToggled.connect(
        [tableViewPtr](bool check) { tableViewPtr->setAlternatingRowColors(check); });
    cbGridPtr->onToggled.connect(
        [tableViewPtr](bool check) { tableViewPtr->setShowGridLines(check); });
    swStretchPtr->onToggled.connect([tableViewPtr](bool check) {
        tableViewPtr->horizontalHeader()->setStretchLastSection(check);
    });

    tabView->addTab("Subsystem Table", std::move(tableView), false);

    auto treeModel = std::make_shared<StandardTreeModel>(
        std::vector<std::string>{"Resource Node", "Type", "Size"});
    auto* rootTree = treeModel->rootItem();

    auto srcDir =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"src/", "Directory", "--"});
    auto* srcPtr = rootTree->appendChild(std::move(srcDir));

    auto coreDir =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"core/", "Directory", "--"});
    auto* corePtr = srcPtr->appendChild(std::move(coreDir));
    corePtr->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"result.hpp", "C++ Header", "4 KB"}));
    corePtr->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"signal.hpp", "C++ Header", "8 KB"}));
    corePtr->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"animation.hpp", "C++ Header", "6 KB"}));

    auto gfxDir = std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"graphics/", "Directory", "--"});
    auto* gfxPtr = srcPtr->appendChild(std::move(gfxDir));
    gfxPtr->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"device.hpp", "C++ Header", "5 KB"}));
    auto vkDir =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"vulkan/", "Directory", "--"});
    auto* vkPtr = gfxPtr->appendChild(std::move(vkDir));
    vkPtr->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"vk_device.cpp", "C++ Source", "28 KB"}));
    vkPtr->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"vk_swapchain.cpp", "C++ Source", "19 KB"}));

    auto widgetsDir =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"widgets/", "Directory", "--"});
    auto* widgetsPtr = srcPtr->appendChild(std::move(widgetsDir));
    widgetsPtr->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"list_view.cpp", "C++ Source", "12 KB"}));
    widgetsPtr->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"table_view.cpp", "C++ Source", "16 KB"}));
    widgetsPtr->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"tree_view.cpp", "C++ Source", "14 KB"}));

    auto shadersDir =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"shaders/", "Directory", "--"});
    auto* shadersPtr = rootTree->appendChild(std::move(shadersDir));
    shadersPtr->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"sdf_primitive.vert", "GLSL", "3 KB"}));
    shadersPtr->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"sdf_primitive.frag", "GLSL", "9 KB"}));

    auto assetsDir =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"assets/", "Directory", "--"});
    auto* assetsPtr = rootTree->appendChild(std::move(assetsDir));
    assetsPtr->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"Inter-Regular.ttf", "Font", "312 KB"}));
    assetsPtr->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"Inter-Medium.ttf", "Font", "318 KB"}));

    auto treeView = std::make_unique<TreeView>(treeModel.get(), bodyFont);
    treeView->setRowHeight(28.0f * dpi);
    treeView->header()->setSectionWidth(0, 260.0f * dpi);
    treeView->header()->setSectionWidth(1, 140.0f * dpi);
    treeView->header()->setSectionWidth(2, 100.0f * dpi);
    treeView->header()->setStretchLastSection(true);
    treeView->expandAll();

    tabView->addTab("Resource Tree", std::move(treeView), true);

    std::vector<std::string> logEntries;
    logEntries.reserve(10000);
    for (int i = 1; i <= 10000; ++i) {
        logEntries.push_back("[Event #" + std::to_string(i) +
                             "] Swapchain frame presented at 60 FPS | SDF batch flushed (0.4ms)");
    }
    auto listModel = std::make_shared<StringListModel>(std::move(logEntries));

    auto listView = std::make_unique<ListView>(listModel.get(), bodyFont);
    listView->setRowHeight(26.0f * dpi);

    tabView->addTab("Virtual Log Stream (10,000 Items)", std::move(listView), true);

    splitView->setPanes(std::move(sidebar), std::move(tabView));
    root->addChild(std::move(splitView));

    auto statusBar = std::make_unique<Container>();
    statusBar->style().flexDirection = FlexDirection::Row;
    statusBar->style().justifyContent = JustifyContent::SpaceBetween;
    statusBar->style().alignItems = AlignItems::Center;
    statusBar->style().width = Length::percent(100.0f);
    statusBar->style().height = Length::points(26.0f * dpi);
    statusBar->style().padding = RectLength{
        .left = Length::points(14.0f * dpi),
        .top = Length::points(0.0f),
        .right = Length::points(14.0f * dpi),
        .bottom = Length::points(0.0f),
    };
    statusBar->setBackgroundColor(theme::ColorTokens::SurfaceDefault);
    statusBar->setBorderColor(theme::ColorTokens::BorderSubtle);
    statusBar->setBorderWidth(1.0f * dpi);

    auto statusLeft =
        std::make_unique<Label>("Vulkan 1.3 GAL | Locked 60 FPS | Retained Tree Active",
                                captionFont ? captionFont : bodyFont);
    statusLeft->setTextColor(theme::ColorTokens::TextSecondary);

    auto statusRight =
        std::make_unique<Label>("Virtual Collection Views Active | 10k Items | 0ms Drop",
                                captionFont ? captionFont : bodyFont);
    statusRight->setTextColor(theme::ColorTokens::TextTertiary);

    statusBar->addChild(std::move(statusLeft));
    statusBar->addChild(std::move(statusRight));
    root->addChild(std::move(statusBar));

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
        } else if (ev.type == MouseEventType::Leave) {
            uiContext.handleMouseLeave();
        }
    });

    window->onKeyEvent.connect([&](const KeyEvent& ev) {
        if (ev.isDown && ev.key == Key::Escape) {
            if (uiContext.hasOverlays()) {
                uiContext.dismissTopOverlay();
                return;
            }
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

    auto lastFrameTime = std::chrono::steady_clock::now();

    while (running) {
        app.pollEvents();

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastFrameTime).count();
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
