#include "widgets/containers/dropdown.hpp"
#include "renderer/paint_context.hpp"
#include "renderer/svg/svg_document.hpp"
#include "text/font_manager.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <numbers>

namespace northern::widgets {

namespace {

constexpr float kTypeAheadTimeoutSeconds = 0.60f;

[[nodiscard]] float itemHeightForSize(DropdownSize size, float dpiScale) noexcept {
    float baseH = 28.0f;
    switch (size) {
    case DropdownSize::Compact:
        baseH = 24.0f;
        break;
    case DropdownSize::Comfortable:
        baseH = 32.0f;
        break;
    case DropdownSize::Standard:
    default:
        baseH = 28.0f;
        break;
    }
    return baseH * dpiScale;
}

[[nodiscard]] float controlHeightForSize(DropdownSize size, float dpiScale) noexcept {
    float baseH = theme::Metrics::ControlHeightStandard;
    switch (size) {
    case DropdownSize::Compact:
        baseH = theme::Metrics::ControlHeightCompact;
        break;
    case DropdownSize::Comfortable:
        baseH = theme::Metrics::ControlHeightComfortable;
        break;
    case DropdownSize::Standard:
    default:
        baseH = theme::Metrics::ControlHeightStandard;
        break;
    }
    return baseH * dpiScale;
}

[[nodiscard]] float defaultPointSizeForSize(DropdownSize size) noexcept {
    switch (size) {
    case DropdownSize::Compact:
        return 12.5f;
    case DropdownSize::Comfortable:
        return 14.5f;
    case DropdownSize::Standard:
    default:
        return 14.0f;
    }
}

} // namespace

class DropdownPopup : public Widget {
public:
    DropdownPopup(Dropdown* owner, std::vector<DropdownItem> items, int selectedIndex,
                  std::shared_ptr<text::Font> font, float targetWidth, DropdownSize size,
                  float dpiScale, size_t maxVisibleItems)
        : owner_(owner), items_(std::move(items)), selectedIndex_(selectedIndex),
          font_(std::move(font)), targetWidth_(targetWidth), dpiScale_(dpiScale) {
        setFocusable(false);
        style().width = layout::Length::points(targetWidth_);

        itemHeight_ = itemHeightForSize(size, dpiScale_);
        separatorHeight_ = 6.0f * dpiScale_;
        scrollbarWidth_ = 6.0f * dpiScale_;
        scrollbarHitWidth_ = 16.0f * dpiScale_;

        float paddingVertical = 8.0f * dpiScale_;
        float contentH = paddingVertical;
        for (const auto& it : items_) {
            contentH += it.isSeparator ? separatorHeight_ : itemHeight_;
        }

        size_t maxVis = maxVisibleItems > 0 ? maxVisibleItems : 6;
        float maxH = static_cast<float>(maxVis) * itemHeight_ + paddingVertical;
        float totalH = std::min(contentH, maxH);
        style().height = layout::Length::points(totalH);

        layoutNode_.setMeasureFunc(
            [this, totalH](const layout::LayoutNode&, float, float) -> core::Size {
                return core::Size{targetWidth_, totalH};
            });

        highlightedIndex_ =
            (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size()) &&
             isSelectable(selectedIndex_))
                ? selectedIndex_
                : findFirstSelectable();

        cachedItemLayouts_.reserve(items_.size());
        for (const auto& item : items_) {
            if (item.isSeparator) {
                cachedItemLayouts_.push_back(std::nullopt);
                continue;
            }
            if (font_ && !item.text.empty()) {
                cachedItemLayouts_.push_back(text::TextLayout::create(item.text, font_));
            } else {
                cachedItemLayouts_.push_back(std::nullopt);
            }
        }

        if (highlightedIndex_ >= 0) {
            scrollItemIntoView(highlightedIndex_);
        }

        itemHoverAnims_.resize(items_.size());
        if (highlightedIndex_ >= 0 &&
            highlightedIndex_ < static_cast<int>(itemHoverAnims_.size())) {
            itemHoverAnims_[highlightedIndex_].setCurrentValue(1.0f);
        }

        openAnim_.start(0.0f, 1.0f, 0.16f, core::animation::EasingCurve::OutCubic);
    }

    void updateAnimations(float deltaSeconds) override {
        bool needsPaint = false;
        if (openAnim_.isRunning()) {
            openAnim_.update(deltaSeconds);
            needsPaint = true;
        }
        for (auto& anim : itemHoverAnims_) {
            if (anim.isRunning()) {
                anim.update(deltaSeconds);
                needsPaint = true;
            }
        }
        if (needsPaint) {
            markPaintDirty();
        }
    }
    [[nodiscard]] bool hasActiveAnimations() const noexcept override {
        if (openAnim_.isRunning()) {
            return true;
        }
        for (const auto& anim : itemHoverAnims_) {
            if (anim.isRunning()) {
                return true;
            }
        }
        return false;
    }
    [[nodiscard]] float openAnimationProgress() const noexcept { return openAnim_.currentValue(); }

