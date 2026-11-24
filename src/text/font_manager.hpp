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

    core::Result<std::shared_ptr<Font>, core::Error> defaultFont(float pointSize = 14.0f);
    core::Result<std::shared_ptr<Font>, core::Error> boldFont(float pointSize = 14.0f);
    core::Result<std::shared_ptr<Font>, core::Error> monospaceFont(float pointSize = 13.0f);
    core::Result<std::shared_ptr<Font>, core::Error> loadFont(std::string_view familyOrPath,
                                                              float pointSize = 14.0f);
    void registerFont(std::string_view familyName, std::string_view filePath);

private:
    FontManager();
    ~FontManager();

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    std::string resolveSystemFont(std::string_view familyName) const;

    std::unordered_map<std::string, std::string> fontRegistry_;
    std::unordered_map<std::string, std::shared_ptr<Font>> fontCache_;
    std::string defaultFontPath_;
    std::string boldFontPath_;
    std::string monospaceFontPath_;
};

} // namespace northern::text
