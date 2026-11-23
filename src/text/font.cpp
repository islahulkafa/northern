#include "text/font.hpp"
#include "text/font_impl.hpp"

#include <atomic>
#include <cmath>

namespace northern::text {

namespace {

std::atomic<uint32_t> sNextFontId{1};

FT_Library getFtLibrary() {
    static FT_Library ftLib = nullptr;
    if (!ftLib) {
        if (FT_Init_FreeType(&ftLib) != 0) {
            return nullptr;
        }
    }
    return ftLib;
}

} // namespace

Font::Font(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

Font::~Font() = default;
Font::Font(Font&&) noexcept = default;
Font& Font::operator=(Font&&) noexcept = default;

uint32_t Font::id() const noexcept {
    return impl_ ? impl_->id : 0;
}

float Font::pointSize() const noexcept {
    return impl_ ? impl_->pointSize : 0.0f;
}

float Font::ascender() const noexcept {
    return impl_ ? impl_->ascender : 0.0f;
}

float Font::descender() const noexcept {
    return impl_ ? impl_->descender : 0.0f;
}

float Font::lineHeight() const noexcept {
    return impl_ ? impl_->lineHeight : 0.0f;
}

core::Result<std::shared_ptr<Font>, core::Error> Font::fromFile(std::string_view filePath,
                                                                float pointSize) {
    FT_Library ftLib = getFtLibrary();
    if (!ftLib) {
        return core::Error(core::ErrorCode::InvalidState, "Failed to initialize FreeType library");
    }

    std::string pathStr(filePath);
    FT_Face face = nullptr;
    FT_Error err = FT_New_Face(ftLib, pathStr.c_str(), 0, &face);
    if (err != 0 || !face) {
        return core::Error(core::ErrorCode::NotFound, "Failed to load font from file: " + pathStr);
    }

    FT_Set_Char_Size(face, 0, static_cast<FT_F26Dot6>(pointSize * 64.0f), 96, 96);

    hb_font_t* hbFont = hb_ft_font_create_referenced(face);
    if (!hbFont) {
        FT_Done_Face(face);
        return core::Error(core::ErrorCode::GraphicsError, "Failed to create HarfBuzz font handle");
    }

    auto impl = std::make_unique<Impl>();
    impl->id = sNextFontId.fetch_add(1, std::memory_order_relaxed);
    impl->ftFace = face;
    impl->hbFont = hbFont;
    impl->pointSize = pointSize;
    impl->ascender = static_cast<float>(face->size->metrics.ascender) / 64.0f;
    impl->descender = static_cast<float>(face->size->metrics.descender) / 64.0f;
    impl->lineHeight = static_cast<float>(face->size->metrics.height) / 64.0f;

    return std::shared_ptr<Font>(new Font(std::move(impl)));
}

core::Result<std::shared_ptr<Font>, core::Error> Font::fromMemory(std::span<const uint8_t> data,
                                                                  float pointSize) {
    if (data.empty()) {
        return core::Error(core::ErrorCode::InvalidArgument, "Font data buffer cannot be empty");
    }

    FT_Library ftLib = getFtLibrary();
    if (!ftLib) {
        return core::Error(core::ErrorCode::InvalidState, "Failed to initialize FreeType library");
    }

    auto impl = std::make_unique<Impl>();
    impl->id = sNextFontId.fetch_add(1, std::memory_order_relaxed);
    impl->memoryBuffer.assign(data.begin(), data.end());

    FT_Face face = nullptr;
    FT_Error err =
        FT_New_Memory_Face(ftLib, reinterpret_cast<const FT_Byte*>(impl->memoryBuffer.data()),
                           static_cast<FT_Long>(impl->memoryBuffer.size()), 0, &face);

    if (err != 0 || !face) {
        return core::Error(core::ErrorCode::InvalidArgument,
                           "Failed to load font from memory buffer");
    }

    FT_Set_Char_Size(face, 0, static_cast<FT_F26Dot6>(pointSize * 64.0f), 96, 96);

    hb_font_t* hbFont = hb_ft_font_create_referenced(face);
    if (!hbFont) {
        FT_Done_Face(face);
        return core::Error(core::ErrorCode::GraphicsError, "Failed to create HarfBuzz font handle");
    }

    impl->ftFace = face;
    impl->hbFont = hbFont;
    impl->pointSize = pointSize;
    impl->ascender = static_cast<float>(face->size->metrics.ascender) / 64.0f;
    impl->descender = static_cast<float>(face->size->metrics.descender) / 64.0f;
    impl->lineHeight = static_cast<float>(face->size->metrics.height) / 64.0f;

    return std::shared_ptr<Font>(new Font(std::move(impl)));
}

} // namespace northern::text
