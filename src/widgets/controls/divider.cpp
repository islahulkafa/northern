#include "widgets/controls/divider.hpp"

#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "widgets/base/theme.hpp"

#include <algorithm>

namespace northern::widgets {

Divider::Divider(DividerOrientation orientation, float thickness)
    : orientation_(orientation), thickness_(std::max(1.0f, thickness)) {
    setFocusable(false);
    updateMeasureAndStyle();
}

Divider::Divider(std::string label, std::shared_ptr<text::Font> font, float thickness)
    : orientation_(DividerOrientation::Horizontal), thickness_(std::max(1.0f, thickness)),
      label_(std::move(label)), font_(std::move(font)) {
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

void Divider::setVariant(DividerVariant variant) noexcept {
    if (variant_ != variant) {
        variant_ = variant;
        markPaintDirty();
    }
}

void Divider::setLabelAlignment(DividerAlignment alignment) noexcept {
    if (labelAlignment_ != alignment) {
        labelAlignment_ = alignment;
        markPaintDirty();
    }
}

void Divider::setLabelOffset(float offset) noexcept {
    float clamped = std::max(0.0f, offset);
    if (labelOffset_ != clamped) {
        labelOffset_ = clamped;
        markPaintDirty();
    }
}

void Divider::setInset(float both) noexcept {
    setInset(both, both);
}

void Divider::setInset(float start, float end) noexcept {
    float s = std::max(0.0f, start);
    float e = std::max(0.0f, end);
    if (insetStart_ != s || insetEnd_ != e) {
        insetStart_ = s;
        insetEnd_ = e;
        markPaintDirty();
    }
}

void Divider::setColor(const core::Color& color) noexcept {
    overrideColor_ = color;
    markPaintDirty();
}

void Divider::setLabel(std::string label) {
    if (label_ != label) {
        label_ = std::move(label);
        isTextLayoutDirty_ = true;
        cachedTextLayout_.reset();
        updateMeasureAndStyle();
        markLayoutDirty();
    }
}

void Divider::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        isTextLayoutDirty_ = true;
        cachedTextLayout_.reset();
        updateMeasureAndStyle();
        markLayoutDirty();
    }
}

void Divider::setTextColor(const core::Color& color) noexcept {
    overrideTextColor_ = color;
    markPaintDirty();
}

void Divider::setGap(float gap) noexcept {
    float clamped = std::max(0.0f, gap);
    if (gap_ != clamped) {
        gap_ = clamped;
        markPaintDirty();
    }
}

std::shared_ptr<text::Font> Divider::resolveFont() const {
    if (font_) {
        return font_;
    }
    return text::FontManager::instance().defaultFont(11.0f).valueOr(nullptr);
}

void Divider::ensureTextLayout() const {
    if (!isTextLayoutDirty_ && cachedTextLayout_.has_value()) {
        return;
    }
    if (label_.empty()) {
        cachedTextLayout_.reset();
        isTextLayoutDirty_ = false;
        return;
    }
    auto f = resolveFont();
    if (!f) {
        cachedTextLayout_.reset();
        isTextLayoutDirty_ = false;
        return;
    }
    cachedTextLayout_ = text::TextLayout::create(label_, f);
    isTextLayoutDirty_ = false;
}

core::Color Divider::effectiveColor() const noexcept {
    if (overrideColor_) {
        return *overrideColor_;
    }
    const auto& pal = colorPalette();
    switch (variant_) {
    case DividerVariant::Subtle:
        return pal.borderSubtle;
    case DividerVariant::Default:
        return pal.borderDefault;
    case DividerVariant::Strong:
        return pal.borderStrong;
    }
    return pal.borderSubtle;
}

core::Color Divider::effectiveTextColor() const noexcept {
    if (overrideTextColor_) {
        return *overrideTextColor_;
    }
    return colorPalette().textTertiary;
}

void Divider::updateMeasureAndStyle() {
    if (orientation_ == DividerOrientation::Horizontal) {
        style().width = layout::Length::percent(100.0f);
        auto f = resolveFont();
        float contentH =
            label_.empty() ? thickness_ : (f ? std::max(thickness_, f->lineHeight()) : 16.0f);
        style().height = layout::Length::points(contentH);

        layoutNode_.setMeasureFunc(
            [this](const layout::LayoutNode&, float availW, float) -> core::Size {
                float w = (availW > 0.0f) ? availW : 100.0f;
                auto resolvedF = resolveFont();
                float h = label_.empty()
                              ? thickness_
                              : (resolvedF ? std::max(thickness_, resolvedF->lineHeight()) : 16.0f);
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

    core::Color lineColor = effectiveColor();

    if (orientation_ == DividerOrientation::Vertical) {
        float x = rect.origin.x + rect.size.width * 0.5f;
        float y0 = rect.origin.y + insetStart_;
        float y1 = rect.origin.y + rect.size.height - insetEnd_;
        if (y1 > y0) {
            ctx.drawLine(core::Point{x, y0}, core::Point{x, y1}, lineColor, thickness_);
        }
        return;
    }

    float y = rect.origin.y + rect.size.height * 0.5f;
    float startX = rect.origin.x + insetStart_;
    float endX = rect.origin.x + rect.size.width - insetEnd_;

    if (endX <= startX) {
        return;
    }

    ensureTextLayout();

    if (label_.empty() || !cachedTextLayout_.has_value()) {
        ctx.drawLine(core::Point{startX, y}, core::Point{endX, y}, lineColor, thickness_);
        return;
    }

    const auto& layout = *cachedTextLayout_;
    float textW = layout.size().width;
    float availW = endX - startX;

    float textX = startX;
    float leftLineEnd = startX;
    float rightLineStart = endX;

    if (textW + 2.0f * gap_ >= availW) {
        textX = startX + (availW - textW) * 0.5f;
        auto f = resolveFont();
        float textY = f ? theme::computeCenteredTextY(y, *f) : (y - layout.size().height * 0.5f);
        ctx.drawTextLayout(core::Point{textX, textY}, layout, effectiveTextColor());
        return;
    }

    switch (labelAlignment_) {
    case DividerAlignment::Center: {
        textX = startX + (availW - textW) * 0.5f;
        leftLineEnd = textX - gap_;
        rightLineStart = textX + textW + gap_;
        break;
    }
    case DividerAlignment::Start: {
        float offset = std::min(labelOffset_, availW - textW - 2.0f * gap_);
        leftLineEnd = startX + offset;
        textX = leftLineEnd + gap_;
        rightLineStart = textX + textW + gap_;
        break;
    }
    case DividerAlignment::End: {
        float offset = std::min(labelOffset_, availW - textW - 2.0f * gap_);
        rightLineStart = endX - offset;
        textX = rightLineStart - gap_ - textW;
        leftLineEnd = textX - gap_;
        break;
    }
    }

    if (leftLineEnd > startX) {
        ctx.drawLine(core::Point{startX, y}, core::Point{leftLineEnd, y}, lineColor, thickness_);
    }

    auto f = resolveFont();
    float textY = f ? theme::computeCenteredTextY(y, *f) : (y - layout.size().height * 0.5f);
    ctx.drawTextLayout(core::Point{textX, textY}, layout, effectiveTextColor());

    if (rightLineStart < endX) {
        ctx.drawLine(core::Point{rightLineStart, y}, core::Point{endX, y}, lineColor, thickness_);
    }
}

} // namespace northern::widgets
