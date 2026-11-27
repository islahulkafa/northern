#include "widgets/containers/tab_view.hpp"

#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/controls/icons.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

namespace {
constexpr float kIndicatorAnimDuration = 0.15f;
constexpr float kHoverAnimDuration = 0.12f;
constexpr float kIconSize = 14.0f;
constexpr float kIconGap = 6.0f;
constexpr float kCloseBtnSize = 14.0f;
constexpr float kCloseGap = 6.0f;
constexpr float kModifiedDotSize = 6.0f;
constexpr float kAddButtonWidth = 32.0f;
constexpr float kDragStartThreshold = 5.0f;
constexpr float kScrollWheelStep = 40.0f;
} // namespace

// TabBar
TabBar::TabBar(std::shared_ptr<text::Font> font) : font_(std::move(font)) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::Arrow);
    style().height = layout::Length::points(tabHeight_);
    style().width = layout::Length::percent(100.0f);
    updateMeasure();
}

void TabBar::setTabBarStyle(TabBarStyle style) noexcept {
    if (style_ != style) {
        style_ = style;
        isTabsLayoutDirty_ = true;
        markLayoutDirty();
        markPaintDirty();
    }
}

void TabBar::setMinTabWidth(float minW) noexcept {
    float clamped = std::max(24.0f, minW);
    if (minTabWidth_ != clamped) {
        minTabWidth_ = clamped;
        if (maxTabWidth_ < minTabWidth_) {
            maxTabWidth_ = minTabWidth_;
        }
        isTabsLayoutDirty_ = true;
        markPaintDirty();
    }
}

void TabBar::setMaxTabWidth(float maxW) noexcept {
    float clamped = std::max(minTabWidth_, maxW);
    if (maxTabWidth_ != clamped) {
        maxTabWidth_ = clamped;
        isTabsLayoutDirty_ = true;
        markPaintDirty();
    }
}

void TabBar::setTabHeight(float height) noexcept {
    hasCustomTabHeight_ = true;
    float clamped = std::max(20.0f, height);
    if (tabHeight_ != clamped) {
        tabHeight_ = clamped;
        style().height = layout::Length::points(tabHeight_);
        isTabsLayoutDirty_ = true;
        updateMeasure();
        markLayoutDirty();
    }
}

int TabBar::addTab(std::string title, bool closable, bool enabled) {
    int idx = static_cast<int>(items_.size());
    insertTab(idx, std::move(title), std::shared_ptr<renderer::SvgDocument>{}, closable, enabled);
    return idx;
}

int TabBar::addTab(std::string title, std::shared_ptr<renderer::SvgDocument> icon, bool closable,
                   bool enabled) {
    int idx = static_cast<int>(items_.size());
    insertTab(idx, std::move(title), std::move(icon), closable, enabled);
    return idx;
}

int TabBar::addTab(std::string title, IconId iconId, bool closable, bool enabled) {
    return addTab(std::move(title), getSvgDocument(iconId), closable, enabled);
}

void TabBar::insertTab(int index, std::string title, bool closable, bool enabled) {
    insertTab(index, std::move(title), std::shared_ptr<renderer::SvgDocument>{}, closable, enabled);
}

void TabBar::insertTab(int index, std::string title, std::shared_ptr<renderer::SvgDocument> icon,
                       bool closable, bool enabled) {
    int clamped = std::clamp(index, 0, static_cast<int>(items_.size()));
    TabItem item{
        .title = std::move(title),
        .closable = closable,
        .enabled = enabled,
        .modified = false,
        .icon = std::move(icon),
    };
    items_.insert(items_.begin() + clamped, std::move(item));
    if (selectedIndex_ == -1 && !items_.empty()) {
        selectedIndex_ = 0;
    } else if (selectedIndex_ >= clamped) {
        selectedIndex_++;
    }
    isTabsLayoutDirty_ = true;
    startIndicatorAnimation(selectedIndex_, false);
    markLayoutDirty();
    markPaintDirty();
}

void TabBar::insertTab(int index, std::string title, IconId iconId, bool closable, bool enabled) {
    insertTab(index, std::move(title), getSvgDocument(iconId), closable, enabled);
}

void TabBar::removeTab(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        return;
    }
    items_.erase(items_.begin() + index);
    isTabsLayoutDirty_ = true;

    if (hoveredIndex_ == index || hoveredIndex_ >= static_cast<int>(items_.size())) {
        hoveredIndex_ = -1;
        hoveredCloseIndex_ = -1;
    } else if (hoveredIndex_ > index) {
        hoveredIndex_--;
        if (hoveredCloseIndex_ > index) {
            hoveredCloseIndex_--;
        }
    }
    if (pressTabIndex_ == index) {
        cancelActiveInteraction();
    } else if (pressTabIndex_ > index) {
        pressTabIndex_--;
    }

    if (items_.empty()) {
        selectedIndex_ = -1;
        indicatorWAnim_.setCurrentValue(0.0f);
    } else if (selectedIndex_ == index || selectedIndex_ >= static_cast<int>(items_.size())) {
        selectedIndex_ = findNearestEnabledTab(index);
        if (selectedIndex_ >= 0) {
            startIndicatorAnimation(selectedIndex_, false);
        } else {
            indicatorWAnim_.setCurrentValue(0.0f);
        }
        onTabSelected.emit(selectedIndex_);
    } else if (selectedIndex_ > index) {
        selectedIndex_--;
        startIndicatorAnimation(selectedIndex_, false);
    }

    setTabScrollOffset(scrollOffset_);
    markLayoutDirty();
    markPaintDirty();
}

void TabBar::clearTabs() {
    items_.clear();
    selectedIndex_ = -1;
    hoveredIndex_ = -1;
    hoveredCloseIndex_ = -1;
    hoveredAddBtn_ = false;
    scrollOffset_ = 0.0f;
    totalTabsWidth_ = 0.0f;
    isTabsLayoutDirty_ = true;
    indicatorWAnim_.setCurrentValue(0.0f);
    markLayoutDirty();
    markPaintDirty();
}

