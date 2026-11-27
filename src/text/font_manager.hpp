#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "text/font.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace northern::text {

class FontManager {
public:
    static FontManager& instance() noexcept;

    core::Result<std::shared_ptr<Font>, core::Error> defaultFont(float pixelSize = 13.0f);
    core::Result<std::shared_ptr<Font>, core::Error> mediumFont(float pixelSize = 13.0f);
    core::Result<std::shared_ptr<Font>, core::Error> semiBoldFont(float pixelSize = 13.0f);
    core::Result<std::shared_ptr<Font>, core::Error> boldFont(float pixelSize = 13.0f);
    core::Result<std::shared_ptr<Font>, core::Error> monospaceFont(float pixelSize = 12.0f);
    core::Result<std::shared_ptr<Font>, core::Error>
    font(FontWeight weight, float pixelSize = 13.0f, float opticalSizePt = 0.0f);
    core::Result<std::shared_ptr<Font>, core::Error>
    loadFont(std::string_view familyOrPath, float pixelSize = 13.0f,
             FontWeight weight = FontWeight::Regular, float opticalSizePt = 0.0f);
    void registerFont(std::string_view familyName, std::string_view filePath);

private:
    FontManager();
    ~FontManager();

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    std::string resolveSystemFont(std::string_view familyName) const;
    std::string resolvePathForWeight(FontWeight weight) const;

    std::unordered_map<std::string, std::string> fontRegistry_;
    std::unordered_map<std::string, std::shared_ptr<Font>> fontCache_;
    std::string variableFontPath_;
    std::string defaultFontPath_;
    std::string mediumFontPath_;
    std::string semiBoldFontPath_;
    std::string boldFontPath_;
    std::string monospaceFontPath_;
};

} // namespace northern::text