    [[nodiscard]] int highlightedIndex() const noexcept { return highlightedIndex_; }
    [[nodiscard]] core::Point scrollOffset() const noexcept override {
        return core::Point{0.0f, scrollOffset_};
    }
    [[nodiscard]] float verticalScrollOffset() const noexcept { return scrollOffset_; }
    [[nodiscard]] bool isDraggingThumb() const noexcept { return isDraggingThumb_; }

    [[nodiscard]] bool isSelectable(int idx) const noexcept {
        if (idx < 0 || idx >= static_cast<int>(items_.size())) {
            return false;
        }
        return items_[idx].isEnabled && !items_[idx].isSeparator;
    }

    [[nodiscard]] int findFirstSelectable() const noexcept {
        for (size_t i = 0; i < items_.size(); ++i) {
            if (items_[i].isEnabled && !items_[i].isSeparator) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    float itemTopOffset(int idx) const noexcept {
        float top = 4.0f * dpiScale_;
        for (int i = 0; i < idx && i < static_cast<int>(items_.size()); ++i) {
            top += items_[i].isSeparator ? separatorHeight_ : itemHeight_;
        }
        return top;
    }

    float totalContentHeight() const noexcept {
        float h = 8.0f * dpiScale_;
        for (const auto& it : items_) {
            h += it.isSeparator ? separatorHeight_ : itemHeight_;
        }
        return h;
    }

    core::Rect localTrackRect() const noexcept {
        float w = layoutRect().size.width > 0.0f ? layoutRect().size.width : targetWidth_;
        float h = layoutRect().size.height > 0.0f
                      ? layoutRect().size.height
                      : (style().height.isPoints() ? style().height.value : (176.0f * dpiScale_));
        return core::Rect{core::Point{w - scrollbarWidth_ - 3.0f * dpiScale_, 3.0f * dpiScale_},
                          core::Size{scrollbarWidth_, std::max(0.0f, h - 6.0f * dpiScale_)}};
    }

    core::Rect localThumbRect() const noexcept {
        core::Rect tRect = localTrackRect();
        float contentH = totalContentHeight();
        float h = layoutRect().size.height > 0.0f
                      ? layoutRect().size.height
                      : (style().height.isPoints() ? style().height.value : (176.0f * dpiScale_));
        if (contentH <= h) {
            return core::Rect{};
        }
        float maxScroll = contentH - h;
        float trackH = tRect.size.height;
        float thumbH = std::clamp(trackH * (h / contentH), 18.0f * dpiScale_, trackH);
        float scrollFraction = std::clamp(scrollOffset_ / maxScroll, 0.0f, 1.0f);
        float thumbY = tRect.origin.y + scrollFraction * (trackH - thumbH);
        return core::Rect{core::Point{tRect.origin.x, thumbY},
                          core::Size{tRect.size.width, thumbH}};
    }

    void setHighlightedIndex(int idx) {
        if (!isSelectable(idx)) {
            return;
        }
        setPopupHighlightInternal(idx, false);
        scrollItemIntoView(highlightedIndex_);
    }

    void setPopupHighlightInternal(int idx, bool animated) {
        if (highlightedIndex_ == idx) {
            return;
        }
        highlightedIndex_ = idx;
        bool startedAny = false;
        for (size_t i = 0; i < itemHoverAnims_.size(); ++i) {
            bool isTarget =
                (static_cast<int>(i) == highlightedIndex_) && isSelectable(static_cast<int>(i));
            float target = isTarget ? 1.0f : 0.0f;
            if (animated) {
                if (std::abs(itemHoverAnims_[i].targetValue() - target) > 0.001f ||
                    (!itemHoverAnims_[i].isRunning() &&
                     std::abs(itemHoverAnims_[i].currentValue() - target) > 0.001f)) {
                    itemHoverAnims_[i].start(itemHoverAnims_[i].currentValue(), target, 0.12f,
                                             core::animation::EasingCurve::OutCubic);
                    startedAny = true;
                }
            } else {
                itemHoverAnims_[i].stop();
                itemHoverAnims_[i].setCurrentValue(target);
            }
        }
        if (startedAny && context_) {
            context_->registerAnimatedWidget(this);
        }
        markPaintDirty();
    }

    void scrollItemIntoView(int idx) {
        if (idx < 0 || idx >= static_cast<int>(items_.size())) {
            return;
        }
        float viewH =
            layoutRect().size.height > 0.0f
                ? layoutRect().size.height
                : (style().height.isPoints() ? style().height.value : (176.0f * dpiScale_));
        float itemTop = itemTopOffset(idx);
        float itemH = items_[idx].isSeparator ? separatorHeight_ : itemHeight_;
        float itemBottom = itemTop + itemH;
        float maxScroll = std::max(0.0f, totalContentHeight() - viewH);

        if (itemTop < scrollOffset_) {
            scrollOffset_ = std::clamp(itemTop, 0.0f, maxScroll);
            markPaintDirty();
        } else if (itemBottom > scrollOffset_ + viewH - 8.0f * dpiScale_) {
            scrollOffset_ = std::clamp(itemBottom - viewH + 8.0f * dpiScale_, 0.0f, maxScroll);
            markPaintDirty();
        }
    }

    int itemIndexAtLocalY(float localY) const noexcept {
        float currY = 4.0f * dpiScale_ - scrollOffset_;
        for (size_t i = 0; i < items_.size(); ++i) {
            float h = items_[i].isSeparator ? separatorHeight_ : itemHeight_;
            if (localY >= currY && localY < currY + h) {
                return static_cast<int>(i);
            }
            currY += h;
        }
        return -1;
    }

    void onMouseMove(WidgetMouseEvent& ev) override {
        float contentH = totalContentHeight();
        float h = layoutRect().size.height > 0.0f
                      ? layoutRect().size.height
                      : (style().height.isPoints() ? style().height.value : (176.0f * dpiScale_));
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

        if (contentH > h && ev.position().x >= w - scrollbarHitWidth_) {
            ev.accept();
            return;
        }

        int idx = itemIndexAtLocalY(ev.position().y);
        int nextIdx = (idx >= 0 && isSelectable(idx)) ? idx : -1;
        if (nextIdx != highlightedIndex_) {
            setPopupHighlightInternal(nextIdx, true);
            if (nextIdx >= 0 && owner_) {
                owner_->setPopupHighlightedIndex(nextIdx);
            }
        }
        ev.accept();
    }

    void onMouseLeave() override {
        if (context_ && context_->mouseGrabWidget() == this) {
            context_->releaseMouseGrab();
        }
        if (pressedIndex_ != -1) {
            pressedIndex_ = -1;
            markPaintDirty();
        }
        setPopupHighlightInternal(-1, true);
    }

    void onMouseDown(WidgetMouseEvent& ev) override {
        if (ev.isPrimaryButton()) {
            float contentH = totalContentHeight();
            float h =
                layoutRect().size.height > 0.0f
                    ? layoutRect().size.height
                    : (style().height.isPoints() ? style().height.value : (176.0f * dpiScale_));
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
                if (isDraggingThumb_ && context_) {
                    context_->setMouseGrabWidget(this);
                }
                markPaintDirty();
                ev.accept();
                return;
            }

            int idx = itemIndexAtLocalY(ev.position().y);
            int nextPressed = (idx >= 0 && isSelectable(idx)) ? idx : -1;
            if (pressedIndex_ != nextPressed) {
                pressedIndex_ = nextPressed;
                markPaintDirty();
            }
            ev.accept();
        }
    }

    void onMouseUp(WidgetMouseEvent& ev) override {
        if (ev.isPrimaryButton()) {
            if (context_ && context_->mouseGrabWidget() == this) {
                context_->releaseMouseGrab();
            }
            if (isDraggingThumb_) {
                isDraggingThumb_ = false;
                markPaintDirty();
                ev.accept();
                return;
            }

            float contentH = totalContentHeight();
            float h =
                layoutRect().size.height > 0.0f
                    ? layoutRect().size.height
                    : (style().height.isPoints() ? style().height.value : (176.0f * dpiScale_));
            float w = layoutRect().size.width > 0.0f ? layoutRect().size.width : targetWidth_;

            int pressed = pressedIndex_;
            if (pressedIndex_ != -1) {
                pressedIndex_ = -1;
                markPaintDirty();
            }

            if (contentH > h && ev.position().x >= w - scrollbarHitWidth_) {
                ev.accept();
                return;
            }

            int idx = itemIndexAtLocalY(ev.position().y);
            if (pressed >= 0 && idx == pressed && isSelectable(idx)) {
                if (owner_) {
                    owner_->selectAndClose(idx);
                }
            }
            ev.accept();
        }
    }

    void onMouseWheel(WidgetMouseEvent& ev) override {
        float contentH = totalContentHeight();
        float h = layoutRect().size.height > 0.0f
                      ? layoutRect().size.height
                      : (style().height.isPoints() ? style().height.value : (176.0f * dpiScale_));
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
        float offsetY = (1.0f - t) * -6.0f * dpiScale_;
        rect.origin.y += offsetY;

        float radius = theme::Metrics::CornerRadiusOverlay * dpiScale_;
        float radii[4] = {radius, radius, radius, radius};
        const auto& pal = colorPalette();

        ctx.drawBox(rect, pal.surfaceOverlay.withAlpha(pal.surfaceOverlay.a * alpha),
                    pal.borderSubtle.withAlpha(pal.borderSubtle.a * alpha),
                    std::max(1.0f, std::round(theme::Metrics::BorderWidthHairline * dpiScale_)),
                    radii);
        ctx.pushClip(rect);

        float contentH = totalContentHeight();
        float viewH = rect.size.height;
        bool hasScrollbar = contentH > viewH;
        float rightMargin =
            hasScrollbar ? (scrollbarHitWidth_ + 4.0f * dpiScale_) : (12.0f * dpiScale_);

        float itemY = rect.origin.y + 4.0f * dpiScale_ - scrollOffset_;
        float padX = 12.0f * dpiScale_;

        for (size_t i = 0; i < items_.size(); ++i) {
            const auto& item = items_[i];
            float currItemH = item.isSeparator ? separatorHeight_ : itemHeight_;

            core::Rect itemRect{
                core::Point{rect.origin.x + 4.0f * dpiScale_, itemY + 1.0f * dpiScale_},
                core::Size{std::max(0.0f, rect.size.width - 8.0f * dpiScale_),
                           std::max(0.0f, currItemH - 2.0f * dpiScale_)}};

            if (itemY + currItemH >= rect.origin.y && itemY <= rect.origin.y + rect.size.height) {
                if (item.isSeparator) {
                    float sepY = itemY + currItemH * 0.5f;
                    ctx.drawLine(core::Point{rect.origin.x + padX, sepY},
                                 core::Point{rect.origin.x + rect.size.width - padX, sepY},
                                 pal.borderSubtle.withAlpha(pal.borderSubtle.a * alpha),
                                 std::max(1.0f, std::round(1.0f * dpiScale_)));
                } else {
                    bool isSelected = (static_cast<int>(i) == selectedIndex_);
                    bool isHighlighted = (static_cast<int>(i) == highlightedIndex_);
                    bool isPressed = isHighlighted && (static_cast<int>(i) == pressedIndex_);
                    bool isEnabled = item.isEnabled;
                    float ht = (i < itemHoverAnims_.size())
                                   ? std::clamp(itemHoverAnims_[i].currentValue(), 0.0f, 1.0f)
                                   : (isHighlighted ? 1.0f : 0.0f);

                    if (isEnabled && (isPressed || ht > 0.001f || isSelected)) {
                        float rowAlpha =
                            isPressed ? 0.85f : std::max(isSelected ? 0.35f : 0.0f, 0.55f * ht);
                        float itemRadius = theme::resolveConcentricRadius(
                                               theme::Metrics::CornerRadiusOverlay, 4.0f) *
                                           dpiScale_;
                        float itemRadii[4] = {itemRadius, itemRadius, itemRadius, itemRadius};
                        ctx.drawRoundedRect(itemRect, pal.surfaceRaised.withAlpha(rowAlpha * alpha),
                                            itemRadii);
                    }

                    float targetCenterY = itemY + currItemH * 0.5f;
                    core::Color textCol =
                        !isEnabled ? pal.textDisabled.withAlpha(pal.textDisabled.a * alpha)
                                   : pal.textPrimary.withAlpha(pal.textPrimary.a * alpha);

                    if (isSelected) {
                        core::Color checkCol =
                            isEnabled ? pal.accentDefault.withAlpha(alpha) : textCol;
                        float checkCenterX =
                            rect.origin.x + rect.size.width - rightMargin - 4.0f * dpiScale_;
                        core::Point p0{checkCenterX - 4.0f * dpiScale_, targetCenterY};
                        core::Point p1{checkCenterX - 1.5f * dpiScale_,
                                       targetCenterY + 3.0f * dpiScale_};
                        core::Point p2{checkCenterX + 4.5f * dpiScale_,
                                       targetCenterY - 3.5f * dpiScale_};
                        ctx.drawLine(p0, p1, checkCol, 1.5f * dpiScale_);
                        ctx.drawLine(p1, p2, checkCol, 1.5f * dpiScale_);
                    }

                    float contentLeft = rect.origin.x + padX;
                    if (item.icon) {
                        float iconSize = 14.0f * dpiScale_;
                        core::Rect iconRect{
                            core::Point{contentLeft, std::round(targetCenterY - iconSize * 0.5f)},
                            core::Size{iconSize, iconSize}};
                        core::Color iconCol = isEnabled ? pal.textSecondary : pal.textDisabled;
                        ctx.drawSvg(*item.icon, iconRect, iconCol.withAlpha(iconCol.a * alpha));
                        contentLeft += iconSize + 8.0f * dpiScale_;
                    }

                    float textMaxW = std::max(
                        0.0f, (rect.origin.x + rect.size.width - rightMargin - 16.0f * dpiScale_) -
                                  contentLeft);
                    core::Rect clipText{core::Point{contentLeft, itemY},
                                        core::Size{textMaxW, currItemH}};
                    ctx.pushClip(clipText);

                    if (font_ && i < cachedItemLayouts_.size() && cachedItemLayouts_[i]) {
                        float textY = theme::computeCenteredTextY(targetCenterY, *font_);
                        ctx.drawTextLayout(core::Point{contentLeft, textY}, *cachedItemLayouts_[i],
                                           textCol);
                    }
                    ctx.popClip();
                }
            }

            itemY += currItemH;
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

            float sRadius = 3.0f * dpiScale_;
            float trackRadii[4] = {sRadius, sRadius, sRadius, sRadius};
            if (isDraggingThumb_) {
                ctx.drawRoundedRect(screenTrack, pal.surfaceRaised.withAlpha(0.3f * alpha),
                                    trackRadii);
            }

            float thumbRadii[4] = {sRadius, sRadius, sRadius, sRadius};
            core::Color thumbCol =
                isDraggingThumb_ ? pal.textSecondary : pal.textTertiary.withAlpha(0.5f);
            ctx.drawRoundedRect(screenThumb, thumbCol.withAlpha(thumbCol.a * alpha), thumbRadii);
        }

        ctx.popClip();
    }

private:
    Dropdown* owner_{nullptr};
    std::vector<DropdownItem> items_{};
    int selectedIndex_{-1};
    int highlightedIndex_{-1};
    int pressedIndex_{-1};
    std::shared_ptr<text::Font> font_{nullptr};
    float targetWidth_{160.0f};
    float dpiScale_{1.0f};
    float itemHeight_{28.0f};
    float separatorHeight_{6.0f};
    float scrollOffset_{0.0f};
    float scrollbarWidth_{6.0f};
    float scrollbarHitWidth_{16.0f};
    bool isDraggingThumb_{false};
    float dragStartMouseY_{0.0f};
    float dragStartScrollOffset_{0.0f};
    std::vector<std::optional<text::TextLayout>> cachedItemLayouts_{};
    std::vector<core::animation::PropertyAnimation<float>> itemHoverAnims_{};
    core::animation::PropertyAnimation<float> openAnim_{0.0f};
};

Dropdown::Dropdown(std::vector<std::string> items, int selectedIndex,
                   std::shared_ptr<text::Font> font, DropdownSize size)
    : selectedIndex_(selectedIndex), font_(std::move(font)), size_(size) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::PointingHand);
    setItems(std::move(items));
    updateDimensions();
}

