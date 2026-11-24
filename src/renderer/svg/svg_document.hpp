#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "core/math/color.hpp"
#include "core/math/geometry.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace northern::renderer {

class PaintContext;

class SvgDocument {
public:
    struct Impl;

    static core::Result<std::shared_ptr<SvgDocument>, core::Error>
    fromString(std::string_view svgContent);
    static core::Result<std::shared_ptr<SvgDocument>, core::Error>
    fromFile(const std::string& filePath);

    ~SvgDocument();

    SvgDocument(const SvgDocument&) = delete;
    SvgDocument& operator=(const SvgDocument&) = delete;
    SvgDocument(SvgDocument&&) noexcept;
    SvgDocument& operator=(SvgDocument&&) noexcept;

    [[nodiscard]] uint32_t id() const noexcept;
    [[nodiscard]] core::Size size() const noexcept;
    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] const std::shared_ptr<Impl>& impl() const noexcept { return impl_; }

    core::Result<void, core::Error> rasterize(uint32_t targetWidth, uint32_t targetHeight,
                                              uint32_t padPixels,
                                              std::span<uint8_t> outAlphaMask) const;

    static uint32_t implId(const Impl& impl) noexcept;
    static core::Result<void, core::Error> rasterizeImpl(const Impl& impl, uint32_t targetWidth,
                                                         uint32_t targetHeight, uint32_t padPixels,
                                                         std::span<uint8_t> outAlphaMask);

    void render(PaintContext& ctx, const core::Rect& bounds,
                const core::Color& tintColor = core::Color::white()) const;

private:
    explicit SvgDocument(std::shared_ptr<Impl> impl) noexcept;

    std::shared_ptr<Impl> impl_;
};

} // namespace northern::renderer
