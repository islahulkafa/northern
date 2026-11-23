#include "widgets/containers/dropdown.hpp"
#include "renderer/paint_context.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace northern::widgets {

class DropdownPopup : public Widget {
public:
    DropdownPopup(Dropdown* owner, std::vector<std::string> items, int selectedIndex,
                  std::shared_ptr<text::Font> font, float targetWidth)
        : owner_(owner), items_(std::move(items)), selectedIndex_(selectedIndex),
          font_(std::move(font)), targetWidth_(targetWidth) {
        setFocusable(false);
        style().width = layout::Length::points(targetWidth_);

        itemHeight_ = 28.0f;
        float paddingVertical = 8.0f;
        float contentH = static_cast<float>(items_.size()) * itemHeight_ + paddingVertical;
        float maxH = 176.0f;
        float totalH = std::min(contentH, maxH);
        style().height = layout::Length::points(totalH);

        layoutNode_.setMeasureFunc(
            [this, totalH](const layout::LayoutNode&, float, float) -> core::Size {
                return core::Size{targetWidth_, totalH};
            });

        highlightedIndex_ =
            (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size()))
                ? selectedIndex_
                : (items_.empty() ? -1 : 0);

        cachedItemLayouts_.reserve(items_.size());
        for (const auto& item : items_) {
            if (font_ && !item.empty()) {
                cachedItemLayouts_.push_back(text::TextLayout::create(item, font_));
            } else {
                cachedItemLayouts_.push_back(std::nullopt);
            }
        }

        if (highlightedIndex_ >= 0) {
            scrollItemIntoView(highlightedIndex_);
        }

