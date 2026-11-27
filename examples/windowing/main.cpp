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
#include "renderer/image/image.hpp"
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
#include "widgets/controls/avatar.hpp"
#include "widgets/controls/badge.hpp"
#include "widgets/controls/button.hpp"
#include "widgets/controls/checkbox.hpp"
#include "widgets/controls/divider.hpp"
#include "widgets/controls/icons.hpp"
#include "widgets/controls/image_view.hpp"
#include "widgets/controls/label.hpp"
#include "widgets/controls/progress_bar.hpp"
#include "widgets/controls/radio_button.hpp"
#include "widgets/controls/radio_group.hpp"
#include "widgets/controls/slider.hpp"
#include "widgets/controls/switch.hpp"
#include "widgets/controls/text_area.hpp"
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
    batchRenderer->setDpiScale(dpi);

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
        std::make_unique<TitleBar>("Studio Demo", nullptr, window.get(), TitleBarSize::Standard);
    TitleBar* titleBarPtr = static_cast<TitleBar*>(root->addChild(std::move(titleBar)));

    window->setHitTestCallback([titleBarPtr, dpi](core::Point logicalPos) {
        return titleBarPtr->hitTestPoint(core::Point{logicalPos.x * dpi, logicalPos.y * dpi});
    });

    auto menuBar = std::make_unique<MenuBar>();

    auto* fileMenu = menuBar->addMenu("File");
    fileMenu->addItem("New File", []() { std::cout << "File -> New File\n"; }, "Ctrl+N");
    fileMenu->addItem("Open File...", []() { std::cout << "File -> Open File\n"; }, "Ctrl+O");
    fileMenu->addItem("Save", []() { std::cout << "File -> Save\n"; }, "Ctrl+S");
    fileMenu->addSeparator();
    fileMenu->addItem(
        "Exit",
        [&uiContext, &running, &app]() {
            MessageBox::confirm(uiContext, "Exit Application",
                                "Are you sure you want to close Studio Demo?",
                                [&running, &app](DialogResult res) {
                                    if (res == DialogResult::Accepted) {
                                        running = false;
                                        app.quit(0);
                                    }
                                });
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
    helpMenu->addItem("About", [&uiContext]() {
        MessageBox::info(uiContext, "About Studio Demo",
                         "Sample Desktop Workspace v2.4\nCross-platform desktop demo.");
    });

    menuBar->registerAccelerators(uiContext);
    root->addChild(std::move(menuBar));

    StandardTableModel tableModel(
        std::vector<std::string>{"Customer", "Status", "Total", "Updated"});
    tableModel.appendRow({"Maya Lin", "Shipped", "$148.00", "2m ago"});
    tableModel.appendRow({"Liam Carter", "Active", "$64.50", "14m ago"});
    tableModel.appendRow({"Sofia Rossi", "Running", "$312.90", "28m ago"});
    tableModel.appendRow({"Noah Patel", "Idle", "$89.00", "1h ago"});
    tableModel.appendRow({"Elena Vance", "Shipped", "$215.40", "2h ago"});
    tableModel.appendRow({"Lucas Meyer", "Idle", "$42.00", "3h ago"});
    tableModel.appendRow({"Chloe Kim", "Active", "$179.25", "5h ago"});
    tableModel.appendRow({"Owen Brooks", "Shipped", "$96.80", "1d ago"});

    for (int r = 0; r < tableModel.rowCount(); ++r) {
        std::string status = tableModel.data(tableModel.index(r, 1));
        tableModel.setData(tableModel.index(r, 1), status, ItemDataRole::BadgeText);
        tableModel.setData(tableModel.index(r, 1), "", ItemDataRole::Display);
    }

    auto tableView = std::make_unique<TableView>(&tableModel);
    tableView->setAlternatingRowColors(true);
    tableView->setShowGridLines(true);
    tableView->horizontalHeader()->setSectionWidth(0, 240.0f * dpi);
    tableView->horizontalHeader()->setSectionWidth(1, 130.0f * dpi);
    tableView->horizontalHeader()->setSectionWidth(2, 130.0f * dpi);
    tableView->horizontalHeader()->setSectionWidth(3, 130.0f * dpi);

    StandardTreeModel treeModel(std::vector<std::string>{"Personal Drive", "Kind", "Size"});
    auto* rootTree = treeModel.rootItem();

    auto docsFolder =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"Documents", "Folder", ""});
    auto travelDir = docsFolder->appendChild(
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"Travel", "Folder", ""}));
    travelDir->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"Kyoto_Itinerary.pdf", "PDF Document", "1.4 MB"}));
    travelDir->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"Flight_Receipts.xlsx", "Spreadsheet", "420 KB"}));

    auto workDir = docsFolder->appendChild(
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"Projects", "Folder", ""}));
    workDir->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"Q3_Budget.csv", "CSV Table", "86.5 KB"}));
    workDir->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"Garden_Plan.sketch", "Sketch File", "4.8 MB"}));
    workDir->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"Meeting_Notes.md", "Markdown", "18.7 KB"}));

    rootTree->appendChild(std::move(docsFolder));

    auto photosDir =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"Photos", "Folder", ""});
    photosDir->appendChild(std::make_unique<StandardTreeItem>(
        std::vector<std::string>{"Coastline_Sunset.jpg", "JPEG Image", "6.2 MB"}));
    rootTree->appendChild(std::move(photosDir));

    auto treeView = std::make_unique<TreeView>(&treeModel);
    treeView->expandAll();
    treeView->header()->setSectionWidth(0, 280.0f * dpi);
    treeView->header()->setSectionWidth(1, 140.0f * dpi);
    treeView->header()->setSectionWidth(2, 140.0f * dpi);

    StringListModel listModel;
    listModel.append("Pick up espresso beans from roastery");
    listModel.append("Review Q3 budget spreadsheet before Friday");
    listModel.append("Book morning train tickets to Osaka");
    listModel.append("Water balcony herbs and repot sweet basil");
    listModel.append("Finish chapter 4 of Designing Data Systems");
    listModel.append("Clean camera sensor and charge spare batteries");
    listModel.append("Send birthday postcard to Hannah");
    listModel.append("Organize weekend hiking pack and trail map");

    auto listView = std::make_unique<ListView>(&listModel);
    listView->setRowHeight(32.0f * dpi);

    auto accordionScroll = std::make_unique<ScrollView>();
    accordionScroll->setSurface(SurfaceRole::Canvas);
    accordionScroll->style().width = Length::percent(100.0f);
    accordionScroll->style().height = Length::percent(100.0f);

    auto accordionPage = std::make_unique<Container>();
    accordionPage->setSurface(SurfaceRole::Canvas);
    accordionPage->setFlexDirection(FlexDirection::Column);
    accordionPage->setAlignItems(AlignItems::FlexStart);
    accordionPage->setPadding(24.0f * dpi);
    accordionPage->setRowGap(16.0f * dpi);
    accordionPage->style().width = Length::percent(100.0f);

    auto accordionTopRow = std::make_unique<Container>();
    accordionTopRow->setFlexDirection(FlexDirection::Row);
    accordionTopRow->setAlignItems(AlignItems::Center);
    accordionTopRow->setColumnGap(12.0f * dpi);

    auto themeSegmented = std::make_unique<TabBar>();
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
    accordionStyleSegmented->setTabBarStyle(TabBarStyle::Segmented);
    accordionStyleSegmented->setTabHeight(32.0f * dpi);
    accordionStyleSegmented->style().width = Length::points(328.0f * dpi);
    accordionStyleSegmented->addTab("Grouped");
    accordionStyleSegmented->addTab("Separated");
    accordionStyleSegmented->addTab("Flush");
    accordionStyleSegmented->setSelectedIndex(0);
    TabBar* accordionStylePtr = accordionStyleSegmented.get();
    accordionTopRow->addChild(std::move(accordionStyleSegmented));
    accordionPage->addChild(std::move(accordionTopRow));

    auto accordion =
        std::make_unique<Accordion>(false, SectionHeaderSize::Standard, AccordionStyle::Grouped);
    accordion->style().width = Length::points(520.0f * dpi);
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

    auto* generalSec = accordion->addSection("General & Sync", nullptr, true);
    auto badgeLbl =
        std::make_unique<Label>("Pro", theme::TypographyStyle::Caption, theme::TextRole::Secondary);
    generalSec->setHeaderAccessory(std::move(badgeLbl));

    auto regionVal = std::make_unique<Label>("US-West (Oregon)", theme::TypographyStyle::Monospace,
                                             theme::TextRole::Secondary);
    generalSec->addRow("Sync Region", std::move(regionVal));

    auto freqDropdown = std::make_unique<Dropdown>(
        std::vector<DropdownItem>{
            DropdownItem{.text = "Every 5 min"},
            DropdownItem{.text = "Every 30 min"},
            DropdownItem::separator(),
            DropdownItem{.text = "Manual Only"},
        },
        0, nullptr, DropdownSize::Compact);
    freqDropdown->style().width = Length::points(140.0f * dpi);
    generalSec->addRow("Backup Interval", std::move(freqDropdown));

    auto cloudSwitch = std::make_unique<Switch>("", true);
    generalSec->addRow("Cloud Sync", std::move(cloudSwitch));

    auto nestedGroup = std::make_unique<DisclosureGroup>("Notification Channels", false, nullptr,
                                                         SectionHeaderSize::Compact);
    nestedGroup->addRow("Weekly Email Digest");
    nestedGroup->addRow("Calendar Reminders");
    generalSec->addContent(std::move(nestedGroup));

    auto* appearanceSec = accordion->addSection("Appearance", nullptr, true);
    auto compactSwitch = std::make_unique<Switch>("", false);
    compactSwitch->onToggled.connect([accPtr](bool compact) {
        accPtr->setHeaderSize(compact ? SectionHeaderSize::Compact : SectionHeaderSize::Standard);
    });
    appearanceSec->addRow("Compact Headers", std::move(compactSwitch));

    auto dividersSwitch = std::make_unique<Switch>("", true);
    dividersSwitch->onToggled.connect([accPtr](bool show) {
        for (size_t i = 0; i < accPtr->sectionCount(); ++i) {
            if (auto* sec = accPtr->sectionAt(i)) {
                sec->setShowRowDividers(show);
            }
        }
    });
    appearanceSec->addRow("Row Dividers", std::move(dividersSwitch));

    auto* storageSec = accordion->addSection("Storage & Cache", nullptr, false);
    auto cacheVal = std::make_unique<Label>("248 MB Used", theme::TypographyStyle::Monospace,
                                            theme::TextRole::Secondary);
    storageSec->addRow("Offline Cache", std::move(cacheVal));

    accordionPage->addChild(std::move(accordion));
    accordionPage->setContextMenuBuilder([accPtr, &uiContext]() {
        auto menu = std::make_unique<ContextMenu>();
        menu->addItem("Expand All Sections", [accPtr]() {
            for (size_t i = 0; i < accPtr->sectionCount(); ++i) {
                if (auto* s = accPtr->sectionAt(i)) {
                    s->setExpanded(true, true);
                }
            }
        });
        menu->addItem(IconId::Copy, "Copy Summary",
                      []() { std::cout << "ContextMenu -> Copy Summary\n"; });
        menu->addSubtitledItem("Reset Preferences", "Restore default account settings", [accPtr]() {
            for (size_t i = 0; i < accPtr->sectionCount(); ++i) {
                if (auto* s = accPtr->sectionAt(i)) {
                    s->setExpanded(i < 2, true);
                }
            }
        });
        menu->addSeparator();
        menu->addDestructiveItem(IconId::Trash, "Clear Offline Cache", [&uiContext]() {
            MessageBox::warning(uiContext, "Cache Cleared",
                                "Offline media and thumbnail caches have been cleared.");
        });
        return menu;
    });
    accordionScroll->addChild(std::move(accordionPage));

    auto tabView = std::make_unique<TabView>();
    tabView->setTabBarStyle(TabBarStyle::Underline);
    tabView->style().width = Length::percent(100.0f);
    tabView->style().flexGrow = 1.0f;
    tabView->style().flexShrink = 1.0f;
    tabView->style().flexBasis = layout::Length::points(0.0f);

    tabView->addTab("Table View", std::move(tableView));
    tabView->addTab("Tree View", std::move(treeView));
    tabView->addTab("List View", std::move(listView));
    tabView->addTab("Accordion", std::move(accordionScroll));

    StringListModel sidebarModel;
    sidebarModel.append("Inbox");
    sidebarModel.append("Travel Notes");
    sidebarModel.append("Reading List");
    sidebarModel.append("Archive");

    auto splitView = std::make_unique<SplitView>(SplitOrientation::Horizontal);
    splitView->setSizeMode(SplitSizeMode::FixedFirst);
    splitView->setFirstPaneSize(220.0f * dpi);
    splitView->setMinFirstPaneSize(150.0f * dpi);

    auto sidebarPane = std::make_unique<Container>();
    sidebarPane->setSurface(SurfaceRole::Default);
    sidebarPane->setClipsChildren(true);
    sidebarPane->setFlexDirection(FlexDirection::Column);

    auto sidebarHeader = std::make_unique<Label>("Notebooks", theme::TypographyStyle::BodyStrong,
                                                 theme::TextRole::Primary);
    sidebarHeader->style().margin =
        RectLength{Length::points(12.0f * dpi), Length::points(10.0f * dpi),
                   Length::points(12.0f * dpi), Length::points(8.0f * dpi)};
    sidebarPane->addChild(std::move(sidebarHeader));

    auto sidebarNav = std::make_unique<ListView>(&sidebarModel);
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

    auto detailTitle = std::make_unique<Label>("Weekly Inbox", theme::TypographyStyle::Title,
                                               theme::TextRole::Primary);
    Label* detailTitlePtr = static_cast<Label*>(detailPane->addChild(std::move(detailTitle)));

    auto detailBody = std::make_unique<Label>(
        "Drag the divider to resize the sidebar, or choose a notebook on the left to preview "
        "recent notes and reminders.",
        theme::TypographyStyle::Body, theme::TextRole::Secondary);
    detailBody->setLineHeightMultiplier(1.08f);
    Label* detailBodyPtr = static_cast<Label*>(detailPane->addChild(std::move(detailBody)));

    auto detailMeta =
        std::make_unique<Label>("Updated Today at 4:15 PM - 4 Unread Items",
                                theme::TypographyStyle::Monospace, theme::TextRole::Tertiary);
    Label* detailMetaPtr = static_cast<Label*>(detailPane->addChild(std::move(detailMeta)));

    ListView* sidebarNavPtr = sidebarNav.get();
    sidebarPane->addChild(std::move(sidebarNav));

    sidebarNavPtr->onItemClicked.connect([detailTitlePtr, detailBodyPtr,
                                          detailMetaPtr](const ModelIndex& idx) {
        switch (idx.row) {
        case 0:
            detailTitlePtr->setText("Weekly Inbox");
            detailBodyPtr->setText("Drag the divider to resize the sidebar, or choose a notebook "
                                   "on the left to preview recent notes and reminders.");
            detailMetaPtr->setText("Updated Today at 4:15 PM - 4 Unread Items");
            break;
        case 1:
            detailTitlePtr->setText("Coastal Road Trip");
            detailBodyPtr->setText("Packed the camping stove, insulated thermos, and offline maps "
                                   "for the highway scenic route along the northern coast.");
            detailMetaPtr->setText("Tag: Travel - 3 Days, 2 Stops");
            break;
        case 2:
            detailTitlePtr->setText("Autumn Reading List");
            detailBodyPtr->setText("Currently reading essays on urban architecture, botanical "
                                   "illustration, and slow coffee brewing techniques.");
            detailMetaPtr->setText("Progress: 142 / 280 Pages");
            break;
        case 3:
            detailTitlePtr->setText("Archived Projects");
            detailBodyPtr->setText("Completed home renovation sketches, spring garden layout, and "
                                   "last year's tax receipts stored for reference.");
            detailMetaPtr->setText("Folder: Archive - 18 Files");
            break;
        default:
            break;
        }
    });

    SplitView* splitPtr = splitView.get();
    auto toggleBtn = std::make_unique<Button>("Toggle Sidebar", nullptr, ButtonVariant::Secondary);
    toggleBtn->onClicked.connect([splitPtr]() { splitPtr->toggleCollapse(true); });
    detailPane->addChild(std::move(toggleBtn));

    auto detailCard = std::make_unique<Container>();
    detailCard->setSurface(SurfaceRole::Default);
    detailCard->setBorderRole(BorderRole::Subtle);
    detailCard->setCornerRadius(theme::Metrics::CornerRadiusCard * dpi);
    detailCard->setFlexDirection(FlexDirection::Column);
    detailCard->setAlignItems(AlignItems::FlexStart);
    detailCard->setPadding(16.0f * dpi, 18.0f * dpi);
    detailCard->setRowGap(6.0f * dpi);
    detailCard->style().width = Length::percent(100.0f);
    detailCard->style().maxWidth = Length::points(560.0f * dpi);
    detailCard->style().margin.top = Length::points(8.0f * dpi);

    detailCard->addChild(std::make_unique<Label>(
        "Weekend Summary", theme::TypographyStyle::BodyStrong, theme::TextRole::Primary));
    auto cardCopy = std::make_unique<Label>(
        "Farmers market opens at 8:00 AM on Saturday. Pick up sourdough, fresh basil, and "
        "roasted hazelnuts before heading to the botanical gardens.",
        theme::TypographyStyle::Body, theme::TextRole::Secondary);
    cardCopy->setLineHeightMultiplier(1.08f);
    detailCard->addChild(std::move(cardCopy));
    detailCard->addChild(std::make_unique<Label>("Location: Portland, OR - 62 F Clear",
                                                 theme::TypographyStyle::Monospace,
                                                 theme::TextRole::Tertiary));

    detailPane->addChild(std::move(detailCard));
    detailScroll->addChild(std::move(detailPane));

    splitView->setPanes(std::move(sidebarPane), std::move(detailScroll));
    tabView->addTab("Split View", std::move(splitView));

    auto controlsScroll = std::make_unique<ScrollView>();
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
        "Profile & Preferences", theme::TypographyStyle::Title, theme::TextRole::Primary);
    controlsPage->addChild(std::move(controlsTitle));

    auto controlsLead = std::make_unique<Label>(
        "Configure general account options, text notes, and interactive form controls.",
        theme::TypographyStyle::Body, theme::TextRole::Secondary);
    controlsPage->addChild(std::move(controlsLead));

    auto columnsRow = std::make_unique<Container>();
    columnsRow->setFlexDirection(FlexDirection::Row);
    columnsRow->setAlignItems(AlignItems::FlexStart);
    columnsRow->setColumnGap(32.0f * dpi);
    columnsRow->style().width = Length::percent(100.0f);

    auto leftCol = std::make_unique<Container>();
    leftCol->setFlexDirection(FlexDirection::Column);
    leftCol->setAlignItems(AlignItems::FlexStart);
    leftCol->setRowGap(14.0f * dpi);
    leftCol->style().width = Length::points(380.0f * dpi);

    auto inputDivider = std::make_unique<Divider>("Inputs & Actions");
    inputDivider->setLabelAlignment(DividerAlignment::Start);
    inputDivider->setLabelOffset(0.0f);
    inputDivider->style().width = Length::percent(100.0f);
    leftCol->addChild(std::move(inputDivider));

    auto searchInput =
        std::make_unique<TextInput>("Search contacts or tags...", TextInputSize::Standard);
    searchInput->setLeadingIcon(IconId::Search);
    searchInput->setClearButtonEnabled(true);
    searchInput->style().width = Length::percent(100.0f);
    leftCol->addChild(std::move(searchInput));

    auto btnRow = std::make_unique<Container>();
    btnRow->setFlexDirection(FlexDirection::Row);
    btnRow->setAlignItems(AlignItems::Center);
    btnRow->setColumnGap(10.0f * dpi);

    auto primaryBtn = std::make_unique<Button>("Save Changes", nullptr, ButtonVariant::Primary);
    primaryBtn->setTooltip("Save active changes", "Ctrl+S");
    Button* primaryPtr = static_cast<Button*>(btnRow->addChild(std::move(primaryBtn)));

    auto secondaryBtn = std::make_unique<Button>("Reset", nullptr, ButtonVariant::Secondary);
    btnRow->addChild(std::move(secondaryBtn));

    auto outlineBtn = std::make_unique<Button>("Sync", IconId::Settings, ButtonVariant::Outline);
    Button* outlinePtr = static_cast<Button*>(btnRow->addChild(std::move(outlineBtn)));
    outlinePtr->onClicked.connect(
        [primaryPtr]() { primaryPtr->setLoading(!primaryPtr->isLoading()); });
    leftCol->addChild(std::move(btnRow));

    auto textArea = std::make_unique<TextArea>("Write a quick note or bio...");
    textArea->setText(
        "Morning coffee by the window, checking the weather forecast before heading out.\n"
        "Remember to pick up fresh tomatoes and olive oil on the way home.");
    textArea->style().width = Length::percent(100.0f);
    textArea->style().height = Length::points(120.0f * dpi);
    TextArea* textAreaPtr = textArea.get();

    auto editorOpts = std::make_unique<Container>();
    editorOpts->setFlexDirection(FlexDirection::Row);
    editorOpts->setAlignItems(AlignItems::Center);
    editorOpts->setColumnGap(14.0f * dpi);
    auto wrapCheck = std::make_unique<Checkbox>("Word Wrap", true);
    wrapCheck->onCheckedChanged.connect([textAreaPtr](bool wrapped) {
        textAreaPtr->setWordWrap(wrapped ? northern::text::WordWrap::Word
                                         : northern::text::WordWrap::None);
    });
    auto lineNumCheck = std::make_unique<Checkbox>("Line Numbers", false);
    lineNumCheck->onCheckedChanged.connect(
        [textAreaPtr](bool show) { textAreaPtr->setShowLineNumbers(show); });
    editorOpts->addChild(std::move(wrapCheck));
    editorOpts->addChild(std::move(lineNumCheck));

    leftCol->addChild(std::move(textArea));
    leftCol->addChild(std::move(editorOpts));

    auto rightCol = std::make_unique<Container>();
    rightCol->setFlexDirection(FlexDirection::Column);
    rightCol->setAlignItems(AlignItems::FlexStart);
    rightCol->setRowGap(14.0f * dpi);
    rightCol->style().width = Length::points(380.0f * dpi);

    auto optionsDivider = std::make_unique<Divider>("Selection & Range");
    optionsDivider->setLabelAlignment(DividerAlignment::Start);
    optionsDivider->setLabelOffset(0.0f);
    optionsDivider->style().width = Length::percent(100.0f);
    rightCol->addChild(std::move(optionsDivider));

    auto toggleSwitch = std::make_unique<Switch>("Desktop Notifications", true);
    toggleSwitch->setTooltip("Notifications", "Ctrl+Shift+N",
                             "Show desktop banners for new messages and calendar alerts.");
    rightCol->addChild(std::move(toggleSwitch));

    auto checkItem = std::make_unique<Checkbox>("Enable Automatic Backups", true);
    auto triCheck =
        std::make_unique<Checkbox>("Include Media (Tristate)", CheckState::Indeterminate);
    triCheck->setTristate(true);
    Checkbox* triPtr = triCheck.get();
    checkItem->onCheckedChanged.connect([triPtr](bool enabled) { triPtr->setEnabled(enabled); });
    rightCol->addChild(std::move(checkItem));
    rightCol->addChild(std::move(triCheck));

    auto radioGroup = std::make_unique<RadioGroup>(RadioGroupOrientation::Horizontal);
    radioGroup->setGap(14.0f * dpi);
    radioGroup->addRadioButton(std::make_unique<RadioButton>("Daily"));
    radioGroup->addRadioButton(std::make_unique<RadioButton>("Weekly"));
    radioGroup->addRadioButton(std::make_unique<RadioButton>("Monthly"));
    radioGroup->setSelectedIndex(1);
    rightCol->addChild(std::move(radioGroup));

    auto progressBar = std::make_unique<ProgressBar>(0.65f, false);
    progressBar->setShowValue(true);
    progressBar->style().width = Length::percent(100.0f);
    ProgressBar* progressPtr = progressBar.get();

    auto slider = std::make_unique<Slider>(0.0f, 100.0f, 65.0f);
    slider->setStep(5.0f);
    slider->setShowValue(true);
    slider->setTooltip("Adjust volume level", "Left/Right");
    slider->style().width = Length::percent(100.0f);
    slider->onValueChanged.connect(
        [progressPtr](float v) { progressPtr->setProgress(v / 100.0f); });

    rightCol->addChild(std::move(slider));
    rightCol->addChild(std::move(progressBar));

    auto interactiveCard = std::make_unique<Container>();
    interactiveCard->setSurface(SurfaceRole::Default);
    interactiveCard->setBorderRole(BorderRole::Strong);
    interactiveCard->setCornerRadius(theme::Metrics::CornerRadiusCard * dpi);
    interactiveCard->setFlexDirection(FlexDirection::Column);
    interactiveCard->setAlignItems(AlignItems::FlexStart);
    interactiveCard->setPadding(12.0f * dpi, 16.0f * dpi);
    interactiveCard->setRowGap(4.0f * dpi);
    interactiveCard->setInteractive(true);
    interactiveCard->style().width = Length::percent(100.0f);

    auto cardTitle = std::make_unique<Label>("Quick Counter", theme::TypographyStyle::BodyStrong,
                                             theme::TextRole::Primary);
    interactiveCard->addChild(std::move(cardTitle));

    auto cardSubtitle =
        std::make_unique<Label>("Click or press Enter/Space (0 clicks)",
                                theme::TypographyStyle::Caption, theme::TextRole::Secondary);
    Label* cardSubPtr = static_cast<Label*>(interactiveCard->addChild(std::move(cardSubtitle)));

    auto cardClickCount = std::make_shared<int>(0);
    interactiveCard->onClicked.connect([cardSubPtr, cardClickCount]() {
        ++(*cardClickCount);
        cardSubPtr->setText("Click or press Enter/Space (" + std::to_string(*cardClickCount) +
                            (*cardClickCount == 1 ? " click)" : " clicks)"));
    });
    rightCol->addChild(std::move(interactiveCard));

    columnsRow->addChild(std::move(leftCol));
    columnsRow->addChild(std::move(rightCol));
    controlsPage->addChild(std::move(columnsRow));

    controlsScroll->addChild(std::move(controlsPage));
    tabView->addTab("Controls", std::move(controlsScroll));

    // Media & Tags Tab
    auto mediaScroll = std::make_unique<ScrollView>();
    mediaScroll->setSurface(SurfaceRole::Canvas);
    mediaScroll->style().width = Length::percent(100.0f);
    mediaScroll->style().height = Length::percent(100.0f);

    auto mediaPage = std::make_unique<Container>();
    mediaPage->setSurface(SurfaceRole::Canvas);
    mediaPage->setFlexDirection(FlexDirection::Column);
    mediaPage->setAlignItems(AlignItems::FlexStart);
    mediaPage->setPadding(24.0f * dpi);
    mediaPage->setRowGap(16.0f * dpi);
    mediaPage->style().width = Length::percent(100.0f);

    mediaPage->addChild(std::make_unique<Label>("Media & Tags", theme::TypographyStyle::Title,
                                                theme::TextRole::Primary));
    mediaPage->addChild(std::make_unique<Label>(
        "GPU-cached RGBA images with SDF rounded corners, user avatars, and semantic badges.",
        theme::TypographyStyle::Body, theme::TextRole::Secondary));

    auto makeSunsetImage = [](uint32_t w, uint32_t h) {
        std::vector<uint8_t> px(static_cast<size_t>(w) * h * 4u);
        for (uint32_t y = 0; y < h; ++y) {
            float fy = static_cast<float>(y) / static_cast<float>(h - 1);
            for (uint32_t x = 0; x < w; ++x) {
                float fx = static_cast<float>(x) / static_cast<float>(w - 1);
                float dx = fx - 0.72f;
                float dy = fy - 0.34f;
                float sunDist = std::sqrt(dx * dx + dy * dy);
                float sun = std::clamp(1.0f - (sunDist - 0.10f) * 14.0f, 0.0f, 1.0f);
                float ridge = 0.62f + 0.07f * std::sin(fx * 7.5f) + 0.03f * std::cos(fx * 17.0f);
                float r = (1.0f - fy) * 0.92f + fy * 0.16f;
                float g = (1.0f - fy) * 0.45f + fy * 0.22f;
                float b = (1.0f - fy) * 0.38f + fy * 0.48f;
                if (fy > ridge) {
                    r *= 0.28f;
                    g *= 0.34f;
                    b *= 0.46f;
                } else if (sun > 0.0f) {
                    r = r * (1.0f - sun) + 1.0f * sun;
                    g = g * (1.0f - sun) + 0.88f * sun;
                    b = b * (1.0f - sun) + 0.62f * sun;
                }
                size_t idx = (static_cast<size_t>(y) * w + x) * 4u;
                px[idx + 0] = static_cast<uint8_t>(std::clamp(r, 0.0f, 1.0f) * 255.0f);
                px[idx + 1] = static_cast<uint8_t>(std::clamp(g, 0.0f, 1.0f) * 255.0f);
                px[idx + 2] = static_cast<uint8_t>(std::clamp(b, 0.0f, 1.0f) * 255.0f);
                px[idx + 3] = 255;
            }
        }
        return Image::fromPixels(w, h, px).valueOr(nullptr);
    };

    auto makeAvatarImage = [](uint32_t s) {
        std::vector<uint8_t> px(static_cast<size_t>(s) * s * 4u);
        for (uint32_t y = 0; y < s; ++y) {
            float fy = static_cast<float>(y) / static_cast<float>(s - 1);
            for (uint32_t x = 0; x < s; ++x) {
                float fx = static_cast<float>(x) / static_cast<float>(s - 1);
                float r = 0.15f + 0.45f * fx;
                float g = 0.42f + 0.35f * (1.0f - fy);
                float b = 0.88f - 0.25f * fx;
                size_t idx = (static_cast<size_t>(y) * s + x) * 4u;
                px[idx + 0] = static_cast<uint8_t>(r * 255.0f);
                px[idx + 1] = static_cast<uint8_t>(g * 255.0f);
                px[idx + 2] = static_cast<uint8_t>(b * 255.0f);
                px[idx + 3] = 255;
            }
        }
        return Image::fromPixels(s, s, px).valueOr(nullptr);
    };

    auto samplePhoto = makeSunsetImage(320, 180);
    auto samplePortrait = makeAvatarImage(96);

    auto mediaColumns = std::make_unique<Container>();
    mediaColumns->setFlexDirection(FlexDirection::Row);
    mediaColumns->setAlignItems(AlignItems::FlexStart);
    mediaColumns->setColumnGap(32.0f * dpi);
    mediaColumns->style().width = Length::percent(100.0f);

    auto imgCol = std::make_unique<Container>();
    imgCol->setFlexDirection(FlexDirection::Column);
    imgCol->setAlignItems(AlignItems::FlexStart);
    imgCol->setRowGap(12.0f * dpi);
    imgCol->style().width = Length::points(340.0f * dpi);

    auto fitSegmented = std::make_unique<TabBar>();
    fitSegmented->setTabBarStyle(TabBarStyle::Segmented);
    fitSegmented->setTabHeight(30.0f * dpi);
    fitSegmented->style().width = Length::points(240.0f * dpi);
    fitSegmented->addTab("Cover");
    fitSegmented->addTab("Contain");
    fitSegmented->addTab("Fill");
    fitSegmented->setSelectedIndex(0);
    TabBar* fitPtr = fitSegmented.get();
    imgCol->addChild(std::move(fitSegmented));

    auto photoView = std::make_unique<ImageView>(samplePhoto, ImageFit::Cover);
    photoView->setCornerRadius(theme::Metrics::CornerRadiusCard * dpi);
    photoView->setShowBorder(true);
    photoView->style().width = Length::points(340.0f * dpi);
    photoView->style().height = Length::points(190.0f * dpi);
    ImageView* photoPtr = photoView.get();
    imgCol->addChild(std::move(photoView));

    fitPtr->onTabSelected.connect([photoPtr](int idx) {
        if (idx == 0) {
            photoPtr->setFit(ImageFit::Cover);
        } else if (idx == 1) {
            photoPtr->setFit(ImageFit::Contain);
        } else {
            photoPtr->setFit(ImageFit::Fill);
        }
    });

    auto metaCol = std::make_unique<Container>();
    metaCol->setFlexDirection(FlexDirection::Column);
    metaCol->setAlignItems(AlignItems::FlexStart);
    metaCol->setRowGap(14.0f * dpi);
    metaCol->style().width = Length::points(380.0f * dpi);

    auto avDivider = std::make_unique<Divider>("Avatars & Presence");
    avDivider->setLabelAlignment(DividerAlignment::Start);
    avDivider->setLabelOffset(0.0f);
    avDivider->style().width = Length::percent(100.0f);
    metaCol->addChild(std::move(avDivider));

    auto avatarRow = std::make_unique<Container>();
    avatarRow->setFlexDirection(FlexDirection::Row);
    avatarRow->setAlignItems(AlignItems::Center);
    avatarRow->setColumnGap(14.0f * dpi);

    auto av1 = std::make_unique<Avatar>(samplePortrait, "Maya Lin", AvatarSize::XLarge,
                                        AvatarShape::Circle);
    av1->setStatus(AvatarStatus::Online);
    auto av2 = std::make_unique<Avatar>("Liam Carter", AvatarSize::Large, AvatarShape::Circle);
    av2->setStatus(AvatarStatus::Busy);
    auto av3 = std::make_unique<Avatar>("Sofia Rossi", AvatarSize::Medium, AvatarShape::Rounded);
    av3->setStatus(AvatarStatus::Away);
    auto av4 = std::make_unique<Avatar>("Noah Patel", AvatarSize::Small, AvatarShape::Circle);
    av4->setStatus(AvatarStatus::Offline);
    avatarRow->addChild(std::move(av1));
    avatarRow->addChild(std::move(av2));
    avatarRow->addChild(std::move(av3));
    avatarRow->addChild(std::move(av4));
    metaCol->addChild(std::move(avatarRow));

    auto badgeDivider = std::make_unique<Divider>("Badges & Dismissible Tags");
    badgeDivider->setLabelAlignment(DividerAlignment::Start);
    badgeDivider->setLabelOffset(0.0f);
    badgeDivider->style().width = Length::percent(100.0f);
    metaCol->addChild(std::move(badgeDivider));

    auto badgeRow1 = std::make_unique<Container>();
    badgeRow1->setFlexDirection(FlexDirection::Row);
    badgeRow1->setAlignItems(AlignItems::Center);
    badgeRow1->setColumnGap(8.0f * dpi);
    auto bLive = std::make_unique<Badge>("Online", BadgeRole::Success, BadgeVariant::Subtle);
    bLive->setShowDot(true);
    badgeRow1->addChild(std::move(bLive));
    badgeRow1->addChild(
        std::make_unique<Badge>("Featured", BadgeRole::Accent, BadgeVariant::Solid));
    badgeRow1->addChild(
        std::make_unique<Badge>("Review", BadgeRole::Warning, BadgeVariant::Outline));
    badgeRow1->addChild(std::make_unique<Badge>("Draft", BadgeRole::Neutral, BadgeVariant::Subtle));
    metaCol->addChild(std::move(badgeRow1));

    auto tagRow = std::make_unique<Container>();
    tagRow->setFlexDirection(FlexDirection::Row);
    tagRow->setAlignItems(AlignItems::Center);
    tagRow->setColumnGap(8.0f * dpi);
    std::vector<Badge*> tagPtrs;
    for (const char* tagName : {"Travel", "Photography", "Design"}) {
        auto tag = std::make_unique<Badge>(tagName, BadgeRole::Info, BadgeVariant::Subtle);
        tag->setDismissible(true);
        Badge* rawTag = tag.get();
        rawTag->onDismissed.connect([rawTag]() { rawTag->setVisible(false); });
        tagPtrs.push_back(rawTag);
        tagRow->addChild(std::move(tag));
    }
    metaCol->addChild(std::move(tagRow));

    auto restoreTagsBtn = std::make_unique<Button>("Restore Tags", nullptr,
                                                   ButtonVariant::Secondary, ButtonSize::Compact);
    restoreTagsBtn->onClicked.connect([tagPtrs]() {
        for (Badge* t : tagPtrs) {
            t->setVisible(true);
        }
    });
    metaCol->addChild(std::move(restoreTagsBtn));

    mediaColumns->addChild(std::move(imgCol));
    mediaColumns->addChild(std::move(metaCol));
    mediaPage->addChild(std::move(mediaColumns));
    mediaScroll->addChild(std::move(mediaPage));
    tabView->addTab("Media & Tags", std::move(mediaScroll));

    root->addChild(std::move(tabView));

    uiContext.setRootWidget(std::move(root));
    uiContext.handleResize(core::Size{static_cast<float>(window->physicalSize().width),
                                      static_cast<float>(window->physicalSize().height)});

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
            uiContext.handleResize(core::Size{static_cast<float>(ev.physicalSize.width),
                                              static_cast<float>(ev.physicalSize.height)});
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
