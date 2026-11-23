#include "widgets/controls/label.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"

namespace northern::widgets {

Label::Label(std::string text, std::shared_ptr<text::Font> font)
    : text_(std::move(text)), font_(std::move(font)) {
    layoutNode_.setMeasureFunc(
        [this](const layout::LayoutNode&, float availW, float) -> core::Size {
            if (!font_ || text_.empty()) {
                return core::Size{0.0f, 0.0f};
            }
            float maxW = availW > 0.0f ? availW : 0.0f;
            if (!cachedLayout_ || cachedWidth_ != maxW) {
                cachedLayout_ = text::TextLayout::create(text_, font_, maxW, alignment_);
                cachedWidth_ = maxW;
            }
            return cachedLayout_->size();
        });
}

void Label::setText(std::string text) {
    if (text_ != text) {
        text_ = std::move(text);
        cachedLayout_.reset();
        cachedWidth_ = -1.0f;
        markLayoutDirty();
    }
}

void Label::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        cachedLayout_.reset();
        cachedWidth_ = -1.0f;
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
        cachedLayout_.reset();
        cachedWidth_ = -1.0f;
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
    if (!cachedLayout_ || cachedWidth_ != rect.size.width) {
        cachedLayout_ = text::TextLayout::create(text_, font_, rect.size.width, alignment_);
        cachedWidth_ = rect.size.width;
    }
    float textY = (rect.size.height > cachedLayout_->size().height)
                      ? theme::computeCenteredTextY(rect.origin.y + rect.size.height * 0.5f, *font_)
                      : rect.origin.y;
    ctx.drawTextLayout(core::Point{rect.origin.x, textY}, *cachedLayout_, textColor_);
}

} // namespace northern::widgets
