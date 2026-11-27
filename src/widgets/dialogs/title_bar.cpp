#include "widgets/dialogs/title_bar.hpp"

#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

namespace {
constexpr float kTitleDefaultPt = 10.5f;
constexpr float kDoubleClickMaxMs = 400.0f;
} // namespace

TitleBar::TitleBar(std::string title, std::shared_ptr<text::Font> font, platform::Window* window,
                   TitleBarSize size)
    : title_(std::move(title)), font_(std::move(font)), size_(size) {
    style().width = layout::Length::percent(100.0f);
    style().flexDirection = layout::FlexDirection::Row;
    style().alignItems = layout::AlignItems::Center;
    style().justifyContent = layout::JustifyContent::SpaceBetween;

    minIcon_ = std::make_unique<Icon>(IconId::WindowMinimize, 14.0f);
    maxIcon_ = std::make_unique<Icon>(IconId::WindowMaximize, 14.0f);
    restoreIcon_ = std::make_unique<Icon>(IconId::WindowRestore, 14.0f);
    closeIcon_ = std::make_unique<Icon>(IconId::WindowClose, 14.0f);

    auto left = std::make_unique<Container>();
    left->style().flexGrow = 1.0f;
    left->style().flexShrink = 1.0f;
    left->style().flexBasis = layout::Length::points(0.0f);
    left->style().height = layout::Length::percent(100.0f);
    leftZone_ = static_cast<Container*>(addChild(std::move(left)));

    auto center = std::make_unique<Container>();
    center->style().flexGrow = 0.0f;
    center->style().flexShrink = 1.0f;
    center->setFlexDirection(layout::FlexDirection::Row);
    center->setAlignItems(layout::AlignItems::Center);
    center->setJustifyContent(layout::JustifyContent::Center);
    center->setVisible(false);
    centerZone_ = static_cast<Container*>(addChild(std::move(center)));

    auto right = std::make_unique<Container>();
    right->style().flexGrow = 1.0f;
    right->style().flexShrink = 1.0f;
    right->style().flexBasis = layout::Length::points(0.0f);
    right->style().height = layout::Length::percent(100.0f);
    right->setFlexDirection(layout::FlexDirection::Row);
    right->setJustifyContent(layout::JustifyContent::FlexEnd);
    right->setAlignItems(layout::AlignItems::Center);
    rightZone_ = static_cast<Container*>(addChild(std::move(right)));

    setSize(size_);
    updateZoneLayout();

    if (window) {
        setWindow(window);
    }
}

TitleBar::~TitleBar() {
    if (window_) {
        window_->setHitTestCallback(nullptr);
    }
    windowResizeConn_.disconnect();
}

void TitleBar::setWindow(platform::Window* window) {
    if (window_ && window_ != window) {
        window_->setHitTestCallback(nullptr);
    }
    windowResizeConn_.disconnect();
    window_ = window;
    if (window_) {
        window_->setHitTestCallback(
            [this](core::Point screenPos) { return hitTestPoint(screenPos); });
        windowResizeConn_ = core::ScopedConnection(
            window_->onResize.connect([this](const auto&) { updateWindowState(); }));
        updateWindowState();
    }
}

void TitleBar::setTitle(std::string title) {
    if (title_ != title) {
        title_ = std::move(title);
        invalidateTitleCache();
        markPaintDirty();
    }
}

void TitleBar::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedDefaultFont_.reset();
        invalidateTitleCache();
        markPaintDirty();
    }
}

std::shared_ptr<text::Font> TitleBar::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
        resolvedDefaultFont_ =
            text::FontManager::instance().defaultFont(kTitleDefaultPt * scale).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void TitleBar::setSize(TitleBarSize size) noexcept {
    size_ = size;
    float h = kDefaultHeight;
    switch (size_) {
    case TitleBarSize::Compact:
        h = kCompactHeight;
        break;
    case TitleBarSize::Comfortable:
        h = kComfortableHeight;
        break;
    case TitleBarSize::Standard:
    default:
        h = kDefaultHeight;
        break;
    }
    style().height = layout::Length::points(h * dpiScale_);
    markLayoutDirty();
}

