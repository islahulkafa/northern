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
#include "widgets/containers/accordion.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/containers/dropdown.hpp"
#include "widgets/containers/menu_bar.hpp"
#include "widgets/containers/scroll_view.hpp"
#include "widgets/containers/split_view.hpp"
#include "widgets/containers/tab_view.hpp"
#include "widgets/controls/button.hpp"
#include "widgets/controls/checkbox.hpp"
#include "widgets/controls/divider.hpp"
#include "widgets/controls/icons.hpp"
#include "widgets/controls/label.hpp"
#include "widgets/controls/progress_bar.hpp"
#include "widgets/controls/slider.hpp"
#include "widgets/controls/switch.hpp"
#include "widgets/controls/text_input.hpp"
#include "widgets/dialogs/dialog.hpp"
#include "widgets/dialogs/message_box.hpp"
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
    uiContext.setDpiScale(dpi);
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

    auto menuBar = std::make_unique<MenuBar>();
    menuBar->setDpiScale(dpi);

    auto* fileMenu = menuBar->addMenu("File");
    fileMenu->addItem("New File", []() { std::cout << "File -> New File\n"; }, "Ctrl+N");
    fileMenu->addItem("Open File...", []() { std::cout << "File -> Open File\n"; }, "Ctrl+O");
    fileMenu->addItem("Save", []() { std::cout << "File -> Save\n"; }, "Ctrl+S");
    fileMenu->addSeparator();
    fileMenu->addItem(
        "Exit",
        [&uiContext, &running, &app, dpi]() {
            MessageBox::confirm(
                uiContext, "Exit Application", "Are you sure you want to close Northern?",
                [&running, &app](DialogResult res) {
                    if (res == DialogResult::Accepted) {
                        running = false;
                        app.quit(0);
                    }
                },
                dpi);
        },
        "Alt+F4");

    auto* editMenu = menuBar->addMenu("Edit");
    editMenu->addItem("Undo", []() { std::cout << "Edit -> Undo\n"; }, "Ctrl+Z");
    editMenu->addItem("Redo", []() { std::cout << "Edit -> Redo\n"; }, "Ctrl+Y");
    editMenu->addSeparator();
    editMenu->addItem("Cut", []() { std::cout << "Edit -> Cut\n"; }, "Ctrl+X");
    editMenu->addItem("Copy", []() { std::cout << "Edit -> Copy\n"; }, "Ctrl+C");
    editMenu->addItem("Paste", []() { std::cout << "Edit -> Paste\n"; }, "Ctrl+V");

    auto* viewMenu = menuBar->addMenu("View");
    viewMenu->addCheckItem("Status Bar", true, [](bool checked) {
        std::cout << "View -> Status Bar: " << (checked ? "ON" : "OFF") << "\n";
    });
    viewMenu->addSubmenu("Zoom", []() {
        auto sub = std::make_unique<ContextMenu>();
        sub->addItem("Zoom In (125%)", []() { std::cout << "Zoom In\n"; }, "Ctrl++");
        sub->addItem("Zoom Out (75%)", []() { std::cout << "Zoom Out\n"; }, "Ctrl+-");
        sub->addItem("Reset Zoom (100%)", []() { std::cout << "Zoom 100%\n"; }, "Ctrl+0");
        return sub;
    });
    viewMenu->addSeparator();
    viewMenu->addItem(
        "Toggle Dark Mode",
        [&uiContext]() {
            bool dark = uiContext.theme().isDark();
            uiContext.setTheme(dark ? theme::Theme::light() : theme::Theme::dark());
        },
        "Ctrl+D");

    auto* helpMenu = menuBar->addMenu("Help");
    helpMenu->addItem("Documentation", []() { std::cout << "Help -> Docs\n"; }, "F1");
    helpMenu->addItem("About Northern", [&uiContext, dpi]() {
        MessageBox::info(uiContext, "About Northern",
                         "Northern UI Framework v0.1.0\nVulkan 1.3+ / Modern C++20", nullptr, dpi);
    });

    menuBar->registerAccelerators(uiContext);
    root->addChild(std::move(menuBar));

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
        tableModel.setData(tableModel.index(r, 1), "", ItemDataRole::Display);
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
    listView->setRowHeight(32.0f * dpi);

    auto accordionPage = std::make_unique<Container>();
    accordionPage->setSurface(SurfaceRole::Canvas);
    accordionPage->setFlexDirection(FlexDirection::Column);
    accordionPage->setAlignItems(AlignItems::FlexStart);
    accordionPage->setPadding(24.0f * dpi);
    accordionPage->setRowGap(16.0f * dpi);

    auto accordionTopRow = std::make_unique<Container>();
    accordionTopRow->setFlexDirection(FlexDirection::Row);
    accordionTopRow->setAlignItems(AlignItems::Center);
    accordionTopRow->setColumnGap(12.0f * dpi);

    auto themeSegmented = std::make_unique<TabBar>();
    themeSegmented->setDpiScale(dpi);
    themeSegmented->setTabBarStyle(TabBarStyle::Segmented);
    themeSegmented->setTabHeight(32.0f * dpi);
    themeSegmented->style().width = Length::points(180.0f * dpi);
    themeSegmented->addTab("Dark");
    themeSegmented->addTab("Light");
    themeSegmented->setSelectedIndex(app.isDarkMode() ? 0 : 1);
    themeSegmented->onTabSelected.connect([&uiContext](int idx) {
        uiContext.setTheme(idx == 0 ? theme::Theme::dark() : theme::Theme::light());
    });
    accordionTopRow->addChild(std::move(themeSegmented));

    auto accordionStyleSegmented = std::make_unique<TabBar>();
    accordionStyleSegmented->setDpiScale(dpi);
    accordionStyleSegmented->setTabBarStyle(TabBarStyle::Segmented);
    accordionStyleSegmented->setTabHeight(32.0f * dpi);
    accordionStyleSegmented->style().width = Length::points(300.0f * dpi);
    accordionStyleSegmented->addTab("Grouped");
    accordionStyleSegmented->addTab("Separated");
    accordionStyleSegmented->addTab("Flush");
    accordionStyleSegmented->setSelectedIndex(0);
    TabBar* accordionStylePtr = accordionStyleSegmented.get();
    accordionTopRow->addChild(std::move(accordionStyleSegmented));
    accordionPage->addChild(std::move(accordionTopRow));

    auto accordion =
        std::make_unique<Accordion>(false, SectionHeaderSize::Standard, AccordionStyle::Grouped);
    accordion->setDpiScale(dpi);
    accordion->style().maxWidth = Length::points(520.0f * dpi);
    Accordion* accPtr = accordion.get();

    accordionStylePtr->onTabSelected.connect([accPtr](int idx) {
        if (idx == 0) {
            accPtr->setAccordionStyle(AccordionStyle::Grouped);
        } else if (idx == 1) {
            accPtr->setAccordionStyle(AccordionStyle::Separated);
        } else {
            accPtr->setAccordionStyle(AccordionStyle::Flush);
        }
    });

    auto* graphicsSec = accordion->addSection("Graphics & Rendering", nullptr, true);
    auto badgeLbl = std::make_unique<Label>("v1.3", theme::TypographyStyle::Caption,
                                            theme::TextRole::Secondary);
    badgeLbl->setDpiScale(dpi);
    graphicsSec->setHeaderAccessory(std::move(badgeLbl));

    auto backendVal = std::make_unique<Label>("Vulkan 1.3+", theme::TypographyStyle::Monospace,
                                              theme::TextRole::Secondary);
    backendVal->setDpiScale(dpi);
    graphicsSec->addRow("Backend", std::move(backendVal));

    auto aaDropdown = std::make_unique<Dropdown>(
        std::vector<DropdownItem>{
            DropdownItem{.text = "4x MSAA"},
            DropdownItem{.text = "8x MSAA"},
            DropdownItem::separator(),
            DropdownItem{.text = "Disabled"},
        },
        0, nullptr, DropdownSize::Compact);
    aaDropdown->setDpiScale(dpi);
    aaDropdown->style().width = Length::points(140.0f * dpi);
    graphicsSec->addRow("Anti-Aliasing", std::move(aaDropdown));

    auto vsyncSwitch = std::make_unique<Switch>("", true);
    vsyncSwitch->setDpiScale(dpi);
    graphicsSec->addRow("Vertical Sync", std::move(vsyncSwitch));

    auto nestedGroup = std::make_unique<DisclosureGroup>("Pipeline Options", false, nullptr,
                                                         SectionHeaderSize::Compact);
    nestedGroup->setDpiScale(dpi);
    nestedGroup->addRow("SDF Anti-Aliasing");
    nestedGroup->addRow("Pipeline Cache");
    graphicsSec->addContent(std::move(nestedGroup));

    auto* appearanceSec = accordion->addSection("Appearance", nullptr, true);
    auto compactSwitch = std::make_unique<Switch>("", false);
    compactSwitch->setDpiScale(dpi);
    compactSwitch->onToggled.connect([accPtr](bool compact) {
        accPtr->setHeaderSize(compact ? SectionHeaderSize::Compact : SectionHeaderSize::Standard);
    });
    appearanceSec->addRow("Compact Headers", std::move(compactSwitch));

    auto dividersSwitch = std::make_unique<Switch>("", true);
    dividersSwitch->setDpiScale(dpi);
    dividersSwitch->onToggled.connect([accPtr](bool show) {
        for (size_t i = 0; i < accPtr->sectionCount(); ++i) {
            if (auto* sec = accPtr->sectionAt(i)) {
                sec->setShowRowDividers(show);
            }
        }
    });
    appearanceSec->addRow("Row Dividers", std::move(dividersSwitch));

    auto* diagSec = accordion->addSection("Diagnostics", nullptr, false);
    auto arenaVal = std::make_unique<Label>("64 KB Scratch", theme::TypographyStyle::Monospace,
                                            theme::TextRole::Secondary);
    arenaVal->setDpiScale(dpi);
    diagSec->addRow("Layout Arena", std::move(arenaVal));

    accordionPage->addChild(std::move(accordion));
    accordionPage->setContextMenuBuilder([accPtr, &uiContext, dpi]() {
        auto menu = std::make_unique<ContextMenu>();
        menu->setDpiScale(dpi);
        menu->addItem("Expand All Sections", [accPtr]() {
            for (size_t i = 0; i < accPtr->sectionCount(); ++i) {
                if (auto* s = accPtr->sectionAt(i)) {
                    s->setExpanded(true, true);
                }
            }
        });
        menu->addItem(IconId::Copy, "Copy Configuration",
                      []() { std::cout << "ContextMenu -> Copy Configuration\n"; });
        menu->addSubtitledItem("Reset Preferences", "Restore default pipeline settings",
                               [accPtr]() {
                                   for (size_t i = 0; i < accPtr->sectionCount(); ++i) {
                                       if (auto* s = accPtr->sectionAt(i)) {
                                           s->setExpanded(i < 2, true);
                                       }
                                   }
                               });
        menu->addSeparator();
        menu->addDestructiveItem(IconId::Trash, "Clear Cache", [&uiContext, dpi]() {
            MessageBox::warning(uiContext, "Cache Cleared",
                                "Pipeline and glyph caches have been flushed.", nullptr, dpi);
        });
        return menu;
    });

    auto tabView = std::make_unique<TabView>();
    tabView->setDpiScale(dpi);
    tabView->setTabBarStyle(TabBarStyle::Underline);
    tabView->style().width = Length::percent(100.0f);
    tabView->style().flexGrow = 1.0f;
    tabView->style().flexShrink = 1.0f;
    tabView->style().flexBasis = layout::Length::points(0.0f);

    tabView->addTab("Table View", std::move(tableView));
    tabView->addTab("Tree View", std::move(treeView));
    tabView->addTab("List View", std::move(listView));
    tabView->addTab("Accordion", std::move(accordionPage));

    StringListModel sidebarModel;
    sidebarModel.append("Overview");
    sidebarModel.append("Shaders");
    sidebarModel.append("Pipeline");
    sidebarModel.append("Telemetry");

    auto splitView = std::make_unique<SplitView>(SplitOrientation::Horizontal);
    splitView->setDpiScale(dpi);
    splitView->setSizeMode(SplitSizeMode::FixedFirst);
    splitView->setFirstPaneSize(220.0f * dpi);
    splitView->setMinFirstPaneSize(150.0f * dpi);

    auto sidebarPane = std::make_unique<Container>();
    sidebarPane->setSurface(SurfaceRole::Default);
    sidebarPane->setClipsChildren(true);
    sidebarPane->setFlexDirection(FlexDirection::Column);

    auto sidebarHeader = std::make_unique<Label>("Workspace", theme::TypographyStyle::BodyStrong,
                                                 theme::TextRole::Primary);
    sidebarHeader->setDpiScale(dpi);
    sidebarHeader->style().margin =
        RectLength{Length::points(12.0f * dpi), Length::points(10.0f * dpi),
                   Length::points(12.0f * dpi), Length::points(8.0f * dpi)};
    sidebarPane->addChild(std::move(sidebarHeader));

    auto sidebarNav = std::make_unique<ListView>(&sidebarModel);
    sidebarNav->setDpiScale(dpi);
    sidebarNav->setListStyle(ListStyle::Sidebar);
    sidebarNav->setRowHeight(32.0f * dpi);
    sidebarNav->style().flexGrow = 1.0f;
    sidebarNav->style().flexShrink = 1.0f;
    sidebarNav->style().flexBasis = Length::points(0.0f);
    auto detailScroll = std::make_unique<ScrollView>();
    detailScroll->setSurface(SurfaceRole::Canvas);
    detailScroll->style().width = Length::percent(100.0f);
    detailScroll->style().height = Length::percent(100.0f);

    auto detailPane = std::make_unique<Container>();
    detailPane->setSurface(SurfaceRole::Canvas);
    detailPane->setClipsChildren(true);
    detailPane->setFlexDirection(FlexDirection::Column);
    detailPane->setAlignItems(AlignItems::FlexStart);
    detailPane->setPadding(24.0f * dpi);
    detailPane->setRowGap(10.0f * dpi);
    detailPane->style().width = Length::percent(100.0f);

    auto detailTitle = std::make_unique<Label>("Workspace Overview", theme::TypographyStyle::Title,
                                               theme::TextRole::Primary);
    Label* detailTitlePtr = static_cast<Label*>(detailPane->addChild(std::move(detailTitle)));

    auto detailBody = std::make_unique<Label>(
        "Drag the 1px hairline divider to resize panes, or select a sidebar section to inspect "
        "its runtime profile.",
        theme::TypographyStyle::Body, theme::TextRole::Secondary);
    detailBody->setLineHeightMultiplier(1.45f);
    Label* detailBodyPtr = static_cast<Label*>(detailPane->addChild(std::move(detailBody)));

    auto detailMeta =
        std::make_unique<Label>("Status: Active / Vulkan 1.3+ / Retained Display List",
                                theme::TypographyStyle::Monospace, theme::TextRole::Tertiary);
    Label* detailMetaPtr = static_cast<Label*>(detailPane->addChild(std::move(detailMeta)));

    ListView* sidebarNavPtr = sidebarNav.get();
    sidebarPane->addChild(std::move(sidebarNav));

    sidebarNavPtr->onItemClicked.connect([detailTitlePtr, detailBodyPtr,
                                          detailMetaPtr](const ModelIndex& idx) {
        switch (idx.row) {
        case 0:
            detailTitlePtr->setText("Workspace Overview");
            detailBodyPtr->setText("Drag the 1px hairline divider to resize panes, or select a "
                                   "sidebar section to inspect its runtime profile.");
            detailMetaPtr->setText("Status: Active / Vulkan 1.3+ / Retained Display List");
            break;
        case 1:
            detailTitlePtr->setText("SDF Shader Pipeline");
            detailBodyPtr->setText("Rounded rects, borders, and crisp hairlines rasterize in a "
                                   "single analytic SDF pass with zero MSAA blur.");
            detailMetaPtr->setText("Pass: BatchRenderer::drawBox / 0.12s OutCubic");
            break;
        case 2:
            detailTitlePtr->setText("Zero-Heap Flexbox Layout");
            detailBodyPtr->setText("Thread-local Scratch Arenas resolve flex constraints and "
                                   "measure caches with deterministic per-frame rollback.");
            detailMetaPtr->setText("Arena: 64 KB Scratch / FlexSolver v2");
            break;
        case 3:
            detailTitlePtr->setText("Idle Power Telemetry");
            detailBodyPtr->setText("Frame ticking suspends cleanly when idle via OS event waits "
                                   "and resumes on interaction or active springs.");
            detailMetaPtr->setText("Mode: MsgWaitForMultipleObjectsEx / 0% Idle CPU");
            break;
        default:
            break;
        }
    });

    SplitView* splitPtr = splitView.get();
    auto toggleBtn = std::make_unique<Button>("Toggle Sidebar", nullptr, ButtonVariant::Secondary);
    toggleBtn->setDpiScale(dpi);
    toggleBtn->onClicked.connect([splitPtr]() { splitPtr->toggleCollapse(true); });
    detailPane->addChild(std::move(toggleBtn));

    auto typeDiv = std::make_unique<Divider>("Typography Hierarchy");
    typeDiv->setLabelAlignment(DividerAlignment::Start);
    typeDiv->setLabelOffset(0.0f);
    typeDiv->style().width = Length::points(520.0f * dpi);
    typeDiv->style().margin.top = Length::points(8.0f * dpi);
    detailPane->addChild(std::move(typeDiv));

    auto typeCard = std::make_unique<Container>();
    typeCard->setSurface(SurfaceRole::Default);
    typeCard->setBorderRole(BorderRole::Subtle);
    typeCard->setCornerRadius(theme::Metrics::CornerRadiusCard * dpi);
    typeCard->setFlexDirection(FlexDirection::Column);
    typeCard->setAlignItems(AlignItems::FlexStart);
    typeCard->setPadding(16.0f * dpi, 18.0f * dpi);
    typeCard->setRowGap(8.0f * dpi);
    typeCard->style().width = Length::points(520.0f * dpi);

    typeCard->addChild(std::make_unique<Label>("Display (24pt Bold) - Empty States",
                                               theme::TypographyStyle::Display,
                                               theme::TextRole::Primary));
    typeCard->addChild(std::make_unique<Label>("Title (18pt Bold) - View Headers",
                                               theme::TypographyStyle::Title,
                                               theme::TextRole::Primary));
    typeCard->addChild(std::make_unique<Label>("Subtitle (15pt Regular) - Section Headings",
                                               theme::TypographyStyle::Subtitle,
                                               theme::TextRole::Primary));
    typeCard->addChild(std::make_unique<Label>("BodyStrong (14pt Bold) - Card Titles & Callouts",
                                               theme::TypographyStyle::BodyStrong,
                                               theme::TextRole::Primary));
    auto bodySpec =
        std::make_unique<Label>("Body (14pt Regular) - Primary reading copy and descriptions.",
                                theme::TypographyStyle::Body, theme::TextRole::Secondary);
    bodySpec->setLineHeightMultiplier(1.45f);
    typeCard->addChild(std::move(bodySpec));
    typeCard->addChild(std::make_unique<Label>("Monospace (13pt) - 0.12s OutCubic / 64 KB Scratch",
                                               theme::TypographyStyle::Monospace,
                                               theme::TextRole::Secondary));
    typeCard->addChild(
        std::make_unique<Label>("Caption (12pt Regular) - Metadata, hints, and secondary labels",
                                theme::TypographyStyle::Caption, theme::TextRole::Tertiary));

    detailPane->addChild(std::move(typeCard));
    detailScroll->addChild(std::move(detailPane));

    splitView->setPanes(std::move(sidebarPane), std::move(detailScroll));
    tabView->addTab("Split View", std::move(splitView));

    auto controlsScroll = std::make_unique<ScrollView>();
    controlsScroll->setDpiScale(dpi);
    controlsScroll->setSurface(SurfaceRole::Canvas);
    controlsScroll->style().width = Length::percent(100.0f);
    controlsScroll->style().height = Length::percent(100.0f);

    auto controlsPage = std::make_unique<Container>();
    controlsPage->setSurface(SurfaceRole::Canvas);
    controlsPage->setFlexDirection(FlexDirection::Column);
    controlsPage->setAlignItems(AlignItems::FlexStart);
    controlsPage->setPadding(24.0f * dpi);
    controlsPage->setRowGap(16.0f * dpi);
    controlsPage->style().width = Length::percent(100.0f);

    auto controlsTitle = std::make_unique<Label>(
        "Controls & Density", theme::TypographyStyle::Title, theme::TextRole::Primary);
    controlsPage->addChild(std::move(controlsTitle));

    auto controlsLead = std::make_unique<Label>(
        "Compact desktop controls paired with Optical Zinc surfaces and Cobalt focus indicators.",
        theme::TypographyStyle::Body, theme::TextRole::Secondary);
    controlsLead->setLineHeightMultiplier(1.45f);
    controlsPage->addChild(std::move(controlsLead));

    auto searchInput = std::make_unique<TextInput>("Search settings...", TextInputSize::Standard);
    searchInput->setDpiScale(dpi);
    searchInput->setLeadingIcon(IconId::Search);
    searchInput->setClearButtonEnabled(true);
    searchInput->style().width = Length::points(320.0f * dpi);
    controlsPage->addChild(std::move(searchInput));

    auto actionsDivider = std::make_unique<Divider>("Actions");
    actionsDivider->setLabelAlignment(DividerAlignment::Start);
    actionsDivider->setLabelOffset(0.0f);
    actionsDivider->style().width = Length::points(360.0f * dpi);
    controlsPage->addChild(std::move(actionsDivider));

    auto btnRow = std::make_unique<Container>();
    btnRow->setFlexDirection(FlexDirection::Row);
    btnRow->setAlignItems(AlignItems::Center);
    btnRow->setColumnGap(10.0f * dpi);

    auto primaryBtn = std::make_unique<Button>("Continue", nullptr, ButtonVariant::Primary);
    primaryBtn->setTooltip("Apply active settings", "Ctrl+Enter");
    Button* primaryPtr = static_cast<Button*>(btnRow->addChild(std::move(primaryBtn)));

    auto secondaryBtn = std::make_unique<Button>("Cancel", nullptr, ButtonVariant::Secondary);
    btnRow->addChild(std::move(secondaryBtn));

    auto outlineBtn = std::make_unique<Button>("Loading", IconId::Settings, ButtonVariant::Outline);
    Button* outlinePtr = static_cast<Button*>(btnRow->addChild(std::move(outlineBtn)));
    outlinePtr->onClicked.connect(
        [primaryPtr]() { primaryPtr->setLoading(!primaryPtr->isLoading()); });

    controlsPage->addChild(std::move(btnRow));

    auto optionsDivider = std::make_unique<Divider>("Options");
    optionsDivider->setLabelAlignment(DividerAlignment::Start);
    optionsDivider->setLabelOffset(0.0f);
    optionsDivider->style().width = Length::points(360.0f * dpi);
    controlsPage->addChild(std::move(optionsDivider));

    auto toggleSwitch = std::make_unique<Switch>("Hardware Acceleration", true);
    toggleSwitch->setTooltip("GPU Acceleration", "Ctrl+Shift+H",
                             "Uses the Vulkan 1.3+ hardware-accelerated rendering pipeline.");
    controlsPage->addChild(std::move(toggleSwitch));

    auto checkItem = std::make_unique<Checkbox>("Enable Optical Zinc Validation", true);
    auto triCheck = std::make_unique<Checkbox>("Batch Sync (Tristate)", CheckState::Indeterminate);
    triCheck->setTristate(true);
    Checkbox* triPtr = triCheck.get();
    checkItem->onCheckedChanged.connect([triPtr](bool enabled) { triPtr->setEnabled(enabled); });
    controlsPage->addChild(std::move(checkItem));
    controlsPage->addChild(std::move(triCheck));

    auto progressBar = std::make_unique<ProgressBar>(0.65f, false);
    progressBar->setShowValue(true);
    progressBar->style().width = Length::points(320.0f * dpi);
    ProgressBar* progressPtr = progressBar.get();

    auto slider = std::make_unique<Slider>(0.0f, 100.0f, 65.0f);
    slider->setStep(5.0f);
    slider->setShowValue(true);
    slider->setTooltip("Adjust render quality scale", "Left/Right");
    slider->style().width = Length::points(320.0f * dpi);
    slider->onValueChanged.connect(
        [progressPtr](float v) { progressPtr->setProgress(v / 100.0f); });

    controlsPage->addChild(std::move(slider));
    controlsPage->addChild(std::move(progressBar));

    auto interactiveCard = std::make_unique<Container>();
    interactiveCard->setSurface(SurfaceRole::Default);
    interactiveCard->setBorderRole(BorderRole::Strong);
    interactiveCard->setCornerRadius(theme::Metrics::CornerRadiusCard * dpi);
    interactiveCard->setFlexDirection(FlexDirection::Column);
    interactiveCard->setAlignItems(AlignItems::FlexStart);
    interactiveCard->setPadding(12.0f * dpi, 16.0f * dpi);
    interactiveCard->setRowGap(4.0f * dpi);
    interactiveCard->setInteractive(true);
    interactiveCard->style().width = Length::points(320.0f * dpi);

    auto cardTitle = std::make_unique<Label>("Interactive Card", theme::TypographyStyle::BodyStrong,
                                             theme::TextRole::Primary);
    cardTitle->setDpiScale(dpi);
    interactiveCard->addChild(std::move(cardTitle));

    auto cardSubtitle =
        std::make_unique<Label>("Click or press Enter/Space (0 clicks)",
                                theme::TypographyStyle::Caption, theme::TextRole::Secondary);
    cardSubtitle->setDpiScale(dpi);
    Label* cardSubPtr = static_cast<Label*>(interactiveCard->addChild(std::move(cardSubtitle)));

    auto cardClickCount = std::make_shared<int>(0);
    interactiveCard->onClicked.connect([cardSubPtr, cardClickCount]() {
        ++(*cardClickCount);
        cardSubPtr->setText("Click or press Enter/Space (" + std::to_string(*cardClickCount) +
                            (*cardClickCount == 1 ? " click)" : " clicks)"));
    });
    controlsPage->addChild(std::move(interactiveCard));

    controlsScroll->addChild(std::move(controlsPage));
    tabView->addTab("Controls", std::move(controlsScroll));

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
        if (ev.isDown && ev.key == Key::Escape && !uiContext.hasOverlays()) {
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
