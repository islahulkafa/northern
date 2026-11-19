#include "renderer/svg/svg_document.hpp"
#include "renderer/paint_context.hpp"

#include <algorithm>
#include <cmath>
#include <string>

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

void subdivideCubic(PaintContext& ctx, const core::Point& p0, const core::Point& p1,
                    const core::Point& p2, const core::Point& p3, const core::Color& color,
                    float thickness, int depth) {
    float dx = p3.x - p0.x;
    float dy = p3.y - p0.y;
    float d1 = std::abs((p1.x - p3.x) * dy - (p1.y - p3.y) * dx);
    float d2 = std::abs((p2.x - p3.x) * dy - (p2.y - p3.y) * dx);

    constexpr float kToleranceSq = 0.5f;
    float lenSq = dx * dx + dy * dy;

    if (lenSq < 1e-4f) {
        if (depth >= 4 || (std::abs(p1.x - p0.x) + std::abs(p1.y - p0.y) + std::abs(p2.x - p0.x) +
                               std::abs(p2.y - p0.y) <
                           1.0f)) {
            return;
        }
    } else if ((d1 + d2) * (d1 + d2) <= kToleranceSq * lenSq || depth >= 4) {
        ctx.drawLine(p0, p3, color, thickness);
        return;
    }

    core::Point p01{(p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f};
    core::Point p12{(p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f};
    core::Point p23{(p2.x + p3.x) * 0.5f, (p2.y + p3.y) * 0.5f};

    core::Point p012{(p01.x + p12.x) * 0.5f, (p01.y + p12.y) * 0.5f};
    core::Point p123{(p12.x + p23.x) * 0.5f, (p12.y + p23.y) * 0.5f};

    core::Point p0123{(p012.x + p123.x) * 0.5f, (p012.y + p123.y) * 0.5f};

    subdivideCubic(ctx, p0, p01, p012, p0123, color, thickness, depth + 1);
    subdivideCubic(ctx, p0123, p123, p23, p3, color, thickness, depth + 1);
}

} // namespace

SvgDocument::SvgDocument(void* nsvgImage) noexcept : image_(nsvgImage) {}

SvgDocument::~SvgDocument() {
    if (image_) {
        nsvgDelete(static_cast<NSVGimage*>(image_));
        image_ = nullptr;
    }
}

SvgDocument::SvgDocument(SvgDocument&& other) noexcept : image_(other.image_) {
    other.image_ = nullptr;
}

SvgDocument& SvgDocument::operator=(SvgDocument&& other) noexcept {
    if (this != &other) {
        if (image_) {
            nsvgDelete(static_cast<NSVGimage*>(image_));
        }
        image_ = other.image_;
        other.image_ = nullptr;
    }
    return *this;
}

core::Result<std::shared_ptr<SvgDocument>, core::Error>
SvgDocument::fromString(std::string_view svgContent) {
    if (svgContent.empty()) {
        return core::Error(core::ErrorCode::InvalidArgument, "Empty SVG content");
    }

    // nsvgParse modifies input buffer in-place
    std::string copy(svgContent);
    NSVGimage* img = nsvgParse(copy.data(), "px", 96.0f);
    if (!img || (img->width <= 0.0f && img->height <= 0.0f && img->shapes == nullptr)) {
        if (img) {
            nsvgDelete(img);
        }
        return core::Error(core::ErrorCode::ParseError, "Failed to parse SVG content");
    }

    return std::shared_ptr<SvgDocument>(new SvgDocument(img));
}

core::Result<std::shared_ptr<SvgDocument>, core::Error>
SvgDocument::fromFile(const std::string& filePath) {
    NSVGimage* img = nsvgParseFromFile(filePath.c_str(), "px", 96.0f);
    if (!img) {
        return core::Error(core::ErrorCode::NotFound, "Failed to load SVG from: " + filePath);
    }
    return std::shared_ptr<SvgDocument>(new SvgDocument(img));
}

core::Size SvgDocument::size() const noexcept {
    if (!image_) {
        return {0.0f, 0.0f};
    }
    auto* img = static_cast<NSVGimage*>(image_);
    return {img->width, img->height};
}

bool SvgDocument::isValid() const noexcept {
    return image_ != nullptr;
}

void SvgDocument::render(PaintContext& ctx, const core::Rect& bounds,
                         const core::Color& tintColor) const {
    if (!image_) {
        return;
    }

    auto* img = static_cast<NSVGimage*>(image_);
    if (img->width <= 0.0f || img->height <= 0.0f) {
        return;
    }

    float scaleX = bounds.size.width / img->width;
    float scaleY = bounds.size.height / img->height;
    float scale = std::min(scaleX, scaleY);

    float offsetX = bounds.origin.x + (bounds.size.width - img->width * scale) * 0.5f;
    float offsetY = bounds.origin.y + (bounds.size.height - img->height * scale) * 0.5f;

    auto transform = [&](float x, float y) -> core::Point {
        return core::Point{offsetX + x * scale, offsetY + y * scale};
    };

    for (NSVGshape* shape = img->shapes; shape != nullptr; shape = shape->next) {
        if (!(shape->flags & NSVG_FLAGS_VISIBLE)) {
            continue;
        }

        core::Color color = tintColor;
        float thickness = (shape->strokeWidth > 0.0f ? shape->strokeWidth : 2.0f) * scale;
        if (thickness < 1.0f) {
            thickness = 1.0f;
        }

        for (NSVGpath* path = shape->paths; path != nullptr; path = path->next) {
            for (int i = 0; i < path->npts - 1; i += 3) {
                float* p = &path->pts[i * 2];
                core::Point p0 = transform(p[0], p[1]);
                core::Point p1 = transform(p[2], p[3]);
                core::Point p2 = transform(p[4], p[5]);
                core::Point p3 = transform(p[6], p[7]);
                subdivideCubic(ctx, p0, p1, p2, p3, color, thickness, 0);
            }
        }
    }
}

} // namespace northern::renderer
