#include "widgets/tab_view.hpp"

#include "renderer/paint_context.hpp"
#include "text/text_layout.hpp"
#include "widgets/container.hpp"
#include "widgets/icons.hpp"
#include "widgets/theme.hpp"
#include "widgets/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

// ============================================================================
// TabBar
// ============================================================================

TabBar::TabBar(std::shared_ptr<text::Font> font) : font_(std::move(font)) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::Arrow);
    style().height = layout::Length::points(tabHeight_);
    style().width = layout::Length::percent(100.0f);
    updateMeasure();
}

int TabBar::addTab(std::string title, bool closable, bool enabled) {
    int idx = static_cast<int>(items_.size());
    insertTab(idx, std::move(title), closable, enabled);
    return idx;
}

void TabBar::insertTab(int index, std::string title, bool closable, bool enabled) {
    int clamped = std::clamp(index, 0, static_cast<int>(items_.size()));
    TabItem item{
        .title = std::move(title),
        .closable = closable,
        .enabled = enabled,
    };
    items_.insert(items_.begin() + clamped, std::move(item));
    if (selectedIndex_ == -1 && !items_.empty()) {
        selectedIndex_ = 0;
    } else if (selectedIndex_ >= clamped) {
        selectedIndex_++;
    }
    markLayoutDirty();
}

void TabBar::removeTab(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        return;
    }
    items_.erase(items_.begin() + index);
    if (items_.empty()) {
        selectedIndex_ = -1;
    } else if (selectedIndex_ >= static_cast<int>(items_.size())) {
        selectedIndex_ = static_cast<int>(items_.size()) - 1;
        onTabSelected.emit(selectedIndex_);
    } else if (selectedIndex_ == index) {
        onTabSelected.emit(selectedIndex_);
    }
    markLayoutDirty();
}

void TabBar::clearTabs() {
    items_.clear();
    selectedIndex_ = -1;
    hoveredIndex_ = -1;
    hoveredCloseIndex_ = -1;
    markLayoutDirty();
}

void TabBar::setSelectedIndex(int index) {
    int clamped = items_.empty() ? -1 : std::clamp(index, 0, static_cast<int>(items_.size()) - 1);
    if (selectedIndex_ != clamped) {
        selectedIndex_ = clamped;
        markPaintDirty();
        if (selectedIndex_ >= 0) {
            onTabSelected.emit(selectedIndex_);
        }
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
        items_[index].title = std::move(title);
        markLayoutDirty();
    }
}

void TabBar::setFont(std::shared_ptr<text::Font> font) {
    font_ = std::move(font);
    updateMeasure();
    markLayoutDirty();
}

void TabBar::updateMeasure() {
    layoutNode_.setMeasureFunc(
        [this](const layout::LayoutNode&, float availW, float) -> core::Size {
            float w = (availW > 0.0f) ? availW : 200.0f;
            return core::Size{w, tabHeight_};
        });
}

void TabBar::layoutTabs(float totalWidth) {
    (void)totalWidth;
    float curX = 0.0f;
    for (auto& item : items_) {
        float textW = static_cast<float>(item.title.size()) * 7.5f;
        if (font_ && !item.title.empty()) {
            textW = text::TextLayout::create(item.title, font_).size().width;
        }
        float w = tabPaddingX_ * 2.0f + textW;
        if (item.closable) {
            w += 6.0f + 14.0f;
        }

        item.tabRect = core::Rect{core::Point{curX, 0.0f}, core::Size{w, tabHeight_}};
        if (item.closable) {
            float btnX = curX + w - tabPaddingX_ - 14.0f + 4.0f;
            float btnY = (tabHeight_ - 14.0f) * 0.5f;
            item.closeBtnRect = core::Rect{core::Point{btnX, btnY}, core::Size{14.0f, 14.0f}};
        } else {
            item.closeBtnRect = core::Rect{};
        }
        curX += w;
    }
}

int TabBar::tabIndexAt(const core::Point& localPos) const noexcept {
    for (size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].tabRect.contains(localPos)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool TabBar::isCloseBtnHit(int tabIndex, const core::Point& localPos) const noexcept {
    if (tabIndex >= 0 && tabIndex < static_cast<int>(items_.size())) {
        const auto& itm = items_[tabIndex];
        return itm.closable && itm.closeBtnRect.contains(localPos);
    }
    return false;
}

void TabBar::onMouseMove(WidgetMouseEvent& ev) {
    layoutTabs(layoutRect().size.width);
    int idx = tabIndexAt(ev.position());
    int closeIdx = isCloseBtnHit(idx, ev.position()) ? idx : -1;

    if (hoveredIndex_ != idx || hoveredCloseIndex_ != closeIdx) {
        hoveredIndex_ = idx;
        hoveredCloseIndex_ = closeIdx;
        setCursorShape(hoveredIndex_ >= 0 ? platform::CursorShape::PointingHand
                                          : platform::CursorShape::Arrow);
        markPaintDirty();
    }
}

void TabBar::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        layoutTabs(layoutRect().size.width);
        int idx = tabIndexAt(ev.position());
        if (idx >= 0) {
            if (isCloseBtnHit(idx, ev.position())) {
                onTabCloseRequested.emit(idx);
                ev.accept();
                return;
            }
            if (items_[idx].enabled) {
                setSelectedIndex(idx);
                ev.accept();
                return;
            }
        }
    }
}

