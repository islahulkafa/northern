#include "widgets/views/table_view.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::core;
using namespace northern::widgets;

TEST(TableViewTest, HeaderSyncAndCellIndexing) {
    StandardTableModel model({"ID", "Process", "CPU"});
    model.appendRow({"101", "Explorer", "1.2%"});
    model.appendRow({"102", "VulkanDriver", "8.5%"});

    TableView table(&model);
    table.setRowHeight(30.0f);
    table.horizontalHeader()->setSectionWidth(0, 50.0f);
    table.horizontalHeader()->setSectionWidth(1, 150.0f);
    table.horizontalHeader()->setSectionWidth(2, 80.0f);
    table.horizontalHeader()->setStretchLastSection(false);

    table.layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{400.0f, 300.0f}});
    table.horizontalHeader()->layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{400.0f, 28.0f}});

    EXPECT_EQ(table.horizontalHeader()->sectionCount(), 3u);
    EXPECT_EQ(table.horizontalHeader()->sectionTitle(0), "ID");
    EXPECT_EQ(table.horizontalHeader()->sectionTitle(1), "Process");

    // Hit test below header (header is 28px)
    // Row 0 is y in [28, 58). Column 1 is x in [50, 200).
    ModelIndex idx = table.indexAt(core::Point{75.0f, 40.0f});
    EXPECT_TRUE(idx.isValid());
    EXPECT_EQ(idx.row, 0);
    EXPECT_EQ(idx.column, 1);
    EXPECT_EQ(model.data(idx), "Explorer");

    // Row 1 is y in [58, 88). Column 2 is x in [200, 280).
    ModelIndex idx2 = table.indexAt(core::Point{210.0f, 65.0f});
    EXPECT_TRUE(idx2.isValid());
    EXPECT_EQ(idx2.row, 1);
    EXPECT_EQ(idx2.column, 2);
    EXPECT_EQ(model.data(idx2), "8.5%");
}

TEST(TableViewTest, CellClickSelection) {
    StandardTableModel model({"A", "B"});
    model.appendRow({"R0C0", "R0C1"});
    model.appendRow({"R1C0", "R1C1"});

    TableView table(&model);
    table.setRowHeight(30.0f);
    table.horizontalHeader()->setSectionWidth(0, 100.0f);
    table.horizontalHeader()->setSectionWidth(1, 100.0f);
    table.layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{300.0f, 200.0f}});
    table.horizontalHeader()->layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{300.0f, 28.0f}});

    ModelIndex clickedCell;
    table.onCellClicked.connect([&](const ModelIndex& idx) { clickedCell = idx; });

    // Click row 1, col 0 at (20, 45) -> y = 45 is row 0
    WidgetMouseEvent ev0{core::Point{20.0f, 45.0f}, northern::platform::MouseButton::Left,
                         northern::platform::KeyModifier::None};
    table.onMouseDown(ev0);
    EXPECT_EQ(clickedCell.row, 0);
    EXPECT_EQ(clickedCell.column, 0);

    // Click row 1 at (20, 70)
    WidgetMouseEvent ev1{core::Point{20.0f, 70.0f}, northern::platform::MouseButton::Left,
                         northern::platform::KeyModifier::None};
    table.onMouseDown(ev1);
    EXPECT_EQ(clickedCell.row, 1);
    EXPECT_EQ(clickedCell.column, 0);
    EXPECT_TRUE(table.selectionModel().isRowSelected(1));
}

TEST(TableViewTest, HorizontalScrollSyncsHeaderOffsetWithoutDisplacingHeaderWidget) {
    StandardTableModel model({"Col 0", "Col 1", "Col 2"});
    model.appendRow({"A", "B", "C"});

    TableView table(&model);
    table.setRowHeight(30.0f);
    table.horizontalHeader()->setSectionWidth(0, 150.0f);
    table.horizontalHeader()->setSectionWidth(1, 150.0f);
    table.horizontalHeader()->setSectionWidth(2, 150.0f);
    table.horizontalHeader()->setStretchLastSection(false);

    // View width 200, total columns width 450 -> maxScrollOffsetX = 250
    table.layoutNode().setLayoutRect(
        core::Rect{core::Point{100.0f, 50.0f}, core::Size{200.0f, 300.0f}});
    table.horizontalHeader()->layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{200.0f, 28.0f}});

    EXPECT_FLOAT_EQ(table.scrollOffsetX(), 0.0f);
    EXPECT_FLOAT_EQ(table.horizontalHeader()->horizontalOffset(), 0.0f);

    // Scroll by 80px horizontally
    table.scrollBy(80.0f, 0.0f);
    EXPECT_FLOAT_EQ(table.scrollOffsetX(), 80.0f);
    EXPECT_FLOAT_EQ(table.horizontalHeader()->horizontalOffset(), 80.0f);

    // HeaderView screenRect origin must remain anchored to TableView origin, NOT displaced by
    // scroll
    core::Rect headerScreen = table.horizontalHeader()->screenRect();
    EXPECT_FLOAT_EQ(headerScreen.origin.x, 100.0f);
    EXPECT_FLOAT_EQ(headerScreen.origin.y, 50.0f);
    EXPECT_FLOAT_EQ(headerScreen.size.width, 200.0f);
}

