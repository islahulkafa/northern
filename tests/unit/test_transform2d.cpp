#include "core/math/math_utils.hpp"
#include "core/math/transform2d.hpp"
#include <gtest/gtest.h>

using namespace northern::core;

TEST(MathUtilsTest, ClampAndLerp) {
    EXPECT_EQ(clamp(5, 0, 10), 5);
    EXPECT_EQ(clamp(-5, 0, 10), 0);
    EXPECT_EQ(clamp(15, 0, 10), 10);

    EXPECT_FLOAT_EQ(lerp(0.0f, 100.0f, 0.5f), 50.0f);
    EXPECT_FLOAT_EQ(lerp(10.0f, 20.0f, 0.25f), 12.5f);
}

TEST(MathUtilsTest, SmoothstepAndAngleConversions) {
    EXPECT_FLOAT_EQ(smoothstep(0.0f, 1.0f, -0.1f), 0.0f);
    EXPECT_FLOAT_EQ(smoothstep(0.0f, 1.0f, 1.1f), 1.0f);
    EXPECT_FLOAT_EQ(smoothstep(0.0f, 1.0f, 0.5f), 0.5f);

    EXPECT_NEAR(radians(180.0f), kPi, 1e-5f);
    EXPECT_NEAR(degrees(kPi), 180.0f, 1e-4f);

    EXPECT_TRUE(approxEqual(1.000001f, 1.000002f, 1e-4f));
    EXPECT_FALSE(approxEqual(1.0f, 1.01f, 1e-3f));

    EXPECT_EQ(sign(-42), -1);
    EXPECT_EQ(sign(0), 0);
    EXPECT_EQ(sign(42), 1);
}

TEST(Transform2DTest, IdentityAndTranslation) {
    auto id = Transform2D::identity();
    Point p{10.0f, 20.0f};
    Point pTransformed = id.transformPoint(p);
    EXPECT_FLOAT_EQ(pTransformed.x, 10.0f);
    EXPECT_FLOAT_EQ(pTransformed.y, 20.0f);

    auto t = Transform2D::translation(5.0f, -10.0f);
    Point p2 = t.transformPoint(p);
    EXPECT_FLOAT_EQ(p2.x, 15.0f);
    EXPECT_FLOAT_EQ(p2.y, 10.0f);
}

TEST(Transform2DTest, ScaleAndRotation) {
    auto s = Transform2D::scale(2.0f, 3.0f);
    Point p{10.0f, 10.0f};
    Point pScaled = s.transformPoint(p);
    EXPECT_FLOAT_EQ(pScaled.x, 20.0f);
    EXPECT_FLOAT_EQ(pScaled.y, 30.0f);

    auto r = Transform2D::rotation(kHalfPi);
    Point pRot = r.transformPoint(Point{1.0f, 0.0f});
    EXPECT_NEAR(pRot.x, 0.0f, 1e-5f);
    EXPECT_NEAR(pRot.y, 1.0f, 1e-5f);
}

TEST(Transform2DTest, CompositionAndInvert) {
    auto t = Transform2D::translation(10.0f, 20.0f);
    auto s = Transform2D::scale(2.0f, 2.0f);
    // Multiply: s * t applies translation then scale
    auto combined = s * t;
    Point p = combined.transformPoint(Point{5.0f, 5.0f});
    EXPECT_FLOAT_EQ(p.x, 30.0f); // (5 + 10) * 2 = 30
    EXPECT_FLOAT_EQ(p.y, 50.0f); // (5 + 20) * 2 = 50

    auto invOpt = combined.inverted();
    ASSERT_TRUE(invOpt.has_value());
    Point pOriginal = invOpt->transformPoint(p);
    EXPECT_NEAR(pOriginal.x, 5.0f, 1e-4f);
    EXPECT_NEAR(pOriginal.y, 5.0f, 1e-4f);
}

TEST(Transform2DTest, TransformRect) {
    Rect orig{{10.0f, 20.0f}, {30.0f, 40.0f}};
    auto t = Transform2D::translation(5.0f, 10.0f);
    Rect moved = t.transformRect(orig);
    EXPECT_FLOAT_EQ(moved.origin.x, 15.0f);
    EXPECT_FLOAT_EQ(moved.origin.y, 30.0f);
    EXPECT_FLOAT_EQ(moved.size.width, 30.0f);
    EXPECT_FLOAT_EQ(moved.size.height, 40.0f);

    auto rot = Transform2D::rotation(kHalfPi);
    Rect r2{{0.0f, 0.0f}, {10.0f, 20.0f}};
    Rect rotRect = rot.transformRect(r2);
    // Rotating 90 deg: (0,0)->(0,0), (10,0)->(0,10), (10,20)->(-20,10), (0,20)->(-20,0)
    EXPECT_NEAR(rotRect.origin.x, -20.0f, 1e-4f);
    EXPECT_NEAR(rotRect.origin.y, 0.0f, 1e-4f);
    EXPECT_NEAR(rotRect.size.width, 20.0f, 1e-4f);
    EXPECT_NEAR(rotRect.size.height, 10.0f, 1e-4f);
}