        openAnim_.start(0.0f, 1.0f, 0.16f, core::animation::EasingCurve::OutCubic);
    }

    void updateAnimations(float deltaSeconds) override {
        openAnim_.update(deltaSeconds);
        markPaintDirty();
    }
    [[nodiscard]] bool hasActiveAnimations() const noexcept override {
        return openAnim_.isRunning();
    }
    [[nodiscard]] float openAnimationProgress() const noexcept { return openAnim_.currentValue(); }

    [[nodiscard]] int highlightedIndex() const noexcept { return highlightedIndex_; }
    [[nodiscard]] core::Point scrollOffset() const noexcept override {
        return core::Point{0.0f, scrollOffset_};
    }
    [[nodiscard]] float verticalScrollOffset() const noexcept { return scrollOffset_; }
    [[nodiscard]] bool isDraggingThumb() const noexcept { return isDraggingThumb_; }

    core::Rect localTrackRect() const noexcept {
        float w = layoutRect().size.width > 0.0f ? layoutRect().size.width : targetWidth_;
        float h = layoutRect().size.height > 0.0f
                      ? layoutRect().size.height
                      : (style().height.isPoints() ? style().height.value : 176.0f);
        return core::Rect{core::Point{w - scrollbarWidth_ - 3.0f, 3.0f},
                          core::Size{scrollbarWidth_, std::max(0.0f, h - 6.0f)}};
    }

    core::Rect localThumbRect() const noexcept {
        core::Rect tRect = localTrackRect();
        float contentH = static_cast<float>(items_.size()) * itemHeight_ + 8.0f;
        float h = layoutRect().size.height > 0.0f
                      ? layoutRect().size.height
                      : (style().height.isPoints() ? style().height.value : 176.0f);
        if (contentH <= h) {
            return core::Rect{};
        }
        float maxScroll = contentH - h;
        float trackH = tRect.size.height;
        float thumbH = std::clamp(trackH * (h / contentH), 18.0f, trackH);
        float scrollFraction = std::clamp(scrollOffset_ / maxScroll, 0.0f, 1.0f);
        float thumbY = tRect.origin.y + scrollFraction * (trackH - thumbH);
        return core::Rect{core::Point{tRect.origin.x, thumbY},
                          core::Size{tRect.size.width, thumbH}};
    }

    void setHighlightedIndex(int idx) {
        int count = static_cast<int>(items_.size());
        if (count == 0) {
            highlightedIndex_ = -1;
            return;
        }
        int clamped = std::clamp(idx, 0, count - 1);
        if (highlightedIndex_ != clamped) {
            highlightedIndex_ = clamped;
            scrollItemIntoView(highlightedIndex_);
            markPaintDirty();
        }
    }

    void scrollItemIntoView(int idx) {
        if (idx < 0 || idx >= static_cast<int>(items_.size())) {
            return;
        }
        float viewH = layoutRect().size.height > 0.0f
                          ? layoutRect().size.height
                          : (style().height.isPoints() ? style().height.value : 176.0f);
        float itemTop = static_cast<float>(idx) * itemHeight_;
        float itemBottom = itemTop + itemHeight_;
        float maxScroll =
            std::max(0.0f, static_cast<float>(items_.size()) * itemHeight_ + 8.0f - viewH);

        if (itemTop < scrollOffset_) {
            scrollOffset_ = std::clamp(itemTop, 0.0f, maxScroll);
            markPaintDirty();
        } else if (itemBottom > scrollOffset_ + viewH - 8.0f) {
            scrollOffset_ = std::clamp(itemBottom - viewH + 8.0f, 0.0f, maxScroll);
            markPaintDirty();
        }
    }

    void onMouseMove(WidgetMouseEvent& ev) override {
        float contentH = static_cast<float>(items_.size()) * itemHeight_ + 8.0f;
        float h = layoutRect().size.height > 0.0f
                      ? layoutRect().size.height
                      : (style().height.isPoints() ? style().height.value : 176.0f);
        float w = layoutRect().size.width > 0.0f ? layoutRect().size.width : targetWidth_;

        if (isDraggingThumb_) {
            float maxScroll = contentH - h;
            core::Rect tRect = localTrackRect();
            float thumbH = localThumbRect().size.height;
            float availableTravel = tRect.size.height - thumbH;

            if (availableTravel > 0.0f) {
                float deltaY = ev.position().y - dragStartMouseY_;
                float scrollDelta = (deltaY / availableTravel) * maxScroll;
                scrollOffset_ = std::clamp(dragStartScrollOffset_ + scrollDelta, 0.0f, maxScroll);
                markPaintDirty();
            }
            ev.accept();
            return;
        }

        // If hovering over scrollbar hit zone, do not change item highlight
        if (contentH > h && ev.position().x >= w - scrollbarHitWidth_) {
            ev.accept();
            return;
        }

        float localY = ev.position().y - 4.0f + scrollOffset_;
        int idx = static_cast<int>(localY / itemHeight_);
        if (idx >= 0 && idx < static_cast<int>(items_.size())) {
            if (idx != highlightedIndex_) {
                highlightedIndex_ = idx;
                if (owner_) {
                    owner_->setPopupHighlightedIndex(idx);
                }
                markPaintDirty();
            }
        }
        ev.accept();
    }

    void onMouseLeave() override {
        // Keep keyboard navigation highlight active
    }

    void onMouseDown(WidgetMouseEvent& ev) override {
        if (ev.button() == platform::MouseButton::Left) {
            float contentH = static_cast<float>(items_.size()) * itemHeight_ + 8.0f;
            float h = layoutRect().size.height > 0.0f
                          ? layoutRect().size.height
                          : (style().height.isPoints() ? style().height.value : 176.0f);
            float w = layoutRect().size.width > 0.0f ? layoutRect().size.width : targetWidth_;

            if (contentH > h && ev.position().x >= w - scrollbarHitWidth_) {
                core::Rect thumb = localThumbRect();
                if (ev.position().y >= thumb.origin.y &&
                    ev.position().y <= thumb.origin.y + thumb.size.height) {
                    isDraggingThumb_ = true;
                    dragStartMouseY_ = ev.position().y;
                    dragStartScrollOffset_ = scrollOffset_;
                } else {
                    core::Rect tRect = localTrackRect();
                    float maxScroll = contentH - h;
                    float trackH = tRect.size.height;
                    float thumbH = thumb.size.height;
                    float availableTravel = trackH - thumbH;
                    if (availableTravel > 0.0f) {
                        float clickFraction = std::clamp(
                            (ev.position().y - tRect.origin.y - thumbH * 0.5f) / availableTravel,
                            0.0f, 1.0f);
                        scrollOffset_ = clickFraction * maxScroll;
                        isDraggingThumb_ = true;
                        dragStartMouseY_ = ev.position().y;
                        dragStartScrollOffset_ = scrollOffset_;
                    }
                }
                markPaintDirty();
                ev.accept();
                return;
            }

            ev.accept();
        }
    }

    void onMouseUp(WidgetMouseEvent& ev) override {
        if (ev.button() == platform::MouseButton::Left) {
            if (isDraggingThumb_) {
                isDraggingThumb_ = false;
                markPaintDirty();
                ev.accept();
                return;
            }

            float contentH = static_cast<float>(items_.size()) * itemHeight_ + 8.0f;
            float h = layoutRect().size.height > 0.0f
                          ? layoutRect().size.height
                          : (style().height.isPoints() ? style().height.value : 176.0f);
            float w = layoutRect().size.width > 0.0f ? layoutRect().size.width : targetWidth_;

            if (contentH > h && ev.position().x >= w - scrollbarHitWidth_) {
                ev.accept();
                return;
            }

            float localY = ev.position().y - 4.0f + scrollOffset_;
            int idx = static_cast<int>(localY / itemHeight_);
            if (idx >= 0 && idx < static_cast<int>(items_.size())) {
                if (owner_) {
                    owner_->selectAndClose(idx);
                }
            }
            ev.accept();
        }
    }

    void onMouseWheel(WidgetMouseEvent& ev) override {
        float contentH = static_cast<float>(items_.size()) * itemHeight_ + 8.0f;
        float h = layoutRect().size.height > 0.0f
                      ? layoutRect().size.height
                      : (style().height.isPoints() ? style().height.value : 176.0f);
        float maxScroll = std::max(0.0f, contentH - h);
        if (maxScroll > 0.0f) {
            float deltaY = ev.wheelDelta().y;
            if (deltaY == 0.0f && ev.wheelDelta().x != 0.0f) {
                deltaY = ev.wheelDelta().x;
            }
            float step = itemHeight_;
            scrollOffset_ = std::clamp(scrollOffset_ - deltaY * step, 0.0f, maxScroll);
            markPaintDirty();
            ev.accept();
        }
    }

    void paint(renderer::PaintContext& ctx) override {
        core::Rect rect = screenRect();
        if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
            return;
        }

        float t = std::clamp(openAnim_.currentValue(), 0.0f, 1.0f);
        float alpha = t;
        float offsetY = (1.0f - t) * -6.0f;
        rect.origin.y += offsetY;

        constexpr float kRadius = theme::Metrics::CornerRadiusControl;
        float radii[4] = {kRadius, kRadius, kRadius, kRadius};
        const auto& pal = colorPalette();

        ctx.drawBox(rect, pal.surfaceOverlay.withAlpha(pal.surfaceOverlay.a * alpha),
                    pal.borderSubtle.withAlpha(pal.borderSubtle.a * alpha),
                    theme::Metrics::BorderWidthHairline, radii);
        ctx.pushClip(rect);

        float contentH = static_cast<float>(items_.size()) * itemHeight_ + 8.0f;
        float viewH = rect.size.height;
        bool hasScrollbar = contentH > viewH;
        float rightMargin = hasScrollbar ? (scrollbarHitWidth_ + 4.0f) : 10.0f;

        float itemY = rect.origin.y + 4.0f - scrollOffset_;
        float padX = 10.0f;

        for (size_t i = 0; i < items_.size(); ++i) {
            core::Rect itemRect{core::Point{rect.origin.x + 3.0f, itemY},
                                core::Size{std::max(0.0f, rect.size.width - 6.0f), itemHeight_}};

            if (itemRect.origin.y + itemHeight_ >= rect.origin.y &&
                itemRect.origin.y <= rect.origin.y + rect.size.height) {
                bool isSelected = (static_cast<int>(i) == selectedIndex_);
                bool isHighlighted = (static_cast<int>(i) == highlightedIndex_);

                if (isHighlighted || isSelected) {
                    core::Color itemBg =
                        isSelected ? pal.surfaceRaised : pal.surfaceRaised.withAlpha(0.5f);
                    float itemRadii[4] = {4.0f, 4.0f, 4.0f, 4.0f};
                    ctx.drawRoundedRect(itemRect, itemBg.withAlpha(itemBg.a * alpha), itemRadii);
                }

                float targetCenterY = itemRect.origin.y + itemHeight_ * 0.5f;
                core::Color textCol = pal.textPrimary.withAlpha(pal.textPrimary.a * alpha);

                // Continuous vector checkmark on the right for selected item
                if (isSelected) {
                    float checkCenterX = rect.origin.x + rect.size.width - rightMargin - 4.0f;
                    core::Point p0{checkCenterX - 4.0f, targetCenterY};
                    core::Point p1{checkCenterX - 1.5f, targetCenterY + 3.0f};
                    core::Point p2{checkCenterX + 4.5f, targetCenterY - 3.5f};
                    ctx.drawLine(p0, p1, textCol, 1.5f);
                    ctx.drawLine(p1, p2, textCol, 1.5f);
                }

                if (font_ && i < cachedItemLayouts_.size() && cachedItemLayouts_[i]) {
                    float textY = theme::computeCenteredTextY(targetCenterY, *font_);
                    float textMaxW = std::max(0.0f, rect.size.width - padX - rightMargin - 16.0f);
                    core::Rect clipText{core::Point{itemRect.origin.x + padX, itemRect.origin.y},
                                        core::Size{textMaxW, itemHeight_}};
                    ctx.pushClip(clipText);
                    ctx.drawTextLayout(core::Point{itemRect.origin.x + padX, textY},
                                       *cachedItemLayouts_[i], textCol);
                    ctx.popClip();
                }
            }

            itemY += itemHeight_;
        }

        // Visual scrollbar indicator for overflow lists
        if (hasScrollbar) {
            core::Rect tRect = localTrackRect();
            core::Rect thRect = localThumbRect();

            core::Rect screenTrack{
                core::Point{rect.origin.x + tRect.origin.x, rect.origin.y + tRect.origin.y},
                tRect.size};
            core::Rect screenThumb{
                core::Point{rect.origin.x + thRect.origin.x, rect.origin.y + thRect.origin.y},
                thRect.size};

            if (isDraggingThumb_) {
                float trackRadii[4] = {3.0f, 3.0f, 3.0f, 3.0f};
                ctx.drawRoundedRect(screenTrack, pal.surfaceRaised.withAlpha(0.3f * alpha),
                                    trackRadii);
            }

            float thumbRadii[4] = {3.0f, 3.0f, 3.0f, 3.0f};
            core::Color thumbCol =
                isDraggingThumb_ ? pal.textSecondary : pal.textTertiary.withAlpha(0.5f);
            ctx.drawRoundedRect(screenThumb, thumbCol.withAlpha(thumbCol.a * alpha), thumbRadii);
        }

        ctx.popClip();
    }

