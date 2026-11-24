#pragma once

#include "core/math/geometry.hpp"
#include "text/font.hpp"
#include "text/types.hpp"
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace northern::text {

enum class TextAlignment : uint8_t { Left, Center, Right };

enum class WordWrap : uint8_t { None, Word };

enum class TextOverflow : uint8_t { Clip, Ellipsis };

struct TextLayoutOptions {
    float maxWidth{0.0f};
    TextAlignment alignment{TextAlignment::Left};
    WordWrap wrapMode{WordWrap::Word};
    TextOverflow overflow{TextOverflow::Clip};
    uint32_t maxLines{0}; // 0 = unlimited
    float lineHeightMultiplier{1.0f};
};

struct TextLine {
    core::Rect bounds{0.0f, 0.0f, 0.0f, 0.0f};
    float baseline{0.0f};
    std::vector<ShapedGlyph> glyphs;
};

class TextLayout {
public:
    static TextLayout create(std::string_view text, const std::shared_ptr<Font>& font,
                             float maxWidth = 0.0f, TextAlignment alignment = TextAlignment::Left);

    static TextLayout create(std::string_view text, const std::shared_ptr<Font>& font,
                             const TextLayoutOptions& options);

    [[nodiscard]] core::Size size() const noexcept { return size_; }
    [[nodiscard]] const std::vector<TextLine>& lines() const noexcept { return lines_; }
    [[nodiscard]] std::shared_ptr<Font> font() const noexcept { return font_; }
    [[nodiscard]] TextAlignment alignment() const noexcept { return alignment_; }
    [[nodiscard]] bool isTruncated() const noexcept { return truncated_; }

private:
    std::shared_ptr<Font> font_;
    core::Size size_{0.0f, 0.0f};
    TextAlignment alignment_{TextAlignment::Left};
    bool truncated_{false};
    std::vector<TextLine> lines_;
};

} // namespace northern::text
