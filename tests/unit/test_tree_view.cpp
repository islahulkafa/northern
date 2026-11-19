#include "widgets/tree_view.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::core;
using namespace northern::widgets;

TEST(TreeViewTest, VisibleRowFlatteningAndExpansion) {
    StandardTreeModel model({"Name", "Type"});
    auto* root = model.rootItem();

    auto folder1 =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"Folder 1", "Directory"});
    auto* f1Ptr = root->appendChild(std::move(folder1));

    auto file1 = std::make_unique<StandardTreeItem>(std::vector<std::string>{"file_a.txt", "Text"});
    auto file2 =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"file_b.png", "Image"});
    f1Ptr->appendChild(std::move(file1));
    f1Ptr->appendChild(std::move(file2));

    auto folder2 =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"Folder 2", "Directory"});
    root->appendChild(std::move(folder2));

    TreeView tree(&model);
    tree.setRowHeight(25.0f);
    tree.layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{300.0f, 300.0f}});
    tree.header()->layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{300.0f, 28.0f}});

    // Initially collapsed: visible rows are Folder 1 and Folder 2 (2 rows)
    EXPECT_EQ(tree.visibleRowAt(35.0f), 0);
    EXPECT_EQ(tree.visibleRowAt(60.0f), 1);
    EXPECT_EQ(tree.visibleRowAt(85.0f), -1);

    ModelIndex idxF1 = model.index(0, 0);
    EXPECT_FALSE(tree.isExpanded(idxF1));

    // Expand Folder 1
    tree.expand(idxF1);
    EXPECT_TRUE(tree.isExpanded(idxF1));

    // Now visible rows: Folder 1 (0), file_a.txt (1), file_b.png (2), Folder 2 (3) (4 rows total)
    EXPECT_EQ(tree.visibleRowAt(35.0f), 0);
    EXPECT_EQ(tree.visibleRowAt(60.0f), 1);
    EXPECT_EQ(tree.visibleRowAt(85.0f), 2);
    EXPECT_EQ(tree.visibleRowAt(110.0f), 3);

    ModelIndex idxRow1 = tree.indexAt(core::Point{50.0f, 60.0f});
    EXPECT_EQ(model.data(idxRow1), "file_a.txt");

    // Collapse Folder 1
    tree.collapse(idxF1);
    EXPECT_FALSE(tree.isExpanded(idxF1));
    EXPECT_EQ(tree.visibleRowAt(60.0f), 1);
    ModelIndex idxRow1After = tree.indexAt(core::Point{50.0f, 60.0f});
    EXPECT_EQ(model.data(idxRow1After), "Folder 2");
}

TEST(TreeViewTest, ChevronClickToggle) {
    StandardTreeModel model({"Name"});
    auto* root = model.rootItem();
    auto folder = std::make_unique<StandardTreeItem>(std::vector<std::string>{"Assets"});
    folder->appendChild(std::make_unique<StandardTreeItem>(std::vector<std::string>{"mesh.obj"}));
    root->appendChild(std::move(folder));

    TreeView tree(&model);
    tree.setRowHeight(30.0f);
    tree.layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{300.0f, 200.0f}});
    tree.header()->layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{300.0f, 28.0f}});

    ModelIndex assetsIdx = model.index(0, 0);
    EXPECT_FALSE(tree.isExpanded(assetsIdx));

    // Click on chevron in column 0 (depth 0, indent is 0, chevron is x in [0, 20], y in [28, 58])
    WidgetMouseEvent evClickChevron{core::Point{10.0f, 40.0f},
                                    northern::platform::MouseButton::Left,
                                    northern::platform::KeyModifier::None};
    tree.onMouseDown(evClickChevron);

    EXPECT_TRUE(tree.isExpanded(assetsIdx));

    // Click again to collapse
    tree.onMouseDown(evClickChevron);
    EXPECT_FALSE(tree.isExpanded(assetsIdx));
}
