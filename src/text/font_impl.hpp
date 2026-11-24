#pragma once

#include "text/font.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include <hb-ft.h>
#include <hb.h>
#include <vector>

namespace northern::text {

struct Font::Impl {
    uint32_t id{0};
    FT_Face ftFace{nullptr};
    hb_font_t* hbFont{nullptr};
    float pointSize{14.0f};
    float pixelSize{14.0f};
    float ascender{0.0f};
    float descender{0.0f};
    float lineHeight{0.0f};
    float capHeight{0.0f};
    std::vector<uint8_t> memoryBuffer;

    ~Impl() {
        if (hbFont) {
            hb_font_destroy(hbFont);
            hbFont = nullptr;
        }
        if (ftFace) {
            FT_Done_Face(ftFace);
            ftFace = nullptr;
        }
    }
};

} // namespace northern::text
