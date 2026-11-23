#include "widgets/controls/divider.hpp"

#include "renderer/paint_context.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"

#include <algorithm>

namespace northern::widgets {

Divider::Divider(DividerOrientation orientation, float thickness)
    : orientation_(orientation), thickness_(std::max(1.0f, thickness)),
      color_(theme::ColorTokens::BorderSubtle), textColor_(theme::ColorTokens::TextTertiary) {
    setFocusable(false);
    updateMeasureAndStyle();
}

Divider::Divider(std::string label, std::shared_ptr<text::Font> font, float thickness)
    : orientation_(DividerOrientation::Horizontal), thickness_(std::max(1.0f, thickness)),
      color_(theme::ColorTokens::BorderSubtle), label_(std::move(label)), font_(std::move(font)),
      textColor_(theme::ColorTokens::TextTertiary) {
    setFocusable(false);
    updateMeasureAndStyle();
}

void Divider::setOrientation(DividerOrientation orientation) noexcept {
    if (orientation_ != orientation) {
        orientation_ = orientation;
        updateMeasureAndStyle();
        markLayoutDirty();
    }
}

void Divider::setThickness(float thickness) noexcept {
    float clamped = std::max(1.0f, thickness);
    if (thickness_ != clamped) {
        thickness_ = clamped;
        updateMeasureAndStyle();
        markLayoutDirty();
    }
}

void Divider::setColor(const core::Color& color) noexcept {
    if (color_ != color) {
        color_ = color;
        markPaintDirty();
    }
}

void Divider::setLabel(std::string label) {
    label_ = std::move(label);
    updateMeasureAndStyle();
    markLayoutDirty();
}

void Divider::setFont(std::shared_ptr<text::Font> font) {
    font_ = std::move(font);
    updateMeasureAndStyle();
    markLayoutDirty();
}

void Divider::setTextColor(const core::Color& color) noexcept {
    if (textColor_ != color) {
        textColor_ = color;
        markPaintDirty();
    }
}

void Divider::setGap(float gap) noexcept {
    float clamped = std::max(0.0f, gap);
    if (gap_ != clamped) {
        gap_ = clamped;
        markPaintDirty();
    }
}

void Divider::updateMeasureAndStyle() {
    if (orientation_ == DividerOrientation::Horizontal) {
        style().width = layout::Length::percent(100.0f);
        style().height = layout::Length::points(
            label_.empty() ? thickness_
                           : (font_ ? std::max(thickness_, font_->lineHeight()) : 16.0f));

        layoutNode_.setMeasureFunc([this](const layout::LayoutNode&, float availW,
                                          float) -> core::Size {
            float w = (availW > 0.0f) ? availW : 100.0f;
            float h = label_.empty() ? thickness_
                                     : (font_ ? std::max(thickness_, font_->lineHeight()) : 16.0f);
            return core::Size{w, h};
        });
    } else {
        style().width = layout::Length::points(thickness_);
        style().height = layout::Length::percent(100.0f);

        layoutNode_.setMeasureFunc(
            [this](const layout::LayoutNode&, float, float availH) -> core::Size {
                float h = (availH > 0.0f) ? availH : 100.0f;
                return core::Size{thickness_, h};
            });
    }
}

void Divider::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    if (orientation_ == DividerOrientation::Vertical) {
        float x = rect.origin.x + rect.size.width * 0.5f;
        ctx.drawLine(core::Point{x, rect.origin.y},
                     core::Point{x, rect.origin.y + rect.size.height}, color_, thickness_);
        return;
    }

    float y = rect.origin.y + rect.size.height * 0.5f;

    if (label_.empty() || !font_) {
        ctx.drawLine(core::Point{rect.origin.x, y}, core::Point{rect.origin.x + rect.size.width, y},
                     color_, thickness_);
        return;
    }

    auto layout = text::TextLayout::create(label_, font_);
    float textW = layout.size().width;
    float textX = rect.origin.x + (rect.size.width - textW) * 0.5f;
    float textY = theme::computeCenteredTextY(y, *font_);

    float leftLineEnd = textX - gap_;
    float rightLineStart = textX + textW + gap_;

    if (leftLineEnd > rect.origin.x) {
        ctx.drawLine(core::Point{rect.origin.x, y}, core::Point{leftLineEnd, y}, color_,
                     thickness_);
    }

    ctx.drawTextLayout(core::Point{textX, textY}, layout, textColor_);

    if (rightLineStart < rect.origin.x + rect.size.width) {
        ctx.drawLine(core::Point{rightLineStart, y},
                     core::Point{rect.origin.x + rect.size.width, y}, color_, thickness_);
    }
}

} // namespace northern::widgets