void TabBar::moveTab(int fromIndex, int toIndex) {
    int count = static_cast<int>(items_.size());
    if (fromIndex < 0 || fromIndex >= count || toIndex < 0 || toIndex >= count ||
        fromIndex == toIndex) {
        return;
    }

    TabItem moving = std::move(items_[fromIndex]);
    items_.erase(items_.begin() + fromIndex);
    items_.insert(items_.begin() + toIndex, std::move(moving));

    if (selectedIndex_ == fromIndex) {
        selectedIndex_ = toIndex;
    } else if (fromIndex < selectedIndex_ && toIndex >= selectedIndex_) {
        selectedIndex_--;
    } else if (fromIndex > selectedIndex_ && toIndex <= selectedIndex_) {
        selectedIndex_++;
    }

    isTabsLayoutDirty_ = true;
    ensureTabsLayout();
    if (selectedIndex_ >= 0) {
        startIndicatorAnimation(selectedIndex_, false);
    }
    markPaintDirty();
    onTabMoved.emit(fromIndex, toIndex);
}

void TabBar::setSelectedIndex(int index) {
    int clamped = items_.empty() ? -1 : std::clamp(index, 0, static_cast<int>(items_.size()) - 1);
    if (clamped >= 0 && !items_[clamped].enabled) {
        return;
    }
    if (selectedIndex_ != clamped) {
        selectedIndex_ = clamped;
        if (selectedIndex_ >= 0) {
            ensureTabVisible(selectedIndex_);
            startIndicatorAnimation(selectedIndex_, true);
            onTabSelected.emit(selectedIndex_);
        } else {
            indicatorWAnim_.setCurrentValue(0.0f);
        }
        markPaintDirty();
    }
}

const std::string& TabBar::tabTitle(int index) const {
    static const std::string empty;
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        return items_[index].title;
    }
    return empty;
}

void TabBar::setTabTitle(int index, std::string title) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        if (items_[index].title != title) {
            items_[index].title = std::move(title);
            items_[index].cachedLayout.reset();
            isTabsLayoutDirty_ = true;
            markLayoutDirty();
            markPaintDirty();
        }
    }
}

void TabBar::setTabIcon(int index, std::shared_ptr<renderer::SvgDocument> icon) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        items_[index].icon = std::move(icon);
        isTabsLayoutDirty_ = true;
        markLayoutDirty();
        markPaintDirty();
    }
}

void TabBar::setTabIcon(int index, IconId iconId) {
    setTabIcon(index, getSvgDocument(iconId));
}

void TabBar::setTabModified(int index, bool modified) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        if (items_[index].modified != modified) {
            items_[index].modified = modified;
            isTabsLayoutDirty_ = true;
            markPaintDirty();
        }
    }
}

bool TabBar::isTabModified(int index) const noexcept {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        return items_[index].modified;
    }
    return false;
}

void TabBar::setTabEnabled(int index, bool enabled) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        if (items_[index].enabled != enabled) {
            items_[index].enabled = enabled;
            if (!enabled) {
                items_[index].hoverAnim.setCurrentValue(0.0f);
                if (hoveredIndex_ == index) {
                    hoveredIndex_ = -1;
                    hoveredCloseIndex_ = -1;
                    setCursorShape(hoveredAddBtn_ ? platform::CursorShape::PointingHand
                                                  : platform::CursorShape::Arrow);
                }
                if (selectedIndex_ == index) {
                    selectedIndex_ = findNearestEnabledTab(index);
                    if (selectedIndex_ >= 0) {
                        startIndicatorAnimation(selectedIndex_, false);
                    } else {
                        indicatorWAnim_.setCurrentValue(0.0f);
                    }
                    onTabSelected.emit(selectedIndex_);
                }
            }
            markPaintDirty();
        }
    }
}

bool TabBar::isTabEnabled(int index) const noexcept {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        return items_[index].enabled;
    }
    return false;
}

void TabBar::setTabClosable(int index, bool closable) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        if (items_[index].closable != closable) {
            items_[index].closable = closable;
            isTabsLayoutDirty_ = true;
            markPaintDirty();
        }
    }
}

bool TabBar::isTabClosable(int index) const noexcept {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        return items_[index].closable;
    }
    return false;
}

void TabBar::setFont(std::shared_ptr<text::Font> font) {
    font_ = std::move(font);
    resolvedDefaultFont_.reset();
    for (auto& item : items_) {
        item.cachedLayout.reset();
    }
    isTabsLayoutDirty_ = true;
    updateMeasure();
    markLayoutDirty();
    markPaintDirty();
}

void TabBar::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiScale(scale);
}

void TabBar::applyDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        if (!hasCustomTabHeight_) {
            tabHeight_ = 32.0f * dpiScale_;
            style().height = layout::Length::points(tabHeight_);
        }
        resolvedDefaultFont_.reset();
        for (auto& item : items_) {
            item.cachedLayout.reset();
            item.cachedTextWidth = 0.0f;
        }
        isTabsLayoutDirty_ = true;
        updateMeasure();
        markLayoutDirty();
        markPaintDirty();
    }
}

std::shared_ptr<text::Font> TabBar::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float px = 13.5f * dpiScale_;
        resolvedDefaultFont_ = text::FontManager::instance().mediumFont(px).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void TabBar::setAddButtonVisible(bool visible) noexcept {
    if (showAddButton_ != visible) {
        showAddButton_ = visible;
        setTabScrollOffset(scrollOffset_);
        markPaintDirty();
    }
}

core::Rect TabBar::addButtonRect() const noexcept {
    if (!showAddButton_) {
        return core::Rect{};
    }
    float w = layoutRect().size.width;
    if (w <= 0.0f) {
        w = screenRect().size.width;
    }
    float addW = kAddButtonWidth * (dpiScale_ > 0.0f ? dpiScale_ : 1.0f);
    if (w < addW) {
        return core::Rect{};
    }
    return core::Rect{core::Point{w - addW, 0.0f}, core::Size{addW, tabHeight_}};
}

