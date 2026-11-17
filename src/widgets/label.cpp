#include "widgets/label.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/theme.hpp"

namespace northern::widgets {

Label::Label(std::string text, std::shared_ptr<text::Font> font)
    : text_(std::move(text)), font_(std::move(font)) {
    layoutNode_.setMeasureFunc(
        [this](const layout::LayoutNode&, float availW, float) -> core::Size {
            if (!font_ || text_.empty()) {
                return core::Size{0.0f, 0.0f};
            }
            float maxW = availW > 0.0f ? availW : 0.0f;
            auto tempLayout = text::TextLayout::create(text_, font_, maxW, alignment_);
            return tempLayout.size();
        });
}

void Label::setText(std::string text) {
    if (text_ != text) {
        text_ = std::move(text);
        markLayoutDirty();
    }
}

void Label::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        markLayoutDirty();
    }
}

void Label::setTextColor(const core::Color& color) noexcept {
    textColor_ = color;
    markPaintDirty();
}

void Label::setTextAlignment(text::TextAlignment alignment) noexcept {
    if (alignment_ != alignment) {
        alignment_ = alignment;
        markPaintDirty();
    }
}

void Label::paint(renderer::PaintContext& ctx) {
    if (text_.empty() || !font_ || textColor_.a <= 0.0f) {
        return;
    }
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }
    auto layout = text::TextLayout::create(text_, font_, rect.size.width, alignment_);
    float textY = (rect.size.height > layout.size().height)
                      ? theme::computeCenteredTextY(rect.origin.y + rect.size.height * 0.5f, *font_)
                      : rect.origin.y;
    ctx.drawTextLayout(core::Point{rect.origin.x, textY}, layout, textColor_);
}

} // namespace northern::widgets