void TitleBar::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiScale(scale);
}

void TitleBar::applyDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        resolvedDefaultFont_.reset();
        invalidateTitleCache();
        setSize(size_);
        updateZoneLayout();
    }
}

void TitleBar::onContextAttached() {
    if (!hasCustomDpiScale_) {
        applyDpiScale(contextDpiScale());
    }
}

void TitleBar::onThemeChanged() {
    resolvedDefaultFont_.reset();
    invalidateTitleCache();
    markLayoutDirty();
    markPaintDirty();
}

void TitleBar::onEnabledChanged() {
    if (!isEnabled()) {
        cancelActiveInteraction();
    }
    markPaintDirty();
}

void TitleBar::setMinimizeVisible(bool visible) noexcept {
    if (showMinimize_ != visible) {
        showMinimize_ = visible;
        if (!showMinimize_) {
            if (hoveredBtn_ == CaptionButton::Minimize) {
                hoveredBtn_ = CaptionButton::None;
                minHoverAnim_.setCurrentValue(0.0f);
            }
            if (pressedBtn_ == CaptionButton::Minimize) {
                pressedBtn_ = CaptionButton::None;
            }
        }
        invalidateTitleCache();
        updateZoneLayout();
        markPaintDirty();
    }
}

void TitleBar::setMaximizeVisible(bool visible) noexcept {
    if (showMaximize_ != visible) {
        showMaximize_ = visible;
        if (!showMaximize_) {
            if (hoveredBtn_ == CaptionButton::Maximize) {
                hoveredBtn_ = CaptionButton::None;
                maxHoverAnim_.setCurrentValue(0.0f);
            }
            if (pressedBtn_ == CaptionButton::Maximize) {
                pressedBtn_ = CaptionButton::None;
            }
        }
        invalidateTitleCache();
        updateZoneLayout();
        markPaintDirty();
    }
}

void TitleBar::setCloseVisible(bool visible) noexcept {
    if (showClose_ != visible) {
        showClose_ = visible;
        if (!showClose_) {
            if (hoveredBtn_ == CaptionButton::Close) {
                hoveredBtn_ = CaptionButton::None;
                closeHoverAnim_.setCurrentValue(0.0f);
            }
            if (pressedBtn_ == CaptionButton::Close) {
                pressedBtn_ = CaptionButton::None;
            }
        }
        invalidateTitleCache();
        updateZoneLayout();
        markPaintDirty();
    }
}

void TitleBar::setBackgroundColor(const core::Color& color) noexcept {
    overrideBgColor_ = color;
    markPaintDirty();
}

void TitleBar::setBorderColor(const core::Color& color) noexcept {
    overrideBorderColor_ = color;
    markPaintDirty();
}

void TitleBar::setTitleColor(const core::Color& color) noexcept {
    overrideTitleColor_ = color;
    markPaintDirty();
}

void TitleBar::clearColorOverrides() noexcept {
    overrideBgColor_.reset();
    overrideBorderColor_.reset();
    overrideTitleColor_.reset();
    markPaintDirty();
}

core::Color TitleBar::effectiveBackgroundColor() const noexcept {
    return overrideBgColor_.value_or(colorPalette().surfaceDefault);
}

core::Color TitleBar::effectiveBorderColor() const noexcept {
    return overrideBorderColor_.value_or(colorPalette().borderSubtle);
}

core::Color TitleBar::effectiveTitleColor() const noexcept {
    return overrideTitleColor_.value_or(colorPalette().textPrimary);
}

void TitleBar::setCenterWidget(std::unique_ptr<Widget> widget) {
    if (!centerZone_) {
        return;
    }
    if (centerWidget_) {
        centerZone_->removeChild(centerWidget_);
        centerWidget_ = nullptr;
    }
    if (widget) {
        centerWidget_ = centerZone_->addChild(std::move(widget));
        centerZone_->setVisible(true);
    } else {
        centerZone_->setVisible(false);
    }
    invalidateTitleCache();
    markLayoutDirty();
}

