#include "renderer/svg/svg_document.hpp"
#include "renderer/paint_context.hpp"
#include "text/vector_rasterizer.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#define NANOSVG_IMPLEMENTATION
#include "renderer/svg/nanosvg.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace northern::renderer {

namespace {

std::atomic<uint32_t> sNextSvgId{1};

text::StrokeCap toTextCap(char nsvgCap) noexcept {
    switch (nsvgCap) {
    case NSVG_CAP_ROUND:
        return text::StrokeCap::Round;
    case NSVG_CAP_SQUARE:
        return text::StrokeCap::Square;
    case NSVG_CAP_BUTT:
    default:
        return text::StrokeCap::Butt;
    }
}

text::StrokeJoin toTextJoin(char nsvgJoin) noexcept {
    switch (nsvgJoin) {
    case NSVG_JOIN_ROUND:
        return text::StrokeJoin::Round;
    case NSVG_JOIN_BEVEL:
        return text::StrokeJoin::Bevel;
    case NSVG_JOIN_MITER:
    default:
        return text::StrokeJoin::Miter;
    }
}

float paintAlpha(const NSVGpaint& paint) noexcept {
    if (paint.type == NSVG_PAINT_COLOR) {
        return static_cast<float>((paint.color >> 24) & 0xFFu) / 255.0f;
    }
    if (paint.type != NSVG_PAINT_NONE) {
        return 1.0f;
    }
    return 0.0f;
}

} // namespace

struct SvgDocument::Impl {
    uint32_t id{0};
    NSVGimage* image{nullptr};

    Impl(uint32_t docId, NSVGimage* img) noexcept : id(docId), image(img) {}

