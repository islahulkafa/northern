#include "widgets/views/item_selection_model.hpp"
#include <gtest/gtest.h>

using namespace northern::widgets;

TEST(ItemSelectionModelTest, SingleSelectionMode) {
    StringListModel model({"A", "B", "C", "D"});
    ItemSelectionModel sel(&model);
    sel.setSelectionMode(SelectionMode::Single);

    EXPECT_FALSE(sel.isSelected(model.index(0, 0)));

    sel.select(model.index(1, 0), SelectionFlag::ClearAndSelect);
    EXPECT_TRUE(sel.isSelected(model.index(1, 0)));
    EXPECT_EQ(sel.selectedIndexes().size(), 1u);

    // Selecting another deselects the first
    sel.select(model.index(2, 0), SelectionFlag::ClearAndSelect);
    EXPECT_FALSE(sel.isSelected(model.index(1, 0)));
    EXPECT_TRUE(sel.isSelected(model.index(2, 0)));
    EXPECT_EQ(sel.selectedIndexes().size(), 1u);

    sel.clearSelection();
    EXPECT_FALSE(sel.isSelected(model.index(2, 0)));
    EXPECT_TRUE(sel.selectedIndexes().empty());
}

TEST(ItemSelectionModelTest, MultiSelectionToggleAndRange) {
    StringListModel model({"0", "1", "2", "3", "4"});
    ItemSelectionModel sel(&model);
    sel.setSelectionMode(SelectionMode::Multi);

    // Toggle 1 and 3
    sel.select(model.index(1, 0), SelectionFlag::Toggle);
    sel.select(model.index(3, 0), SelectionFlag::Toggle);
    EXPECT_TRUE(sel.isSelected(model.index(1, 0)));
    EXPECT_TRUE(sel.isSelected(model.index(3, 0)));
    EXPECT_FALSE(sel.isSelected(model.index(2, 0)));
    EXPECT_EQ(sel.selectedIndexes().size(), 2u);

    // Toggle 1 off
    sel.select(model.index(1, 0), SelectionFlag::Toggle);
    EXPECT_FALSE(sel.isSelected(model.index(1, 0)));
    EXPECT_TRUE(sel.isSelected(model.index(3, 0)));
    EXPECT_EQ(sel.selectedIndexes().size(), 1u);

    // Range select from 1 to 3
    sel.selectRange(model.index(1, 0), model.index(3, 0), SelectionFlag::ClearAndSelect);
    EXPECT_TRUE(sel.isSelected(model.index(1, 0)));
    EXPECT_TRUE(sel.isSelected(model.index(2, 0)));
    EXPECT_TRUE(sel.isSelected(model.index(3, 0)));
    EXPECT_FALSE(sel.isSelected(model.index(0, 0)));
    EXPECT_FALSE(sel.isSelected(model.index(4, 0)));

    // Select all
    sel.selectAll();
    EXPECT_EQ(sel.selectedIndexes().size(), 5u);
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(sel.isSelected(model.index(i, 0)));
    }
}

TEST(ItemSelectionModelTest, SelectionSignalsAndCurrentIndex) {
    StringListModel model({"X", "Y"});
    ItemSelectionModel sel(&model);

    int changeCount = 0;
    sel.onSelectionChanged.connect([&](const auto&, const auto&) { ++changeCount; });

    sel.select(model.index(0, 0));
    EXPECT_EQ(changeCount, 1);

    ModelIndex cur;
    sel.onCurrentChanged.connect([&](const ModelIndex& c, const ModelIndex&) { cur = c; });

    sel.setCurrentIndex(model.index(1, 0));
    EXPECT_EQ(sel.currentIndex(), model.index(1, 0));
    EXPECT_EQ(cur, model.index(1, 0));
}
