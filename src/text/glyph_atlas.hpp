#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "core/math/geometry.hpp"
#include "graphics/device.hpp"
#include "graphics/gpu_handle.hpp"
#include "text/font.hpp"
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace northern::text {

struct CachedGlyph {
    core::Rect uvRect{0.0f, 0.0f, 0.0f, 0.0f};
    core::Point offset{0.0f, 0.0f};
    core::Size size{0.0f, 0.0f};
    uint8_t subpixelBin{0};
};

class GlyphAtlas {
public:
    static core::Result<std::unique_ptr<GlyphAtlas>, core::Error>
    create(graphics::Device& device, uint32_t width = 1024, uint32_t height = 1024,
           GlyphRenderMode mode = GlyphRenderMode::Grayscale);

    ~GlyphAtlas();

    GlyphAtlas(const GlyphAtlas&) = delete;
    GlyphAtlas& operator=(const GlyphAtlas&) = delete;
    GlyphAtlas(GlyphAtlas&&) noexcept = default;
    GlyphAtlas& operator=(GlyphAtlas&&) = delete;

    core::Result<CachedGlyph, core::Error> getOrInsert(const Font& font, uint32_t glyphId,
                                                       uint8_t subpixelBin = 0, bool useLcd = true);

    [[nodiscard]] std::optional<CachedGlyph> findCustomMask(uint64_t maskKey) const;
    core::Result<CachedGlyph, core::Error> insertCustomMask(uint64_t maskKey, uint32_t width,
                                                            uint32_t height,
                                                            std::span<const uint8_t> alphaData);

    core::Result<void, core::Error> flush();
    [[nodiscard]] bool isDirty() const noexcept { return dirty_; }

    [[nodiscard]] graphics::TextureHandle texture() const noexcept { return texture_; }
    [[nodiscard]] uint32_t width() const noexcept { return width_; }
    [[nodiscard]] uint32_t height() const noexcept { return height_; }
    [[nodiscard]] GlyphRenderMode renderMode() const noexcept { return mode_; }

private:
    GlyphAtlas(graphics::Device& device, graphics::TextureHandle texture, uint32_t width,
               uint32_t height, GlyphRenderMode mode) noexcept;

    graphics::Device& device_;
    graphics::TextureHandle texture_;
    uint32_t width_{0};
    uint32_t height_{0};
    GlyphRenderMode mode_{GlyphRenderMode::Grayscale};
    uint32_t bytesPerPixel_{1};

    uint32_t currentShelfX_{1};
    uint32_t currentShelfY_{1};
    uint32_t shelfHeight_{0};

    std::vector<uint8_t> cpuAtlas_;
    bool dirty_{false};
    uint32_t dirtyMinY_{0};
    uint32_t dirtyMaxY_{0};

    std::unordered_map<uint64_t, CachedGlyph> cache_;
};

} // namespace northern::text