float TabBar::tabsViewportWidth() const noexcept {
    float w = layoutRect().size.width;
    if (w <= 0.0f) {
        w = screenRect().size.width;
    }
    if (showAddButton_) {
        float addW = kAddButtonWidth * (dpiScale_ > 0.0f ? dpiScale_ : 1.0f);
        w = std::max(0.0f, w - addW);
    }
    return w;
}

float TabBar::maxScrollOffset() const noexcept {
    ensureTabsLayout();
    float vp = tabsViewportWidth();
    if (vp <= 0.0f) {
        return 0.0f;
    }
    return std::max(0.0f, totalTabsWidth_ - vp);
}

void TabBar::setTabScrollOffset(float offset) noexcept {
    float clamped = std::clamp(offset, 0.0f, maxScrollOffset());
    if (scrollOffset_ != clamped) {
        scrollOffset_ = clamped;
        markPaintDirty();
    }
}

void TabBar::ensureTabVisible(int index) noexcept {
    ensureTabsLayout();
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        return;
    }
    float vp = tabsViewportWidth();
    if (vp <= 0.0f) {
        return;
    }
    const auto& r = items_[index].tabRect;
    float left = r.origin.x;
    float right = left + r.size.width;
    if (left < scrollOffset_) {
        setTabScrollOffset(left);
    } else if (right > scrollOffset_ + vp) {
        setTabScrollOffset(right - vp);
    }
}

void TabBar::updateMeasure() {
    layoutNode_.setMeasureFunc(
        [this](const layout::LayoutNode&, float availW, float) -> core::Size {
            float w = (availW > 0.0f) ? availW : 200.0f;
            return core::Size{w, tabHeight_};
        });
}

void TabBar::ensureTabsLayout() const {
    float w = layoutRect().size.width;
    if (!isTabsLayoutDirty_ && std::abs(w - lastLayoutWidth_) < 0.1f) {
        return;
    }

    auto f = effectiveFont();
    float curX = 0.0f;
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    bool isSegmented = (style_ == TabBarStyle::Segmented);
    float segW = 0.0f;
    if (isSegmented && !items_.empty()) {
        float vp = tabsViewportWidth();
        if (vp <= 0.0f) {
            vp = 300.0f * scale;
        }
        segW = vp / static_cast<float>(items_.size());
    }

    for (auto& item : items_) {
        if (!item.cachedLayout.has_value() && f && !item.title.empty()) {
            item.cachedLayout = text::TextLayout::create(item.title, f);
            item.cachedTextWidth = item.cachedLayout->size().width;
        } else if (!f || item.title.empty()) {
            item.cachedTextWidth = static_cast<float>(item.title.size()) * 7.5f * scale;
        }

        float contentW = item.cachedTextWidth;
        if (item.icon) {
            contentW += (kIconSize + kIconGap) * scale;
        }
        bool hasTrailingSlot = !isSegmented && (item.closable || item.modified);
        if (hasTrailingSlot) {
            contentW += (kCloseGap + kCloseBtnSize) * scale;
        }

        float tabW = isSegmented ? segW
                                 : std::clamp(tabPaddingX_ * 2.0f * scale + contentW,
                                              minTabWidth_ * scale, maxTabWidth_ * scale);
        item.tabRect = core::Rect{core::Point{curX, 0.0f}, core::Size{tabW, tabHeight_}};

        if (hasTrailingSlot) {
            float closeSz = kCloseBtnSize * scale;
            float btnX = curX + tabW - tabPaddingX_ * scale - closeSz + 2.0f * scale;
            float btnY = (tabHeight_ - closeSz) * 0.5f;
            item.closeBtnRect = core::Rect{core::Point{btnX, btnY}, core::Size{closeSz, closeSz}};
        } else {
            item.closeBtnRect = core::Rect{};
        }
        curX += tabW;
    }

    totalTabsWidth_ = curX;
    lastLayoutWidth_ = w;
    isTabsLayoutDirty_ = false;

    if (!indicatorXAnim_.isRunning() && !indicatorWAnim_.isRunning() && selectedIndex_ >= 0 &&
        selectedIndex_ < static_cast<int>(items_.size())) {
        const core::Rect& selRect = items_[selectedIndex_].tabRect;
        indicatorXAnim_.setCurrentValue(selRect.origin.x);
        indicatorWAnim_.setCurrentValue(selRect.size.width);
    }
}

