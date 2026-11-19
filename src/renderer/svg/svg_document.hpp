#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "core/math/color.hpp"
#include "core/math/geometry.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace northern::renderer {

class PaintContext;

class SvgDocument {
public:
    static core::Result<std::shared_ptr<SvgDocument>, core::Error>
    fromString(std::string_view svgContent);
    static core::Result<std::shared_ptr<SvgDocument>, core::Error>
    fromFile(const std::string& filePath);

    ~SvgDocument();

    SvgDocument(const SvgDocument&) = delete;
    SvgDocument& operator=(const SvgDocument&) = delete;
    SvgDocument(SvgDocument&&) noexcept;
    SvgDocument& operator=(SvgDocument&&) noexcept;

    [[nodiscard]] core::Size size() const noexcept;
    [[nodiscard]] bool isValid() const noexcept;

    void render(PaintContext& ctx, const core::Rect& bounds,
                const core::Color& tintColor = core::Color::white()) const;

private:
    explicit SvgDocument(void* nsvgImage) noexcept;

    void* image_{nullptr};
};

} // namespace northern::renderer
