#include "core/math/geometry.hpp"
#include <gtest/gtest.h>

using namespace northern::core;

TEST(GeometryTest, PointArithmetic) {
    Point p1{10.0f, 20.0f};
    Point p2{5.0f, 15.0f};

    Point sum = p1 + p2;
    EXPECT_FLOAT_EQ(sum.x, 15.0f);
    EXPECT_FLOAT_EQ(sum.y, 35.0f);

    Point scaled = p1 * 2.0f;
    EXPECT_FLOAT_EQ(scaled.x, 20.0f);
    EXPECT_FLOAT_EQ(scaled.y, 40.0f);

    EXPECT_FLOAT_EQ(p1.dot(p2), 10.0f * 5.0f + 20.0f * 15.0f);
}

TEST(GeometryTest, SizeProperties) {
    Size s{100.0f, 50.0f};
    EXPECT_FALSE(s.isEmpty());
    EXPECT_FLOAT_EQ(s.area(), 5000.0f);
    EXPECT_FLOAT_EQ(s.aspectRatio(), 2.0f);

    Size emptySize{0.0f, 50.0f};
    EXPECT_TRUE(emptySize.isEmpty());
}

TEST(GeometryTest, RectContainmentAndIntersection) {
    Rect r1{0.0f, 0.0f, 100.0f, 100.0f};
    Point inside{50.0f, 50.0f};
    Point outside{150.0f, 50.0f};

    EXPECT_TRUE(r1.contains(inside));
    EXPECT_FALSE(r1.contains(outside));

    Rect r2{50.0f, 50.0f, 100.0f, 100.0f};
    EXPECT_TRUE(r1.intersects(r2));

    Rect intersection = r1.intersect(r2);
    EXPECT_FLOAT_EQ(intersection.x(), 50.0f);
    EXPECT_FLOAT_EQ(intersection.y(), 50.0f);
    EXPECT_FLOAT_EQ(intersection.width(), 50.0f);
    EXPECT_FLOAT_EQ(intersection.height(), 50.0f);

    Rect r3{200.0f, 200.0f, 50.0f, 50.0f};
    EXPECT_FALSE(r1.intersects(r3));
    EXPECT_TRUE(r1.intersect(r3).isEmpty());
}

TEST(GeometryTest, RectInsetsAndOutsets) {
    Rect r{10.0f, 10.0f, 100.0f, 80.0f};
    Insets insets{5.0f, 10.0f, 5.0f, 10.0f}; // top, right, bottom, left

    Rect insetRect = r.inset(insets);
    EXPECT_FLOAT_EQ(insetRect.x(), 20.0f);
    EXPECT_FLOAT_EQ(insetRect.y(), 15.0f);
    EXPECT_FLOAT_EQ(insetRect.width(), 80.0f);
    EXPECT_FLOAT_EQ(insetRect.height(), 70.0f);
}