private:
    Dropdown* owner_{nullptr};
    std::vector<std::string> items_{};
    int selectedIndex_{-1};
    int highlightedIndex_{-1};
    std::shared_ptr<text::Font> font_{nullptr};
    float targetWidth_{160.0f};
    float itemHeight_{28.0f};
    float scrollOffset_{0.0f};
    float scrollbarWidth_{6.0f};
    float scrollbarHitWidth_{16.0f};
    bool isDraggingThumb_{false};
    float dragStartMouseY_{0.0f};
    float dragStartScrollOffset_{0.0f};
    std::vector<std::optional<text::TextLayout>> cachedItemLayouts_{};
    core::animation::PropertyAnimation<float> openAnim_{0.0f};
};

Dropdown::Dropdown(std::vector<std::string> items, int selectedIndex,
                   std::shared_ptr<text::Font> font)
    : items_(std::move(items)), selectedIndex_(selectedIndex), font_(std::move(font)) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::PointingHand);
    style().height = layout::Length::points(theme::Metrics::ControlHeightStandard);
    style().width = layout::Length::percent(100.0f);

    layoutNode_.setMeasureFunc([](const layout::LayoutNode&, float availW, float) -> core::Size {
        float w = std::max(120.0f, availW);
        return core::Size{w, theme::Metrics::ControlHeightStandard};
    });
}

