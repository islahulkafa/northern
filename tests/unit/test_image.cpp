#include "renderer/image/image.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace northern;
using namespace northern::renderer;

TEST(ImageTest, ImageFromPixelsValidation) {
    std::vector<uint8_t> pixels(4 * 4 * 4, 255);
    auto imgRes = Image::fromPixels(4, 4, pixels);
    ASSERT_TRUE(imgRes.isOk());
    auto img = imgRes.value();
    EXPECT_TRUE(img->isValid());
    EXPECT_GT(img->id(), 0u);
    EXPECT_EQ(img->width(), 4u);
    EXPECT_EQ(img->height(), 4u);
    EXPECT_FLOAT_EQ(img->aspectRatio(), 1.0f);
    EXPECT_EQ(img->byteSize(), 64u);
    EXPECT_EQ(img->pixels().size(), 64u);

    auto img2Res = Image::fromPixels(4, 4, pixels);
    ASSERT_TRUE(img2Res.isOk());
    EXPECT_NE(img->id(), img2Res.value()->id());

    EXPECT_TRUE(Image::fromPixels(0, 4, pixels).isErr());
    EXPECT_TRUE(Image::fromPixels(4, 0, pixels).isErr());
    std::vector<uint8_t> tooSmall(10, 0);
    EXPECT_TRUE(Image::fromPixels(4, 4, tooSmall).isErr());
}

TEST(ImageTest, ImageFromMemoryWicBmpDecode) {
    // Minimal 2x2 32-bit BGRA uncompressed BMP (54-byte header + 16 bytes pixel data, bottom-up)
    std::vector<uint8_t> bmp = {
        'B',
        'M',
        70,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        54,
        0,
        0,
        0, // File header
        40,
        0,
        0,
        0,
        2,
        0,
        0,
        0,
        2,
        0,
        0,
        0,
        1,
        0, // DIB header (2x2)
        32,
        0,
        0,
        0,
        0,
        0,
        16,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        // Bottom row (y=1): Blue (BGRA: 255,0,0,255), Green (BGRA: 0,255,0,255)
        255,
        0,
        0,
        255,
        0,
        255,
        0,
        255,
        // Top row (y=0): Red (BGRA: 0,0,255,255), White (BGRA: 255,255,255,255)
        0,
        0,
        255,
        255,
        255,
        255,
        255,
        255,
    };

    auto res = Image::fromMemory(bmp);
    ASSERT_TRUE(res.isOk());
    auto img = res.value();
    EXPECT_EQ(img->width(), 2u);
    EXPECT_EQ(img->height(), 2u);
    ASSERT_EQ(img->pixels().size(), 16u);

    auto p = img->pixels();
    // Top-left (0,0) = Red RGBA(255,0,0,255)
    EXPECT_EQ(p[0], 255);
    EXPECT_EQ(p[1], 0);
    EXPECT_EQ(p[2], 0);
    EXPECT_EQ(p[3], 255);
    // Top-right (1,0) = White RGBA(255,255,255,255)
    EXPECT_EQ(p[4], 255);
    EXPECT_EQ(p[5], 255);
    EXPECT_EQ(p[6], 255);
    EXPECT_EQ(p[7], 255);
}

TEST(ImageTest, ImageFromFileErrorOnMissing) {
    auto res = Image::fromFile("nonexistent_image_path_12345.png");
    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().code, core::ErrorCode::NotFound);
    EXPECT_TRUE(Image::fromMemory({}).isErr());
}
