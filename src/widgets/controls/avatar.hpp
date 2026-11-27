#pragma once

#include "core/math/color.hpp"
#include "renderer/image/image.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/widget.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace northern::widgets {

enum class AvatarShape : uint8_t {
    Circle,
    Rounded,
};

enum class AvatarSize : uint8_t {
    Small = 24,
    Medium = 32,
    Large = 40,
    XLarge = 56,
};

enum class AvatarStatus : uint8_t {
    None,
    Online,
    Busy,
    Away,
    Offline,
};

class Avatar : public Widget {
public:
    explicit Avatar(std::string name = "", AvatarSize size = AvatarSize::Medium,
                    AvatarShape shape = AvatarShape::Circle);
    Avatar(std::shared_ptr<renderer::Image> image, std::string name = "",
           AvatarSize size = AvatarSize::Medium, AvatarShape shape = AvatarShape::Circle);
    ~Avatar() override = default;

    void setName(std::string name);
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::string& initials() const noexcept { return initials_; }

    void setImage(std::shared_ptr<renderer::Image> image);
    [[nodiscard]] const std::shared_ptr<renderer::Image>& image() const noexcept { return image_; }

    void setSize(AvatarSize size) noexcept;
    [[nodiscard]] AvatarSize size() const noexcept { return size_; }

    void setShape(AvatarShape shape) noexcept;
    [[nodiscard]] AvatarShape shape() const noexcept { return shape_; }

    void setStatus(AvatarStatus status) noexcept;
    [[nodiscard]] AvatarStatus status() const noexcept { return status_; }

    void setBackgroundColor(const core::Color& color) noexcept;
    void clearBackgroundColor() noexcept;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    [[nodiscard]] float diameter() const noexcept;
    [[nodiscard]] static std::string extractInitials(std::string_view name);

    void paint(renderer::PaintContext& ctx) override;

protected:
    void onContextAttached() override;
    void onThemeChanged() override;

private:
    void applyMetrics();
    void ensureInitialsLayout() const;

    std::string name_;
    std::string initials_;
    std::shared_ptr<renderer::Image> image_;
    AvatarSize size_{AvatarSize::Medium};
    AvatarShape shape_{AvatarShape::Circle};
    AvatarStatus status_{AvatarStatus::None};
    std::optional<core::Color> customBgColor_;
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};

    mutable std::shared_ptr<text::Font> cachedFont_;
    mutable std::optional<text::TextLayout> cachedInitialsLayout_;
};

} // namespace northern::widgets