void TitleBar::addActionWidget(std::unique_ptr<Widget> widget) {
    if (widget && rightZone_) {
        Widget* ptr = rightZone_->addChild(std::move(widget));
        actionWidgets_.push_back(ptr);
        invalidateTitleCache();
        markLayoutDirty();
    }
}

void TitleBar::removeActionWidget(Widget* widget) {
    if (!widget || !rightZone_) {
        return;
    }
    auto it = std::find(actionWidgets_.begin(), actionWidgets_.end(), widget);
    if (it != actionWidgets_.end()) {
        actionWidgets_.erase(it);
        rightZone_->removeChild(widget);
        invalidateTitleCache();
        markLayoutDirty();
    }
}

void TitleBar::clearActionWidgets() {
    if (actionWidgets_.empty() || !rightZone_) {
        return;
    }
    std::vector<Widget*> copy = actionWidgets_;
    actionWidgets_.clear();
    for (Widget* w : copy) {
        rightZone_->removeChild(w);
    }
    invalidateTitleCache();
    markLayoutDirty();
}

void TitleBar::updateWindowState() {
    if (window_) {
        bool maximized = window_->isMaximized();
        if (isMaximized_ != maximized) {
            isMaximized_ = maximized;
            if (hoveredBtn_ == CaptionButton::Maximize) {
                updateCaptionTooltip(CaptionButton::Maximize);
            }
            markPaintDirty();
        }
    }
}

int TitleBar::visibleCaptionButtonCount() const noexcept {
    return (showMinimize_ ? 1 : 0) + (showMaximize_ ? 1 : 0) + (showClose_ ? 1 : 0);
}

void TitleBar::updateZoneLayout() noexcept {
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    if (rightZone_) {
        float captionW =
            static_cast<float>(visibleCaptionButtonCount()) * kCaptionButtonWidth * scale;
        float rightPad = captionW + 6.0f * scale;
        rightZone_->setGap(4.0f * scale);
        rightZone_->style().padding.right = layout::Length::points(rightPad);
        rightZone_->markLayoutDirty();
    }
}

void TitleBar::invalidateTitleCache() noexcept {
    titleCacheValid_ = false;
    cachedTitleLayout_.reset();
    cachedTitleMaxWidth_ = -1.0f;
}

void TitleBar::ensureTitleLayout(float maxWidth) const {
    float clampedMax = std::max(0.0f, maxWidth);
    if (titleCacheValid_ && std::abs(cachedTitleMaxWidth_ - clampedMax) <= 0.5f) {
        return;
    }

    cachedTitleLayout_.reset();
    cachedTitleMaxWidth_ = clampedMax;
    auto f = effectiveFont();
    if (f && !title_.empty() && clampedMax > 4.0f) {
        text::TextLayoutOptions opts{};
        opts.maxWidth = clampedMax;
        opts.wrapMode = text::WordWrap::None;
        opts.overflow = text::TextOverflow::Ellipsis;
        opts.maxLines = 1;
        cachedTitleLayout_ = text::TextLayout::create(title_, f, opts);
    }
    titleCacheValid_ = true;
}

TitleBar::CaptionButton TitleBar::hitTestCaptionButton(core::Point localPos) const {
    float width = layoutRect().size.width;
    float height =
        layoutRect().size.height > 0.0f ? layoutRect().size.height : (kDefaultHeight * dpiScale_);

    if (localPos.y < 0.0f || localPos.y > height) {
        return CaptionButton::None;
    }

    float btnW = kCaptionButtonWidth * dpiScale_;
    float rightEdge = width;

    if (showClose_) {
        float closeLeft = rightEdge - btnW;
        if (localPos.x >= closeLeft && localPos.x <= rightEdge) {
            return CaptionButton::Close;
        }
        rightEdge = closeLeft;
    }
    if (showMaximize_) {
        float maxLeft = rightEdge - btnW;
        if (localPos.x >= maxLeft && localPos.x < rightEdge) {
            return CaptionButton::Maximize;
        }
        rightEdge = maxLeft;
    }
    if (showMinimize_) {
        float minLeft = rightEdge - btnW;
        if (localPos.x >= minLeft && localPos.x < rightEdge) {
            return CaptionButton::Minimize;
        }
    }
    return CaptionButton::None;
}

