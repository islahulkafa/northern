#include "widgets/controls/progress_bar.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace northern::widgets {

namespace {
constexpr float kReadoutGap = 8.0f;
} // namespace

ProgressBar::ProgressBar(float progress, bool isIndeterminate)
    : progress_(std::clamp(progress, 0.0f, 1.0f)), isIndeterminate_(isIndeterminate),
      progressAnim_(progress_) {
    style().width = layout::Length::percent(100.0f);
    style().height = layout::Length::points(barHeight_);

    layoutNode_.setMeasureFunc(
        [this](const layout::LayoutNode&, float availW, float) -> core::Size {
            float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
            float minW = 60.0f * scale;
            float h = barHeight_;
            if (showValue_ && !isIndeterminate_) {
                updateReservedLabelWidth();
                minW += kReadoutGap * scale + reservedLabelWidth_;
                h = std::max(h, reservedLabelHeight_);
            }
            float w = std::max(minW, availW);
            return core::Size{w, h};
        });
}

ProgressBar::ProgressBar(ProgressSize size, ProgressRole role, float progress, bool isIndeterminate)
    : ProgressBar(progress, isIndeterminate) {
    baseBarHeight_ = toPoints(size);
    barHeight_ = baseBarHeight_ * dpiScale_;
    role_ = role;
    syncLayoutHeight();
}

std::shared_ptr<text::Font> ProgressBar::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        resolvedDefaultFont_ = theme::resolveFont(theme::TypographyStyle::Caption, dpiScale_);
    }
    return resolvedDefaultFont_;
}

void ProgressBar::updateReservedLabelWidth() const {
    auto f = effectiveFont();
    if (!f) {
        reservedLabelWidth_ = 0.0f;
        reservedLabelHeight_ = 0.0f;
        return;
    }
    if (reservedLabelWidth_ <= 0.0f) {
        auto sample = text::TextLayout::create("100%", f);
        reservedLabelWidth_ = std::ceil(sample.size().width);
        reservedLabelHeight_ = sample.size().height;
    }
}

void ProgressBar::ensureValueLayout(int percent) const {
    if (cachedValueLayout_ && cachedPercent_ == percent) {
        return;
    }
    auto f = effectiveFont();
    if (!f) {
        cachedValueLayout_.reset();
        return;
    }
    std::string str = std::to_string(percent) + "%";
    cachedValueLayout_ = text::TextLayout::create(str, f);
    cachedPercent_ = percent;
}

void ProgressBar::setProgress(float progress) {
    float clamped = std::clamp(progress, 0.0f, 1.0f);
    if (std::abs(progress_ - clamped) > 0.0001f) {
        bool wasComplete = progress_ >= 0.9999f;
        progress_ = clamped;
        progressAnim_.start(progressAnim_.currentValue(), progress_, 0.20f,
                            core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
        markPaintDirty();
        onProgressChanged.emit(progress_);
        if (!wasComplete && progress_ >= 0.9999f) {
            onCompleted.emit();
        }
    }
}

void ProgressBar::setRange(float minimum, float maximum) {
    float prevVal = value();
    minimum_ = minimum;
    maximum_ = maximum > minimum ? maximum : minimum;
    setValue(prevVal);
}

void ProgressBar::setValue(float value) {
    if (maximum_ <= minimum_) {
        setProgress(0.0f);
        return;
    }
    float normalized = (value - minimum_) / (maximum_ - minimum_);
    setProgress(normalized);
}

float ProgressBar::value() const noexcept {
    return minimum_ + progress_ * (maximum_ - minimum_);
}

void ProgressBar::setIndeterminate(bool isIndeterminate) {
    if (isIndeterminate_ != isIndeterminate) {
        isIndeterminate_ = isIndeterminate;
        indeterminateOffset_ = 0.0f;
        if (isIndeterminate_ && context_) {
            context_->registerAnimatedWidget(this);
        }
        if (showValue_) {
            markLayoutDirty();
        } else {
            markPaintDirty();
        }
    }
}

void ProgressBar::syncLayoutHeight() {
    if (showValue_ && !isIndeterminate_) {
        updateReservedLabelWidth();
        style().height = layout::Length::points(std::max(barHeight_, reservedLabelHeight_));
    } else {
        style().height = layout::Length::points(barHeight_);
    }
}

void ProgressBar::setSize(ProgressSize size) noexcept {
    hasCustomBarHeight_ = false;
    baseBarHeight_ = toPoints(size);
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float scaled = baseBarHeight_ * scale;
    if (barHeight_ != scaled) {
        barHeight_ = scaled;
        syncLayoutHeight();
        markLayoutDirty();
        markPaintDirty();
    }
}

void ProgressBar::setBarHeight(float height) noexcept {
    hasCustomBarHeight_ = true;
    float clamped = std::max(2.0f, height);
    if (barHeight_ != clamped) {
        barHeight_ = clamped;
        syncLayoutHeight();
        markLayoutDirty();
        markPaintDirty();
    }
}

void ProgressBar::setRole(ProgressRole role) noexcept {
    if (role_ != role) {
        role_ = role;
        markPaintDirty();
    }
}

void ProgressBar::setShowValue(bool show) {
    if (showValue_ != show) {
        showValue_ = show;
        syncLayoutHeight();
        markLayoutDirty();
        markPaintDirty();
    }
}

void ProgressBar::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        cachedValueLayout_.reset();
        cachedPercent_ = -1;
        reservedLabelWidth_ = 0.0f;
        reservedLabelHeight_ = 0.0f;
        syncLayoutHeight();
        markLayoutDirty();
        markPaintDirty();
    }
}