Dropdown::~Dropdown() {
    if (isOpen_ && context_) {
        context_->dismissOverlaysForOwner(this);
    }
}

void Dropdown::invalidateTextLayouts() {
    cachedSelectedLayout_.reset();
    cachedPlaceholderLayout_.reset();
    cachedSelectedText_.clear();
}

void Dropdown::setItems(std::vector<std::string> items) {
    items_ = std::move(items);
    if (selectedIndex_ >= static_cast<int>(items_.size())) {
        selectedIndex_ = items_.empty() ? -1 : static_cast<int>(items_.size()) - 1;
    }
    invalidateTextLayouts();
    markPaintDirty();
}

void Dropdown::addItem(std::string item) {
    items_.push_back(std::move(item));
    invalidateTextLayouts();
    markPaintDirty();
}

void Dropdown::clearItems() {
    items_.clear();
    selectedIndex_ = -1;
    invalidateTextLayouts();
    markPaintDirty();
}

void Dropdown::setSelectedIndex(int index) {
    int clamped = std::clamp(index, -1, static_cast<int>(items_.size()) - 1);
    if (selectedIndex_ != clamped) {
        selectedIndex_ = clamped;
        invalidateTextLayouts();
        markPaintDirty();
        onSelectionChanged.emit(selectedIndex_, selectedText());
    }
}