Dropdown::Dropdown(std::initializer_list<std::string> items, int selectedIndex,
                   std::shared_ptr<text::Font> font, DropdownSize size)
    : Dropdown(std::vector<std::string>(items), selectedIndex, std::move(font), size) {}

Dropdown::Dropdown(std::vector<DropdownItem> items, int selectedIndex,
                   std::shared_ptr<text::Font> font, DropdownSize size)
    : selectedIndex_(selectedIndex), font_(std::move(font)), size_(size) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::PointingHand);
    setItems(std::move(items));
    updateDimensions();
}

Dropdown::~Dropdown() {
    if (isOpen_ && context_) {
        context_->dismissOverlaysForOwner(this);
    }
}

float Dropdown::preferredHeight() const noexcept {
    return controlHeightForSize(size_, dpiScale_);
}

void Dropdown::updateDimensions() noexcept {
    float h = preferredHeight();
    style().height = layout::Length::points(h);
    style().width = layout::Length::percent(100.0f);

    layoutNode_.setMeasureFunc(
        [h, this](const layout::LayoutNode&, float availW, float) -> core::Size {
            float minW = 120.0f * dpiScale_;
            float w = std::max(minW, availW);
            return core::Size{w, h};
        });
    markLayoutDirty();
}

void Dropdown::invalidateTextLayouts() {
    cachedSelectedLayout_.reset();
    cachedPlaceholderLayout_.reset();
    cachedSelectedText_.clear();
}

