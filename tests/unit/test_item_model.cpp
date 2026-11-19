#include "widgets/item_model.hpp"
#include <gtest/gtest.h>

using namespace northern::widgets;

TEST(ItemModelTest, StringListModelOperations) {
    StringListModel model({"Alpha", "Beta", "Gamma"});
    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.columnCount(), 1);

    ModelIndex idx0 = model.index(0, 0);
    EXPECT_TRUE(idx0.isValid());
    EXPECT_EQ(model.data(idx0), "Alpha");

    ModelIndex idx1 = model.index(1, 0);
    EXPECT_EQ(model.data(idx1), "Beta");

    bool dataChanged = false;
    model.onDataChanged.connect([&](const ModelIndex&, const ModelIndex&) { dataChanged = true; });

    EXPECT_TRUE(model.setData(idx0, "Alpha Updated"));
    EXPECT_EQ(model.data(idx0), "Alpha Updated");
    EXPECT_TRUE(dataChanged);

    model.append("Delta");
    EXPECT_EQ(model.rowCount(), 4);
    EXPECT_EQ(model.data(model.index(3, 0)), "Delta");

    model.insert(1, "Inserted");
    EXPECT_EQ(model.rowCount(), 5);
    EXPECT_EQ(model.data(model.index(1, 0)), "Inserted");

    model.remove(1);
    EXPECT_EQ(model.rowCount(), 4);
    EXPECT_EQ(model.data(model.index(1, 0)), "Beta");

    model.clear();
    EXPECT_EQ(model.rowCount(), 0);
}

TEST(ItemModelTest, StandardTableModelOperations) {
    StandardTableModel model({"Name", "Role", "Department"});
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.columnCount(), 3);
    EXPECT_EQ(model.headerData(0), "Name");
    EXPECT_EQ(model.headerData(1), "Role");
    EXPECT_EQ(model.headerData(2), "Department");

    model.appendRow({"Alice", "Engineer", "Graphics"});
    model.appendRow({"Bob", "Designer", "UI"});
    EXPECT_EQ(model.rowCount(), 2);

    ModelIndex idx00 = model.index(0, 0);
    EXPECT_EQ(model.data(idx00), "Alice");

    ModelIndex idx11 = model.index(1, 1);
    EXPECT_EQ(model.data(idx11), "Designer");

    model.setCell(1, 2, "Core Systems");
    EXPECT_EQ(model.data(model.index(1, 2)), "Core Systems");

    model.insertRow(1, {"Charlie", "Manager", "Product"});
    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.data(model.index(1, 0)), "Charlie");

    model.removeRow(1);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.data(model.index(1, 0)), "Bob");
}

TEST(ItemModelTest, StandardTreeModelOperations) {
    StandardTreeModel model({"Name", "Size"});
    EXPECT_EQ(model.columnCount(), 2);
    EXPECT_EQ(model.headerData(0), "Name");
    EXPECT_EQ(model.headerData(1), "Size");

    auto* root = model.rootItem();
    auto folder1 = std::make_unique<StandardTreeItem>(std::vector<std::string>{"Folder 1", "--"});
    auto* f1Ptr = root->appendChild(std::move(folder1));

    auto file1 =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"file_a.txt", "12 KB"});
    auto file2 =
        std::make_unique<StandardTreeItem>(std::vector<std::string>{"file_b.png", "240 KB"});
    f1Ptr->appendChild(std::move(file1));
    f1Ptr->appendChild(std::move(file2));

    EXPECT_EQ(model.rowCount(), 1);

    ModelIndex idxF1 = model.index(0, 0);
    EXPECT_TRUE(idxF1.isValid());
    EXPECT_EQ(model.data(idxF1), "Folder 1");
    EXPECT_EQ(model.rowCount(idxF1), 2);

    ModelIndex idxFileA = model.index(0, 0, idxF1);
    EXPECT_TRUE(idxFileA.isValid());
    EXPECT_EQ(model.data(idxFileA), "file_a.txt");
    EXPECT_EQ(model.data(model.index(0, 1, idxF1)), "12 KB");

    ModelIndex parentIdx = model.parent(idxFileA);
    EXPECT_TRUE(parentIdx.isValid());
    EXPECT_EQ(parentIdx, idxF1);

    EXPECT_FALSE(model.parent(idxF1).isValid());
}