void TabBar::startIndicatorAnimation(int targetIndex, bool animated) {
    ensureTabsLayout();
    if (targetIndex < 0 || targetIndex >= static_cast<int>(items_.size())) {
        indicatorWAnim_.setCurrentValue(0.0f);
        return;
    }

    const core::Rect& r = items_[targetIndex].tabRect;
    if (animated && indicatorWAnim_.currentValue() > 0.0f) {
        indicatorXAnim_.start(indicatorXAnim_.currentValue(), r.origin.x, kIndicatorAnimDuration,
                              core::animation::EasingCurve::OutCubic);
        indicatorWAnim_.start(indicatorWAnim_.currentValue(), r.size.width, kIndicatorAnimDuration,
                              core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
    } else {
        indicatorXAnim_.setCurrentValue(r.origin.x);
        indicatorWAnim_.setCurrentValue(r.size.width);
    }
}

int TabBar::tabIndexAt(const core::Point& localPos) const noexcept {
    ensureTabsLayout();
    float vp = tabsViewportWidth();
    if (vp > 0.0f && localPos.x >= vp) {
        return -1;
    }
    core::Point contentPos{localPos.x + scrollOffset_, localPos.y};
    for (size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].tabRect.contains(contentPos)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool TabBar::isCloseBtnHit(int tabIndex, const core::Point& localPos) const noexcept {
    if (tabIndex >= 0 && tabIndex < static_cast<int>(items_.size())) {
        const auto& itm = items_[tabIndex];
        core::Point contentPos{localPos.x + scrollOffset_, localPos.y};
        return itm.closable && itm.closeBtnRect.contains(contentPos);
    }
    return false;
}

int TabBar::findNextEnabledTab(int startIndex, int delta) const noexcept {
    int count = static_cast<int>(items_.size());
    if (count == 0) {
        return -1;
    }
    for (int step = 1; step <= count; ++step) {
        int idx = (startIndex + step * delta) % count;
        if (idx < 0) {
            idx += count;
        }
        if (items_[idx].enabled) {
            return idx;
        }
    }
    return -1;
}

int TabBar::findFirstEnabledTab() const noexcept {
    for (size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].enabled) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int TabBar::findLastEnabledTab() const noexcept {
    for (size_t i = items_.size(); i > 0; --i) {
        if (items_[i - 1].enabled) {
            return static_cast<int>(i - 1);
        }
    }
    return -1;
}

int TabBar::findNearestEnabledTab(int fromIndex) const noexcept {
    int count = static_cast<int>(items_.size());
    if (count == 0) {
        return -1;
    }
    int start = std::clamp(fromIndex, 0, count - 1);
    for (int i = start; i < count; ++i) {
        if (items_[i].enabled) {
            return i;
        }
    }
    for (int i = start - 1; i >= 0; --i) {
        if (items_[i].enabled) {
            return i;
        }
    }
    return -1;
}

void TabBar::cancelActiveInteraction() {
    if (isDraggingTab_ && context_) {
        context_->releaseMouseGrab();
    }
    pressTabIndex_ = -1;
    isDraggingTab_ = false;
}

void TabBar::onMouseMove(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    ensureTabsLayout();

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    if (pressTabIndex_ >= 0 && reorderable_) {
        float dist = std::abs(ev.position().x - dragStartX_);
        if (!isDraggingTab_ && dist >= kDragStartThreshold * scale) {
            isDraggingTab_ = true;
            if (context_) {
                context_->setMouseGrabWidget(this);
            }
        }
        if (isDraggingTab_) {
            dragCurrentX_ = ev.position().x + scrollOffset_;
            // Check swap with previous tab
            if (pressTabIndex_ > 0) {
                const auto& prevRect = items_[pressTabIndex_ - 1].tabRect;
                float prevMid = prevRect.origin.x + prevRect.size.width * 0.5f;
                if (dragCurrentX_ < prevMid) {
                    int target = pressTabIndex_ - 1;
                    moveTab(pressTabIndex_, target);
                    pressTabIndex_ = target;
                    hoveredIndex_ = target;
                }
            }
            // Check swap with next tab
            if (pressTabIndex_ + 1 < static_cast<int>(items_.size())) {
                const auto& nextRect = items_[pressTabIndex_ + 1].tabRect;
                float nextMid = nextRect.origin.x + nextRect.size.width * 0.5f;
                if (dragCurrentX_ > nextMid) {
                    int target = pressTabIndex_ + 1;
                    moveTab(pressTabIndex_, target);
                    pressTabIndex_ = target;
                    hoveredIndex_ = target;
                }
            }
            ev.accept();
            return;
        }
    }

    bool addHit = showAddButton_ && addButtonRect().contains(ev.position());
    int rawIdx = addHit ? -1 : tabIndexAt(ev.position());
    int idx = (rawIdx >= 0 && items_[rawIdx].enabled) ? rawIdx : -1;
    int closeIdx = (idx >= 0 && isCloseBtnHit(idx, ev.position())) ? idx : -1;

    if (hoveredIndex_ != idx || hoveredCloseIndex_ != closeIdx || hoveredAddBtn_ != addHit) {
        int prevHover = hoveredIndex_;
        hoveredIndex_ = idx;
        hoveredCloseIndex_ = closeIdx;
        hoveredAddBtn_ = addHit;
        setCursorShape((hoveredIndex_ >= 0 || hoveredAddBtn_) ? platform::CursorShape::PointingHand
                                                              : platform::CursorShape::Arrow);
        if (prevHover >= 0 && prevHover < static_cast<int>(items_.size()) &&
            prevHover != hoveredIndex_) {
            items_[prevHover].hoverAnim.start(items_[prevHover].hoverAnim.currentValue(), 0.0f,
                                              kHoverAnimDuration,
                                              core::animation::EasingCurve::OutCubic);
        }
        if (hoveredIndex_ >= 0 && hoveredIndex_ != prevHover) {
            items_[hoveredIndex_].hoverAnim.start(items_[hoveredIndex_].hoverAnim.currentValue(),
                                                  1.0f, kHoverAnimDuration,
                                                  core::animation::EasingCurve::OutCubic);
        }
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
        markPaintDirty();
    }
}

void TabBar::onMouseDown(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    ensureTabsLayout();

    if (ev.isPrimaryButton()) {
        if (showAddButton_ && addButtonRect().contains(ev.position())) {
            onTabAddRequested.emit();
            ev.accept();
            return;
        }

        int idx = tabIndexAt(ev.position());
        if (idx >= 0) {
            if (items_[idx].enabled && isCloseBtnHit(idx, ev.position())) {
                onTabCloseRequested.emit(idx);
                ev.accept();
                return;
            }
            if (items_[idx].enabled) {
                setSelectedIndex(idx);
                if (reorderable_) {
                    pressTabIndex_ = idx;
                    dragStartX_ = ev.position().x;
                    dragCurrentX_ = ev.position().x + scrollOffset_;
                }
            }
            if (isFocusable()) {
                setFocus(FocusReason::Pointer);
            }
            ev.accept();
            return;
        }
    } else if (ev.button() == platform::MouseButton::Middle) {
        int idx = tabIndexAt(ev.position());
        if (idx >= 0 && items_[idx].enabled && items_[idx].closable) {
            onTabCloseRequested.emit(idx);
            ev.accept();
        }
    }
}

void TabBar::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.isPrimaryButton()) {
        cancelActiveInteraction();
        ev.accept();
    }
}