void Dropdown::setItems(std::vector<std::string> items) {
    std::vector<DropdownItem> richItems;
    richItems.reserve(items.size());
    for (auto& s : items) {
        richItems.push_back(DropdownItem{.text = std::move(s)});
    }
    setItems(std::move(richItems));
}

void Dropdown::setItems(std::vector<DropdownItem> items) {
    items_ = std::move(items);
    if (selectedIndex_ >= static_cast<int>(items_.size())) {
        selectedIndex_ = items_.empty() ? -1 : static_cast<int>(items_.size()) - 1;
    }
    invalidateTextLayouts();
    markPaintDirty();
}

void Dropdown::addItem(std::string item) {
    items_.push_back(DropdownItem{.text = std::move(item)});
    invalidateTextLayouts();
    markPaintDirty();
}

void Dropdown::addItem(DropdownItem item) {
    items_.push_back(std::move(item));
    invalidateTextLayouts();
    markPaintDirty();
}

void Dropdown::addSeparator() {
    items_.push_back(DropdownItem{.isSeparator = true, .isEnabled = false});
    markPaintDirty();
}

void Dropdown::clearItems() {
    items_.clear();
    selectedIndex_ = -1;
    invalidateTextLayouts();
    markPaintDirty();
}

const DropdownItem& Dropdown::itemAt(size_t index) const {
    static const DropdownItem kEmptyItem{};
    if (index >= items_.size()) {
        return kEmptyItem;
    }
    return items_[index];
}

