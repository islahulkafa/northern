#include "text/vector_rasterizer.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_STROKER_H

#include <algorithm>
#include <cmath>
#include <cstring>
#include <type_traits>

namespace northern::text {

namespace {

struct FtThreadLibrary {
    FT_Library lib{nullptr};

    FtThreadLibrary() noexcept { FT_Init_FreeType(&lib); }
    ~FtThreadLibrary() {
        if (lib) {
            FT_Done_FreeType(lib);
            lib = nullptr;
        }
    }

    FtThreadLibrary(const FtThreadLibrary&) = delete;
    FtThreadLibrary& operator=(const FtThreadLibrary&) = delete;
};

FT_Library getThreadFtLibrary() noexcept {
    thread_local FtThreadLibrary sInstance;
    return sInstance.lib;
}

FT_Stroker_LineCap toFtCap(StrokeCap cap) noexcept {
    switch (cap) {
    case StrokeCap::Butt:
        return FT_STROKER_LINECAP_BUTT;
    case StrokeCap::Round:
        return FT_STROKER_LINECAP_ROUND;
    case StrokeCap::Square:
        return FT_STROKER_LINECAP_SQUARE;
    }
    return FT_STROKER_LINECAP_BUTT;
}

FT_Stroker_LineJoin toFtJoin(StrokeJoin join) noexcept {
    switch (join) {
    case StrokeJoin::Miter:
        return FT_STROKER_LINEJOIN_MITER_FIXED;
    case StrokeJoin::Round:
        return FT_STROKER_LINEJOIN_ROUND;
    case StrokeJoin::Bevel:
        return FT_STROKER_LINEJOIN_BEVEL;
    }
    return FT_STROKER_LINEJOIN_MITER_FIXED;
}

void compositeMask(std::span<uint8_t> dst, std::span<const uint8_t> src, float weight) noexcept {
    uint32_t w256 = static_cast<uint32_t>(std::clamp(weight, 0.0f, 1.0f) * 256.0f + 0.5f);
    if (w256 == 0) {
        return;
    }
    for (size_t i = 0; i < dst.size(); ++i) {
        uint32_t sa = (static_cast<uint32_t>(src[i]) * w256) >> 8;
        if (sa == 0) {
            continue;
        }
        uint32_t da = dst[i];
        dst[i] = static_cast<uint8_t>(sa + ((da * (255u - sa) + 127u) / 255u));
    }
}

void renderOutlineToBuffer(FT_Library ftLib, FT_Outline& outline, uint32_t width, uint32_t height,
                           std::span<uint8_t> targetBuf) noexcept {
    std::memset(targetBuf.data(), 0, targetBuf.size());
    FT_Bitmap bmp{};
    bmp.rows = width > 0 ? height : 0;
    bmp.width = width;
    bmp.pitch = static_cast<int>(width);
    bmp.buffer = targetBuf.data();
    bmp.num_grays = 256;
    bmp.pixel_mode = FT_PIXEL_MODE_GRAY;

    FT_Outline_Get_Bitmap(ftLib, &outline, &bmp);
}

bool canMergePureOpaqueStroke(const VectorShapeDesc& a, const VectorShapeDesc& b) noexcept {
    if (a.hasFill || b.hasFill || !a.hasStroke || !b.hasStroke) {
        return false;
    }
    if (a.opacity * a.strokeOpacity < 0.999f || b.opacity * b.strokeOpacity < 0.999f) {
        return false;
    }
    return std::abs(a.strokeWidth - b.strokeWidth) <= 1e-3f && a.strokeCap == b.strokeCap &&
           a.strokeJoin == b.strokeJoin && std::abs(a.miterLimit - b.miterLimit) <= 1e-3f;
}

void addSubpathsToStroker(FT_Stroker stroker, const std::vector<CubicSubpath>& subpaths,
                          StrokeCap cap, uint32_t height) {
    auto toFtVec = [height](const core::Point& pt) -> FT_Vector {
        return FT_Vector{
            static_cast<FT_Pos>(std::llround(pt.x * 64.0f)),
            static_cast<FT_Pos>(std::llround((static_cast<float>(height) - pt.y) * 64.0f)),
        };
    };

    for (const auto& sub : subpaths) {
        if (sub.points.size() < 4 || ((sub.points.size() - 1) % 3) != 0) {
            continue;
        }

        FT_Vector startVec = toFtVec(sub.points[0]);
        FT_Pos maxDelta = 0;
        for (size_t i = 1; i < sub.points.size(); ++i) {
            FT_Vector v = toFtVec(sub.points[i]);
            maxDelta = std::max(maxDelta, std::abs(v.x - startVec.x) + std::abs(v.y - startVec.y));
        }

        if (!sub.closed && maxDelta < 2) {
            if (cap == StrokeCap::Butt) {
                continue;
            }
            FT_Vector nudged{startVec.x + 1, startVec.y};
            if (FT_Stroker_BeginSubPath(stroker, &startVec, 1) == 0) {
                FT_Stroker_LineTo(stroker, &nudged);
                FT_Stroker_EndSubPath(stroker);
            }
            continue;
        }

        FT_Bool isOpen = sub.closed ? 0 : 1;
        if (FT_Stroker_BeginSubPath(stroker, &startVec, isOpen) != 0) {
            continue;
        }

        FT_Vector prevVec = startVec;
        for (size_t i = 0; i + 3 < sub.points.size(); i += 3) {
            FT_Vector c1 = toFtVec(sub.points[i + 1]);
            FT_Vector c2 = toFtVec(sub.points[i + 2]);
            FT_Vector p3 = toFtVec(sub.points[i + 3]);
            if (c1.x == prevVec.x && c1.y == prevVec.y && c2.x == prevVec.x && c2.y == prevVec.y &&
                p3.x == prevVec.x && p3.y == prevVec.y) {
                continue;
            }
            FT_Stroker_CubicTo(stroker, &c1, &c2, &p3);
            prevVec = p3;
        }

        FT_Stroker_EndSubPath(stroker);
    }
}

} // namespace

core::Result<void, core::Error> rasterizeVectorShapes(std::span<const VectorShapeDesc> shapes,
                                                      uint32_t width, uint32_t height,
                                                      std::span<uint8_t> outAlphaMask) {
    size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    if (width == 0 || height == 0 || outAlphaMask.size() < pixelCount) {
        return core::Error(core::ErrorCode::InvalidArgument,
                           "Invalid target alpha mask dimensions");
    }

    std::memset(outAlphaMask.data(), 0, pixelCount);
    if (shapes.empty()) {
        return {};
    }

    FT_Library ftLib = getThreadFtLibrary();
    if (!ftLib) {
        return core::Error(core::ErrorCode::InvalidState, "Failed to initialize FreeType library");
    }

    std::vector<uint8_t> scratch(pixelCount, 0);

    auto toFtVec = [height](const core::Point& pt) -> FT_Vector {
        return FT_Vector{
            static_cast<FT_Pos>(std::llround(pt.x * 64.0f)),
            static_cast<FT_Pos>(std::llround((static_cast<float>(height) - pt.y) * 64.0f)),
        };
    };

    for (size_t idx = 0; idx < shapes.size(); ++idx) {
        const auto& shape = shapes[idx];

        if (canMergePureOpaqueStroke(shape, shape)) {
            FT_Stroker stroker = nullptr;
            if (FT_Stroker_New(ftLib, &stroker) == 0 && stroker) {
                FT_Fixed halfWidth = std::max<FT_Fixed>(
                    16, static_cast<FT_Fixed>(std::llround(shape.strokeWidth * 32.0f)));
                FT_Fixed miterLimit =
                    static_cast<FT_Fixed>(std::clamp(shape.miterLimit, 1.0f, 32.0f) * 65536.0f);
                FT_Stroker_Set(stroker, halfWidth, toFtCap(shape.strokeCap),
                               toFtJoin(shape.strokeJoin), miterLimit);

                addSubpathsToStroker(stroker, shape.subpaths, shape.strokeCap, height);
                while (idx + 1 < shapes.size() &&
                       canMergePureOpaqueStroke(shape, shapes[idx + 1])) {
                    ++idx;
                    addSubpathsToStroker(stroker, shapes[idx].subpaths, shapes[idx].strokeCap,
                                         height);
                }

                FT_UInt numPoints = 0;
                FT_UInt numContours = 0;
                FT_Stroker_GetCounts(stroker, &numPoints, &numContours);
                if (numPoints > 0 && numContours > 0) {
                    FT_Outline outline{};
                    if (FT_Outline_New(ftLib, numPoints, static_cast<FT_Int>(numContours),
                                       &outline) == 0) {
                        outline.n_points = 0;
                        outline.n_contours = 0;
                        outline.flags = FT_OUTLINE_HIGH_PRECISION;
                        FT_Stroker_Export(stroker, &outline);
                        renderOutlineToBuffer(ftLib, outline, width, height, scratch);
                        compositeMask(outAlphaMask.subspan(0, pixelCount), scratch, 1.0f);
                        FT_Outline_Done(ftLib, &outline);
                    }
                }
                FT_Stroker_Done(stroker);
            }
            continue;
        }

        if (shape.hasFill && shape.opacity * shape.fillOpacity > 0.0f) {
            FT_UInt totalPts = 0;
            FT_Int totalContours = 0;
            for (const auto& sub : shape.subpaths) {
                if (sub.points.size() >= 4 && ((sub.points.size() - 1) % 3) == 0) {
                    totalPts += static_cast<FT_UInt>(sub.points.size());
                    ++totalContours;
                }
            }

            if (totalPts > 0 && totalContours > 0) {
                FT_Outline outline{};
                if (FT_Outline_New(ftLib, totalPts, totalContours, &outline) == 0) {
                    using TagType = std::remove_pointer_t<decltype(outline.tags)>;
                    using ContourType = std::remove_pointer_t<decltype(outline.contours)>;

                    FT_UInt pCursor = 0;
                    FT_Int cCursor = 0;
                    for (const auto& sub : shape.subpaths) {
                        if (sub.points.size() < 4 || ((sub.points.size() - 1) % 3) != 0) {
                            continue;
                        }
                        outline.points[pCursor] = toFtVec(sub.points[0]);
                        outline.tags[pCursor] = static_cast<TagType>(FT_CURVE_TAG_ON);
                        ++pCursor;

                        for (size_t i = 0; i + 3 < sub.points.size(); i += 3) {
                            outline.points[pCursor] = toFtVec(sub.points[i + 1]);
                            outline.tags[pCursor] = static_cast<TagType>(FT_CURVE_TAG_CUBIC);
                            ++pCursor;

                            outline.points[pCursor] = toFtVec(sub.points[i + 2]);
                            outline.tags[pCursor] = static_cast<TagType>(FT_CURVE_TAG_CUBIC);
                            ++pCursor;

                            outline.points[pCursor] = toFtVec(sub.points[i + 3]);
                            outline.tags[pCursor] = static_cast<TagType>(FT_CURVE_TAG_ON);
                            ++pCursor;
                        }

                        outline.contours[cCursor] = static_cast<ContourType>(pCursor - 1);
                        ++cCursor;
                    }

                    outline.n_points = static_cast<ContourType>(pCursor);
                    outline.n_contours = static_cast<ContourType>(cCursor);
                    outline.flags = FT_OUTLINE_HIGH_PRECISION;
                    if (shape.fillRule == FillRule::EvenOdd) {
                        outline.flags |= FT_OUTLINE_EVEN_ODD_FILL;
                    }

                    renderOutlineToBuffer(ftLib, outline, width, height, scratch);
                    compositeMask(outAlphaMask.subspan(0, pixelCount), scratch,
                                  shape.opacity * shape.fillOpacity);
                    FT_Outline_Done(ftLib, &outline);
                }
            }
        }

        if (shape.hasStroke && shape.strokeWidth > 0.0f &&
            shape.opacity * shape.strokeOpacity > 0.0f) {
            FT_Stroker stroker = nullptr;
            if (FT_Stroker_New(ftLib, &stroker) == 0 && stroker) {
                FT_Fixed halfWidth = std::max<FT_Fixed>(
                    16, static_cast<FT_Fixed>(std::llround(shape.strokeWidth * 32.0f)));
                FT_Fixed miterLimit =
                    static_cast<FT_Fixed>(std::clamp(shape.miterLimit, 1.0f, 32.0f) * 65536.0f);
                FT_Stroker_Set(stroker, halfWidth, toFtCap(shape.strokeCap),
                               toFtJoin(shape.strokeJoin), miterLimit);

                addSubpathsToStroker(stroker, shape.subpaths, shape.strokeCap, height);

                FT_UInt numPoints = 0;
                FT_UInt numContours = 0;
                FT_Stroker_GetCounts(stroker, &numPoints, &numContours);
                if (numPoints > 0 && numContours > 0) {
                    FT_Outline outline{};
                    if (FT_Outline_New(ftLib, numPoints, static_cast<FT_Int>(numContours),
                                       &outline) == 0) {
                        outline.n_points = 0;
                        outline.n_contours = 0;
                        outline.flags = FT_OUTLINE_HIGH_PRECISION;
                        FT_Stroker_Export(stroker, &outline);
                        renderOutlineToBuffer(ftLib, outline, width, height, scratch);
                        compositeMask(outAlphaMask.subspan(0, pixelCount), scratch,
                                      shape.opacity * shape.strokeOpacity);
                        FT_Outline_Done(ftLib, &outline);
                    }
                }
                FT_Stroker_Done(stroker);
            }
        }
    }

    return {};
}

} // namespace northern::text
