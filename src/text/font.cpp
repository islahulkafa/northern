#include "text/font.hpp"
#include "text/font_impl.hpp"

#include <freetype/tttables.h>

#include <algorithm>
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
        FT_Bool noStemDarkening = 0;
        FT_Property_Set(ftLib, "autofitter", "no-stem-darkening", &noStemDarkening);
        FT_Property_Set(ftLib, "cff", "no-stem-darkening", &noStemDarkening);
        FT_Int darkenParams[8] = {500, 250, 1000, 175, 1667, 150, 2333, 0};
        FT_Property_Set(ftLib, "autofitter", "darken-params", darkenParams);
        FT_Property_Set(ftLib, "cff", "darken-params", darkenParams);
        FT_Library_SetLcdFilter(ftLib, FT_LCD_FILTER_DEFAULT);
    }
    return ftLib;
}

void applyVariableAxes(FT_Face face, FontWeight weight, float pixelSize, float opticalSizePt) {
    if (!face || !FT_HAS_MULTIPLE_MASTERS(face)) {
        return;
    }
    FT_MM_Var* mmVar = nullptr;
    if (FT_Get_MM_Var(face, &mmVar) != 0 || !mmVar) {
        return;
    }
    std::vector<FT_Fixed> coords(mmVar->num_axis);
    for (FT_UInt i = 0; i < mmVar->num_axis; ++i) {
        coords[i] = mmVar->axis[i].def;
        FT_ULong tag = mmVar->axis[i].tag;
        double minVal = static_cast<double>(mmVar->axis[i].minimum) / 65536.0;
        double maxVal = static_cast<double>(mmVar->axis[i].maximum) / 65536.0;
        if (tag == FT_MAKE_TAG('w', 'g', 'h', 't')) {
            double target = std::clamp(static_cast<double>(weight), minVal, maxVal);
            coords[i] = static_cast<FT_Fixed>(std::llround(target * 65536.0));
        } else if (tag == FT_MAKE_TAG('o', 'p', 's', 'z')) {
            double targetPt =
                (opticalSizePt > 0.0f) ? static_cast<double>(opticalSizePt) : (pixelSize * 0.75);
            double target = std::clamp(targetPt, minVal, maxVal);
            coords[i] = static_cast<FT_Fixed>(std::llround(target * 65536.0));
        }
    }
    FT_Set_Var_Design_Coordinates(face, mmVar->num_axis, coords.data());
    FT_Done_MM_Var(getFtLibrary(), mmVar);
}

core::Result<std::unique_ptr<Font::Impl>, core::Error>
buildFontFromFace(FT_Face face, float pixelSize, FontWeight weight, float opticalSizePt,
                  std::vector<uint8_t> memoryBuffer) {
    if (pixelSize <= 0.0f) {
        pixelSize = 13.0f;
    }

    applyVariableAxes(face, weight, pixelSize, opticalSizePt);
    FT_Set_Char_Size(face, 0, static_cast<FT_F26Dot6>(std::lround(pixelSize * 64.0f)), 72, 72);

    hb_font_t* hbFont = hb_ft_font_create_referenced(face);
    if (!hbFont) {
        FT_Done_Face(face);
        return core::Error(core::ErrorCode::GraphicsError, "Failed to create HarfBuzz font handle");
    }
    hb_ft_font_set_load_flags(hbFont, FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_LIGHT);
    hb_ft_font_changed(hbFont);

    auto impl = std::make_unique<Font::Impl>();
    impl->id = sNextFontId.fetch_add(1, std::memory_order_relaxed);
    impl->ftFace = face;
    impl->hbFont = hbFont;
    impl->pixelSize = pixelSize;
    impl->pointSize = pixelSize * (72.0f / 96.0f);
    impl->weight = weight;
    impl->ascender = static_cast<float>(face->size->metrics.ascender) / 64.0f;
    impl->descender = static_cast<float>(face->size->metrics.descender) / 64.0f;
    impl->lineHeight = static_cast<float>(face->size->metrics.height) / 64.0f;
    impl->memoryBuffer = std::move(memoryBuffer);

    TT_OS2* os2 = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(face, FT_SFNT_OS2));
    if (os2 && os2->sCapHeight > 0 && face->units_per_EM > 0) {
        impl->capHeight = static_cast<float>(os2->sCapHeight) *
                          (impl->pixelSize / static_cast<float>(face->units_per_EM));
    } else {
        impl->capHeight = impl->pixelSize * 0.70f;
    }

    return impl;
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

float Font::pixelSize() const noexcept {
    return impl_ ? impl_->pixelSize : 0.0f;
}

FontWeight Font::weight() const noexcept {
    return impl_ ? impl_->weight : FontWeight::Regular;
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

float Font::capHeight() const noexcept {
    return impl_ ? impl_->capHeight : 0.0f;
}

core::Result<std::shared_ptr<Font>, core::Error>
Font::fromFile(std::string_view filePath, float pixelSize, FontWeight weight, float opticalSizePt) {
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

    auto res = buildFontFromFace(face, pixelSize, weight, opticalSizePt, {});
    if (res.isErr()) {
        return res.error();
    }
    return std::shared_ptr<Font>(new Font(std::move(res.value())));
}

core::Result<std::shared_ptr<Font>, core::Error> Font::fromMemory(std::span<const uint8_t> data,
                                                                  float pixelSize,
                                                                  FontWeight weight,
                                                                  float opticalSizePt) {
    if (data.empty()) {
        return core::Error(core::ErrorCode::InvalidArgument, "Font data buffer cannot be empty");
    }

    FT_Library ftLib = getFtLibrary();
    if (!ftLib) {
        return core::Error(core::ErrorCode::InvalidState, "Failed to initialize FreeType library");
    }

    std::vector<uint8_t> memoryBuffer(data.begin(), data.end());
    FT_Face face = nullptr;
    FT_Error err = FT_New_Memory_Face(ftLib, reinterpret_cast<const FT_Byte*>(memoryBuffer.data()),
                                      static_cast<FT_Long>(memoryBuffer.size()), 0, &face);

    if (err != 0 || !face) {
        return core::Error(core::ErrorCode::InvalidArgument,
                           "Failed to load font from memory buffer");
    }

    auto res = buildFontFromFace(face, pixelSize, weight, opticalSizePt, std::move(memoryBuffer));
    if (res.isErr()) {
        return res.error();
    }
    return std::shared_ptr<Font>(new Font(std::move(res.value())));
}

} // namespace northern::text