std::string Dropdown::itemText(size_t index) const {
    if (index >= items_.size()) {
        return "";
    }
    return items_[index].text;
}

void Dropdown::setItemEnabled(int index, bool enabled) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        if (!items_[index].isSeparator) {
            items_[index].isEnabled = enabled;
            if (selectedIndex_ == index && !enabled) {
                selectedIndex_ = -1;
                invalidateTextLayouts();
            }
            markPaintDirty();
        }
    }
}

bool Dropdown::isItemEnabled(int index) const noexcept {
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        return false;
    }
    return items_[index].isEnabled && !items_[index].isSeparator;
}

bool Dropdown::isSeparator(size_t index) const noexcept {
    if (index >= items_.size()) {
        return false;
    }
    return items_[index].isSeparator;
}

void Dropdown::setSelectedIndex(int index) {
    int clamped = std::clamp(index, -1, static_cast<int>(items_.size()) - 1);
    if (clamped >= 0 && !isItemEnabled(clamped)) {
        return;
    }
    if (selectedIndex_ != clamped) {
        selectedIndex_ = clamped;
        invalidateTextLayouts();
        markPaintDirty();
        onSelectionChanged.emit(selectedIndex_, selectedText());
    }
}

std::string Dropdown::selectedText() const {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        return items_[selectedIndex_].text;
    }
    return "";
}

