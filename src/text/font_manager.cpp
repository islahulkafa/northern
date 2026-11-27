#include "text/font_manager.hpp"
#include <cmath>
#include <filesystem>
#include <vector>

namespace northern::text {

FontManager& FontManager::instance() noexcept {
    static FontManager instance;
    return instance;
}

FontManager::FontManager() {
    auto findFirstExisting = [](const std::vector<std::string>& candidates) -> std::string {
        for (const auto& path : candidates) {
            if (std::filesystem::exists(path)) {
                return path;
            }
        }
        return {};
    };

#ifdef _WIN32
    variableFontPath_ = findFirstExisting({"C:/Windows/Fonts/SegUIVar.ttf"});
    defaultFontPath_ =
        findFirstExisting({"C:/Windows/Fonts/SegUIVar.ttf", "C:/Windows/Fonts/segoeui.ttf",
                           "C:/Windows/Fonts/arial.ttf", "C:/Windows/Fonts/calibri.ttf"});
    mediumFontPath_ =
        findFirstExisting({"C:/Windows/Fonts/SegUIVar.ttf", "C:/Windows/Fonts/seguisb.ttf",
                           "C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf"});
    semiBoldFontPath_ =
        findFirstExisting({"C:/Windows/Fonts/SegUIVar.ttf", "C:/Windows/Fonts/seguisb.ttf",
                           "C:/Windows/Fonts/segoeuib.ttf", "C:/Windows/Fonts/arialbd.ttf"});
    boldFontPath_ =
        findFirstExisting({"C:/Windows/Fonts/SegUIVar.ttf", "C:/Windows/Fonts/segoeuib.ttf",
                           "C:/Windows/Fonts/arialbd.ttf", "C:/Windows/Fonts/calibrib.ttf"});
    monospaceFontPath_ =
        findFirstExisting({"C:/Windows/Fonts/CascadiaMono.ttf", "C:/Windows/Fonts/CascadiaCode.ttf",
                           "C:/Windows/Fonts/consola.ttf", "C:/Windows/Fonts/lucon.ttf"});
#else
    defaultFontPath_ = findFirstExisting(
        {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf",
         "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
         "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf"});
    mediumFontPath_ = defaultFontPath_;
    semiBoldFontPath_ = findFirstExisting({"/usr/share/fonts/truetype/ubuntu/Ubuntu-M.ttf",
                                           "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
                                           defaultFontPath_});
    boldFontPath_ =
        findFirstExisting({"/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
                           "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
                           "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
                           "/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf"});
    monospaceFontPath_ =
        findFirstExisting({"/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
                           "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
                           "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf"});
#endif
}

FontManager::~FontManager() = default;

void FontManager::registerFont(std::string_view familyName, std::string_view filePath) {
    fontRegistry_[std::string(familyName)] = std::string(filePath);
}

std::string FontManager::resolveSystemFont(std::string_view familyName) const {
    auto it = fontRegistry_.find(std::string(familyName));
    if (it != fontRegistry_.end()) {
        return it->second;
    }

    if (std::filesystem::exists(familyName)) {
        return std::string(familyName);
    }

#ifdef _WIN32
    std::string winPath = "C:/Windows/Fonts/" + std::string(familyName) + ".ttf";
    if (std::filesystem::exists(winPath)) {
        return winPath;
    }
#endif

    return defaultFontPath_;
}

std::string FontManager::resolvePathForWeight(FontWeight weight) const {
    if (!variableFontPath_.empty()) {
        return variableFontPath_;
    }
    switch (weight) {
    case FontWeight::Medium:
        return !mediumFontPath_.empty() ? mediumFontPath_ : defaultFontPath_;
    case FontWeight::SemiBold:
        return !semiBoldFontPath_.empty() ? semiBoldFontPath_ : boldFontPath_;
    case FontWeight::Bold:
        return !boldFontPath_.empty() ? boldFontPath_ : defaultFontPath_;
    case FontWeight::Regular:
    default:
        return defaultFontPath_;
    }
}

core::Result<std::shared_ptr<Font>, core::Error> FontManager::defaultFont(float pixelSize) {
    return font(FontWeight::Regular, pixelSize);
}

core::Result<std::shared_ptr<Font>, core::Error> FontManager::mediumFont(float pixelSize) {
    return font(FontWeight::Medium, pixelSize);
}

core::Result<std::shared_ptr<Font>, core::Error> FontManager::semiBoldFont(float pixelSize) {
    return font(FontWeight::SemiBold, pixelSize);
}

core::Result<std::shared_ptr<Font>, core::Error> FontManager::boldFont(float pixelSize) {
    return font(FontWeight::Bold, pixelSize);
}

core::Result<std::shared_ptr<Font>, core::Error> FontManager::monospaceFont(float pixelSize) {
    if (!monospaceFontPath_.empty()) {
        return loadFont(monospaceFontPath_, pixelSize, FontWeight::Regular);
    }
    return defaultFont(pixelSize);
}

core::Result<std::shared_ptr<Font>, core::Error>
FontManager::font(FontWeight weight, float pixelSize, float opticalSizePt) {
    std::string path = resolvePathForWeight(weight);
    if (path.empty()) {
        return core::Error(core::ErrorCode::NotFound, "No default system font found on platform");
    }
    return loadFont(path, pixelSize, weight, opticalSizePt);
}

core::Result<std::shared_ptr<Font>, core::Error>
FontManager::loadFont(std::string_view familyOrPath, float pixelSize, FontWeight weight,
                      float opticalSizePt) {
    std::string resolvedPath = resolveSystemFont(familyOrPath);
    if (resolvedPath.empty()) {
        return core::Error(core::ErrorCode::NotFound,
                           "Failed to resolve font: " + std::string(familyOrPath));
    }

    long quantizedPx = std::lround(pixelSize * 4.0f);
    long quantizedOpsz = std::lround(opticalSizePt * 4.0f);
    std::string cacheKey = resolvedPath + ":" + std::to_string(quantizedPx) + ":" +
                           std::to_string(static_cast<uint16_t>(weight)) + ":" +
                           std::to_string(quantizedOpsz);
    auto it = fontCache_.find(cacheKey);
    if (it != fontCache_.end()) {
        return it->second;
    }

    auto fontRes = Font::fromFile(resolvedPath, pixelSize, weight, opticalSizePt);
    if (fontRes.isErr()) {
        return fontRes.error();
    }

    auto loaded = fontRes.value();
    fontCache_[cacheKey] = loaded;
    return loaded;
}

} // namespace northern::text