void TabBar::onMouseLeave() {
    if (hoveredIndex_ != -1 || hoveredCloseIndex_ != -1) {
        hoveredIndex_ = -1;
        hoveredCloseIndex_ = -1;
        setCursorShape(platform::CursorShape::Arrow);
        markPaintDirty();
    }
}

void TabBar::onKeyDown(WidgetKeyEvent& ev) {
    if (items_.empty()) {
        return;
    }
    const int count = static_cast<int>(items_.size());
    if (ev.key() == platform::Key::Left) {
        ev.accept();
        int prev = (selectedIndex_ <= 0) ? (count - 1) : (selectedIndex_ - 1);
        setSelectedIndex(prev);
    } else if (ev.key() == platform::Key::Right) {
        ev.accept();
        int next = (selectedIndex_ + 1) % count;
        setSelectedIndex(next);
    }
}

void TabBar::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    layoutTabs(rect.size.width);

    // Background and bottom hairline border
    ctx.drawRect(rect, theme::ColorTokens::SurfaceDefault);
    float bottomLineY = rect.origin.y + rect.size.height - 1.0f;
    ctx.drawLine(core::Point{rect.origin.x, bottomLineY},
                 core::Point{rect.origin.x + rect.size.width, bottomLineY},
                 theme::ColorTokens::BorderSubtle, 1.0f);

    for (size_t i = 0; i < items_.size(); ++i) {
        const auto& itm = items_[i];
        core::Rect tabScreenRect{
            core::Point{rect.origin.x + itm.tabRect.origin.x, rect.origin.y + itm.tabRect.origin.y},
            itm.tabRect.size};

        bool isSelected = (static_cast<int>(i) == selectedIndex_);
        bool isHovered = (static_cast<int>(i) == hoveredIndex_);

        if (isSelected) {
            float indicatorY = tabScreenRect.origin.y + tabScreenRect.size.height - 2.0f;
            ctx.drawLine(core::Point{tabScreenRect.origin.x, indicatorY},
                         core::Point{tabScreenRect.origin.x + tabScreenRect.size.width, indicatorY},
                         theme::ColorTokens::AccentDefault, 2.0f);
        } else if (isHovered) {
            ctx.drawRect(tabScreenRect, core::Color{1.0f, 1.0f, 1.0f, 0.04f});
        }

        // Draw Title text
        if (font_ && !itm.title.empty()) {
            float centerY = tabScreenRect.origin.y + tabHeight_ * 0.5f;
            float textY = theme::computeCenteredTextY(centerY, *font_);
            core::Color textCol = !itm.enabled ? theme::ColorTokens::TextDisabled
                                               : (isSelected ? theme::ColorTokens::TextPrimary
                                                             : theme::ColorTokens::TextSecondary);
            auto layout = text::TextLayout::create(itm.title, font_);
            ctx.drawTextLayout(core::Point{tabScreenRect.origin.x + tabPaddingX_, textY}, layout,
                               textCol);
        }

        // Draw Close button
        if (itm.closable) {
            core::Rect closeScreenRect{core::Point{rect.origin.x + itm.closeBtnRect.origin.x,
                                                   rect.origin.y + itm.closeBtnRect.origin.y},
                                       itm.closeBtnRect.size};

            bool isCloseHovered = (static_cast<int>(i) == hoveredCloseIndex_);
            if (isCloseHovered) {
                float r[4] = {3.0f, 3.0f, 3.0f, 3.0f};
                ctx.drawRoundedRect(closeScreenRect, core::Color{1.0f, 1.0f, 1.0f, 0.12f}, r);
            }

            core::Color xCol =
                isCloseHovered ? theme::ColorTokens::TextPrimary : theme::ColorTokens::TextTertiary;
            if (auto doc = getSvgDocument(IconId::Close)) {
                core::Rect xRect{
                    core::Point{closeScreenRect.origin.x + 2.0f, closeScreenRect.origin.y + 2.0f},
                    core::Size{closeScreenRect.size.width - 4.0f, closeScreenRect.size.height - 4.0f}};
                ctx.drawSvg(*doc, xRect, xCol);
            }
        }

        if (isSelected && hasKeyboardFocus()) {
            theme::paintFocusRing(ctx, tabScreenRect, 0.0f);
        }
    }
}

// ============================================================================
// TabView
// ============================================================================

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
    deck_->style().width = layout::Length::percent(100.0f);
    deck_->style().height = layout::Length::points(0.0f);

    tabBar_->onTabSelected.connect([this](int idx) {
        updatePageVisibility();
        onTabChanged.emit(idx);
    });

    tabBar_->onTabCloseRequested.connect([this](int idx) {
        removeTab(idx);
        onTabClosed.emit(idx);
    });
}

int TabView::addTab(std::string title, std::unique_ptr<Widget> page, bool closable) {
    int idx = tabBar_->addTab(std::move(title), closable);
    if (page) {
        page->style().width = layout::Length::percent(100.0f);
        page->style().height = layout::Length::percent(100.0f);
        Widget* raw = deck_->addChild(std::move(page));
        pages_.push_back(raw);
    } else {
        pages_.push_back(nullptr);
    }
    updatePageVisibility();
    return idx;
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

void TabView::updatePageVisibility() {
    int sel = tabBar_->selectedIndex();
    for (size_t i = 0; i < pages_.size(); ++i) {
        if (pages_[i]) {
            pages_[i]->setVisible(static_cast<int>(i) == sel);
        }
    }
}

} // namespace northern::widgets
