#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/views/item_delegate.hpp"
#include "widgets/views/item_model.hpp"
#include "widgets/views/item_selection_model.hpp"
#include "widgets/views/list_view.hpp"
#include "widgets/views/table_view.hpp"
#include "widgets/views/tree_view.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::core;
using namespace northern::widgets;

TEST(CollectionViewsTest, ItemDelegateOptionMetrics) {
    ItemDelegate delegate;
    StyleOptionViewItem opt;
    opt.bounds = core::Rect{core::Point{0.0f, 0.0f}, core::Size{200.0f, 30.0f}};
    opt.text = "Sample Item";
    opt.badgeText = "Active";
    opt.showCheckbox = true;
    opt.checkboxChecked = true;
    opt.dpiScale = 1.0f;

    core::Size size = delegate.sizeHint(opt, ModelIndex{});
    EXPECT_GT(size.width, 0.0f);
    EXPECT_GE(size.height, 28.0f);
}

TEST(CollectionViewsTest, ListViewShiftRangeSelectionAndCtrlA) {
    StringListModel model;
    for (int i = 0; i < 20; ++i) {
        model.append("Row " + std::to_string(i));
    }

    ListView list(&model);
    list.selectionModel().setSelectionMode(SelectionMode::Multi);
    list.setRowHeight(25.0f);
    list.layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{200.0f, 250.0f}});

    // Select row 2
    ModelIndex idx2 = model.index(2, 0);
    list.selectionModel().select(idx2, SelectionFlag::ClearAndSelect);
    list.selectionModel().setCurrentIndex(idx2);

    // Press Shift+Down three times
    WidgetKeyEvent shiftDown{northern::platform::Key::Down, true,
                             northern::platform::KeyModifier::Shift};
    list.onKeyDown(shiftDown);
    list.onKeyDown(shiftDown);
    list.onKeyDown(shiftDown);

    // Current row should be 5, and rows 2, 3, 4, 5 should be selected
    EXPECT_EQ(list.selectionModel().currentIndex().row, 5);
    EXPECT_TRUE(list.selectionModel().isSelected(model.index(2, 0)));
    EXPECT_TRUE(list.selectionModel().isSelected(model.index(3, 0)));
    EXPECT_TRUE(list.selectionModel().isSelected(model.index(4, 0)));
    EXPECT_TRUE(list.selectionModel().isSelected(model.index(5, 0)));
    EXPECT_FALSE(list.selectionModel().isSelected(model.index(1, 0)));
    EXPECT_FALSE(list.selectionModel().isSelected(model.index(6, 0)));

    // Ctrl+A select all
    WidgetKeyEvent ctrlA{northern::platform::Key::A, true,
                         northern::platform::KeyModifier::Control};
    list.onKeyDown(ctrlA);
    EXPECT_EQ(list.selectionModel().selectedIndexes().size(), 20u);
}

TEST(CollectionViewsTest, TableViewSortingAndPaging) {
    StandardTableModel model({"Name", "Score"});
    model.appendRow({"Charlie", "70"});
    model.appendRow({"Alice", "95"});
    model.appendRow({"Bob", "85"});

    TableView table(&model);
    table.setRowHeight(25.0f);
    table.horizontalHeader()->setSectionWidth(0, 100.0f);
    table.horizontalHeader()->setSectionWidth(1, 100.0f);
    table.layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{250.0f, 150.0f}});
    table.horizontalHeader()->layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{250.0f, 28.0f}});

    // Sort ascending by column 0 (Name)
    table.sortByColumn(0, SortOrder::Ascending);
    EXPECT_EQ(table.toSourceRow(0), 1); // Alice
    EXPECT_EQ(table.toSourceRow(1), 2); // Bob
    EXPECT_EQ(table.toSourceRow(2), 0); // Charlie

    // Sort descending by column 0 (Name)
    table.sortByColumn(0, SortOrder::Descending);
    EXPECT_EQ(table.toSourceRow(0), 0); // Charlie
    EXPECT_EQ(table.toSourceRow(1), 2); // Bob
    EXPECT_EQ(table.toSourceRow(2), 1); // Alice

    // Keyboard navigation with PageDown / PageUp
    table.selectionModel().setCurrentIndex(model.index(0, 0));
    WidgetKeyEvent pgDown{northern::platform::Key::PageDown, true,
                          northern::platform::KeyModifier::None};
    table.onKeyDown(pgDown);
    EXPECT_EQ(table.selectionModel().currentIndex().row, table.toSourceRow(2));
}