void ProgressBar::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiScale(scale);
}

void ProgressBar::applyDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        if (!hasCustomBarHeight_) {
            barHeight_ = baseBarHeight_ * dpiScale_;
        }
        resolvedDefaultFont_.reset();
        cachedValueLayout_.reset();
        cachedPercent_ = -1;
        reservedLabelWidth_ = 0.0f;
        reservedLabelHeight_ = 0.0f;
        syncLayoutHeight();
        markLayoutDirty();
        markPaintDirty();
    }
}

void ProgressBar::setTrackColor(const core::Color& color) noexcept {
    overrideTrackColor_ = color;
    markPaintDirty();
}

void ProgressBar::setFillColor(const core::Color& color) noexcept {
    overrideFillColor_ = color;
    markPaintDirty();
}

void ProgressBar::setTrackBorderColor(const core::Color& color) noexcept {
    overrideBorderColor_ = color;
    markPaintDirty();
}

void ProgressBar::clearColorOverrides() noexcept {
    if (overrideTrackColor_ || overrideFillColor_ || overrideBorderColor_) {
        overrideTrackColor_.reset();
        overrideFillColor_.reset();
        overrideBorderColor_.reset();
        markPaintDirty();
    }
}

core::Color ProgressBar::effectiveTrackColor() const noexcept {
    core::Color col = overrideTrackColor_ ? *overrideTrackColor_ : [&]() {
        const auto& pal = colorPalette();
        return (pal.mode == theme::ThemeMode::Dark) ? pal.surfaceRaised : pal.borderDefault;
    }();
    return isEnabled() ? col : col.withAlpha(col.a * 0.50f);
}

core::Color ProgressBar::effectiveFillColor() const noexcept {
    if (overrideFillColor_) {
        return isEnabled() ? *overrideFillColor_
                           : overrideFillColor_->withAlpha(overrideFillColor_->a * 0.40f);
    }
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.textDisabled;
    }
    switch (role_) {
    case ProgressRole::Accent:
        return pal.accentDefault;
    case ProgressRole::Success:
        return pal.success;
    case ProgressRole::Warning:
        return pal.warning;
    case ProgressRole::Danger:
        return pal.danger;
    case ProgressRole::Info:
        return pal.info;
    }
    return pal.accentDefault;
}

core::Color ProgressBar::effectiveBorderColor() const noexcept {
    core::Color col = core::Color::transparent();
    if (overrideBorderColor_) {
        col = *overrideBorderColor_;
    } else {
        float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
        if (barHeight_ >= 6.0f * scale) {
            col = colorPalette().borderSubtle;
        }
    }
    return isEnabled() ? col : col.withAlpha(col.a * 0.50f);
}

void ProgressBar::onContextAttached() {
    if (!hasCustomDpiScale_) {
        applyDpiScale(contextDpiScale());
    }
    if (hasActiveAnimations() && context_) {
        context_->registerAnimatedWidget(this);
    }
}

void ProgressBar::onThemeChanged() {
    cachedValueLayout_.reset();
    cachedPercent_ = -1;
    reservedLabelWidth_ = 0.0f;
    reservedLabelHeight_ = 0.0f;
    syncLayoutHeight();
    markLayoutDirty();
    markPaintDirty();
}

void ProgressBar::onEnabledChanged() {
    if (isEnabled() && hasActiveAnimations() && context_) {
        context_->registerAnimatedWidget(this);
    }
    markPaintDirty();
}

