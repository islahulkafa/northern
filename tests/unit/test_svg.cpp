#include "renderer/paint_context.hpp"
#include "renderer/svg/svg_document.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace northern;
using namespace northern::renderer;

TEST(SvgTest, ParseValidSvgString) {
    std::string_view svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 "
        "24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" "
        "stroke-linejoin=\"round\"><line x1=\"4\" y1=\"12\" x2=\"20\" y2=\"12\"/></svg>";

    auto res = SvgDocument::fromString(svg);
    ASSERT_TRUE(res.isOk());

    auto doc = res.value();
    ASSERT_NE(doc, nullptr);
    EXPECT_TRUE(doc->isValid());
    EXPECT_GT(doc->id(), 0u);
    EXPECT_FLOAT_EQ(doc->size().width, 24.0f);
    EXPECT_FLOAT_EQ(doc->size().height, 24.0f);
}

TEST(SvgTest, ParseInvalidSvgString) {
    auto resEmpty = SvgDocument::fromString("");
    EXPECT_TRUE(resEmpty.isErr());

    auto resInvalid = SvgDocument::fromString("not-valid-xml-at-all");
    EXPECT_TRUE(resInvalid.isErr());
}

TEST(SvgTest, RenderDecomposesToPaintContext) {
    std::string_view svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 "
        "24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\"><line x1=\"0\" y1=\"0\" "
        "x2=\"24\" y2=\"24\"/></svg>";

    auto res = SvgDocument::fromString(svg);
    ASSERT_TRUE(res.isOk());

    PaintContext ctx;
    EXPECT_EQ(ctx.commands().size(), 0u);

    core::Rect targetRect{core::Point{10.0f, 10.0f}, core::Size{48.0f, 48.0f}};
    ctx.drawSvg(*res.value(), targetRect, core::Color::white());

    ASSERT_EQ(ctx.commands().size(), 1u);
    EXPECT_TRUE(std::holds_alternative<DrawSvgCommand>(ctx.commands()[0]));
}

TEST(SvgTest, CircleAnalyticalCoverageAndSymmetry) {
    std::string_view svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 "
        "24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\"><circle cx=\"12\" cy=\"12\" "
        "r=\"9\"/></svg>";

    auto res = SvgDocument::fromString(svg);
    ASSERT_TRUE(res.isOk());

    constexpr uint32_t kSize = 24;
    constexpr uint32_t kPad = 1;
    constexpr uint32_t kStride = kSize + kPad * 2;
    std::vector<uint8_t> mask(kStride * kStride, 0);

    auto rastRes = res.value()->rasterize(kSize, kSize, kPad, mask);
    ASSERT_TRUE(rastRes.isOk());

    uint32_t partialAaCount = 0;
    uint32_t solidCount = 0;
    for (uint32_t y = 0; y < kStride; ++y) {
        for (uint32_t x = 0; x < kStride; ++x) {
            uint8_t v = mask[y * kStride + x];
            uint8_t mirrorX = mask[y * kStride + (kStride - 1 - x)];
            uint8_t mirrorY = mask[(kStride - 1 - y) * kStride + x];
            EXPECT_NEAR(static_cast<int>(v), static_cast<int>(mirrorX), 6);
            EXPECT_NEAR(static_cast<int>(v), static_cast<int>(mirrorY), 6);

            if (v == 255) {
                ++solidCount;
            } else if (v > 0) {
                ++partialAaCount;
            }
        }
    }

    EXPECT_GT(solidCount, 20u);
    EXPECT_GT(partialAaCount, 40u);

    // Top outer arc row (y = 3) must rise monotonically toward the apex and fall monotonically
    // without non-monotonic capsule-joint beading
    for (uint32_t x = 8; x < 12; ++x) {
        EXPECT_LE(mask[3 * kStride + x], mask[3 * kStride + x + 1]);
    }
    for (uint32_t x = 13; x < 17; ++x) {
        EXPECT_GE(mask[3 * kStride + x], mask[3 * kStride + x + 1]);
    }
}

TEST(SvgTest, DotAndFilledShapeRasterization) {
    std::string_view dotSvg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 "
        "24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
        "stroke-linecap=\"round\"><line x1=\"12\" y1=\"8\" x2=\"12.01\" y2=\"8\"/></svg>";

    auto dotRes = SvgDocument::fromString(dotSvg);
    ASSERT_TRUE(dotRes.isOk());

    std::vector<uint8_t> dotMask(26 * 26, 0);
    ASSERT_TRUE(dotRes.value()->rasterize(24, 24, 1, dotMask).isOk());
    EXPECT_GT(dotMask[(8 + 1) * 26 + (12 + 1)], 180u);

    std::string_view fillSvg =
        "<svg viewBox=\"0 0 24 24\"><rect x=\"4\" y=\"4\" width=\"16\" height=\"16\"/></svg>";
    auto fillRes = SvgDocument::fromString(fillSvg);
    ASSERT_TRUE(fillRes.isOk());

    std::vector<uint8_t> fillMask(26 * 26, 0);
    ASSERT_TRUE(fillRes.value()->rasterize(24, 24, 1, fillMask).isOk());
    EXPECT_EQ(fillMask[13 * 26 + 13], 255u);
    EXPECT_EQ(fillMask[1 * 26 + 1], 0u);
}