core::Rect TitleBar::captionButtonRect(CaptionButton btn) const {
    core::Rect sRect = screenRect();
    float height = sRect.size.height > 0.0f ? sRect.size.height : (kDefaultHeight * dpiScale_);
    float btnW = kCaptionButtonWidth * dpiScale_;
    float rightEdge = sRect.origin.x + sRect.size.width;

    if (showClose_) {
        float closeLeft = rightEdge - btnW;
        if (btn == CaptionButton::Close) {
            return core::Rect{core::Point{closeLeft, sRect.origin.y}, core::Size{btnW, height}};
        }
        rightEdge = closeLeft;
    }
    if (showMaximize_) {
        float maxLeft = rightEdge - btnW;
        if (btn == CaptionButton::Maximize) {
            return core::Rect{core::Point{maxLeft, sRect.origin.y}, core::Size{btnW, height}};
        }
        rightEdge = maxLeft;
    }
    if (showMinimize_) {
        float minLeft = rightEdge - btnW;
        if (btn == CaptionButton::Minimize) {
            return core::Rect{core::Point{minLeft, sRect.origin.y}, core::Size{btnW, height}};
        }
    }
    return core::Rect{};
}

platform::HitTestResult TitleBar::hitTestPoint(core::Point screenPos) const {
    core::Rect sRect = screenRect();
    if (!sRect.contains(screenPos)) {
        return platform::HitTestResult::Client;
    }

    core::Point localPos{screenPos.x - sRect.origin.x, screenPos.y - sRect.origin.y};
    CaptionButton btn = hitTestCaptionButton(localPos);
    if (btn != CaptionButton::None) {
        return platform::HitTestResult::Client;
    }

    if (centerWidget_ && centerWidget_->isVisible() &&
        centerWidget_->screenRect().contains(screenPos)) {
        return platform::HitTestResult::Client;
    }

    for (const Widget* action : actionWidgets_) {
        if (action && action->isVisible() && action->screenRect().contains(screenPos)) {
            return platform::HitTestResult::Client;
        }
    }

    return platform::HitTestResult::Caption;
}

void TitleBar::updateAnimations(float deltaSeconds) {
    bool dirty = false;
    if (minHoverAnim_.isRunning()) {
        minHoverAnim_.update(deltaSeconds);
        dirty = true;
    }
    if (maxHoverAnim_.isRunning()) {
        maxHoverAnim_.update(deltaSeconds);
        dirty = true;
    }
    if (closeHoverAnim_.isRunning()) {
        closeHoverAnim_.update(deltaSeconds);
        dirty = true;
    }
    if (dirty) {
        markPaintDirty();
    }
}

bool TitleBar::hasActiveAnimations() const noexcept {
    return minHoverAnim_.isRunning() || maxHoverAnim_.isRunning() || closeHoverAnim_.isRunning();
}

void TitleBar::updateHoverAnimations(CaptionButton prevBtn, CaptionButton nextBtn) {
    auto animateButton = [&](CaptionButton target,
                             core::animation::PropertyAnimation<float>& anim) {
        float targetVal = (nextBtn == target) ? 1.0f : 0.0f;
        if (prevBtn == target || nextBtn == target) {
            anim.start(anim.currentValue(), targetVal, 0.10f,
                       core::animation::EasingCurve::OutCubic);
        }
    };
    animateButton(CaptionButton::Minimize, minHoverAnim_);
    animateButton(CaptionButton::Maximize, maxHoverAnim_);
    animateButton(CaptionButton::Close, closeHoverAnim_);

    if (hasActiveAnimations() && context_) {
        context_->registerAnimatedWidget(this);
    }
}

void TitleBar::updateCaptionTooltip(CaptionButton btn) {
    switch (btn) {
    case CaptionButton::Minimize:
        setTooltip("Minimize");
        break;
    case CaptionButton::Maximize:
        setTooltip(isMaximized_ ? "Restore" : "Maximize");
        break;
    case CaptionButton::Close:
        setTooltip("Close");
        break;
    case CaptionButton::None:
    default:
        setTooltip("");
        break;
    }
}