void TabBar::onMouseWheel(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    float maxOff = maxScrollOffset();
    if (maxOff <= 0.0f) {
        return;
    }
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float delta = (std::abs(ev.wheelDelta().x) > 0.001f) ? ev.wheelDelta().x : ev.wheelDelta().y;
    if (std::abs(delta) > 0.001f) {
        setTabScrollOffset(scrollOffset_ - delta * kScrollWheelStep * scale);
        ev.accept();
    }
}

void TabBar::onMouseLeave() {
    if (hoveredIndex_ != -1 || hoveredCloseIndex_ != -1 || hoveredAddBtn_) {
        if (hoveredIndex_ >= 0 && hoveredIndex_ < static_cast<int>(items_.size())) {
            items_[hoveredIndex_].hoverAnim.start(items_[hoveredIndex_].hoverAnim.currentValue(),
                                                  0.0f, kHoverAnimDuration,
                                                  core::animation::EasingCurve::OutCubic);
            if (context_) {
                context_->registerAnimatedWidget(this);
            }
        }
        hoveredIndex_ = -1;
        hoveredCloseIndex_ = -1;
        hoveredAddBtn_ = false;
        setCursorShape(platform::CursorShape::Arrow);
        markPaintDirty();
    }
}

void TabBar::onKeyDown(WidgetKeyEvent& ev) {
    if (!isEnabled() || !ev.isDown() || items_.empty()) {
        return;
    }

    if (ev.hasControl() && ev.key() == platform::Key::W) {
        if (selectedIndex_ >= 0 && items_[selectedIndex_].closable) {
            onTabCloseRequested.emit(selectedIndex_);
            ev.accept();
        }
        return;
    }
    if (ev.hasControl() && ev.key() == platform::Key::Tab) {
        int next = findNextEnabledTab(selectedIndex_, ev.hasShift() ? -1 : 1);
        if (next >= 0) {
            setSelectedIndex(next);
            ev.accept();
        }
        return;
    }

    switch (ev.key()) {
    case platform::Key::Left: {
        int prev = findNextEnabledTab(selectedIndex_ < 0 ? 0 : selectedIndex_, -1);
        if (prev >= 0) {
            setSelectedIndex(prev);
        }
        ev.accept();
        break;
    }
    case platform::Key::Right: {
        int next = findNextEnabledTab(selectedIndex_ < 0 ? -1 : selectedIndex_, 1);
        if (next >= 0) {
            setSelectedIndex(next);
        }
        ev.accept();
        break;
    }
    case platform::Key::Home: {
        int first = findFirstEnabledTab();
        if (first >= 0) {
            setSelectedIndex(first);
        }
        ev.accept();
        break;
    }
    case platform::Key::End: {
        int last = findLastEnabledTab();
        if (last >= 0) {
            setSelectedIndex(last);
        }
        ev.accept();
        break;
    }
    case platform::Key::Delete:
        if (selectedIndex_ >= 0 && items_[selectedIndex_].closable) {
            onTabCloseRequested.emit(selectedIndex_);
            ev.accept();
        }
        break;
    default:
        break;
    }
}

void TabBar::onFocusGained() {
    markPaintDirty();
}

void TabBar::onFocusLost() {
    cancelActiveInteraction();
    markPaintDirty();
}

void TabBar::onContextAttached() {
    if (!hasCustomDpiScale_) {
        applyDpiScale(contextDpiScale());
    }
}

void TabBar::onThemeChanged() {
    for (auto& item : items_) {
        item.cachedLayout.reset();
        item.cachedTextWidth = 0.0f;
    }
    isTabsLayoutDirty_ = true;
    updateMeasure();
    markLayoutDirty();
    markPaintDirty();
}

void TabBar::onEnabledChanged() {
    if (!isEnabled()) {
        cancelActiveInteraction();
        hoveredIndex_ = -1;
        hoveredCloseIndex_ = -1;
        hoveredAddBtn_ = false;
        setCursorShape(platform::CursorShape::Arrow);
        for (auto& item : items_) {
            item.hoverAnim.setCurrentValue(0.0f);
        }
    }
    markPaintDirty();
}

void TabBar::updateAnimations(float deltaSeconds) {
    bool changed = false;
    if (indicatorXAnim_.isRunning()) {
        indicatorXAnim_.update(deltaSeconds);
        changed = true;
    }
    if (indicatorWAnim_.isRunning()) {
        indicatorWAnim_.update(deltaSeconds);
        changed = true;
    }
    for (auto& item : items_) {
        if (item.hoverAnim.isRunning()) {
            item.hoverAnim.update(deltaSeconds);
            changed = true;
        }
    }
    if (changed) {
        markPaintDirty();
    }
}

bool TabBar::hasActiveAnimations() const noexcept {
    if (indicatorXAnim_.isRunning() || indicatorWAnim_.isRunning()) {
        return true;
    }
    for (const auto& item : items_) {
        if (item.hoverAnim.isRunning()) {
            return true;
        }
    }
    return false;
}

