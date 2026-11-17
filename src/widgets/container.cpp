#include "widgets/container.hpp"
#include "renderer/paint_context.hpp"

namespace northern::widgets {

void Container::setBackgroundColor(const core::Color& color) noexcept {
    backgroundColor_ = color;
    markPaintDirty();
}

void Container::setBorderColor(const core::Color& color) noexcept {
    borderColor_ = color;
    markPaintDirty();
}

void Container::setBorderWidth(float width) noexcept {
    borderWidth_ = width;
    markPaintDirty();
}

void Container::setCornerRadius(float radius) noexcept {
    cornerRadii_[0] = radius;
    cornerRadii_[1] = radius;
    cornerRadii_[2] = radius;
    cornerRadii_[3] = radius;
    markPaintDirty();
}

void Container::setCornerRadii(float r0, float r1, float r2, float r3) noexcept {
    cornerRadii_[0] = r0;
    cornerRadii_[1] = r1;
    cornerRadii_[2] = r2;
    cornerRadii_[3] = r3;
    markPaintDirty();
}

void Container::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    bool hasBg = backgroundColor_.a > 0.0f;
    bool hasBorder = borderWidth_ > 0.0f && borderColor_.a > 0.0f;

    if (hasBg && hasBorder) {
        ctx.drawBox(rect, backgroundColor_, borderColor_, borderWidth_, cornerRadii_);
    } else if (hasBg) {
        bool hasRadius = cornerRadii_[0] > 0.0f || cornerRadii_[1] > 0.0f ||
                         cornerRadii_[2] > 0.0f || cornerRadii_[3] > 0.0f;
        if (hasRadius) {
            ctx.drawRoundedRect(rect, backgroundColor_, cornerRadii_);
        } else {
            ctx.drawRect(rect, backgroundColor_);
        }
    } else if (hasBorder) {
        ctx.drawBorder(rect, borderColor_, borderWidth_, cornerRadii_[0]);
    }
}

} // namespace northern::widgets
