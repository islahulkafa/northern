#include "renderer/paint_context.hpp"
#include "renderer/svg/svg_document.hpp"

namespace northern::renderer {

void PaintContext::drawRect(const core::Rect& rect, const core::Color& color) {
    DrawBoxCommand cmd{};
    cmd.rect = rect;
    cmd.fillColor = color;
    commands_.push_back(cmd);
}

void PaintContext::drawRoundedRect(const core::Rect& rect, const core::Color& color, float radius) {
    DrawBoxCommand cmd{};
    cmd.rect = rect;
    cmd.fillColor = color;
    cmd.cornerRadii[0] = radius;
    cmd.cornerRadii[1] = radius;
    cmd.cornerRadii[2] = radius;
    cmd.cornerRadii[3] = radius;
    commands_.push_back(cmd);
}

void PaintContext::drawRoundedRect(const core::Rect& rect, const core::Color& color,
                                   const float radii[4]) {
    DrawBoxCommand cmd{};
    cmd.rect = rect;
    cmd.fillColor = color;
    if (radii) {
        cmd.cornerRadii[0] = radii[0];
        cmd.cornerRadii[1] = radii[1];
        cmd.cornerRadii[2] = radii[2];
        cmd.cornerRadii[3] = radii[3];
    }
    commands_.push_back(cmd);
}

void PaintContext::drawBorder(const core::Rect& rect, const core::Color& color, float width,
                              float radius) {
    DrawBoxCommand cmd{};
    cmd.rect = rect;
    cmd.borderColor = color;
    cmd.borderWidth = width;
    cmd.cornerRadii[0] = radius;
    cmd.cornerRadii[1] = radius;
    cmd.cornerRadii[2] = radius;
    cmd.cornerRadii[3] = radius;
    commands_.push_back(cmd);
}

void PaintContext::drawBox(const core::Rect& rect, const core::Color& fillColor,
                           const core::Color& borderColor, float borderWidth,
                           const float radii[4]) {
    DrawBoxCommand cmd{};
    cmd.rect = rect;
    cmd.fillColor = fillColor;
    cmd.borderColor = borderColor;
    cmd.borderWidth = borderWidth;
    if (radii) {
        cmd.cornerRadii[0] = radii[0];
        cmd.cornerRadii[1] = radii[1];
        cmd.cornerRadii[2] = radii[2];
        cmd.cornerRadii[3] = radii[3];
    }
    commands_.push_back(cmd);
}

void PaintContext::drawLine(const core::Point& p0, const core::Point& p1, const core::Color& color,
                            float thickness) {
    DrawLineCommand cmd{};
    cmd.p0 = p0;
    cmd.p1 = p1;
    cmd.color = color;
    cmd.thickness = thickness;
    commands_.push_back(cmd);
}

void PaintContext::drawSvg(const SvgDocument& doc, const core::Rect& bounds,
                           const core::Color& tintColor) {
    doc.render(*this, bounds, tintColor);
}

void PaintContext::drawText(const core::Point& pos, std::string_view text,
                            const std::shared_ptr<text::Font>& font, const core::Color& color) {
    if (text.empty() || !font) {
        return;
    }
    auto layout = text::TextLayout::create(text, font);
    drawTextLayout(pos, layout, color);
}

void PaintContext::drawTextLayout(const core::Point& pos, const text::TextLayout& layout,
                                  const core::Color& color) {
    if (!layout.font()) {
        return;
    }
    DrawGlyphsCommand cmd;
    cmd.origin = pos;
    cmd.font = layout.font();
    cmd.color = color;

    size_t totalGlyphs = 0;
    for (const auto& line : layout.lines()) {
        totalGlyphs += line.glyphs.size();
    }
    cmd.glyphs.reserve(totalGlyphs);
    for (const auto& line : layout.lines()) {
        cmd.glyphs.insert(cmd.glyphs.end(), line.glyphs.begin(), line.glyphs.end());
    }

    if (!cmd.glyphs.empty()) {
        commands_.push_back(std::move(cmd));
    }
}

void PaintContext::pushClip(const core::Rect& rect) {
    PushClipCommand cmd{};
    cmd.clipRect = rect;
    commands_.push_back(cmd);
}

void PaintContext::popClip() {
    commands_.push_back(PopClipCommand{});
}

void PaintContext::append(const PaintContext& other) {
    commands_.insert(commands_.end(), other.commands_.begin(), other.commands_.end());
}

void PaintContext::clear() noexcept {
    commands_.clear();
}

} // namespace northern::renderer