void Dropdown::setPlaceholder(std::string placeholder) {
    if (placeholder_ != placeholder) {
        placeholder_ = std::move(placeholder);
        invalidateTextLayouts();
        markPaintDirty();
    }
}

void Dropdown::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedDefaultFont_.reset();
        invalidateTextLayouts();
        markPaintDirty();
    }
}

std::shared_ptr<text::Font> Dropdown::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float pt = defaultPointSizeForSize(size_) * dpiScale_;
        resolvedDefaultFont_ = text::FontManager::instance().defaultFont(pt).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void Dropdown::setSize(DropdownSize size) noexcept {
    if (size_ != size) {
        size_ = size;
        resolvedDefaultFont_.reset();
        invalidateTextLayouts();
        updateDimensions();
        markPaintDirty();
    }
}

void Dropdown::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiMetrics(scale);
}

void Dropdown::applyDpiMetrics(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        resolvedDefaultFont_.reset();
        invalidateTextLayouts();
        updateDimensions();
        markPaintDirty();
    }
}

void Dropdown::onContextAttached() {
    if (!hasCustomDpiScale_) {
        float ctxDpi = contextDpiScale();
        if (ctxDpi > 0.0f && ctxDpi != dpiScale_) {
            applyDpiMetrics(ctxDpi);
        }
    }
    markPaintDirty();
}

void Dropdown::onThemeChanged() {
    invalidateTextLayouts();
    markPaintDirty();
}

void Dropdown::onEnabledChanged() {
    if (!isEnabled()) {
        isHovered_ = false;
        hoverAnim_.start(hoverAnim_.currentValue(), 0.0f, 0.12f,
                         core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
        if (isOpen_) {
            close();
        }
        markPaintDirty();
    }
}

void Dropdown::setMaxVisibleItems(size_t maxItems) noexcept {
    maxVisibleItems_ = std::max(size_t{1}, maxItems);
}

void Dropdown::setBackgroundColor(const core::Color& color) noexcept {
    overrideBgColor_ = color;
    markPaintDirty();
}

void Dropdown::setBorderColor(const core::Color& color) noexcept {
    overrideBorderColor_ = color;
    markPaintDirty();
}

void Dropdown::setTextColor(const core::Color& color) noexcept {
    overrideTextColor_ = color;
    markPaintDirty();
}

void Dropdown::setPlaceholderColor(const core::Color& color) noexcept {
    overridePlaceholderColor_ = color;
    markPaintDirty();
}

void Dropdown::clearColorOverrides() noexcept {
    overrideBgColor_.reset();
    overrideBorderColor_.reset();
    overrideTextColor_.reset();
    overridePlaceholderColor_.reset();
    markPaintDirty();
}

core::Color Dropdown::effectiveBackgroundColor() const noexcept {
    if (overrideBgColor_) {
        return *overrideBgColor_;
    }
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.surfaceBase;
    }
    return pal.surfaceDefault.lerp(pal.surfaceRaised, hoverAnim_.currentValue());
}

core::Color Dropdown::effectiveBorderColor() const noexcept {
    if (overrideBorderColor_) {
        return *overrideBorderColor_;
    }
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.borderSubtle;
    }
    return pal.borderSubtle.lerp(pal.borderStrong, hoverAnim_.currentValue());
}

core::Color Dropdown::effectiveTextColor() const noexcept {
    if (!isEnabled()) {
        return colorPalette().textDisabled;
    }
    return overrideTextColor_.value_or(colorPalette().textPrimary);
}

core::Color Dropdown::effectivePlaceholderColor() const noexcept {
    if (!isEnabled()) {
        return colorPalette().textDisabled;
    }
    return overridePlaceholderColor_.value_or(colorPalette().textTertiary);
}

int Dropdown::findNextSelectable(int start, int delta) const noexcept {
    if (items_.empty()) {
        return -1;
    }
    int count = static_cast<int>(items_.size());
    if (delta > 0) {
        int first = (start < 0) ? 0 : start + 1;
        for (int i = first; i < count; ++i) {
            if (isItemEnabled(i)) {
                return i;
            }
        }
        return isItemEnabled(start) ? start : -1;
    } else if (delta < 0) {
        int first = (start >= count) ? count - 1 : start - 1;
        for (int i = first; i >= 0; --i) {
            if (isItemEnabled(i)) {
                return i;
            }
        }
        return isItemEnabled(start) ? start : -1;
    }
    return isItemEnabled(start) ? start : -1;
}

