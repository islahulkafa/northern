#include "text/glyph_atlas.hpp"
#include "text/font_impl.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace northern::text {

GlyphAtlas::GlyphAtlas(graphics::Device& device, graphics::TextureHandle texture, uint32_t width,
                       uint32_t height, GlyphRenderMode mode) noexcept
    : device_(device), texture_(texture), width_(width), height_(height), mode_(mode),
      bytesPerPixel_(mode == GlyphRenderMode::LcdRgb ? 4u : 1u),
      cpuAtlas_(static_cast<size_t>(width) * height * bytesPerPixel_, 0), dirtyMinY_(height),
      dirtyMaxY_(0) {}

GlyphAtlas::~GlyphAtlas() {
    if (texture_.isValid()) {
        device_.destroyTexture(texture_);
    }
}

core::Result<std::unique_ptr<GlyphAtlas>, core::Error> GlyphAtlas::create(graphics::Device& device,
                                                                          uint32_t width,
                                                                          uint32_t height,
                                                                          GlyphRenderMode mode) {
    graphics::TextureDesc desc{
        .width = width,
        .height = height,
        .format = (mode == GlyphRenderMode::LcdRgb) ? graphics::Format::R8G8B8A8_Unorm
                                                    : graphics::Format::R8_Unorm,
        .usage = graphics::TextureUsage::Sampled | graphics::TextureUsage::TransferDst,
        .mipLevels = 1,
        .sampler =
            {
                .minFilter = graphics::Filter::Nearest,
                .magFilter = graphics::Filter::Nearest,
                .addressModeU = graphics::AddressMode::ClampToEdge,
                .addressModeV = graphics::AddressMode::ClampToEdge,
            },
        .debugName = "GlyphAtlas",
    };

    auto texRes = device.createTexture(desc);
    if (texRes.isErr()) {
        return texRes.error();
    }

    return std::unique_ptr<GlyphAtlas>(new GlyphAtlas(device, texRes.value(), width, height, mode));
}

core::Result<CachedGlyph, core::Error> GlyphAtlas::getOrInsert(const Font& font, uint32_t glyphId,
                                                               uint8_t subpixelBin, bool useLcd) {
    uint64_t fontId = font.id();
    uint64_t bin = subpixelBin & 0x03;
    bool renderLcd = (mode_ == GlyphRenderMode::LcdRgb && useLcd);
    uint64_t lcdBit = renderLcd ? 1ULL : 0ULL;
    uint64_t key = (fontId << 34) | (static_cast<uint64_t>(glyphId) << 3) | (lcdBit << 2) | bin;
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second;
    }

    if (!font.impl() || !font.impl()->ftFace) {
        return core::Error(core::ErrorCode::InvalidArgument, "Font implementation is invalid");
    }

    FT_Face face = font.impl()->ftFace;
    FT_Error err = FT_Load_Glyph(face, glyphId, FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_LIGHT);
    if (err != 0) {
        return core::Error(core::ErrorCode::InternalError, "Failed to load glyph outline");
    }

    FT_GlyphSlot slot = face->glyph;
    if (slot->format == FT_GLYPH_FORMAT_OUTLINE && bin > 0) {
        FT_Outline_Translate(&slot->outline, static_cast<FT_Pos>(bin * 16), 0);
    }

    err = FT_Render_Glyph(slot, renderLcd ? FT_RENDER_MODE_LCD : FT_RENDER_MODE_NORMAL);
    if (err != 0) {
        return core::Error(core::ErrorCode::InternalError, "Failed to render glyph");
    }

    FT_Bitmap& bitmap = slot->bitmap;
    uint32_t w = renderLcd ? (bitmap.width / 3) : bitmap.width;
    uint32_t h = bitmap.rows;

    CachedGlyph cached{};
    cached.offset = {static_cast<float>(slot->bitmap_left), static_cast<float>(slot->bitmap_top)};
    cached.size = {static_cast<float>(w), static_cast<float>(h)};
    cached.subpixelBin = static_cast<uint8_t>(bin);

    if (w == 0 || h == 0) {
        cache_[key] = cached;
        return cached;
    }

    uint32_t pad = 1;

    if (currentShelfX_ + w + pad > width_) {
        currentShelfX_ = 1;
        currentShelfY_ += shelfHeight_ + pad;
        shelfHeight_ = 0;
    }

    if (currentShelfY_ + h + pad > height_) {
        return core::Error(core::ErrorCode::OutOfMemory, "Glyph atlas is full");
    }

    uint32_t x = currentShelfX_;
    uint32_t y = currentShelfY_;

    currentShelfX_ += w + pad;
    shelfHeight_ = std::max(shelfHeight_, h);

    for (uint32_t r = 0; r < h; ++r) {
        const uint8_t* srcRow = bitmap.buffer + static_cast<size_t>(r) * bitmap.pitch;
        uint8_t* dstRow =
            cpuAtlas_.data() + (static_cast<size_t>(y + r) * width_ + x) * bytesPerPixel_;
        if (renderLcd) {
            for (uint32_t c = 0; c < w; ++c) {
                dstRow[c * 4 + 0] = srcRow[c * 3 + 0];
                dstRow[c * 4 + 1] = srcRow[c * 3 + 1];
                dstRow[c * 4 + 2] = srcRow[c * 3 + 2];
                dstRow[c * 4 + 3] = 255;
            }
        } else if (bytesPerPixel_ == 4) {
            for (uint32_t c = 0; c < w; ++c) {
                uint8_t a = srcRow[c];
                dstRow[c * 4 + 0] = a;
                dstRow[c * 4 + 1] = a;
                dstRow[c * 4 + 2] = a;
                dstRow[c * 4 + 3] = 255;
            }
        } else {
            std::memcpy(dstRow, srcRow, w);
        }
    }

    dirty_ = true;
    dirtyMinY_ = std::min(dirtyMinY_, y);
    dirtyMaxY_ = std::max(dirtyMaxY_, y + h);

    cached.uvRect = {
        static_cast<float>(x) / static_cast<float>(width_),
        static_cast<float>(y) / static_cast<float>(height_),
        static_cast<float>(w) / static_cast<float>(width_),
        static_cast<float>(h) / static_cast<float>(height_),
    };

    cache_[key] = cached;
    return cached;
}

