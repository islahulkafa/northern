#include "platform/file_dialog.hpp"
#include <gtest/gtest.h>

using namespace northern::platform;

TEST(FileDialogTest, DescAndFilterStructure) {
    FileDialogDesc desc;
    desc.title = "Open Document";
    desc.defaultPath = "C:/Docs";
    desc.defaultFilename = "notes.txt";
    desc.allowMultiSelect = true;
    desc.filters.push_back({"Text Documents (*.txt)", "*.txt"});
    desc.filters.push_back({"All Files (*.*)", "*.*"});

    EXPECT_EQ(desc.title, "Open Document");
    EXPECT_EQ(desc.defaultPath, "C:/Docs");
    EXPECT_EQ(desc.defaultFilename, "notes.txt");
    EXPECT_TRUE(desc.allowMultiSelect);
    EXPECT_EQ(desc.filters.size(), 2u);
    EXPECT_EQ(desc.filters[0].name, "Text Documents (*.txt)");
    EXPECT_EQ(desc.filters[0].pattern, "*.txt");
}
