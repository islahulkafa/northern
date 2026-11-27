#include "widgets/controls/image_view.hpp"
#include "renderer/paint_context.hpp"
#include "renderer/svg/svg_document.hpp"
#include "widgets/base/theme.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

ImageView::ImageView(std::shared_ptr<renderer::Image> image, ImageFit fit)
    : image_(std::move(image)), fit_(fit) {
    layoutNode_.setMeasureFunc(
        [this](const layout::LayoutNode&, float availW, float availH) -> core::Size {
            if (!image_ || !image_->isValid()) {
                float fallback = 64.0f * dpiScale_;
                return core::Size{availW > 0.0f ? std::min(availW, fallback) : fallback,
                                  availH > 0.0f ? std::min(availH, fallback) : fallback};
            }
            float natW = static_cast<float>(image_->width());
            float natH = static_cast<float>(image_->height());
            if (availW > 0.0f && availH > 0.0f) {
                return core::Size{availW, availH};
            }
            if (availW > 0.0f && natW > 0.0f) {
                float scale = availW / natW;
                return core::Size{availW, natH * scale};
            }
            if (availH > 0.0f && natH > 0.0f) {
                float scale = availH / natH;
                return core::Size{natW * scale, availH};
            }
            return core::Size{natW, natH};
        });
}

void ImageView::setImage(std::shared_ptr<renderer::Image> image) {
    if (image_ != image) {
        image_ = std::move(image);
        markLayoutDirty();
        markPaintDirty();
    }
}

void ImageView::setFit(ImageFit fit) noexcept {
    if (fit_ != fit) {
        fit_ = fit;
        markPaintDirty();
    }
}

void ImageView::setFilter(graphics::Filter filter) noexcept {
    if (filter_ != filter) {
        filter_ = filter;
        markPaintDirty();
    }
}

void ImageView::setCornerRadius(float radius) noexcept {
    setCornerRadii(radius, radius, radius, radius);
}

void ImageView::setCornerRadii(float tl, float tr, float br, float bl) noexcept {
    hasCustomRadius_ = true;
    cornerRadii_[0] = std::max(0.0f, tl);
    cornerRadii_[1] = std::max(0.0f, tr);
    cornerRadii_[2] = std::max(0.0f, br);
    cornerRadii_[3] = std::max(0.0f, bl);
    markPaintDirty();
}

void ImageView::setShowBorder(bool show) noexcept {
    if (showBorder_ != show) {
        showBorder_ = show;
        markPaintDirty();
    }
}

void ImageView::setBorderWidth(float width) noexcept {
    float clamped = std::max(0.0f, width);
    if (borderWidth_ != clamped) {
        borderWidth_ = clamped;
        showBorder_ = clamped > 0.0f;
        markPaintDirty();
    }
}

void ImageView::setBorderColor(const core::Color& color) noexcept {
    customBorderColor_ = color;
    showBorder_ = true;
    markPaintDirty();
}

core::Color ImageView::effectiveBorderColor() const noexcept {
    if (customBorderColor_) {
        return *customBorderColor_;
    }
    return colorPalette().borderDefault;
}

void ImageView::setTintColor(const core::Color& color) noexcept {
    tintColor_ = color;
    markPaintDirty();
}

void ImageView::setOpacity(float opacity) noexcept {
    float clamped = std::clamp(opacity, 0.0f, 1.0f);
    if (opacity_ != clamped) {
        opacity_ = clamped;
        markPaintDirty();
    }
}

void ImageView::setPlaceholderIcon(std::optional<IconId> icon) noexcept {
    if (placeholderIcon_ != icon) {
        placeholderIcon_ = icon;
        markPaintDirty();
    }
}

void ImageView::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiScale(scale);
}

void ImageView::applyDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        markLayoutDirty();
        markPaintDirty();
    }
}

void ImageView::onContextAttached() {
    if (!hasCustomDpiScale_) {
        applyDpiScale(contextDpiScale());
    }
}

void ImageView::onThemeChanged() {
    markPaintDirty();
}