    ~Impl() {
        if (image) {
            nsvgDelete(image);
            image = nullptr;
        }
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
};

SvgDocument::SvgDocument(std::shared_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

SvgDocument::~SvgDocument() = default;

SvgDocument::SvgDocument(SvgDocument&& other) noexcept = default;

SvgDocument& SvgDocument::operator=(SvgDocument&& other) noexcept = default;

core::Result<std::shared_ptr<SvgDocument>, core::Error>
SvgDocument::fromString(std::string_view svgContent) {
    if (svgContent.empty()) {
        return core::Error(core::ErrorCode::InvalidArgument, "Empty SVG content");
    }

    std::string copy(svgContent);
    NSVGimage* img = nsvgParse(copy.data(), "px", 96.0f);
    if (!img || (img->width <= 0.0f && img->height <= 0.0f && img->shapes == nullptr)) {
        if (img) {
            nsvgDelete(img);
        }
        return core::Error(core::ErrorCode::ParseError, "Failed to parse SVG content");
    }

    uint32_t docId = sNextSvgId.fetch_add(1, std::memory_order_relaxed);
    auto impl = std::make_shared<Impl>(docId, img);
    return std::shared_ptr<SvgDocument>(new SvgDocument(std::move(impl)));
}

core::Result<std::shared_ptr<SvgDocument>, core::Error>
SvgDocument::fromFile(const std::string& filePath) {
    NSVGimage* img = nsvgParseFromFile(filePath.c_str(), "px", 96.0f);
    if (!img) {
        return core::Error(core::ErrorCode::NotFound, "Failed to load SVG from: " + filePath);
    }

    uint32_t docId = sNextSvgId.fetch_add(1, std::memory_order_relaxed);
    auto impl = std::make_shared<Impl>(docId, img);
    return std::shared_ptr<SvgDocument>(new SvgDocument(std::move(impl)));
}

uint32_t SvgDocument::id() const noexcept {
    return impl_ ? impl_->id : 0;
}

uint32_t SvgDocument::implId(const Impl& impl) noexcept {
    return impl.id;
}

core::Size SvgDocument::size() const noexcept {
    if (!impl_ || !impl_->image) {
        return {0.0f, 0.0f};
    }
    return {impl_->image->width, impl_->image->height};
}

bool SvgDocument::isValid() const noexcept {
    return impl_ != nullptr && impl_->image != nullptr;
}

core::Result<void, core::Error> SvgDocument::rasterize(uint32_t targetWidth, uint32_t targetHeight,
                                                       uint32_t padPixels,
                                                       std::span<uint8_t> outAlphaMask) const {
    if (!impl_) {
        return core::Error(core::ErrorCode::InvalidState, "SVG document is empty");
    }
    return rasterizeImpl(*impl_, targetWidth, targetHeight, padPixels, outAlphaMask);
}

core::Result<void, core::Error> SvgDocument::rasterizeImpl(const Impl& impl, uint32_t targetWidth,
                                                           uint32_t targetHeight,
                                                           uint32_t padPixels,
                                                           std::span<uint8_t> outAlphaMask) {
    NSVGimage* img = impl.image;
    if (!img || img->width <= 0.0f || img->height <= 0.0f || targetWidth == 0 ||
        targetHeight == 0) {
        return core::Error(core::ErrorCode::InvalidArgument, "Invalid SVG rasterize parameters");
    }

    uint32_t maskW = targetWidth + padPixels * 2;
    uint32_t maskH = targetHeight + padPixels * 2;
    if (outAlphaMask.size() < static_cast<size_t>(maskW) * static_cast<size_t>(maskH)) {
        return core::Error(core::ErrorCode::InvalidArgument, "SVG output mask buffer too small");
    }

    float scaleX = static_cast<float>(targetWidth) / img->width;
    float scaleY = static_cast<float>(targetHeight) / img->height;
    float scale = std::min(scaleX, scaleY);

    float offsetX = static_cast<float>(padPixels) +
                    (static_cast<float>(targetWidth) - img->width * scale) * 0.5f;
    float offsetY = static_cast<float>(padPixels) +
                    (static_cast<float>(targetHeight) - img->height * scale) * 0.5f;

    auto transformPt = [offsetX, offsetY, scale](float x, float y) -> core::Point {
        return core::Point{offsetX + x * scale, offsetY + y * scale};
    };

    std::vector<text::VectorShapeDesc> shapes;
    for (NSVGshape* shape = img->shapes; shape != nullptr; shape = shape->next) {
        if (!(shape->flags & NSVG_FLAGS_VISIBLE)) {
            continue;
        }

        text::VectorShapeDesc desc{};
        desc.opacity = std::clamp(shape->opacity, 0.0f, 1.0f);
        desc.hasFill = (shape->fill.type != NSVG_PAINT_NONE);
        desc.fillOpacity = paintAlpha(shape->fill);
        desc.fillRule = (shape->fillRule == NSVG_FILLRULE_EVENODD) ? text::FillRule::EvenOdd
                                                                   : text::FillRule::NonZero;

        desc.hasStroke = (shape->stroke.type != NSVG_PAINT_NONE && shape->strokeWidth > 0.0f);
        desc.strokeOpacity = paintAlpha(shape->stroke);
        if (desc.hasStroke) {
            desc.strokeWidth = std::max(1.0f, shape->strokeWidth * scale);
            desc.strokeCap = toTextCap(shape->strokeLineCap);
            desc.strokeJoin = toTextJoin(shape->strokeLineJoin);
            desc.miterLimit = shape->miterLimit > 0.0f ? shape->miterLimit : 4.0f;
        }

        if (!desc.hasFill && !desc.hasStroke) {
            continue;
        }

        for (NSVGpath* path = shape->paths; path != nullptr; path = path->next) {
            if (path->npts < 4) {
                continue;
            }
            text::CubicSubpath sub{};
            sub.closed = (path->closed != 0);
            sub.points.reserve(static_cast<size_t>(path->npts));
            for (int i = 0; i < path->npts; ++i) {
                sub.points.push_back(transformPt(path->pts[i * 2], path->pts[i * 2 + 1]));
            }
            desc.subpaths.push_back(std::move(sub));
        }

        if (!desc.subpaths.empty()) {
            shapes.push_back(std::move(desc));
        }
    }

    return text::rasterizeVectorShapes(shapes, maskW, maskH, outAlphaMask);
}

void SvgDocument::render(PaintContext& ctx, const core::Rect& bounds,
                         const core::Color& tintColor) const {
    ctx.drawSvg(*this, bounds, tintColor);
}

} // namespace northern::renderer
