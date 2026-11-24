#include "text/font_manager.hpp"
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
    defaultFontPath_ =
        findFirstExisting({"C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/segoeuib.ttf",
                           "C:/Windows/Fonts/arial.ttf", "C:/Windows/Fonts/calibri.ttf"});
    boldFontPath_ =
        findFirstExisting({"C:/Windows/Fonts/segoeuib.ttf", "C:/Windows/Fonts/arialbd.ttf",
                           "C:/Windows/Fonts/calibrib.ttf"});
    monospaceFontPath_ =
        findFirstExisting({"C:/Windows/Fonts/CascadiaMono.ttf", "C:/Windows/Fonts/consola.ttf",
                           "C:/Windows/Fonts/lucon.ttf"});
#else
    defaultFontPath_ = findFirstExisting(
        {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf",
         "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
         "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf"});
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

core::Result<std::shared_ptr<Font>, core::Error> FontManager::defaultFont(float pointSize) {
    if (defaultFontPath_.empty()) {
        return core::Error(core::ErrorCode::NotFound, "No default system font found on platform");
    }
    return loadFont(defaultFontPath_, pointSize);
}

core::Result<std::shared_ptr<Font>, core::Error> FontManager::boldFont(float pointSize) {
    if (!boldFontPath_.empty()) {
        return loadFont(boldFontPath_, pointSize);
    }
    return defaultFont(pointSize);
}

core::Result<std::shared_ptr<Font>, core::Error> FontManager::monospaceFont(float pointSize) {
    if (!monospaceFontPath_.empty()) {
        return loadFont(monospaceFontPath_, pointSize);
    }
    return defaultFont(pointSize);
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
