#pragma once

#include "text/font.hpp"
#include "text/types.hpp"
#include <string_view>
#include <vector>

namespace northern::text {

class TextShaper {
public:
    [[nodiscard]] static std::vector<ShapedGlyph> shape(const Font& font,
                                                        std::string_view utf8Text);
};

} // namespace northern::text
