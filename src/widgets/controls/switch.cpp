#include "widgets/controls/switch.hpp"
#include "renderer/paint_context.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

Switch::Switch(std::string text, bool checked, std::shared_ptr<text::Font> font)
    : text_(std::move(text)), font_(std::move(font)), isChecked_(checked),
      thumbAnim_(checked ? 1.0f : 0.0f) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::PointingHand);
    style().flexDirection = layout::FlexDirection::Row;
    style().alignItems = layout::AlignItems::Center;

    layoutNode_.setMeasureFunc([this](const layout::LayoutNode&, float availW,
                                      float) -> core::Size {
        float textW = 0.0f;
        float textH = trackHeight_;
        if (font_ && !text_.empty()) {
            auto l = text::TextLayout::create(text_, font_);
            textW = l.size().width + gap_;
            textH = std::max(textH, l.size().height);
        }
        float padH = style().padding.left.resolve(availW, 0.0f) +
                     style().padding.right.resolve(availW, 0.0f);
        float padV =
            style().padding.top.resolve(0.0f, 0.0f) + style().padding.bottom.resolve(0.0f, 0.0f);
        return core::Size{trackWidth_ + textW + padH, textH + padV};
    });
}

void Switch::setChecked(bool checked) {
    if (isChecked_ != checked) {
        isChecked_ = checked;
        float target = isChecked_ ? 1.0f : 0.0f;
        thumbAnim_.start(thumbAnim_.currentValue(), target, 0.18f,
                         core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
        markPaintDirty();
        onToggled.emit(isChecked_);
    }
}

void Switch::updateAnimations(float deltaSeconds) {
    thumbAnim_.update(deltaSeconds);
    markPaintDirty();
}

bool Switch::hasActiveAnimations() const noexcept {
    return thumbAnim_.isRunning();
}

void Switch::toggle() {
    setChecked(!isChecked_);
}

void Switch::setText(std::string text) {
    if (text_ != text) {
        text_ = std::move(text);
        markLayoutDirty();
    }
}

void Switch::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        markLayoutDirty();
    }
}

void Switch::setTrackSize(float width, float height) noexcept {
    trackWidth_ = width;
    trackHeight_ = height;
    thumbDiameter_ = std::max(4.0f, height - 4.0f);
    markLayoutDirty();
}

void Switch::setCheckedTrackColor(const core::Color& color) noexcept {
    checkedTrackColor_ = color;
    markPaintDirty();
}

void Switch::setUncheckedTrackColor(const core::Color& color) noexcept {
    uncheckedTrackColor_ = color;
    markPaintDirty();
}

void Switch::setBorderColor(const core::Color& color) noexcept {
    borderColor_ = color;
    markPaintDirty();
}

void Switch::setCheckedThumbColor(const core::Color& color) noexcept {
    checkedThumbColor_ = color;
    markPaintDirty();
}

void Switch::setUncheckedThumbColor(const core::Color& color) noexcept {
    uncheckedThumbColor_ = color;
    markPaintDirty();
}

void Switch::setTextColor(const core::Color& color) noexcept {
    textColor_ = color;
    markPaintDirty();
}

void Switch::onMouseEnter() {
    isHovered_ = true;
    markPaintDirty();
}

void Switch::onMouseLeave() {
    isHovered_ = false;
    isPressed_ = false;
    markPaintDirty();
}

void Switch::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        isPressed_ = true;
        setFocus();
        ev.accept();
        markPaintDirty();
    }
}

void Switch::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left && isPressed_) {
        isPressed_ = false;
        ev.accept();
        toggle();
    }
}

void Switch::onKeyDown(WidgetKeyEvent& ev) {
    if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        ev.accept();
        toggle();
    }
}

void Switch::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;
    float trackY = targetCenterY - trackHeight_ * 0.5f;
    core::Rect trackRect{core::Point{rect.origin.x, trackY}, core::Size{trackWidth_, trackHeight_}};

    float trackRadius = trackHeight_ * 0.5f;
    float radii[4] = {trackRadius, trackRadius, trackRadius, trackRadius};
    float progress = thumbAnim_.currentValue();
    core::Color bg = uncheckedTrackColor_.lerp(checkedTrackColor_, progress);
    core::Color border = isHovered_ ? borderHoverColor_ : borderColor_;

    ctx.drawBox(trackRect, bg, border, 1.0f, radii);

    if (hasKeyboardFocus()) {
        theme::paintFocusRing(ctx, trackRect, trackRadius);
    }

    float pad = (trackHeight_ - thumbDiameter_) * 0.5f;
    float travel = trackWidth_ - pad * 2.0f - thumbDiameter_;
    float thumbX = trackRect.origin.x + pad + travel * progress;
    float thumbY = trackRect.origin.y + pad;
    core::Rect thumbRect{core::Point{thumbX, thumbY}, core::Size{thumbDiameter_, thumbDiameter_}};
    core::Color thumbColor = uncheckedThumbColor_.lerp(checkedThumbColor_, progress);

    ctx.drawRoundedRect(thumbRect, thumbColor, thumbDiameter_ * 0.5f);

    if (font_ && !text_.empty()) {
        auto layout = text::TextLayout::create(text_, font_);
        float textX = trackRect.origin.x + trackWidth_ + gap_;
        float textY = theme::computeCenteredTextY(targetCenterY, *font_);
        ctx.drawTextLayout(core::Point{textX, textY}, layout, textColor_);
    }
}

} // namespace northern::widgets
