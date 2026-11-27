#pragma once

#include "core/math/color.hpp"
#include "core/math/geometry.hpp"
#include "renderer/draw_command.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include <string_view>
#include <vector>

namespace northern::renderer {

class SvgDocument;
class Image;

class PaintContext {
public:
    void drawRect(const core::Rect& rect, const core::Color& color);
    void drawRoundedRect(const core::Rect& rect, const core::Color& color, float radius);
    void drawRoundedRect(const core::Rect& rect, const core::Color& color, const float radii[4]);
    void drawBorder(const core::Rect& rect, const core::Color& color, float width,
                    float radius = 0.0f);
    void drawBox(const core::Rect& rect, const core::Color& fillColor,
                 const core::Color& borderColor, float borderWidth, const float radii[4]);
    void drawLine(const core::Point& p0, const core::Point& p1, const core::Color& color,
                  float thickness = 1.5f);

    void drawSvg(const SvgDocument& doc, const core::Rect& bounds,
                 const core::Color& tintColor = core::Color::white());

    void drawImage(const Image& image, const core::Rect& rect, float cornerRadius = 0.0f,
                   const core::Color& tintColor = core::Color::white(),
                   graphics::Filter filter = graphics::Filter::Linear);

    void drawImage(const Image& image, const core::Rect& rect, const core::Rect& uvRect,
                   const float radii[4], const core::Color& borderColor, float borderWidth,
                   const core::Color& tintColor = core::Color::white(),
                   graphics::Filter filter = graphics::Filter::Linear);

    void drawText(const core::Point& pos, std::string_view text,
                  const std::shared_ptr<text::Font>& font,
                  const core::Color& color = core::Color::white());
    void drawTextLayout(const core::Point& pos, const text::TextLayout& layout,
                        const core::Color& color = core::Color::white());

    void pushClip(const core::Rect& rect);
    void popClip();

    void append(const PaintContext& other);
    void clear() noexcept;
    [[nodiscard]] const std::vector<DrawCommand>& commands() const noexcept { return commands_; }

private:
    std::vector<DrawCommand> commands_;
};

} // namespace northern::renderer