void Dropdown::open() {
    if (isOpen_ || !isEnabled()) {
        return;
    }
    isOpen_ = true;
    highlightedIndex_ = (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size()) &&
                         isItemEnabled(selectedIndex_))
                            ? selectedIndex_
                            : findNextSelectable(-1, 1);

    chevronAnim_.start(chevronAnim_.currentValue(), 1.0f, 0.18f,
                       core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }

    if (context_) {
        core::Rect r = screenRect();
        float w = std::max(120.0f * dpiScale_, r.size.width);
        auto popup = std::make_unique<DropdownPopup>(this, items_, selectedIndex_, effectiveFont(),
                                                     w, size_, dpiScale_, maxVisibleItems_);
        activePopup_ = popup.get();

        OverlayDesc desc{};
        desc.anchorRect = r;
        desc.preferredPlacement = OverlayPlacement::Bottom;
        desc.autoFlip = true;
        desc.dismissOnOutsideClick = true;
        desc.dismissOnEscape = true;
        desc.gap = 4.0f * dpiScale_;
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
    if (isItemEnabled(index)) {
        setSelectedIndex(index);
    }
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
    int next = findNextSelectable(cur, delta);
    if (next >= 0) {
        setPopupHighlightedIndex(next);
    }
}

void Dropdown::setPopupHighlightedIndex(int idx) {
    if (idx >= 0 && idx < static_cast<int>(items_.size()) && isItemEnabled(idx)) {
        if (highlightedIndex_ != idx) {
            highlightedIndex_ = idx;
            if (activePopup_) {
                static_cast<DropdownPopup*>(activePopup_)->setHighlightedIndex(idx);
            }
            markPaintDirty();
        }
    }
}

void Dropdown::confirmHighlight() {
    if (highlightedIndex_ >= 0 && highlightedIndex_ < static_cast<int>(items_.size()) &&
        isItemEnabled(highlightedIndex_)) {
        selectAndClose(highlightedIndex_);
    } else {
        close();
    }
}

void Dropdown::clearTypeAhead() noexcept {
    typeAheadBuffer_.clear();
}

void Dropdown::handleTypeAhead(char c) {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - lastTypeAheadTime_).count();
    if (elapsed > kTypeAheadTimeoutSeconds) {
        typeAheadBuffer_.clear();
    }
    lastTypeAheadTime_ = now;
    typeAheadBuffer_.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

    int startIdx = (highlightedIndex_ >= 0) ? highlightedIndex_ : 0;
    int count = static_cast<int>(items_.size());

    auto matchesPrefix = [this](int idx) -> bool {
        if (!isItemEnabled(idx)) {
            return false;
        }
        const std::string& txt = items_[idx].text;
        if (txt.size() < typeAheadBuffer_.size()) {
            return false;
        }
        for (size_t i = 0; i < typeAheadBuffer_.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(txt[i])) != typeAheadBuffer_[i]) {
                return false;
            }
        }
        return true;
    };

    bool matched = false;
    for (int step = 0; step < count; ++step) {
        int idx = (startIdx + step) % count;
        if (matchesPrefix(idx)) {
            matched = true;
            if (isOpen_) {
                setPopupHighlightedIndex(idx);
            } else {
                setSelectedIndex(idx);
            }
            break;
        }
    }

    // If multi-char buffer failed to match any items, restart buffer with the newest character
    if (!matched && typeAheadBuffer_.size() > 1) {
        typeAheadBuffer_ =
            std::string(1, static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        for (int step = 0; step < count; ++step) {
            int idx = (startIdx + step) % count;
            if (matchesPrefix(idx)) {
                if (isOpen_) {
                    setPopupHighlightedIndex(idx);
                } else {
                    setSelectedIndex(idx);
                }
                break;
            }
        }
    }
}

