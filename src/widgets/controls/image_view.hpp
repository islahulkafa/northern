#pragma once

#include "core/math/color.hpp"
#include "core/math/geometry.hpp"
#include "graphics/types.hpp"
#include "renderer/image/image.hpp"
#include "widgets/base/widget.hpp"
#include "widgets/controls/icons.hpp"

#include <memory>
#include <optional>

namespace northern::widgets {

enum class ImageFit : uint8_t {
    Contain,
    Cover,
    Fill,
    ScaleDown,
    None,
};

struct ImageDrawGeometry {
    core::Rect drawRect{};
    core::Rect uvRect{0.0f, 0.0f, 1.0f, 1.0f};
};

class ImageView : public Widget {
public:
    explicit ImageView(std::shared_ptr<renderer::Image> image = nullptr,
                       ImageFit fit = ImageFit::Contain);
    ~ImageView() override = default;

    void setImage(std::shared_ptr<renderer::Image> image);
    [[nodiscard]] const std::shared_ptr<renderer::Image>& image() const noexcept { return image_; }

    void setFit(ImageFit fit) noexcept;
    [[nodiscard]] ImageFit fit() const noexcept { return fit_; }

    void setFilter(graphics::Filter filter) noexcept;
    [[nodiscard]] graphics::Filter filter() const noexcept { return filter_; }

    void setCornerRadius(float radius) noexcept;
    void setCornerRadii(float tl, float tr, float br, float bl) noexcept;
    [[nodiscard]] const float* cornerRadii() const noexcept { return cornerRadii_; }

    void setShowBorder(bool show) noexcept;
    [[nodiscard]] bool showBorder() const noexcept { return showBorder_; }

    void setBorderWidth(float width) noexcept;
    [[nodiscard]] float borderWidth() const noexcept { return borderWidth_; }

    void setBorderColor(const core::Color& color) noexcept;
    [[nodiscard]] core::Color effectiveBorderColor() const noexcept;

    void setTintColor(const core::Color& color) noexcept;
    [[nodiscard]] const core::Color& tintColor() const noexcept { return tintColor_; }

    void setOpacity(float opacity) noexcept;
    [[nodiscard]] float opacity() const noexcept { return opacity_; }

    void setPlaceholderIcon(std::optional<IconId> icon) noexcept;
    [[nodiscard]] std::optional<IconId> placeholderIcon() const noexcept {
        return placeholderIcon_;
    }

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    [[nodiscard]] static ImageDrawGeometry computeImageLayout(const core::Rect& bounds, float imgW,
                                                              float imgH, ImageFit fit) noexcept;

    void paint(renderer::PaintContext& ctx) override;

protected:
    void onContextAttached() override;
    void onThemeChanged() override;

private:
    void applyDpiScale(float scale);

    std::shared_ptr<renderer::Image> image_;
    ImageFit fit_{ImageFit::Contain};
    graphics::Filter filter_{graphics::Filter::Linear};
    float cornerRadii_[4]{0.0f, 0.0f, 0.0f, 0.0f};
    bool hasCustomRadius_{false};
    bool showBorder_{false};
    float borderWidth_{1.0f};
    std::optional<core::Color> customBorderColor_;
    core::Color tintColor_{1.0f, 1.0f, 1.0f, 1.0f};
    float opacity_{1.0f};
    std::optional<IconId> placeholderIcon_{IconId::File};
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};
};

} // namespace northern::widgets