void TabBar::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    ensureTabsLayout();
    const auto& pal = colorPalette();
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    bool barEnabled = isEnabled();

    if (style_ == TabBarStyle::Segmented) {
        float trackRadius = theme::Metrics::CornerRadiusCard * scale;
        ctx.drawRoundedRect(rect, pal.surfaceBase, trackRadius);
        ctx.drawBorder(rect, pal.borderSubtle, theme::Metrics::BorderWidthHairline * scale,
                       trackRadius);
    } else {
        // Background and bottom hairline border
        ctx.drawRect(rect, pal.surfaceDefault);
        float borderThick = theme::Metrics::BorderWidthHairline * scale;
        float bottomLineY = rect.origin.y + rect.size.height - borderThick;
        ctx.drawLine(core::Point{rect.origin.x, bottomLineY},
                     core::Point{rect.origin.x + rect.size.width, bottomLineY}, pal.borderSubtle,
                     borderThick);
    }

    float vpWidth = tabsViewportWidth();
    core::Rect clipRect{rect.origin, core::Size{vpWidth, rect.size.height}};
    ctx.pushClip(clipRect);

    float indX = indicatorXAnim_.currentValue();
    float indW = indicatorWAnim_.currentValue();
    if (selectedIndex_ >= 0 && indW <= 0.0f) {
        indX = items_[selectedIndex_].tabRect.origin.x;
        indW = items_[selectedIndex_].tabRect.size.width;
    }

    if (style_ == TabBarStyle::Segmented && items_.size() > 1) {
        for (size_t i = 0; i + 1 < items_.size(); ++i) {
            bool adjActive =
                (static_cast<int>(i) == selectedIndex_ ||
                 static_cast<int>(i + 1) == selectedIndex_ ||
                 static_cast<int>(i) == hoveredIndex_ || static_cast<int>(i + 1) == hoveredIndex_);
            if (!adjActive) {
                float divX = rect.origin.x - scrollOffset_ + items_[i].tabRect.origin.x +
                             items_[i].tabRect.size.width;
                ctx.drawLine(core::Point{divX, rect.origin.y + 8.0f * scale},
                             core::Point{divX, rect.origin.y + rect.size.height - 8.0f * scale},
                             pal.borderSubtle, theme::Metrics::BorderWidthHairline * scale);
            }
        }
    }

    if ((style_ == TabBarStyle::Pill || style_ == TabBarStyle::Segmented) && selectedIndex_ >= 0 &&
        indW > 0.0f) {
        float insetX = 3.0f * scale;
        float insetY = (style_ == TabBarStyle::Segmented) ? (3.0f * scale) : (4.0f * scale);
        float thumbRadius = theme::Metrics::CornerRadiusControl * scale;
        core::Rect pillRect{
            core::Point{rect.origin.x - scrollOffset_ + indX + insetX, rect.origin.y + insetY},
            core::Size{std::max(0.0f, indW - insetX * 2.0f),
                       std::max(0.0f, rect.size.height - insetY * 2.0f)}};
        ctx.drawRoundedRect(pillRect, pal.surfaceRaised, thumbRadius);
        if (style_ == TabBarStyle::Segmented) {
            ctx.drawBorder(pillRect, pal.borderDefault, theme::Metrics::BorderWidthHairline * scale,
                           thumbRadius);
        }
    }

    for (size_t i = 0; i < items_.size(); ++i) {
        const auto& itm = items_[i];
        core::Rect tabScreenRect{core::Point{rect.origin.x - scrollOffset_ + itm.tabRect.origin.x,
                                             rect.origin.y + itm.tabRect.origin.y},
                                 itm.tabRect.size};

        if (tabScreenRect.origin.x + tabScreenRect.size.width < rect.origin.x ||
            tabScreenRect.origin.x > rect.origin.x + vpWidth) {
            continue;
        }

        bool isSelected = (static_cast<int>(i) == selectedIndex_);
        bool isHovered = (static_cast<int>(i) == hoveredIndex_);
        float ht = (barEnabled && itm.enabled) ? itm.hoverAnim.currentValue() : 0.0f;

        if (style_ == TabBarStyle::EditorCard) {
            if (isSelected) {
                ctx.drawRect(tabScreenRect, pal.surfaceCanvas);
                ctx.drawLine(tabScreenRect.origin,
                             core::Point{tabScreenRect.origin.x + tabScreenRect.size.width,
                                         tabScreenRect.origin.y},
                             barEnabled ? pal.accentDefault : pal.textDisabled, 2.0f * scale);
            } else if (ht > 0.001f) {
                ctx.drawRect(tabScreenRect, pal.surfaceRaised.withAlpha(0.55f * ht));
            }
            // Vertical right hairline divider
            float divX = tabScreenRect.origin.x + tabScreenRect.size.width;
            ctx.drawLine(core::Point{divX, tabScreenRect.origin.y + 6.0f * scale},
                         core::Point{divX, tabScreenRect.origin.y + tabScreenRect.size.height -
                                               6.0f * scale},
                         pal.borderSubtle, theme::Metrics::BorderWidthHairline * scale);
        } else if (style_ == TabBarStyle::Underline && !isSelected && ht > 0.001f) {
            ctx.drawRect(tabScreenRect, pal.surfaceRaised.withAlpha(0.55f * ht));
        } else if (style_ == TabBarStyle::Pill && !isSelected && ht > 0.001f) {
            core::Rect hoverPill{
                core::Point{tabScreenRect.origin.x + 3.0f * scale,
                            tabScreenRect.origin.y + 4.0f * scale},
                core::Size{std::max(0.0f, tabScreenRect.size.width - 6.0f * scale),
                           std::max(0.0f, tabScreenRect.size.height - 8.0f * scale)}};
            ctx.drawRoundedRect(hoverPill, pal.surfaceRaised.withAlpha(0.55f * ht),
                                theme::Metrics::CornerRadiusControl * scale);
        } else if (style_ == TabBarStyle::Segmented && !isSelected && ht > 0.001f) {
            core::Rect hoverPill{
                core::Point{tabScreenRect.origin.x + 3.0f * scale,
                            tabScreenRect.origin.y + 3.0f * scale},
                core::Size{std::max(0.0f, tabScreenRect.size.width - 6.0f * scale),
                           std::max(0.0f, tabScreenRect.size.height - 6.0f * scale)}};
            ctx.drawRoundedRect(hoverPill, pal.surfaceRaised.withAlpha(0.55f * ht),
                                theme::Metrics::CornerRadiusControl * scale);
        }

        float contentStartX = tabScreenRect.origin.x + tabPaddingX_ * scale;
        if (style_ == TabBarStyle::Segmented) {
            float totalContentW = itm.cachedTextWidth;
            if (itm.icon) {
                totalContentW += (kIconSize + kIconGap) * scale;
            }
            contentStartX = tabScreenRect.origin.x +
                            std::max(0.0f, (tabScreenRect.size.width - totalContentW) * 0.5f);
        }
        core::Color fgCol = (!barEnabled || !itm.enabled)
                                ? pal.textDisabled
                                : (isSelected ? pal.textPrimary : pal.textSecondary);

        // Leading SVG icon
        if (itm.icon) {
            float iconSz = kIconSize * scale;
            float iconY = tabScreenRect.origin.y + (tabHeight_ - iconSz) * 0.5f;
            core::Rect iconRect{core::Point{contentStartX, iconY}, core::Size{iconSz, iconSz}};
            ctx.drawSvg(*itm.icon, iconRect, fgCol);
            contentStartX += iconSz + kIconGap * scale;
        }

        // Memoized Title text
        auto f = effectiveFont();
        if (f && itm.cachedLayout.has_value()) {
            float centerY = tabScreenRect.origin.y + tabHeight_ * 0.5f;
            float textY = theme::computeCenteredTextY(centerY, *f);
            ctx.drawTextLayout(core::Point{contentStartX, textY}, *itm.cachedLayout, fgCol);
        }

        // Trailing Close / Modified Dot
        if (itm.closable || itm.modified) {
            core::Rect closeScreenRect{
                core::Point{rect.origin.x - scrollOffset_ + itm.closeBtnRect.origin.x,
                            rect.origin.y + itm.closeBtnRect.origin.y},
                itm.closeBtnRect.size};

            bool isCloseHovered =
                barEnabled && itm.enabled && (static_cast<int>(i) == hoveredCloseIndex_);

            if (itm.modified && (!itm.closable || (!isCloseHovered && !isHovered))) {
                float dotSz = kModifiedDotSize * scale;
                float dotX = closeScreenRect.origin.x + (closeScreenRect.size.width - dotSz) * 0.5f;
                float dotY =
                    closeScreenRect.origin.y + (closeScreenRect.size.height - dotSz) * 0.5f;
                ctx.drawRoundedRect(core::Rect{core::Point{dotX, dotY}, core::Size{dotSz, dotSz}},
                                    (barEnabled && itm.enabled) ? pal.accentDefault
                                                                : pal.textDisabled,
                                    dotSz * 0.5f);
            } else if (itm.closable) {
                if (isCloseHovered) {
                    ctx.drawRoundedRect(closeScreenRect, pal.surfaceOverlay, 3.0f * scale);
                }
                core::Color xCol = (!barEnabled || !itm.enabled)
                                       ? pal.textDisabled
                                       : (isCloseHovered ? pal.textPrimary : pal.textTertiary);
                if (auto doc = getSvgDocument(IconId::Close)) {
                    float inset = 2.0f * scale;
                    core::Rect xRect{
                        core::Point{closeScreenRect.origin.x + inset,
                                    closeScreenRect.origin.y + inset},
                        core::Size{std::max(0.0f, closeScreenRect.size.width - inset * 2.0f),
                                   std::max(0.0f, closeScreenRect.size.height - inset * 2.0f)}};
                    ctx.drawSvg(*doc, xRect, xCol);
                }
            }
        }

        if (barEnabled && isSelected && hasKeyboardFocus()) {
            float focusRadius = (style_ == TabBarStyle::Pill || style_ == TabBarStyle::Segmented)
                                    ? theme::Metrics::CornerRadiusControl * scale
                                    : 0.0f;
            paintFocusRing(ctx, tabScreenRect, focusRadius);
        }
    }

    // Sliding Underline indicator
    if (style_ == TabBarStyle::Underline && selectedIndex_ >= 0 && indW > 0.0f) {
        float indThickness = 2.0f * scale;
        float indicatorY = rect.origin.y + rect.size.height - indThickness;
        float screenIndX = rect.origin.x - scrollOffset_ + indX;
        ctx.drawLine(core::Point{screenIndX, indicatorY},
                     core::Point{screenIndX + indW, indicatorY},
                     barEnabled ? pal.accentDefault : pal.textDisabled, indThickness);
    }

    ctx.popClip();

    // Trailing '+' Add Button
    if (showAddButton_) {
        core::Rect addLocal = addButtonRect();
        if (addLocal.size.width > 0.0f) {
            core::Rect addScreen{
                core::Point{rect.origin.x + addLocal.origin.x, rect.origin.y + addLocal.origin.y},
                addLocal.size};
            bool addHovered = barEnabled && hoveredAddBtn_;
            if (addHovered) {
                ctx.drawRect(addScreen, pal.surfaceRaised);
            }
            ctx.drawLine(
                addScreen.origin,
                core::Point{addScreen.origin.x, addScreen.origin.y + addScreen.size.height},
                pal.borderSubtle, theme::Metrics::BorderWidthHairline * scale);
            if (auto plusDoc = getSvgDocument(IconId::Plus)) {
                float pSize = 14.0f * scale;
                core::Rect pRect{
                    core::Point{addScreen.origin.x + (addScreen.size.width - pSize) * 0.5f,
                                addScreen.origin.y + (addScreen.size.height - pSize) * 0.5f},
                    core::Size{pSize, pSize}};
                core::Color plusCol = !barEnabled
                                          ? pal.textDisabled
                                          : (addHovered ? pal.textPrimary : pal.textSecondary);
                ctx.drawSvg(*plusDoc, pRect, plusCol);
            }
        }
    }
}