TEST(CollectionViewsTest, TreeViewKeyboardHierarchyNavigation) {
    StandardTreeModel model({"Hierarchy"});
    auto* root = model.rootItem();

    auto folder = std::make_unique<StandardTreeItem>(std::vector<std::string>{"Folder"});
    auto child1 = std::make_unique<StandardTreeItem>(std::vector<std::string>{"Child1"});
    auto child2 = std::make_unique<StandardTreeItem>(std::vector<std::string>{"Child2"});
    folder->appendChild(std::move(child1));
    folder->appendChild(std::move(child2));
    root->appendChild(std::move(folder));

    TreeView tree(&model);
    tree.setRowHeight(25.0f);
    tree.layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{300.0f, 200.0f}});
    tree.header()->layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{300.0f, 28.0f}});

    ModelIndex folderIdx = model.index(0, 0);
    tree.selectionModel().setCurrentIndex(folderIdx);
    tree.selectionModel().select(folderIdx);

    EXPECT_FALSE(tree.isExpanded(folderIdx));

    // Right arrow on collapsed folder expands it
    WidgetKeyEvent rightEv{northern::platform::Key::Right, true,
                           northern::platform::KeyModifier::None};
    tree.onKeyDown(rightEv);
    EXPECT_TRUE(tree.isExpanded(folderIdx));

    // Right arrow again moves to first child
    tree.onKeyDown(rightEv);
    EXPECT_EQ(model.data(tree.selectionModel().currentIndex()), "Child1");

    // Left arrow moves to parent
    WidgetKeyEvent leftEv{northern::platform::Key::Left, true,
                          northern::platform::KeyModifier::None};
    tree.onKeyDown(leftEv);
    EXPECT_EQ(tree.selectionModel().currentIndex(), folderIdx);

    // Left arrow again collapses folder
    tree.onKeyDown(leftEv);
    EXPECT_FALSE(tree.isExpanded(folderIdx));
}

TEST(CollectionViewsTest, DpiScalingPropagation) {
    StringListModel model({"Alpha", "Beta"});
    ListView list(&model);
    list.setDpiScale(1.5f);
    EXPECT_FLOAT_EQ(list.dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(list.rowHeight(), ListView::kDefaultRowHeight * 1.5f);

    StandardTableModel tableModel({"A", "B"});
    TableView table(&tableModel);
    table.setDpiScale(2.0f);
    EXPECT_FLOAT_EQ(table.dpiScale(), 2.0f);
    EXPECT_FLOAT_EQ(table.rowHeight(), TableView::kDefaultRowHeight * 2.0f);
    EXPECT_FLOAT_EQ(table.horizontalHeader()->dpiScale(), 2.0f);

    StandardTreeModel treeModel({"Tree"});
    TreeView tree(&treeModel);
    tree.setDpiScale(1.25f);
    EXPECT_FLOAT_EQ(tree.dpiScale(), 1.25f);
    EXPECT_FLOAT_EQ(tree.rowHeight(), TreeView::kDefaultRowHeight * 1.25f);
    EXPECT_FLOAT_EQ(tree.indentPerLevel(), TreeView::kDefaultIndentPerLevel * 1.25f);
}

TEST(CollectionViewsTest, ThemePaletteAndBorderTogglesAcrossViews) {
    WidgetContext ctx;
    ctx.setTheme(theme::Theme::light());
    auto root = std::make_unique<Container>();
    root->setFlexDirection(layout::FlexDirection::Column);

    StringListModel listModel({"Alpha", "Beta", "Gamma"});
    auto list = std::make_unique<ListView>(&listModel);
    list->setListStyle(ListStyle::Sidebar);
    list->selectionModel().select(listModel.index(0, 0), SelectionFlag::ClearAndSelect);
    list->style().height = layout::Length::points(100.0f);
    ListView* listPtr = list.get();
    root->addChild(std::move(list));

    StandardTableModel tableModel({"Name", "Value"});
    tableModel.appendRow({"Row1", "100"});
    auto table = std::make_unique<TableView>(&tableModel);
    EXPECT_TRUE(table->showBorder());
    table->setShowBorder(false);
    EXPECT_FALSE(table->showBorder());
    table->style().height = layout::Length::points(100.0f);
    TableView* tablePtr = table.get();
    root->addChild(std::move(table));

    StandardTreeModel treeModel({"Item"});
    treeModel.rootItem()->appendChild(
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"Node"}));
    auto tree = std::make_unique<TreeView>(&treeModel);
    EXPECT_TRUE(tree->showBorder());
    tree->setShowBorder(false);
    EXPECT_FALSE(tree->showBorder());
    tree->style().height = layout::Length::points(100.0f);
    TreeView* treePtr = tree.get();
    root->addChild(std::move(tree));

    ctx.setRootWidget(std::move(root));
    ctx.handleResize(core::Size{400.0f, 360.0f});

    renderer::PaintContext paintCtx;
    ctx.render(paintCtx, core::Size{400.0f, 360.0f});
    EXPECT_FALSE(paintCtx.commands().empty());
    EXPECT_EQ(listPtr->listStyle(), ListStyle::Sidebar);
    EXPECT_FALSE(tablePtr->showBorder());
    EXPECT_FALSE(treePtr->showBorder());
}

