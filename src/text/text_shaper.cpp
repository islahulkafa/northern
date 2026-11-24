#include "text/text_shaper.hpp"
#include "text/font_impl.hpp"

namespace northern::text {

namespace {

struct ThreadHbBuffer {
    hb_buffer_t* buf{nullptr};

    ThreadHbBuffer() noexcept : buf(hb_buffer_create()) {}
    ~ThreadHbBuffer() {
        if (buf) {
            hb_buffer_destroy(buf);
            buf = nullptr;
        }
    }

    ThreadHbBuffer(const ThreadHbBuffer&) = delete;
    ThreadHbBuffer& operator=(const ThreadHbBuffer&) = delete;
};

hb_buffer_t* getReusableHbBuffer() noexcept {
    thread_local ThreadHbBuffer sBuffer;
    if (sBuffer.buf) {
        hb_buffer_clear_contents(sBuffer.buf);
    }
    return sBuffer.buf;
}

} // namespace

std::vector<ShapedGlyph> TextShaper::shape(const Font& font, std::string_view utf8Text) {
    std::vector<ShapedGlyph> result;
    if (utf8Text.empty() || !font.impl() || !font.impl()->hbFont) {
        return result;
    }

    hb_buffer_t* buf = getReusableHbBuffer();
    if (!buf) {
        return result;
    }
    hb_buffer_add_utf8(buf, utf8Text.data(), static_cast<int>(utf8Text.size()), 0,
                       static_cast<int>(utf8Text.size()));
    hb_buffer_guess_segment_properties(buf);

    hb_shape(font.impl()->hbFont, buf, nullptr, 0);

    unsigned int glyphCount = 0;
    hb_glyph_info_t* glyphInfo = hb_buffer_get_glyph_infos(buf, &glyphCount);
    hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions(buf, &glyphCount);

    result.reserve(glyphCount);
    float currentX = 0.0f;
    float currentY = 0.0f;

    for (unsigned int i = 0; i < glyphCount; ++i) {
        ShapedGlyph sg{};
        sg.glyphId = glyphInfo[i].codepoint;
        sg.cluster = glyphInfo[i].cluster;

        float xOffset = static_cast<float>(glyphPos[i].x_offset) / 64.0f;
        float yOffset = static_cast<float>(glyphPos[i].y_offset) / 64.0f;
        float xAdvance = static_cast<float>(glyphPos[i].x_advance) / 64.0f;
        float yAdvance = static_cast<float>(glyphPos[i].y_advance) / 64.0f;

        sg.position = {currentX + xOffset, currentY - yOffset};
        sg.advance = {xAdvance, yAdvance};

        currentX += xAdvance;
        currentY += yAdvance;

        result.push_back(sg);
    }

    return result;
}

} // namespace northern::text