std::optional<CachedGlyph> GlyphAtlas::findCustomMask(uint64_t maskKey) const {
    uint64_t key = maskKey | (1ULL << 63);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second;
    }
    return std::nullopt;
}

core::Result<CachedGlyph, core::Error>
GlyphAtlas::insertCustomMask(uint64_t maskKey, uint32_t w, uint32_t h,
                             std::span<const uint8_t> alphaData) {
    uint64_t key = maskKey | (1ULL << 63);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second;
    }

    if (w == 0 || h == 0 || alphaData.size() < static_cast<size_t>(w) * static_cast<size_t>(h)) {
        return core::Error(core::ErrorCode::InvalidArgument, "Invalid custom mask buffer");
    }

    uint32_t pad = 1;
    if (currentShelfX_ + w + pad > width_) {
        currentShelfX_ = 1;
        currentShelfY_ += shelfHeight_ + pad;
        shelfHeight_ = 0;
    }

    if (currentShelfY_ + h + pad > height_) {
        return core::Error(core::ErrorCode::OutOfMemory, "Glyph atlas is full");
    }

    uint32_t x = currentShelfX_;
    uint32_t y = currentShelfY_;

    currentShelfX_ += w + pad;
    shelfHeight_ = std::max(shelfHeight_, h);

    for (uint32_t r = 0; r < h; ++r) {
        const uint8_t* srcRow = alphaData.data() + static_cast<size_t>(r) * w;
        uint8_t* dstRow =
            cpuAtlas_.data() + (static_cast<size_t>(y + r) * width_ + x) * bytesPerPixel_;
        if (bytesPerPixel_ == 4) {
            for (uint32_t c = 0; c < w; ++c) {
                uint8_t a = srcRow[c];
                dstRow[c * 4 + 0] = a;
                dstRow[c * 4 + 1] = a;
                dstRow[c * 4 + 2] = a;
                dstRow[c * 4 + 3] = 255;
            }
        } else {
            std::memcpy(dstRow, srcRow, w);
        }
    }

    dirty_ = true;
    dirtyMinY_ = std::min(dirtyMinY_, y);
    dirtyMaxY_ = std::max(dirtyMaxY_, y + h);

    CachedGlyph cached{};
    cached.offset = {0.0f, 0.0f};
    cached.size = {static_cast<float>(w), static_cast<float>(h)};
    cached.uvRect = {
        static_cast<float>(x) / static_cast<float>(width_),
        static_cast<float>(y) / static_cast<float>(height_),
        static_cast<float>(w) / static_cast<float>(width_),
        static_cast<float>(h) / static_cast<float>(height_),
    };

    cache_[key] = cached;
    return cached;
}

core::Result<void, core::Error> GlyphAtlas::flush() {
    if (!dirty_ || dirtyMinY_ >= dirtyMaxY_) {
        return {};
    }

    uint32_t y = dirtyMinY_;
    uint32_t h = dirtyMaxY_ - dirtyMinY_;
    const uint8_t* sliceData = cpuAtlas_.data() + static_cast<size_t>(y) * width_ * bytesPerPixel_;

    auto writeRes = device_.writeTexture(texture_, 0, y, width_, h, sliceData);
    if (writeRes.isErr()) {
        return writeRes.error();
    }

    dirty_ = false;
    dirtyMinY_ = height_;
    dirtyMaxY_ = 0;

    return {};
}

} // namespace northern::text