TEST(CollectionViewsTest, ContextDpiAdoptionCustomRowHeightThumbGrabCancelAndThemeRefresh) {
    // 1. ListView: contextDpiScale adoption, custom rowHeight preservation, thumb grab cancel
    {
        WidgetContext ctx;
        ctx.setDpiScale(1.5f);
        std::vector<std::string> items;
        for (int i = 0; i < 30; ++i) {
            items.push_back("Row " + std::to_string(i));
        }
        StringListModel model(std::move(items));

        auto owned = std::make_unique<ListView>(&model);
        ListView* list = owned.get();
        ctx.setRootWidget(std::move(owned));
        ctx.handleResize(Size{240.0f, 120.0f});

        EXPECT_FLOAT_EQ(list->dpiScale(), 1.5f);
        EXPECT_FLOAT_EQ(list->rowHeight(), ListView::kDefaultRowHeight * 1.5f);

        list->setRowHeight(45.0f); // 30.0f logical at 1.5x
        list->setDpiScale(2.0f);
        EXPECT_FLOAT_EQ(list->rowHeight(), 60.0f);

        // Start dragging vertical scrollbar thumb and cancel via disable
        float thumbX = 240.0f - 6.0f * 2.0f;
        WidgetMouseEvent downThumb{Point{thumbX, 10.0f}, platform::MouseButton::Left,
                                   platform::KeyModifier::None};
        list->onMouseDown(downThumb);
        EXPECT_EQ(ctx.mouseGrabWidget(), list);
        list->setEnabled(false);
        EXPECT_EQ(ctx.mouseGrabWidget(), nullptr);

        list->setEnabled(true);
        ctx.setTheme(theme::Theme::light());
        renderer::PaintContext pctx;
        list->paint(pctx);
        EXPECT_FALSE(pctx.commands().empty());
    }

    // 2. TableView: contextDpiScale adoption, custom rowHeight preservation, theme forward
    {
        WidgetContext ctx;
        ctx.setDpiScale(1.5f);
        StandardTableModel model({"Name", "Value"});
        for (int i = 0; i < 20; ++i) {
            model.appendRow({"Item " + std::to_string(i), "Val"});
        }

        auto owned = std::make_unique<TableView>(&model);
        TableView* table = owned.get();
        ctx.setRootWidget(std::move(owned));
        ctx.handleResize(Size{300.0f, 140.0f});

        EXPECT_FLOAT_EQ(table->dpiScale(), 1.5f);
        EXPECT_FLOAT_EQ(table->horizontalHeader()->dpiScale(), 1.5f);
        table->setRowHeight(45.0f);
        table->setDpiScale(2.0f);
        EXPECT_FLOAT_EQ(table->rowHeight(), 60.0f);

        ctx.setTheme(theme::Theme::light());
        renderer::PaintContext pctx;
        table->paint(pctx);
        EXPECT_FALSE(pctx.commands().empty());
    }

    // 3. TreeView: contextDpiScale adoption, custom rowHeight & indent preservation, theme forward
    {
        WidgetContext ctx;
        ctx.setDpiScale(1.5f);
        StandardTreeModel model({"Node"});
        model.rootItem()->appendChild(
            std::make_unique<StandardTreeItem>(std::vector<std::string>{"Root"}));

        auto owned = std::make_unique<TreeView>(&model);
        TreeView* tree = owned.get();
        ctx.setRootWidget(std::move(owned));
        ctx.handleResize(Size{300.0f, 140.0f});

        EXPECT_FLOAT_EQ(tree->dpiScale(), 1.5f);
        EXPECT_FLOAT_EQ(tree->header()->dpiScale(), 1.5f);
        tree->setRowHeight(45.0f);
        tree->setIndentPerLevel(30.0f);
        tree->setDpiScale(2.0f);
        EXPECT_FLOAT_EQ(tree->rowHeight(), 60.0f);
        EXPECT_FLOAT_EQ(tree->indentPerLevel(), 40.0f);

        ctx.setTheme(theme::Theme::light());
        renderer::PaintContext pctx;
        tree->paint(pctx);
        EXPECT_FALSE(pctx.commands().empty());
    }
}
