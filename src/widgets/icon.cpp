#include "widgets/icon.hpp"
#include "renderer/paint_context.hpp"

#include <algorithm>

namespace northern::widgets {

Icon::Icon(IconId iconId, float size, core::Color color)
    : iconId_(iconId), doc_(getSvgDocument(iconId)), iconSize_(size), color_(color) {
    style().width = layout::Length::points(size);
    style().height = layout::Length::points(size);
}

Icon::Icon(std::shared_ptr<renderer::SvgDocument> doc, float size, core::Color color)
    : doc_(std::move(doc)), iconSize_(size), color_(color) {
    style().width = layout::Length::points(size);
    style().height = layout::Length::points(size);
}

Icon::Icon(std::string_view svgContent, float size, core::Color color)
    : iconSize_(size), color_(color) {
    auto res = renderer::SvgDocument::fromString(svgContent);
    if (res.isOk()) {
        doc_ = res.value();
    }
    style().width = layout::Length::points(size);
    style().height = layout::Length::points(size);
}

void Icon::setIcon(IconId iconId) {
    if (iconId_ != iconId) {
        iconId_ = iconId;
        doc_ = getSvgDocument(iconId);
        markPaintDirty();
    }
}

void Icon::setSvgDocument(std::shared_ptr<renderer::SvgDocument> doc) {
    iconId_.reset();
    doc_ = std::move(doc);
    markPaintDirty();
}

void Icon::setSvgContent(std::string_view svgContent) {
    iconId_.reset();
    auto res = renderer::SvgDocument::fromString(svgContent);
    doc_ = res.isOk() ? res.value() : nullptr;
    markPaintDirty();
}

void Icon::setColor(core::Color color) {
    if (color_ != color) {
        color_ = color;
        markPaintDirty();
    }
}

void Icon::setIconSize(float size) {
    if (iconSize_ != size) {
        iconSize_ = size;
        style().width = layout::Length::points(size);
        style().height = layout::Length::points(size);
        markLayoutDirty();
    }
}

void Icon::paint(renderer::PaintContext& ctx) {
    if (!doc_ || !doc_->isValid()) {
        return;
    }

    core::Rect sRect = screenRect();
    float s = std::min(iconSize_, std::min(sRect.size.width, sRect.size.height));
    if (s <= 0.0f) {
        return;
    }

    float ox = sRect.origin.x + (sRect.size.width - s) * 0.5f;
    float oy = sRect.origin.y + (sRect.size.height - s) * 0.5f;
    core::Rect targetRect{core::Point{ox, oy}, core::Size{s, s}};

    ctx.drawSvg(*doc_, targetRect, color_);
}

} // namespace northern::widgets