void Dropdown::onMouseEnter() {
    if (!isEnabled()) {
        return;
    }
    isHovered_ = true;
    hoverAnim_.start(hoverAnim_.currentValue(), 1.0f, 0.12f,
                     core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
    markPaintDirty();
}

void Dropdown::onMouseLeave() {
    if (!isEnabled() && isOpen_) {
        close();
    }
    isHovered_ = false;
    hoverAnim_.start(hoverAnim_.currentValue(), 0.0f, 0.12f,
                     core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
    markPaintDirty();
}

void Dropdown::onMouseDown(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.isPrimaryButton()) {
        setFocus(FocusReason::Pointer);
        toggle();
        ev.accept();
    }
}

void Dropdown::onKeyDown(WidgetKeyEvent& ev) {
    if (!ev.isDown() || !isEnabled()) {
        return;
    }

    // Type-ahead detection for alphanumeric and space keys
    char typedChar = 0;
    if (ev.key() >= platform::Key::A && ev.key() <= platform::Key::Z) {
        typedChar = static_cast<char>(
            'a' + (static_cast<int>(ev.key()) - static_cast<int>(platform::Key::A)));
    } else if (ev.key() >= platform::Key::Num0 && ev.key() <= platform::Key::Num9) {
        typedChar = static_cast<char>(
            '0' + (static_cast<int>(ev.key()) - static_cast<int>(platform::Key::Num0)));
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
            int first = findNextSelectable(-1, 1);
            if (first >= 0) {
                setPopupHighlightedIndex(first);
            }
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::End) {
            int last = findNextSelectable(static_cast<int>(items_.size()), -1);
            if (last >= 0) {
                setPopupHighlightedIndex(last);
            }
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

        if (typedChar != 0) {
            handleTypeAhead(typedChar);
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

        if (typedChar != 0) {
            handleTypeAhead(typedChar);
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

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float borderW = std::max(1.0f, std::round(borderWidth_ * scale));
    float radii[4] = {cornerRadii_[0] * scale, cornerRadii_[1] * scale, cornerRadii_[2] * scale,
                      cornerRadii_[3] * scale};

    ctx.drawBox(rect, effectiveBackgroundColor(), effectiveBorderColor(), borderW, radii);

    if (isEnabled() && hasKeyboardFocus()) {
        paintFocusRing(ctx, rect, radii[0]);
    }

    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;
    float padL = 12.0f * scale;
    float chevronW = 24.0f * scale;

    auto f = effectiveFont();

    // Draw Selected text, optional icon, or Placeholder
    if (f) {
        std::string display = selectedText();
        bool isPlaceholder = display.empty();
        const DropdownItem* curItem =
            (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size()))
                ? &items_[selectedIndex_]
                : nullptr;

        if (isPlaceholder) {
            display = placeholder_;
            if (!cachedPlaceholderLayout_) {
                cachedPlaceholderLayout_ = text::TextLayout::create(display, f);
            }
        } else {
            if (!cachedSelectedLayout_ || cachedSelectedText_ != display) {
                cachedSelectedLayout_ = text::TextLayout::create(display, f);
                cachedSelectedText_ = display;
            }
        }

        const auto& activeLayout = isPlaceholder ? cachedPlaceholderLayout_ : cachedSelectedLayout_;
        core::Color textCol = isPlaceholder ? effectivePlaceholderColor() : effectiveTextColor();
        float textY = theme::computeCenteredTextY(targetCenterY, *f);

        float contentLeft = rect.origin.x + padL;
        if (curItem && curItem->icon) {
            float iconSize = 14.0f * scale;
            core::Rect iconRect{
                core::Point{contentLeft, std::round(targetCenterY - iconSize * 0.5f)},
                core::Size{iconSize, iconSize}};
            ctx.drawSvg(*curItem->icon, iconRect,
                        isEnabled() ? colorPalette().textSecondary : colorPalette().textDisabled);
            contentLeft += iconSize + 8.0f * scale;
        }

        core::Rect clipText{
            core::Point{contentLeft, rect.origin.y},
            core::Size{std::max(0.0f, rect.origin.x + rect.size.width - contentLeft - chevronW),
                       rect.size.height}};
        ctx.pushClip(clipText);
        if (activeLayout) {
            ctx.drawTextLayout(core::Point{contentLeft, textY}, *activeLayout, textCol);
        }
        ctx.popClip();
    }

    // Draw animated continuous vector chevron
    float t = chevronAnim_.currentValue();
    float angle = t * std::numbers::pi_v<float>;
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);

    float chevCenterX = rect.origin.x + rect.size.width - 15.0f * scale;
    float chevCenterY = targetCenterY;

    float vx0 = 0.0f;
    float vy0 = 2.0f * scale;
    float w1x0 = -4.0f * scale;
    float w1y0 = -2.0f * scale;
    float w2x0 = 4.0f * scale;
    float w2y0 = -2.0f * scale;

    core::Point pVertex{chevCenterX + (vx0 * cosA - vy0 * sinA),
                        chevCenterY + (vx0 * sinA + vy0 * cosA)};
    core::Point pWing1{chevCenterX + (w1x0 * cosA - w1y0 * sinA),
                       chevCenterY + (w1x0 * sinA + w1y0 * cosA)};
    core::Point pWing2{chevCenterX + (w2x0 * cosA - w2y0 * sinA),
                       chevCenterY + (w2x0 * sinA + w2y0 * cosA)};

    const auto& pal = colorPalette();
    float accentMix = std::max(hoverAnim_.currentValue(), chevronAnim_.currentValue());
    core::Color chevCol =
        isEnabled() ? pal.textTertiary.lerp(pal.accentDefault, accentMix) : pal.textDisabled;
    ctx.drawLine(pWing1, pVertex, chevCol, 1.5f * scale);
    ctx.drawLine(pVertex, pWing2, chevCol, 1.5f * scale);
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
