#pragma once

#include "core/math/color.hpp"
#include "core/math/geometry.hpp"
#include "renderer/svg/svg_document.hpp"
#include "text/font.hpp"
#include "text/types.hpp"
#include <memory>
#include <variant>
#include <vector>

namespace northern::renderer {

struct DrawBoxCommand {
    core::Rect rect;
    core::Color fillColor{0.0f, 0.0f, 0.0f, 0.0f};
    core::Color borderColor{0.0f, 0.0f, 0.0f, 0.0f};
    float borderWidth{0.0f};
    float cornerRadii[4]{0.0f, 0.0f, 0.0f, 0.0f};
};

struct DrawLineCommand {
    core::Point p0{0.0f, 0.0f};
    core::Point p1{0.0f, 0.0f};
    core::Color color{1.0f, 1.0f, 1.0f, 1.0f};
    float thickness{1.5f};
};

struct DrawGlyphsCommand {
    core::Point origin{0.0f, 0.0f};
    std::shared_ptr<text::Font> font;
    std::vector<text::ShapedGlyph> glyphs;
    core::Color color{1.0f, 1.0f, 1.0f, 1.0f};
};

struct DrawSvgCommand {
    std::shared_ptr<const SvgDocument::Impl> docImpl;
    core::Rect bounds;
    core::Color tintColor{1.0f, 1.0f, 1.0f, 1.0f};
};

struct PushClipCommand {
    core::Rect clipRect;
};

struct PopClipCommand {};

using DrawCommand = std::variant<DrawBoxCommand, DrawLineCommand, DrawGlyphsCommand, DrawSvgCommand,
                                 PushClipCommand, PopClipCommand>;

} // namespace northern::renderer