// TabView
TabView::TabView(std::shared_ptr<text::Font> font) {
    setFocusable(false);
    style().flexDirection = layout::FlexDirection::Column;
    style().width = layout::Length::percent(100.0f);
    style().height = layout::Length::percent(100.0f);

    tabBar_ = addChild(std::make_unique<TabBar>(font));
    deck_ = addChild(std::make_unique<Container>());
    deck_->style().flexDirection = layout::FlexDirection::Column;
    deck_->style().flexGrow = 1.0f;
    deck_->style().flexShrink = 1.0f;
    deck_->style().flexBasis = layout::Length::points(0.0f);
    deck_->style().width = layout::Length::percent(100.0f);
    deck_->style().height = layout::Length::percent(100.0f);

    tabBar_->onTabSelected.connect([this](int idx) {
        updatePageVisibility();
        onTabChanged.emit(idx);
    });

    tabBar_->onTabCloseRequested.connect([this](int idx) {
        removeTab(idx);
        onTabClosed.emit(idx);
    });

    tabBar_->onTabMoved.connect([this](int fromIndex, int toIndex) {
        if (fromIndex >= 0 && fromIndex < static_cast<int>(pages_.size()) && toIndex >= 0 &&
            toIndex < static_cast<int>(pages_.size()) && fromIndex != toIndex) {
            Widget* movingPage = pages_[fromIndex];
            pages_.erase(pages_.begin() + fromIndex);
            pages_.insert(pages_.begin() + toIndex, movingPage);
            updatePageVisibility();
            onTabMoved.emit(fromIndex, toIndex);
        }
    });

    tabBar_->onTabAddRequested.connect([this]() { onTabAddRequested.emit(); });
}