void ProgressBar::updateAnimations(float deltaSeconds) {
    bool changed = false;
    if (progressAnim_.isRunning()) {
        progressAnim_.update(deltaSeconds);
        changed = true;
    }
    if (isIndeterminate_ && isEnabled()) {
        advanceAnimation(deltaSeconds);
        changed = true;
    }
    if (changed) {
        markPaintDirty();
    }
}

bool ProgressBar::hasActiveAnimations() const noexcept {
    return (isIndeterminate_ && isEnabled()) || progressAnim_.isRunning();
}

void ProgressBar::advanceAnimation(float deltaSeconds) noexcept {
    if (isIndeterminate_ && isEnabled()) {
        constexpr float speed = 0.75f;
        indeterminateOffset_ += deltaSeconds * speed;
        if (indeterminateOffset_ >= 1.0f) {
            indeterminateOffset_ -= std::floor(indeterminateOffset_);
        }
        markPaintDirty();
    }
}

void ProgressBar::paint(renderer::PaintContext& ctx) {
    if (hasActiveAnimations() && context_) {
        context_->registerAnimatedWidget(this);
    }

    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float readoutGap = kReadoutGap * scale;
    float trackTotalW = rect.size.width;
    bool drawReadout = (showValue_ && !isIndeterminate_);
    if (drawReadout) {
        updateReservedLabelWidth();
        if (reservedLabelWidth_ > 0.0f &&
            rect.size.width > reservedLabelWidth_ + readoutGap + 12.0f * scale) {
            trackTotalW = rect.size.width - reservedLabelWidth_ - readoutGap;
        } else {
            drawReadout = false;
        }
    }

    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;
    float trackY = targetCenterY - barHeight_ * 0.5f;
    core::Rect trackRect{core::Point{rect.origin.x, trackY}, core::Size{trackTotalW, barHeight_}};

    float radius = barHeight_ * 0.5f;
    float radii[4] = {radius, radius, radius, radius};

    core::Color trackCol = effectiveTrackColor();
    core::Color borderCol = effectiveBorderColor();
    float borderW = borderCol.a > 0.0f
                        ? std::max(1.0f, std::round(theme::Metrics::BorderWidthHairline * scale))
                        : 0.0f;
    ctx.drawBox(trackRect, trackCol, borderCol, borderW, radii);

    core::Color fillCol = effectiveFillColor();

    if (isIndeterminate_) {
        float t = std::clamp(indeterminateOffset_, 0.0f, 1.0f);
        float eased =
            (t < 0.5f) ? (4.0f * t * t * t) : (1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f);
        float widthFactor = 0.22f + 0.16f * std::sin(t * 3.14159265f);
        float pulseW = trackRect.size.width * widthFactor;
        float totalTravel = trackRect.size.width + pulseW;
        float pulseLeft = trackRect.origin.x - pulseW + totalTravel * eased;
        float pulseRight = pulseLeft + pulseW;

        float visLeft = std::max(trackRect.origin.x, pulseLeft);
        float visRight = std::min(trackRect.origin.x + trackRect.size.width, pulseRight);
        float visW = visRight - visLeft;
        if (visW >= 0.5f) {
            float pulseRadius = std::min(radius, visW * 0.5f);
            ctx.drawRoundedRect(
                core::Rect{core::Point{visLeft, trackY}, core::Size{visW, barHeight_}}, fillCol,
                pulseRadius);
        }
    } else {
        float cur = std::clamp(progressAnim_.currentValue(), 0.0f, 1.0f);
        if (cur > 0.001f) {
            float fillW = trackRect.size.width * cur;
            if (fillW >= 0.5f) {
                float fillRadius = std::min(radius, fillW * 0.5f);
                ctx.drawRoundedRect(core::Rect{core::Point{trackRect.origin.x, trackY},
                                               core::Size{fillW, barHeight_}},
                                    fillCol, fillRadius);
            }
        }
    }

    if (drawReadout) {
        int pct = static_cast<int>(std::round(progressAnim_.currentValue() * 100.0f));
        ensureValueLayout(pct);
        if (cachedValueLayout_) {
            auto f = effectiveFont();
            float textX = trackRect.origin.x + trackTotalW + readoutGap +
                          std::max(0.0f, reservedLabelWidth_ - cachedValueLayout_->size().width);
            float textY = f ? theme::computeCenteredTextY(targetCenterY, *f) : rect.origin.y;
            const auto& pal = colorPalette();
            core::Color textCol = isEnabled() ? pal.textSecondary : pal.textDisabled;
            ctx.drawTextLayout(core::Point{textX, textY}, *cachedValueLayout_, textCol);
        }
    }
}

} // namespace northern::widgets