ImageDrawGeometry ImageView::computeImageLayout(const core::Rect& bounds, float imgW, float imgH,
                                                ImageFit fit) noexcept {
    ImageDrawGeometry out{};
    out.drawRect = bounds;
    out.uvRect = core::Rect{0.0f, 0.0f, 1.0f, 1.0f};

    if (bounds.size.width <= 0.0f || bounds.size.height <= 0.0f || imgW <= 0.0f || imgH <= 0.0f) {
        return out;
    }

    float bw = bounds.size.width;
    float bh = bounds.size.height;

    switch (fit) {
    case ImageFit::Fill:
        return out;

    case ImageFit::Contain:
    case ImageFit::ScaleDown: {
        float scale = std::min(bw / imgW, bh / imgH);
        if (fit == ImageFit::ScaleDown) {
            scale = std::min(scale, 1.0f);
        }
        float dw = imgW * scale;
        float dh = imgH * scale;
        out.drawRect = core::Rect{
            bounds.origin.x + (bw - dw) * 0.5f,
            bounds.origin.y + (bh - dh) * 0.5f,
            dw,
            dh,
        };
        return out;
    }

    case ImageFit::Cover: {
        float boxAspect = bw / bh;
        float imgAspect = imgW / imgH;
        if (imgAspect > boxAspect) {
            float visibleU = boxAspect / imgAspect;
            out.uvRect = core::Rect{(1.0f - visibleU) * 0.5f, 0.0f, visibleU, 1.0f};
        } else if (imgAspect < boxAspect) {
            float visibleV = imgAspect / boxAspect;
            out.uvRect = core::Rect{0.0f, (1.0f - visibleV) * 0.5f, 1.0f, visibleV};
        }
        return out;
    }

    case ImageFit::None: {
        out.drawRect = core::Rect{
            bounds.origin.x + (bw - imgW) * 0.5f,
            bounds.origin.y + (bh - imgH) * 0.5f,
            imgW,
            imgH,
        };
        return out;
    }
    }

    return out;
}

void ImageView::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    const auto& pal = colorPalette();
    float borderW = showBorder_ ? std::max(1.0f, std::round(borderWidth_ * dpiScale_)) : 0.0f;
    core::Color borderCol = showBorder_ ? effectiveBorderColor() : core::Color{0, 0, 0, 0};

    if (!image_ || !image_->isValid()) {
        ctx.drawBox(rect, pal.surfaceDefault, showBorder_ ? borderCol : pal.borderSubtle,
                    showBorder_ ? borderW : std::max(1.0f, std::round(1.0f * dpiScale_)),
                    cornerRadii_);
        if (placeholderIcon_) {
            if (auto doc = getSvgDocument(*placeholderIcon_)) {
                float iconSz = std::min(
                    {24.0f * dpiScale_, rect.size.width * 0.45f, rect.size.height * 0.45f});
                if (iconSz >= 6.0f) {
                    core::Rect iconRect{
                        rect.origin.x + (rect.size.width - iconSz) * 0.5f,
                        rect.origin.y + (rect.size.height - iconSz) * 0.5f,
                        iconSz,
                        iconSz,
                    };
                    ctx.drawSvg(*doc, iconRect, pal.textTertiary);
                }
            }
        }
        return;
    }

    ImageDrawGeometry geom = computeImageLayout(rect, static_cast<float>(image_->width()),
                                                static_cast<float>(image_->height()), fit_);
    core::Color finalTint = tintColor_.withAlpha(tintColor_.a * opacity_);

    bool needsClip = (fit_ == ImageFit::None) && (geom.drawRect.size.width > rect.size.width ||
                                                  geom.drawRect.size.height > rect.size.height);
    if (needsClip) {
        ctx.pushClip(rect);
    }

    ctx.drawImage(*image_, geom.drawRect, geom.uvRect, cornerRadii_, borderCol, borderW, finalTint,
                  filter_);

    if (needsClip) {
        ctx.popClip();
    }
}

} // namespace northern::widgets