TEST(TableViewTest, ColumnSorting) {
    StandardTableModel model({"PID", "Module", "CPU", "Memory"});
    model.appendRow({"1002", "BatchRenderer", "1.2%", "48 MB"});
    model.appendRow({"1001", "VulkanGAL", "4.8%", "124 MB"});
    model.appendRow({"1003", "HarfBuzz", "0.1%", "32 MB"});

    TableView table(&model);
    table.setRowHeight(30.0f);
    table.horizontalHeader()->setSectionWidth(0, 80.0f);
    table.horizontalHeader()->setSectionWidth(1, 150.0f);
    table.horizontalHeader()->setSectionWidth(2, 80.0f);
    table.horizontalHeader()->setSectionWidth(3, 80.0f);
    table.layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{400.0f, 300.0f}});
    table.horizontalHeader()->layoutNode().setLayoutRect(
        core::Rect{core::Point{0.0f, 0.0f}, core::Size{400.0f, 28.0f}});

    EXPECT_EQ(table.toSourceRow(0), 0);
    EXPECT_EQ(table.toSourceRow(1), 1);
    EXPECT_EQ(table.toSourceRow(2), 2);

    // 1. Sort by PID (col 0) Ascending
    table.sortByColumn(0, SortOrder::Ascending);
    EXPECT_EQ(table.sortColumn(), 0);
    EXPECT_EQ(table.sortOrder(), SortOrder::Ascending);
    EXPECT_EQ(table.toSourceRow(0), 1); // "1001"
    EXPECT_EQ(table.toSourceRow(1), 0); // "1002"
    EXPECT_EQ(table.toSourceRow(2), 2); // "1003"

    // Verify indexAt maps visual row 0 to source row 1 ("1001")
    ModelIndex hitIdx = table.indexAt(core::Point{20.0f, 40.0f});
    EXPECT_EQ(hitIdx.row, 1);
    EXPECT_EQ(model.data(hitIdx), "1001");

    // 2. Sort by PID Descending
    table.sortByColumn(0, SortOrder::Descending);
    EXPECT_EQ(table.toSourceRow(0), 2); // "1003"
    EXPECT_EQ(table.toSourceRow(1), 0); // "1002"
    EXPECT_EQ(table.toSourceRow(2), 1); // "1001"

    // 3. Sort by CPU (col 2) Ascending: 0.1% (row 2) < 1.2% (row 0) < 4.8% (row 1)
    table.sortByColumn(2, SortOrder::Ascending);
    EXPECT_EQ(table.toSourceRow(0), 2); // "0.1%"
    EXPECT_EQ(table.toSourceRow(1), 0); // "1.2%"
    EXPECT_EQ(table.toSourceRow(2), 1); // "4.8%"

    // 4. Sort by Memory (col 3) Ascending: 32 MB (row 2) < 48 MB (row 0) < 124 MB (row 1)
    table.sortByColumn(3, SortOrder::Ascending);
    EXPECT_EQ(table.toSourceRow(0), 2); // "32 MB"
    EXPECT_EQ(table.toSourceRow(1), 0); // "48 MB"
    EXPECT_EQ(table.toSourceRow(2), 1); // "124 MB"

    // 5. Revert to SortOrder::None
    table.sortByColumn(3, SortOrder::None);
    EXPECT_EQ(table.toSourceRow(0), 0);
    EXPECT_EQ(table.toSourceRow(1), 1);
    EXPECT_EQ(table.toSourceRow(2), 2);

    // 6. Header click simulation (cycling SortOrder)
    // Click header col 1 ("Module")
    table.horizontalHeader()->setSectionSortOrder(1, SortOrder::Ascending);
    EXPECT_EQ(table.sortColumn(), 1);
    EXPECT_EQ(table.sortOrder(), SortOrder::Ascending);
    // Ascending: "BatchRenderer" (row 0) < "HarfBuzz" (row 2) < "VulkanGAL" (row 1)
    EXPECT_EQ(table.toSourceRow(0), 0);
    EXPECT_EQ(table.toSourceRow(1), 2);
    EXPECT_EQ(table.toSourceRow(2), 1);
}
