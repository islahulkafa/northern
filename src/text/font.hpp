#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "text/types.hpp"
#include <memory>
#include <span>
#include <string_view>

namespace northern::text {

class Font {
public:
    static core::Result<std::shared_ptr<Font>, core::Error>
    fromFile(std::string_view filePath, float pixelSize = 13.0f,
             FontWeight weight = FontWeight::Regular, float opticalSizePt = 0.0f);

    static core::Result<std::shared_ptr<Font>, core::Error>
    fromMemory(std::span<const uint8_t> data, float pixelSize = 13.0f,
               FontWeight weight = FontWeight::Regular, float opticalSizePt = 0.0f);

    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&&) noexcept;
    Font& operator=(Font&&) noexcept;

    [[nodiscard]] uint32_t id() const noexcept;
    [[nodiscard]] float pointSize() const noexcept;
    [[nodiscard]] float pixelSize() const noexcept;
    [[nodiscard]] FontWeight weight() const noexcept;
    [[nodiscard]] float ascender() const noexcept;
    [[nodiscard]] float descender() const noexcept;
    [[nodiscard]] float lineHeight() const noexcept;
    [[nodiscard]] float capHeight() const noexcept;

    struct Impl;
    [[nodiscard]] Impl* impl() const noexcept { return impl_.get(); }

private:
    explicit Font(std::unique_ptr<Impl> impl) noexcept;
    std::unique_ptr<Impl> impl_;
};

} // namespace northern::text