void TitleBar::paint(renderer::PaintContext& ctx) {
    core::Rect sRect = screenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return;
    }

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float height = sRect.size.height;
    const auto& palette = colorPalette();

    float borderW = std::max(1.0f, std::round(1.0f * scale));
    ctx.drawRect(sRect, effectiveBackgroundColor());
    ctx.drawLine(
        core::Point{sRect.origin.x, sRect.origin.y + height - borderW * 0.5f},
        core::Point{sRect.origin.x + sRect.size.width, sRect.origin.y + height - borderW * 0.5f},
        effectiveBorderColor(), borderW);

    float titleX = sRect.origin.x + 12.0f * scale;

    float captionAreaWidth =
        static_cast<float>(visibleCaptionButtonCount()) * kCaptionButtonWidth * scale;
    float rightBound = sRect.origin.x + sRect.size.width - captionAreaWidth;
    if (centerWidget_ && centerWidget_->isVisible()) {
        rightBound = std::min(rightBound, centerWidget_->screenRect().origin.x);
    }
    for (const Widget* action : actionWidgets_) {
        if (action && action->isVisible()) {
            rightBound = std::min(rightBound, action->screenRect().origin.x);
        }
    }
    float availTitleW = std::max(0.0f, rightBound - titleX - 8.0f * scale);

    auto f = effectiveFont();
    if (f && !title_.empty() && availTitleW > 4.0f) {
        ensureTitleLayout(availTitleW);
        if (cachedTitleLayout_) {
            float titleY = theme::computeCenteredTextY(sRect.origin.y + height * 0.5f, *f);
            ctx.drawTextLayout(core::Point{titleX, titleY}, *cachedTitleLayout_,
                               effectiveTitleColor());
        }
    }

    auto paintBtn = [&](CaptionButton btn, Icon* icon, bool isClose,
                        const core::animation::PropertyAnimation<float>& hoverAnim) {
        core::Rect btnRect = captionButtonRect(btn);
        if (btnRect.size.width <= 0.0f) {
            return;
        }
        bool isHovered = (hoveredBtn_ == btn);
        bool isPressed = (pressedBtn_ == btn);
        float hoverT = hoverAnim.isRunning() ? hoverAnim.currentValue() : (isHovered ? 1.0f : 0.0f);

        if (isClose) {
            if (isPressed) {
                ctx.drawRect(btnRect, core::Color::fromRgba8(196, 43, 28));
            } else if (hoverT > 0.001f) {
                ctx.drawRect(btnRect, core::Color::fromRgba8(232, 17, 35).withAlpha(hoverT));
            }
        } else {
            if (isPressed) {
                ctx.drawRect(btnRect, palette.surfaceOverlay);
            } else if (hoverT > 0.001f) {
                ctx.drawRect(btnRect, palette.surfaceRaised.withAlpha(hoverT));
            }
        }

        if (icon) {
            core::Color iconColor = palette.textSecondary;
            if (isClose) {
                float activeT = isPressed ? 1.0f : hoverT;
                iconColor = palette.textSecondary.lerp(core::Color::white(), activeT);
            } else {
                float activeT = isPressed ? 1.0f : hoverT;
                iconColor = palette.textSecondary.lerp(palette.textPrimary, activeT);
            }
            float iconSize = 14.0f * scale;
            core::Rect iconRect{
                core::Point{btnRect.origin.x + (btnRect.size.width - iconSize) * 0.5f,
                            btnRect.origin.y + (btnRect.size.height - iconSize) * 0.5f},
                core::Size{iconSize, iconSize}};
            if (auto doc = icon->svgDocument()) {
                ctx.drawSvg(*doc, iconRect, iconColor);
            }
        }
    };

    if (showMinimize_) {
        paintBtn(CaptionButton::Minimize, minIcon_.get(), false, minHoverAnim_);
    }
    if (showMaximize_) {
        paintBtn(CaptionButton::Maximize, isMaximized_ ? restoreIcon_.get() : maxIcon_.get(), false,
                 maxHoverAnim_);
    }
    if (showClose_) {
        paintBtn(CaptionButton::Close, closeIcon_.get(), true, closeHoverAnim_);
    }
}

