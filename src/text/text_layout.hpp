#pragma once

#include "core/math/geometry.hpp"
#include "text/font.hpp"
#include "text/types.hpp"
#include <memory>
#include <string_view>
#include <vector>

namespace northern::text {

enum class TextAlignment { Left, Center, Right };

struct TextLine {
    core::Rect bounds{0.0f, 0.0f, 0.0f, 0.0f};
    float baseline{0.0f};
    std::vector<ShapedGlyph> glyphs;
};

class TextLayout {
public:
    static TextLayout create(std::string_view text, const std::shared_ptr<Font>& font,
                             float maxWidth = 0.0f, TextAlignment alignment = TextAlignment::Left);

    [[nodiscard]] core::Size size() const noexcept { return size_; }
    [[nodiscard]] const std::vector<TextLine>& lines() const noexcept { return lines_; }
    [[nodiscard]] std::shared_ptr<Font> font() const noexcept { return font_; }
    [[nodiscard]] TextAlignment alignment() const noexcept { return alignment_; }

private:
    std::shared_ptr<Font> font_;
    core::Size size_{0.0f, 0.0f};
    TextAlignment alignment_{TextAlignment::Left};
    std::vector<TextLine> lines_;
};

} // namespace northern::text
