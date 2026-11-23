#include "widgets/controls/divider.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(DividerTest, HorizontalDefaultLayoutAndThickness) {
    Divider div;
    EXPECT_EQ(div.orientation(), DividerOrientation::Horizontal);
    EXPECT_FLOAT_EQ(div.thickness(), 1.0f);
    EXPECT_TRUE(div.label().empty());
    EXPECT_FALSE(div.isFocusable());

    EXPECT_TRUE(div.style().width.isPercent());
    EXPECT_FLOAT_EQ(div.style().width.value, 100.0f);
    EXPECT_TRUE(div.style().height.isPoints());
    EXPECT_FLOAT_EQ(div.style().height.value, 1.0f);

    div.setThickness(2.0f);
    EXPECT_FLOAT_EQ(div.thickness(), 2.0f);
    EXPECT_FLOAT_EQ(div.style().height.value, 2.0f);
}

TEST(DividerTest, VerticalOrientationLayout) {
    Divider div(DividerOrientation::Vertical);
    EXPECT_EQ(div.orientation(), DividerOrientation::Vertical);
    EXPECT_FLOAT_EQ(div.thickness(), 1.0f);

    EXPECT_TRUE(div.style().height.isPercent());
    EXPECT_FLOAT_EQ(div.style().height.value, 100.0f);
    EXPECT_TRUE(div.style().width.isPoints());
    EXPECT_FLOAT_EQ(div.style().width.value, 1.0f);

    div.setThickness(3.0f);
    EXPECT_FLOAT_EQ(div.thickness(), 3.0f);
    EXPECT_FLOAT_EQ(div.style().width.value, 3.0f);

    div.setOrientation(DividerOrientation::Horizontal);
    EXPECT_EQ(div.orientation(), DividerOrientation::Horizontal);
    EXPECT_FLOAT_EQ(div.style().height.value, 3.0f);
}

TEST(DividerTest, LabeledDividerProperties) {
    Divider div("SECTION");
    EXPECT_EQ(div.label(), "SECTION");
    EXPECT_FLOAT_EQ(div.gap(), 8.0f);

    div.setLabel("NEW SECTION");
    EXPECT_EQ(div.label(), "NEW SECTION");

    div.setGap(12.0f);
    EXPECT_FLOAT_EQ(div.gap(), 12.0f);

    div.setColor(core::Color{1.0f, 0.0f, 0.0f, 1.0f});
    EXPECT_EQ(div.color(), (core::Color{1.0f, 0.0f, 0.0f, 1.0f}));

    div.setTextColor(core::Color{0.0f, 1.0f, 0.0f, 1.0f});
    EXPECT_EQ(div.textColor(), (core::Color{0.0f, 1.0f, 0.0f, 1.0f}));
}