void TitleBar::cancelActiveInteraction() {
    if (context_ && context_->mouseGrabWidget() == this) {
        context_->releaseMouseGrab();
    }
    bool wasActive = hoveredBtn_ != CaptionButton::None || pressedBtn_ != CaptionButton::None;
    CaptionButton prev = hoveredBtn_;
    hoveredBtn_ = CaptionButton::None;
    pressedBtn_ = CaptionButton::None;
    hasLastCaptionClick_ = false;
    updateHoverAnimations(prev, CaptionButton::None);
    updateCaptionTooltip(CaptionButton::None);
    if (context_) {
        context_->dismissTooltip();
    }
    if (wasActive) {
        markPaintDirty();
    }
}

void TitleBar::onMouseEnter() {}

void TitleBar::onMouseLeave() {
    if (hoveredBtn_ != CaptionButton::None) {
        CaptionButton prev = hoveredBtn_;
        hoveredBtn_ = CaptionButton::None;
        updateHoverAnimations(prev, CaptionButton::None);
        updateCaptionTooltip(CaptionButton::None);
        if (context_) {
            context_->dismissTooltip();
        }
        markPaintDirty();
    }
}

void TitleBar::onMouseMove(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    CaptionButton btn = hitTestCaptionButton(ev.position());
    if (hoveredBtn_ != btn) {
        CaptionButton prev = hoveredBtn_;
        hoveredBtn_ = btn;
        updateHoverAnimations(prev, btn);
        updateCaptionTooltip(btn);
        if (context_) {
            context_->dismissTooltip();
        }
        markPaintDirty();
    }
}

void TitleBar::onMouseDown(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.isPrimaryButton()) {
        CaptionButton btn = hitTestCaptionButton(ev.position());
        if (btn != CaptionButton::None) {
            pressedBtn_ = btn;
            hasLastCaptionClick_ = false;
            if (context_) {
                context_->setMouseGrabWidget(this);
            }
            ev.accept();
            markPaintDirty();
            return;
        }

        // Double-click empty caption area toggles maximize
        core::Point localPos = ev.position();
        auto now = std::chrono::steady_clock::now();
        float tol = 4.0f * dpiScale_;
        if (hasLastCaptionClick_) {
            float elapsedMs =
                std::chrono::duration<float, std::milli>(now - lastCaptionClickTime_).count();
            if (elapsedMs <= kDoubleClickMaxMs &&
                std::abs(localPos.x - lastCaptionClickPos_.x) <= tol &&
                std::abs(localPos.y - lastCaptionClickPos_.y) <= tol) {
                hasLastCaptionClick_ = false;
                if (window_ && showMaximize_) {
                    window_->toggleMaximize();
                    updateWindowState();
                }
                if (showMaximize_) {
                    onMaximizeClicked.emit();
                }
                ev.accept();
                return;
            }
        }
        lastCaptionClickTime_ = now;
        lastCaptionClickPos_ = localPos;
        hasLastCaptionClick_ = true;
    }
}

void TitleBar::onMouseUp(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        cancelActiveInteraction();
        return;
    }
    if (ev.isPrimaryButton()) {
        if (context_ && context_->mouseGrabWidget() == this) {
            context_->releaseMouseGrab();
        }
        CaptionButton btn = hitTestCaptionButton(ev.position());
        if (btn != CaptionButton::None && btn == pressedBtn_) {
            ev.accept();
            pressedBtn_ = CaptionButton::None;
            markPaintDirty();

            switch (btn) {
            case CaptionButton::Minimize:
                if (window_) {
                    window_->minimize();
                }
                onMinimizeClicked.emit();
                break;
            case CaptionButton::Maximize:
                if (window_) {
                    window_->toggleMaximize();
                    updateWindowState();
                }
                onMaximizeClicked.emit();
                break;
            case CaptionButton::Close:
                if (window_) {
                    window_->requestClose();
                }
                onCloseClicked.emit();
                break;
            default:
                break;
            }
        } else if (pressedBtn_ != CaptionButton::None) {
            pressedBtn_ = CaptionButton::None;
            markPaintDirty();
        }
    }
}

} // namespace northern::widgets