std::string Dropdown::selectedText() const {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        return items_[selectedIndex_];
    }
    return "";
}

void Dropdown::setPlaceholder(std::string placeholder) {
    placeholder_ = std::move(placeholder);
    invalidateTextLayouts();
    markPaintDirty();
}

void Dropdown::setFont(std::shared_ptr<text::Font> font) {
    font_ = std::move(font);
    invalidateTextLayouts();
    markPaintDirty();
}

void Dropdown::open() {
    if (isOpen_) {
        return;
    }
    isOpen_ = true;
    highlightedIndex_ = (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size()))
                            ? selectedIndex_
                            : (items_.empty() ? -1 : 0);

    chevronAnim_.start(chevronAnim_.currentValue(), 1.0f, 0.18f,
                       core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }

    if (context_) {
        core::Rect r = screenRect();
        float w = std::max(120.0f, r.size.width);
        auto popup = std::make_unique<DropdownPopup>(this, items_, selectedIndex_, font_, w);
        activePopup_ = popup.get();

        OverlayDesc desc{};
        desc.anchorRect = r;
        desc.preferredPlacement = OverlayPlacement::Bottom;
        desc.autoFlip = true;
        desc.dismissOnOutsideClick = true;
        desc.dismissOnEscape = true;
        desc.gap = 4.0f;
        desc.onDismiss = [this]() { onPopupDismissed(); };

        Widget* overlayWidget = context_->showOverlay(std::move(popup), desc, this);
        if (overlayWidget) {
            context_->registerAnimatedWidget(overlayWidget);
        }
    }
    markPaintDirty();
}

void Dropdown::close() {
    if (!isOpen_) {
        return;
    }
    isOpen_ = false;
    activePopup_ = nullptr;
    chevronAnim_.start(chevronAnim_.currentValue(), 0.0f, 0.15f,
                       core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
        context_->dismissOverlaysForOwner(this);
    }
    markPaintDirty();
}

void Dropdown::toggle() {
    if (isOpen_) {
        close();
    } else {
        open();
    }
}

void Dropdown::selectAndClose(int index) {
    setSelectedIndex(index);
    close();
}

void Dropdown::onPopupDismissed() {
    isOpen_ = false;
    activePopup_ = nullptr;
    chevronAnim_.start(chevronAnim_.currentValue(), 0.0f, 0.15f,
                       core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
    markPaintDirty();
}

void Dropdown::navigateHighlight(int delta) {
    if (items_.empty()) {
        return;
    }
    int cur = (highlightedIndex_ >= 0) ? highlightedIndex_ : 0;
    int next = std::clamp(cur + delta, 0, static_cast<int>(items_.size()) - 1);
    setPopupHighlightedIndex(next);
}

void Dropdown::setPopupHighlightedIndex(int idx) {
    int clamped = items_.empty() ? -1 : std::clamp(idx, 0, static_cast<int>(items_.size()) - 1);
    if (highlightedIndex_ != clamped) {
        highlightedIndex_ = clamped;
        if (activePopup_) {
            static_cast<DropdownPopup*>(activePopup_)->setHighlightedIndex(clamped);
        }
        markPaintDirty();
    }
}

void Dropdown::confirmHighlight() {
    if (highlightedIndex_ >= 0 && highlightedIndex_ < static_cast<int>(items_.size())) {
        selectAndClose(highlightedIndex_);
    } else {
        close();
    }
}

void Dropdown::onMouseEnter() {
    isHovered_ = true;
    hoverAnim_.start(hoverAnim_.currentValue(), 1.0f, 0.12f,
                     core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
    markPaintDirty();
}

void Dropdown::onMouseLeave() {
    isHovered_ = false;
    hoverAnim_.start(hoverAnim_.currentValue(), 0.0f, 0.12f,
                     core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
    markPaintDirty();
}

void Dropdown::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        setFocus();
        toggle();
        ev.accept();
    }
}

void Dropdown::onKeyDown(WidgetKeyEvent& ev) {
    if (!ev.isDown()) {
        return;
    }

    if (isOpen_) {
        if (ev.key() == platform::Key::Down) {
            navigateHighlight(1);
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::Up) {
            navigateHighlight(-1);
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::Home) {
            setPopupHighlightedIndex(0);
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::End) {
            setPopupHighlightedIndex(static_cast<int>(items_.size()) - 1);
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::Enter || ev.key() == platform::Key::Space) {
            confirmHighlight();
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::Escape) {
            close();
            ev.accept();
            return;
        }
    } else {
        if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter ||
            ev.key() == platform::Key::Down || ev.key() == platform::Key::Up) {
            open();
            ev.accept();
            return;
        }
    }
}

