#include "renderer/paint_context.hpp"
#include "renderer/svg/svg_document.hpp"
#include <gtest/gtest.h>

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

    // Verified that draw commands (lines) were issued to PaintContext
    EXPECT_GT(ctx.commands().size(), 0u);
}