int TabView::addTab(std::string title, std::unique_ptr<Widget> page, bool closable) {
    return addTab(std::move(title), std::shared_ptr<renderer::SvgDocument>{}, std::move(page),
                  closable);
}

int TabView::addTab(std::string title, IconId iconId, std::unique_ptr<Widget> page, bool closable) {
    return addTab(std::move(title), getSvgDocument(iconId), std::move(page), closable);
}

int TabView::addTab(std::string title, std::shared_ptr<renderer::SvgDocument> icon,
                    std::unique_ptr<Widget> page, bool closable) {
    int idx = tabBar_->addTab(std::move(title), std::move(icon), closable);
    if (page) {
        page->style().width = layout::Length::percent(100.0f);
        page->style().height = layout::Length::percent(100.0f);
        page->style().flexGrow = 1.0f;
        page->style().flexShrink = 1.0f;
        page->style().flexBasis = layout::Length::points(0.0f);
        Widget* raw = deck_->addChild(std::move(page));
        pages_.push_back(raw);
    } else {
        pages_.push_back(nullptr);
    }
    updatePageVisibility();
    return idx;
}

void TabView::insertTab(int index, std::string title, std::unique_ptr<Widget> page, bool closable) {
    insertTabImpl(index, std::move(title), std::shared_ptr<renderer::SvgDocument>{},
                  std::move(page), closable);
}

void TabView::insertTab(int index, std::string title, IconId iconId, std::unique_ptr<Widget> page,
                        bool closable) {
    insertTabImpl(index, std::move(title), getSvgDocument(iconId), std::move(page), closable);
}

void TabView::insertTab(int index, std::string title, std::shared_ptr<renderer::SvgDocument> icon,
                        std::unique_ptr<Widget> page, bool closable) {
    insertTabImpl(index, std::move(title), std::move(icon), std::move(page), closable);
}

void TabView::insertTabImpl(int index, std::string title,
                            std::shared_ptr<renderer::SvgDocument> icon,
                            std::unique_ptr<Widget> page, bool closable) {
    int clamped = std::clamp(index, 0, static_cast<int>(pages_.size()));
    tabBar_->insertTab(clamped, std::move(title), std::move(icon), closable);
    if (page) {
        page->style().width = layout::Length::percent(100.0f);
        page->style().height = layout::Length::percent(100.0f);
        page->style().flexGrow = 1.0f;
        page->style().flexShrink = 1.0f;
        page->style().flexBasis = layout::Length::points(0.0f);
        Widget* raw = deck_->addChild(std::move(page));
        pages_.insert(pages_.begin() + clamped, raw);
    } else {
        pages_.insert(pages_.begin() + clamped, nullptr);
    }
    updatePageVisibility();
}

void TabView::removeTab(int index) {
    if (index >= 0 && index < static_cast<int>(pages_.size())) {
        if (pages_[index]) {
            deck_->removeChild(pages_[index]);
        }
        pages_.erase(pages_.begin() + index);
        tabBar_->removeTab(index);
        updatePageVisibility();
    }
}

void TabView::moveTab(int fromIndex, int toIndex) {
    tabBar_->moveTab(fromIndex, toIndex);
}

void TabView::setCurrentIndex(int index) {
    tabBar_->setSelectedIndex(index);
}

int TabView::currentIndex() const noexcept {
    return tabBar_->selectedIndex();
}

size_t TabView::tabCount() const noexcept {
    return tabBar_->tabCount();
}

Widget* TabView::pageAt(int index) const {
    if (index >= 0 && index < static_cast<int>(pages_.size())) {
        return pages_[index];
    }
    return nullptr;
}

void TabView::setTabModified(int index, bool modified) {
    tabBar_->setTabModified(index, modified);
}

void TabView::setTabEnabled(int index, bool enabled) {
    tabBar_->setTabEnabled(index, enabled);
}

void TabView::setTabBarStyle(TabBarStyle style) noexcept {
    tabBar_->setTabBarStyle(style);
}

void TabView::setReorderable(bool reorderable) noexcept {
    tabBar_->setReorderable(reorderable);
}

void TabView::setAddButtonVisible(bool visible) noexcept {
    tabBar_->setAddButtonVisible(visible);
}

void TabView::setFont(std::shared_ptr<text::Font> font) {
    if (tabBar_) {
        tabBar_->setFont(std::move(font));
    }
}

void TabView::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiScale(scale);
}

void TabView::applyDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        if (tabBar_) {
            tabBar_->setDpiScale(clamped);
            tabBar_->setTabHeight(34.0f * clamped);
        }
    }
}

void TabView::onContextAttached() {
    if (!hasCustomDpiScale_) {
        applyDpiScale(contextDpiScale());
    }
}

void TabView::onThemeChanged() {
    markPaintDirty();
}

void TabView::updatePageVisibility() {
    int sel = tabBar_->selectedIndex();
    for (size_t i = 0; i < pages_.size(); ++i) {
        if (pages_[i]) {
            pages_[i]->setVisible(static_cast<int>(i) == sel);
        }
    }
}

} // namespace northern::widgets
