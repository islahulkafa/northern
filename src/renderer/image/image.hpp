#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "core/math/geometry.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace northern::renderer {

class Image {
public:
    struct Impl;

    static core::Result<std::shared_ptr<Image>, core::Error> fromFile(std::string_view filePath);

    static core::Result<std::shared_ptr<Image>, core::Error>
    fromMemory(std::span<const uint8_t> encodedBytes);

    static core::Result<std::shared_ptr<Image>, core::Error>
    fromPixels(uint32_t width, uint32_t height, std::span<const uint8_t> rgbaPixels);

    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&) noexcept;
    Image& operator=(Image&&) noexcept;

    [[nodiscard]] uint32_t id() const noexcept;
    [[nodiscard]] uint32_t width() const noexcept;
    [[nodiscard]] uint32_t height() const noexcept;
    [[nodiscard]] core::Size size() const noexcept;
    [[nodiscard]] float aspectRatio() const noexcept;
    [[nodiscard]] std::span<const uint8_t> pixels() const noexcept;
    [[nodiscard]] size_t byteSize() const noexcept;
    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] const std::shared_ptr<const Impl>& sharedImpl() const noexcept;

    static uint32_t implId(const Impl& impl) noexcept;
    static uint32_t implWidth(const Impl& impl) noexcept;
    static uint32_t implHeight(const Impl& impl) noexcept;
    static std::span<const uint8_t> implPixels(const Impl& impl) noexcept;

private:
    explicit Image(std::shared_ptr<const Impl> impl) noexcept;

    std::shared_ptr<const Impl> impl_;
};

} // namespace northern::renderer
