#include "text/font_manager.hpp"
#include <filesystem>
#include <vector>

namespace northern::text {

FontManager& FontManager::instance() noexcept {
    static FontManager instance;
    return instance;
}

FontManager::FontManager() {
    // Standard system font fallbacks
    const std::vector<std::string> candidateFonts = {
#ifdef _WIN32
        "C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arial.ttf", "C:/Windows/Fonts/calibri.ttf"
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf"
#endif
    };

    for (const auto& path : candidateFonts) {
        if (std::filesystem::exists(path)) {
            defaultFontPath_ = path;
            break;
        }
    }
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

core::Result<std::shared_ptr<Font>, core::Error> FontManager::defaultFont(float pointSize) {
    if (defaultFontPath_.empty()) {
        return core::Error(core::ErrorCode::NotFound, "No default system font found on platform");
    }
    return loadFont(defaultFontPath_, pointSize);
}

core::Result<std::shared_ptr<Font>, core::Error>
FontManager::loadFont(std::string_view familyOrPath, float pointSize) {
    std::string resolvedPath = resolveSystemFont(familyOrPath);
    if (resolvedPath.empty()) {
        return core::Error(core::ErrorCode::NotFound,
                           "Failed to resolve font: " + std::string(familyOrPath));
    }

    std::string cacheKey = resolvedPath + ":" + std::to_string(pointSize);
    auto it = fontCache_.find(cacheKey);
    if (it != fontCache_.end()) {
        return it->second;
    }

    auto fontRes = Font::fromFile(resolvedPath, pointSize);
    if (fontRes.isErr()) {
        return fontRes.error();
    }

    auto font = fontRes.value();
    fontCache_[cacheKey] = font;
    return font;
}

} // namespace northern::text
