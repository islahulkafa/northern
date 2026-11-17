#include "core/math/color.hpp"
#include <gtest/gtest.h>

using namespace northern::core;

TEST(ColorTest, DefaultAndPredefinedConstants) {
    Color def{};
    EXPECT_FLOAT_EQ(def.r, 0.0f);
    EXPECT_FLOAT_EQ(def.g, 0.0f);
    EXPECT_FLOAT_EQ(def.b, 0.0f);
    EXPECT_FLOAT_EQ(def.a, 0.0f);

    Color white = Color::white();
    EXPECT_FLOAT_EQ(white.r, 1.0f);
    EXPECT_FLOAT_EQ(white.g, 1.0f);
    EXPECT_FLOAT_EQ(white.b, 1.0f);
    EXPECT_FLOAT_EQ(white.a, 1.0f);
}

TEST(ColorTest, FromRgba8AndPacking) {
    Color col = Color::fromRgba8(255, 128, 0, 255);
    EXPECT_NEAR(col.r, 1.0f, 0.01f);
    EXPECT_NEAR(col.g, 0.502f, 0.01f);
    EXPECT_NEAR(col.b, 0.0f, 0.01f);
    EXPECT_FLOAT_EQ(col.a, 1.0f);

    uint32_t packed = col.toRgba8();
    EXPECT_EQ((packed >> 24) & 0xFF, 255U);
    EXPECT_EQ((packed >> 16) & 0xFF, 128U);
    EXPECT_EQ((packed >> 8) & 0xFF, 0U);
    EXPECT_EQ(packed & 0xFF, 255U);
}

TEST(ColorTest, HexParsing) {
    auto res6 = Color::fromHex("#FF8000");
    ASSERT_TRUE(res6.isOk());
    EXPECT_NEAR(res6.value().r, 1.0f, 0.01f);
    EXPECT_NEAR(res6.value().g, 0.502f, 0.01f);
    EXPECT_NEAR(res6.value().b, 0.0f, 0.01f);
    EXPECT_FLOAT_EQ(res6.value().a, 1.0f);

    auto res8 = Color::fromHex("#FF800080");
    ASSERT_TRUE(res8.isOk());
    EXPECT_NEAR(res8.value().a, 0.502f, 0.01f);

    auto res3 = Color::fromHex("#F80");
    ASSERT_TRUE(res3.isOk());

    auto resInvalid = Color::fromHex("NotAHex");
    EXPECT_TRUE(resInvalid.isErr());
    EXPECT_EQ(resInvalid.error().code, ErrorCode::InvalidArgument);
}

TEST(ColorTest, Interpolation) {
    Color c1 = Color::black();
    Color c2 = Color::white();
    Color mid = c1.lerp(c2, 0.5f);

    EXPECT_NEAR(mid.r, 0.5f, 0.001f);
    EXPECT_NEAR(mid.g, 0.5f, 0.001f);
    EXPECT_NEAR(mid.b, 0.5f, 0.001f);
    EXPECT_FLOAT_EQ(mid.a, 1.0f);
}
