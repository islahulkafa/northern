#include "text/glyph_atlas.hpp"
#include "text/font_impl.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace northern::text {

GlyphAtlas::GlyphAtlas(graphics::Device& device, graphics::TextureHandle texture, uint32_t width,
                       uint32_t height) noexcept
    : device_(device), texture_(texture), width_(width), height_(height) {}

GlyphAtlas::~GlyphAtlas() {
    if (texture_.isValid()) {
        device_.destroyTexture(texture_);
    }
}

core::Result<std::unique_ptr<GlyphAtlas>, core::Error>
GlyphAtlas::create(graphics::Device& device, uint32_t width, uint32_t height) {
    graphics::TextureDesc desc{
        .width = width,
        .height = height,
        .format = graphics::Format::R8_Unorm,
        .usage = graphics::TextureUsage::Sampled | graphics::TextureUsage::TransferDst,
        .mipLevels = 1,
        .debugName = "GlyphAtlas",
    };

    auto texRes = device.createTexture(desc);
    if (texRes.isErr()) {
        return texRes.error();
    }

    return std::unique_ptr<GlyphAtlas>(new GlyphAtlas(device, texRes.value(), width, height));
}

core::Result<CachedGlyph, core::Error> GlyphAtlas::getOrInsert(const Font& font, uint32_t glyphId) {
    uint64_t key = (reinterpret_cast<uint64_t>(&font) ^ (static_cast<uint64_t>(glyphId) << 32));
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second;
    }

    if (!font.impl() || !font.impl()->ftFace) {
        return core::Error(core::ErrorCode::InvalidArgument, "Font implementation is invalid");
    }

    FT_Face face = font.impl()->ftFace;
    FT_Error err = FT_Load_Glyph(face, glyphId, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL);
    if (err != 0) {
        return core::Error(core::ErrorCode::InternalError, "Failed to load and render glyph");
    }

    FT_GlyphSlot slot = face->glyph;
    FT_Bitmap& bitmap = slot->bitmap;

    CachedGlyph cached{};
    cached.offset = {static_cast<float>(slot->bitmap_left), static_cast<float>(slot->bitmap_top)};
    cached.size = {static_cast<float>(bitmap.width), static_cast<float>(bitmap.rows)};

    if (bitmap.width == 0 || bitmap.rows == 0) {
        cache_[key] = cached;
        return cached;
    }

    uint32_t w = bitmap.width;
    uint32_t h = bitmap.rows;
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

    if (bitmap.pitch == static_cast<int>(w)) {
        auto writeRes = device_.writeTexture(texture_, x, y, w, h, bitmap.buffer);
        if (writeRes.isErr()) {
            return writeRes.error();
        }
    } else {
        std::vector<uint8_t> tightlyPacked(w * h);
        for (uint32_t r = 0; r < h; ++r) {
            std::memcpy(tightlyPacked.data() + r * w, bitmap.buffer + r * bitmap.pitch, w);
        }
        auto writeRes = device_.writeTexture(texture_, x, y, w, h, tightlyPacked.data());
        if (writeRes.isErr()) {
            return writeRes.error();
        }
    }

    cached.uvRect = {
        static_cast<float>(x) / static_cast<float>(width_),
        static_cast<float>(y) / static_cast<float>(height_),
        static_cast<float>(w) / static_cast<float>(width_),
        static_cast<float>(h) / static_cast<float>(height_),
    };

    cache_[key] = cached;
    return cached;
}

} // namespace northern::text