void Dropdown::onFocusLost() {
    if (isOpen_) {
        close();
    }
    markPaintDirty();
}

void Dropdown::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    const auto& pal = colorPalette();
    float hoverT = hoverAnim_.currentValue();
    core::Color bg = pal.surfaceDefault.lerp(pal.surfaceRaised, hoverT);
    core::Color border = pal.borderSubtle.lerp(pal.borderStrong, hoverT);

    ctx.drawBox(rect, bg, border, borderWidth_, cornerRadii_);

    if (hasKeyboardFocus()) {
        theme::paintFocusRing(ctx, rect, cornerRadii_[0]);
    }

    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;
    float padL = 12.0f;
    float chevronW = 24.0f;

    // Draw Selected text or Placeholder
    if (font_) {
        std::string display = selectedText();
        bool isPlaceholder = display.empty();
        if (isPlaceholder) {
            display = placeholder_;
            if (!cachedPlaceholderLayout_) {
                cachedPlaceholderLayout_ = text::TextLayout::create(display, font_);
            }
        } else {
            if (!cachedSelectedLayout_ || cachedSelectedText_ != display) {
                cachedSelectedLayout_ = text::TextLayout::create(display, font_);
                cachedSelectedText_ = display;
            }
        }

        const auto& activeLayout = isPlaceholder ? cachedPlaceholderLayout_ : cachedSelectedLayout_;
        core::Color textCol = isPlaceholder ? pal.textTertiary : pal.textPrimary;
        float textY = theme::computeCenteredTextY(targetCenterY, *font_);

        core::Rect clipText{
            core::Point{rect.origin.x + padL, rect.origin.y},
            core::Size{std::max(0.0f, rect.size.width - padL - chevronW), rect.size.height}};
        ctx.pushClip(clipText);
        if (activeLayout) {
            ctx.drawTextLayout(core::Point{rect.origin.x + padL, textY}, *activeLayout, textCol);
        }
        ctx.popClip();
    }

    // Draw animated continuous vector chevron
    float t = chevronAnim_.currentValue();
    float angle = t * std::numbers::pi_v<float>;
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);

    float chevCenterX = rect.origin.x + rect.size.width - 15.0f;
    float chevCenterY = targetCenterY;

    float vx0 = 0.0f;
    float vy0 = 2.0f;
    float w1x0 = -4.0f;
    float w1y0 = -2.0f;
    float w2x0 = 4.0f;
    float w2y0 = -2.0f;

    core::Point pVertex{chevCenterX + (vx0 * cosA - vy0 * sinA),
                        chevCenterY + (vx0 * sinA + vy0 * cosA)};
    core::Point pWing1{chevCenterX + (w1x0 * cosA - w1y0 * sinA),
                       chevCenterY + (w1x0 * sinA + w1y0 * cosA)};
    core::Point pWing2{chevCenterX + (w2x0 * cosA - w2y0 * sinA),
                       chevCenterY + (w2x0 * sinA + w2y0 * cosA)};

    core::Color chevCol = pal.textTertiary.lerp(pal.textSecondary, hoverT);
    ctx.drawLine(pWing1, pVertex, chevCol, 1.5f);
    ctx.drawLine(pVertex, pWing2, chevCol, 1.5f);
}

void Dropdown::updateAnimations(float deltaSeconds) {
    chevronAnim_.update(deltaSeconds);
    hoverAnim_.update(deltaSeconds);
    markPaintDirty();
}

bool Dropdown::hasActiveAnimations() const noexcept {
    return chevronAnim_.isRunning() || hoverAnim_.isRunning();
}

float Dropdown::popupAnimationProgress() const noexcept {
    if (activePopup_) {
        return static_cast<DropdownPopup*>(activePopup_)->openAnimationProgress();
    }
    return 0.0f;
}

} // namespace northern::widgets
