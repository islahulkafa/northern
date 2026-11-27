#include "widgets/controls/avatar.hpp"
#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/controls/image_view.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace northern::widgets {

Avatar::Avatar(std::string name, AvatarSize size, AvatarShape shape)
    : name_(std::move(name)), initials_(extractInitials(name_)), size_(size), shape_(shape) {
    applyMetrics();
}

Avatar::Avatar(std::shared_ptr<renderer::Image> image, std::string name, AvatarSize size,
               AvatarShape shape)
    : name_(std::move(name)), initials_(extractInitials(name_)), image_(std::move(image)),
      size_(size), shape_(shape) {
    applyMetrics();
}

std::string Avatar::extractInitials(std::string_view name) {
    std::string firstWord;
    std::string lastWord;
    size_t i = 0;
    while (i < name.size()) {
        while (i < name.size() && std::isspace(static_cast<unsigned char>(name[i]))) {
            ++i;
        }
        if (i >= name.size()) {
            break;
        }
        size_t start = i;
        while (i < name.size() && !std::isspace(static_cast<unsigned char>(name[i]))) {
            ++i;
        }
        std::string_view word = name.substr(start, i - start);
        if (firstWord.empty()) {
            firstWord = std::string(word);
        } else {
            lastWord = std::string(word);
        }
    }

    if (firstWord.empty()) {
        return {};
    }

    std::string out;
    out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(firstWord[0]))));
    if (!lastWord.empty()) {
        out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(lastWord[0]))));
    } else if (firstWord.size() > 1) {
        out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(firstWord[1]))));
    }
    return out;
}

float Avatar::diameter() const noexcept {
    return static_cast<float>(static_cast<uint8_t>(size_)) * dpiScale_;
}

void Avatar::setName(std::string name) {
    if (name_ != name) {
        name_ = std::move(name);
        initials_ = extractInitials(name_);
        cachedInitialsLayout_.reset();
        markPaintDirty();
    }
}

void Avatar::setImage(std::shared_ptr<renderer::Image> image) {
    if (image_ != image) {
        image_ = std::move(image);
        markPaintDirty();
    }
}

void Avatar::setSize(AvatarSize size) noexcept {
    if (size_ != size) {
        size_ = size;
        cachedFont_.reset();
        cachedInitialsLayout_.reset();
        applyMetrics();
        markPaintDirty();
    }
}

void Avatar::setShape(AvatarShape shape) noexcept {
    if (shape_ != shape) {
        shape_ = shape;
        markPaintDirty();
    }
}

void Avatar::setStatus(AvatarStatus status) noexcept {
    if (status_ != status) {
        status_ = status;
        markPaintDirty();
    }
}

void Avatar::setBackgroundColor(const core::Color& color) noexcept {
    customBgColor_ = color;
    markPaintDirty();
}

void Avatar::clearBackgroundColor() noexcept {
    if (customBgColor_.has_value()) {
        customBgColor_.reset();
        markPaintDirty();
    }
}

void Avatar::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        cachedFont_.reset();
        cachedInitialsLayout_.reset();
        applyMetrics();
        markPaintDirty();
    }
}

void Avatar::onContextAttached() {
    if (!hasCustomDpiScale_) {
        float scale = contextDpiScale();
        if (dpiScale_ != scale) {
            dpiScale_ = scale;
            cachedFont_.reset();
            cachedInitialsLayout_.reset();
            applyMetrics();
        }
    }
}

void Avatar::onThemeChanged() {
    cachedInitialsLayout_.reset();
    markPaintDirty();
}

void Avatar::applyMetrics() {
    float d = diameter();
    style().width = layout::Length::points(d);
    style().height = layout::Length::points(d);
    markLayoutDirty();
}

void Avatar::ensureInitialsLayout() const {
    if (cachedInitialsLayout_ || initials_.empty()) {
        return;
    }
    if (!cachedFont_) {
        float px = std::max(10.0f * dpiScale_, diameter() * 0.40f);
        cachedFont_ = text::FontManager::instance().semiBoldFont(px).valueOr(nullptr);
    }
    if (cachedFont_) {
        cachedInitialsLayout_ = text::TextLayout::create(initials_, cachedFont_);
    }
}

void Avatar::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    const auto& pal = colorPalette();
    float d = std::min(rect.size.width, rect.size.height);
    core::Rect avatarRect{
        rect.origin.x + (rect.size.width - d) * 0.5f,
        rect.origin.y + (rect.size.height - d) * 0.5f,
        d,
        d,
    };

    float radius = (shape_ == AvatarShape::Circle) ? (d * 0.5f)
                                                   : (theme::Metrics::CornerRadiusCard * dpiScale_);
    float radii[4]{radius, radius, radius, radius};
    float borderW = std::max(1.0f, std::round(1.0f * dpiScale_));

    if (image_ && image_->isValid()) {
        ImageDrawGeometry geom =
            ImageView::computeImageLayout(avatarRect, static_cast<float>(image_->width()),
                                          static_cast<float>(image_->height()), ImageFit::Cover);
        ctx.drawImage(*image_, geom.drawRect, geom.uvRect, radii, pal.borderDefault, borderW);
    } else {
        core::Color bg = customBgColor_.value_or(pal.surfaceRaised);
        ctx.drawBox(avatarRect, bg, pal.borderDefault, borderW, radii);
        ensureInitialsLayout();
        if (cachedInitialsLayout_ && cachedFont_) {
            float textW = cachedInitialsLayout_->size().width;
            float textX = avatarRect.origin.x + (d - textW) * 0.5f;
            float textY = theme::computeCenteredTextY(avatarRect.origin.y + d * 0.5f, *cachedFont_);
            ctx.drawTextLayout(core::Point{textX, textY}, *cachedInitialsLayout_, pal.textPrimary);
        }
    }

    if (status_ != AvatarStatus::None) {
        core::Color statusCol = pal.textTertiary;
        switch (status_) {
        case AvatarStatus::Online:
            statusCol = pal.success;
            break;
        case AvatarStatus::Busy:
            statusCol = pal.danger;
            break;
        case AvatarStatus::Away:
            statusCol = pal.warning;
            break;
        case AvatarStatus::Offline:
        default:
            statusCol = pal.textTertiary;
            break;
        }

        float dotSize = std::clamp(d * 0.28f, 7.0f * dpiScale_, 14.0f * dpiScale_);
        float ringW = std::max(1.5f, std::round(2.0f * dpiScale_));
        float cx = avatarRect.origin.x + d - dotSize * 0.45f;
        float cy = avatarRect.origin.y + d - dotSize * 0.45f;
        if (shape_ == AvatarShape::Circle) {
            float offset = d * 0.146f; // ~ (1 - 1/sqrt(2)) * 0.5 * d
            cx = avatarRect.origin.x + d - offset;
            cy = avatarRect.origin.y + d - offset;
        }
        core::Rect dotRect{cx - dotSize * 0.5f, cy - dotSize * 0.5f, dotSize, dotSize};
        core::Rect ringRect = dotRect.outset(core::Insets{ringW});
        float ringRadii[4]{ringRect.size.width * 0.5f, ringRect.size.width * 0.5f,
                           ringRect.size.width * 0.5f, ringRect.size.width * 0.5f};
        ctx.drawRoundedRect(ringRect, pal.surfaceCanvas, ringRadii);
        float dotRadii[4]{dotSize * 0.5f, dotSize * 0.5f, dotSize * 0.5f, dotSize * 0.5f};
        ctx.drawRoundedRect(dotRect, statusCol, dotRadii);
    }
}

} // namespace northern::widgets
