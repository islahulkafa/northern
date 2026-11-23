#include "widgets/controls/icon.hpp"
#include "widgets/controls/icons.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(IconTest, IconCatalogCoverage) {
    // Verify all IconIds return valid non-empty SVG strings
    std::vector<IconId> allIcons = {
        IconId::WindowMinimize, IconId::WindowMaximize, IconId::WindowRestore,  IconId::WindowClose,
        IconId::AppLogo,        IconId::ChevronDown,    IconId::ChevronRight,   IconId::ChevronUp,
        IconId::ChevronLeft,    IconId::SortAscending,  IconId::SortDescending, IconId::DialogInfo,
        IconId::DialogWarning,  IconId::DialogConfirm,  IconId::DialogError,    IconId::Search,
        IconId::Folder,         IconId::FolderOpen,     IconId::File,           IconId::Check,
        IconId::Close,          IconId::Plus,           IconId::Minus,          IconId::Settings,
        IconId::Refresh,        IconId::Trash};

    for (IconId id : allIcons) {
        std::string_view svg = getSvgString(id);
        EXPECT_FALSE(svg.empty());
        EXPECT_NE(svg.find("<svg"), std::string_view::npos);

        auto doc = getSvgDocument(id);
        ASSERT_NE(doc, nullptr);
        EXPECT_TRUE(doc->isValid());
        EXPECT_FLOAT_EQ(doc->size().width, 24.0f);
        EXPECT_FLOAT_EQ(doc->size().height, 24.0f);
    }
}

TEST(IconTest, IconWidgetProperties) {
    Icon icon(IconId::Search, 20.0f, core::Color::fromRgba8(255, 0, 0));
    EXPECT_EQ(icon.iconId(), IconId::Search);
    EXPECT_FLOAT_EQ(icon.iconSize(), 20.0f);
    EXPECT_EQ(icon.color(), core::Color::fromRgba8(255, 0, 0));

    icon.setIcon(IconId::Folder);
    EXPECT_EQ(icon.iconId(), IconId::Folder);

    icon.setIconSize(32.0f);
    EXPECT_FLOAT_EQ(icon.iconSize(), 32.0f);
    EXPECT_FLOAT_EQ(icon.style().width.value, 32.0f);
    EXPECT_FLOAT_EQ(icon.style().height.value, 32.0f);

    icon.setColor(core::Color::white());
    EXPECT_EQ(icon.color(), core::Color::white());
}
