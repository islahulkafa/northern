#include "widgets/containers/context_menu.hpp"

#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

namespace {
constexpr float kPaddingY = 5.0f;
constexpr float kItemHeight = 32.0f;
constexpr float kSubtitledItemHeight = 48.0f;
constexpr float kSeparatorHeight = 8.0f;
constexpr float kMinWidth = 200.0f;
constexpr float kPadX = 14.0f;
constexpr float kPillInsetX = 4.0f;
constexpr float kPillInsetY = 2.0f;
constexpr float kHoverAnimDuration = 0.12f;
constexpr float kGutterWidth = 24.0f;
constexpr float kTrailingGap = 20.0f;
constexpr float kIconSize = 16.0f;
} // namespace

ContextMenu::ContextMenu(std::shared_ptr<text::Font> font) : font_(std::move(font)) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::Arrow);
    layoutNode_.setMeasureFunc(
        [this](const layout::LayoutNode&, float, float) -> core::Size { return preferredSize(); });
}

void ContextMenu::addItem(std::string label, std::function<void()> onTriggered,
                          std::string shortcut, bool enabled) {
    items_.push_back(ContextMenuItem{
        .type = MenuItemType::Standard,
        .isSeparator = false,
        .label = std::move(label),
        .shortcut = std::move(shortcut),
        .enabled = enabled,
        .checked = false,
        .isDestructive = false,
        .icon = nullptr,
        .onTriggered = std::move(onTriggered),
        .onToggled = nullptr,
    });
    markPaintDirty();
    markLayoutDirty();
}

void ContextMenu::addItem(std::shared_ptr<renderer::SvgDocument> icon, std::string label,
                          std::function<void()> onTriggered, std::string shortcut, bool enabled) {
    items_.push_back(ContextMenuItem{
        .type = MenuItemType::Standard,
        .isSeparator = false,
        .label = std::move(label),
        .shortcut = std::move(shortcut),
        .enabled = enabled,
        .checked = false,
        .isDestructive = false,
        .icon = std::move(icon),
        .onTriggered = std::move(onTriggered),
        .onToggled = nullptr,
    });
    markPaintDirty();
    markLayoutDirty();
}

void ContextMenu::addItem(IconId iconId, std::string label, std::function<void()> onTriggered,
                          std::string shortcut, bool enabled) {
    addItem(getSvgDocument(iconId), std::move(label), std::move(onTriggered), std::move(shortcut),
            enabled);
}

void ContextMenu::addSubtitledItem(std::string label, std::string subtitle,
                                   std::function<void()> onTriggered, std::string shortcut,
                                   bool enabled) {
    items_.push_back(ContextMenuItem{
        .type = MenuItemType::Standard,
        .isSeparator = false,
        .label = std::move(label),
        .subtitle = std::move(subtitle),
        .shortcut = std::move(shortcut),
        .enabled = enabled,
        .checked = false,
        .isDestructive = false,
        .icon = nullptr,
        .onTriggered = std::move(onTriggered),
        .onToggled = nullptr,
    });
    markPaintDirty();
    markLayoutDirty();
}

void ContextMenu::addSubtitledItem(IconId iconId, std::string label, std::string subtitle,
                                   std::function<void()> onTriggered, std::string shortcut,
                                   bool enabled) {
    items_.push_back(ContextMenuItem{
        .type = MenuItemType::Standard,
        .isSeparator = false,
        .label = std::move(label),
        .subtitle = std::move(subtitle),
        .shortcut = std::move(shortcut),
        .enabled = enabled,
        .checked = false,
        .isDestructive = false,
        .icon = getSvgDocument(iconId),
        .onTriggered = std::move(onTriggered),
        .onToggled = nullptr,
    });
    markPaintDirty();
    markLayoutDirty();
}

void ContextMenu::addCheckItem(std::string label, bool checked, std::function<void(bool)> onToggled,
                               std::string shortcut, bool enabled) {
    items_.push_back(ContextMenuItem{
        .type = MenuItemType::Checkbox,
        .isSeparator = false,
        .label = std::move(label),
        .shortcut = std::move(shortcut),
        .enabled = enabled,
        .checked = checked,
        .isDestructive = false,
        .icon = nullptr,
        .onTriggered = nullptr,
        .onToggled = std::move(onToggled),
    });
    markPaintDirty();
    markLayoutDirty();
}

void ContextMenu::addRadioItem(std::string label, bool checked, std::function<void()> onSelected,
                               std::string shortcut, bool enabled) {
    items_.push_back(ContextMenuItem{
        .type = MenuItemType::Radio,
        .isSeparator = false,
        .label = std::move(label),
        .shortcut = std::move(shortcut),
        .enabled = enabled,
        .checked = checked,
        .isDestructive = false,
        .icon = nullptr,
        .onTriggered = std::move(onSelected),
        .onToggled = nullptr,
    });
    markPaintDirty();
    markLayoutDirty();
}

void ContextMenu::addDestructiveItem(std::string label, std::function<void()> onTriggered,
                                     std::string shortcut, bool enabled) {
    items_.push_back(ContextMenuItem{
        .type = MenuItemType::Standard,
        .isSeparator = false,
        .label = std::move(label),
        .shortcut = std::move(shortcut),
        .enabled = enabled,
        .checked = false,
        .isDestructive = true,
        .icon = nullptr,
        .onTriggered = std::move(onTriggered),
        .onToggled = nullptr,
        .submenuBuilder = nullptr,
    });
    markPaintDirty();
    markLayoutDirty();
}

void ContextMenu::addDestructiveItem(IconId iconId, std::string label,
                                     std::function<void()> onTriggered, std::string shortcut,
                                     bool enabled) {
    items_.push_back(ContextMenuItem{
        .type = MenuItemType::Standard,
        .isSeparator = false,
        .label = std::move(label),
        .shortcut = std::move(shortcut),
        .enabled = enabled,
        .checked = false,
        .isDestructive = true,
        .icon = getSvgDocument(iconId),
        .onTriggered = std::move(onTriggered),
        .onToggled = nullptr,
        .submenuBuilder = nullptr,
    });
    markPaintDirty();
    markLayoutDirty();
}

void ContextMenu::addSubmenu(std::string label,
                             std::function<std::unique_ptr<ContextMenu>()> builder,
                             std::shared_ptr<renderer::SvgDocument> icon, bool enabled) {
    items_.push_back(ContextMenuItem{
        .type = MenuItemType::Submenu,
        .isSeparator = false,
        .label = std::move(label),
        .shortcut = {},
        .enabled = enabled,
        .checked = false,
        .isDestructive = false,
        .icon = std::move(icon),
        .onTriggered = nullptr,
        .onToggled = nullptr,
        .submenuBuilder = std::move(builder),
    });
    markPaintDirty();
    markLayoutDirty();
}

void ContextMenu::addSubmenu(IconId iconId, std::string label,
                             std::function<std::unique_ptr<ContextMenu>()> builder, bool enabled) {
    addSubmenu(std::move(label), std::move(builder), getSvgDocument(iconId), enabled);
}

void ContextMenu::addSeparator() {
    items_.push_back(ContextMenuItem{
        .type = MenuItemType::Separator,
        .isSeparator = true,
        .label = {},
        .shortcut = {},
        .enabled = false,
        .checked = false,
        .isDestructive = false,
        .icon = nullptr,
        .onTriggered = nullptr,
        .onToggled = nullptr,
    });
    markPaintDirty();
    markLayoutDirty();
}

void ContextMenu::clearItems() {
    items_.clear();
    highlightedIndex_ = -1;
    pressedIndex_ = -1;
    markPaintDirty();
    markLayoutDirty();
}

bool ContextMenu::hasLeadingGutter() const noexcept {
    for (const auto& item : items_) {
        if (item.isSeparator) {
            continue;
        }
        if (item.type == MenuItemType::Checkbox || item.type == MenuItemType::Radio ||
            (iconPlacement_ == MenuIconPlacement::Leading && item.icon != nullptr)) {
            return true;
        }
    }
    return false;
}

void ContextMenu::setIconPlacement(MenuIconPlacement placement) noexcept {
    if (iconPlacement_ != placement) {
        iconPlacement_ = placement;
        markLayoutDirty();
        markPaintDirty();
    }
}

void ContextMenu::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiMetrics(scale);
}

void ContextMenu::applyDpiMetrics(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        resolvedDefaultFont_.reset();
        resolvedSubtitleFont_.reset();
        for (auto& item : items_) {
            item.cachedLabelLayout.reset();
            item.cachedSubtitleLayout.reset();
            item.cachedShortcutLayout.reset();
        }
        markLayoutDirty();
        markPaintDirty();
    }
}

void ContextMenu::onContextAttached() {
    if (!hasCustomDpiScale_) {
        float ctxDpi = contextDpiScale();
        if (ctxDpi > 0.0f && ctxDpi != dpiScale_) {
            applyDpiMetrics(ctxDpi);
        }
    }
    markPaintDirty();
}

void ContextMenu::onThemeChanged() {
    markPaintDirty();
}

void ContextMenu::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedDefaultFont_.reset();
        resolvedSubtitleFont_.reset();
        for (auto& item : items_) {
            item.cachedLabelLayout.reset();
            item.cachedSubtitleLayout.reset();
            item.cachedShortcutLayout.reset();
        }
        markPaintDirty();
        markLayoutDirty();
    }
}

std::shared_ptr<text::Font> ContextMenu::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        resolvedDefaultFont_ =
            text::FontManager::instance().defaultFont(13.5f * dpiScale_).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

std::shared_ptr<text::Font> ContextMenu::subtitleFont() const {
    if (!resolvedSubtitleFont_) {
        float basePx = font_ ? std::max(10.0f, font_->pixelSize() - 2.0f) : (12.0f * dpiScale_);
        resolvedSubtitleFont_ = text::FontManager::instance().defaultFont(basePx).valueOr(nullptr);
    }
    return resolvedSubtitleFont_;
}

void ContextMenu::registerAccelerators(WidgetContext& ctx) {
    for (size_t i = 0; i < items_.size(); ++i) {
        const auto& item = items_[i];
        if (!item.shortcut.empty() && item.enabled) {
            if (item.onTriggered) {
                ctx.registerAccelerator(item.shortcut, item.onTriggered);
            } else if (item.type == MenuItemType::Checkbox && item.onToggled) {
                size_t idx = i;
                ctx.registerAccelerator(item.shortcut, [this, idx]() {
                    if (idx < items_.size()) {
                        items_[idx].checked = !items_[idx].checked;
                        if (items_[idx].onToggled) {
                            items_[idx].onToggled(items_[idx].checked);
                        }
                    }
                });
            }
        }
        if (item.type == MenuItemType::Submenu && item.submenuBuilder) {
            auto sub = item.submenuBuilder();
            if (sub) {
                sub->registerAccelerators(ctx);
            }
        }
    }
}

float ContextMenu::itemHeightFor(const ContextMenuItem& item) const noexcept {
    if (item.isSeparator) {
        return kSeparatorHeight * dpiScale_;
    }
    return (item.subtitle.empty() ? kItemHeight : kSubtitledItemHeight) * dpiScale_;
}

core::Size ContextMenu::preferredSize() const {
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float maxLabelW = 0.0f;
    float maxTrailingW = 0.0f;
    float totalH = (kPaddingY * 2.0f) * scale;
    bool gutter = hasLeadingGutter();
    auto f = effectiveFont();
    auto sf = subtitleFont();

    for (const auto& item : items_) {
        if (item.isSeparator) {
            totalH += kSeparatorHeight * scale;
            continue;
        }
        totalH += itemHeightFor(item);

        float lw = static_cast<float>(item.label.size()) * 7.0f * scale;
        float sw = 0.0f;
        float tw = 0.0f;
        if (!item.shortcut.empty()) {
            tw += static_cast<float>(item.shortcut.size()) * 6.5f * scale;
        }

        if (f) {
            if (!item.label.empty()) {
                if (!item.cachedLabelLayout) {
                    item.cachedLabelLayout = text::TextLayout::create(item.label, f);
                }
                lw = item.cachedLabelLayout->size().width;
            }
            if (!item.shortcut.empty()) {
                if (!item.cachedShortcutLayout) {
                    item.cachedShortcutLayout = text::TextLayout::create(item.shortcut, f);
                }
                tw = item.cachedShortcutLayout->size().width;
            }
        }
        if (!item.subtitle.empty()) {
            if (sf) {
                if (!item.cachedSubtitleLayout) {
                    item.cachedSubtitleLayout = text::TextLayout::create(item.subtitle, sf);
                }
                sw = item.cachedSubtitleLayout->size().width;
            } else {
                sw = static_cast<float>(item.subtitle.size()) * 6.0f * scale;
            }
        }

        if (item.type == MenuItemType::Submenu) {
            tw = (tw > 0.0f) ? (tw + 20.0f * scale) : (16.0f * scale);
        } else if (iconPlacement_ == MenuIconPlacement::Trailing && item.icon &&
                   item.icon->isValid()) {
            float iconW = kIconSize * scale;
            tw = (tw > 0.0f) ? (tw + 10.0f * scale + iconW) : iconW;
        }

        maxLabelW = std::max({maxLabelW, lw, sw});
        maxTrailingW = std::max(maxTrailingW, tw);
    }

    float contentW = (kPadX * 2.0f) * scale + maxLabelW;
    if (gutter) {
        contentW += kGutterWidth * scale;
    }
    if (maxTrailingW > 0.0f) {
        contentW += kTrailingGap * scale + maxTrailingW;
    }

    return core::Size{std::max(kMinWidth * scale, contentW),
                      std::max((kItemHeight + kPaddingY * 2.0f) * scale, totalH)};
}

ContextMenu* ContextMenu::showAt(WidgetContext& ctx, std::unique_ptr<ContextMenu> menu,
                                 const core::Point& screenPos, Widget* owner) {
    if (!menu) {
        return nullptr;
    }

    ctx.dismissAllOverlays();

    if (!menu->hasCustomDpiScale_) {
        menu->applyDpiMetrics(ctx.dpiScale());
    }

    core::Size pref = menu->preferredSize();
    menu->style().width = layout::Length::points(pref.width);
    menu->style().height = layout::Length::points(pref.height);
    menu->layoutNode().setLayoutRect(core::Rect{core::Point{0.0f, 0.0f}, pref});

    OverlayDesc desc{};
    desc.anchorRect = core::Rect{screenPos, core::Size{0.0f, 0.0f}};
    desc.preferredPlacement = OverlayPlacement::Bottom;
    desc.autoFlip = true;
    desc.dismissOnOutsideClick = true;
    desc.dismissOnEscape = true;
    desc.gap = 0.0f;

    Widget* rawMenu = ctx.showOverlay(std::move(menu), desc, owner);
    if (rawMenu) {
        ctx.setFocusedWidget(rawMenu, FocusReason::Programmatic);
    }
    return static_cast<ContextMenu*>(rawMenu);
}

void ContextMenu::openSubmenu(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        return;
    }
    const auto& item = items_[index];
    if (item.type != MenuItemType::Submenu || !item.enabled || !item.submenuBuilder) {
        return;
    }
    if (activeSubmenuIndex_ == index && activeSubmenuWidget_) {
        return;
    }
    if (activeSubmenuWidget_) {
        closeSubmenu();
    }

    auto child = item.submenuBuilder();
    if (!child) {
        return;
    }

    child->setDpiScale(dpiScale_);
    child->setIconPlacement(iconPlacement_);
    if (!child->font() && font_) {
        child->setFont(font_);
    }

    core::Size pref = child->preferredSize();
    child->style().width = layout::Length::points(pref.width);
    child->style().height = layout::Length::points(pref.height);
    child->layoutNode().setLayoutRect(core::Rect{core::Point{0.0f, 0.0f}, pref});

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float rowY = kPaddingY * scale;
    for (int i = 0; i < index; ++i) {
        rowY += itemHeightFor(items_[i]);
    }
    core::Rect sRect = screenRect();
    core::Rect itemScreenRect{core::Point{sRect.origin.x, sRect.origin.y + rowY},
                              core::Size{sRect.size.width, itemHeightFor(items_[index])}};

    OverlayDesc desc{};
    desc.anchorRect = itemScreenRect;
    desc.preferredPlacement = OverlayPlacement::Right;
    desc.autoFlip = true;
    desc.dismissOnOutsideClick = true;
    desc.dismissOnEscape = true;
    desc.gap = -2.0f * scale;

    if (context_) {
        ContextMenu* rawChild = child.get();
        rawChild->parentMenu_ = this;
        rawChild->parentItemIndex_ = index;
        Widget* spawned = context_->showOverlay(std::move(child), desc, this);
        if (spawned) {
            activeSubmenuIndex_ = index;
            activeSubmenuWidget_ = static_cast<ContextMenu*>(spawned);
        }
    }
}

void ContextMenu::closeSubmenu() {
    if (activeSubmenuWidget_ && context_) {
        if (activeSubmenuWidget_->activeSubmenuWidget_) {
            activeSubmenuWidget_->closeSubmenu();
        }
        context_->dismissOverlaysForOwner(this);
    }
    activeSubmenuWidget_ = nullptr;
    activeSubmenuIndex_ = -1;
    pendingSubmenuIndex_ = -1;
    submenuHoverTimer_ = 0.0f;
}

int ContextMenu::itemIndexAtY(float localY) const noexcept {
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float y = kPaddingY * scale;
    for (size_t i = 0; i < items_.size(); ++i) {
        float rowH = itemHeightFor(items_[i]);
        if (localY >= y && localY < y + rowH) {
            return static_cast<int>(i);
        }
        y += rowH;
    }
    return -1;
}

void ContextMenu::setHighlightedIndex(int nextHighlight, bool animated) {
    if (highlightedIndex_ == nextHighlight) {
        return;
    }
    highlightedIndex_ = nextHighlight;
    bool startedAny = false;
    for (size_t i = 0; i < items_.size(); ++i) {
        auto& item = items_[i];
        bool isTarget =
            (static_cast<int>(i) == highlightedIndex_) && !item.isSeparator && item.enabled;
        float target = isTarget ? 1.0f : 0.0f;
        if (animated) {
            if (std::abs(item.hoverAnim.targetValue() - target) > 0.001f ||
                (!item.hoverAnim.isRunning() &&
                 std::abs(item.hoverAnim.currentValue() - target) > 0.001f)) {
                item.hoverAnim.start(item.hoverAnim.currentValue(), target, kHoverAnimDuration,
                                     core::animation::EasingCurve::OutCubic);
                startedAny = true;
            }
        } else {
            item.hoverAnim.stop();
            item.hoverAnim.setCurrentValue(target);
        }
    }
    if (startedAny && context_) {
        context_->registerAnimatedWidget(this);
    }
    markPaintDirty();
}

void ContextMenu::moveHighlight(int delta) noexcept {
    if (items_.empty() || delta == 0) {
        return;
    }
    const int count = static_cast<int>(items_.size());
    int start = highlightedIndex_;
    for (int step = 0; step < count; ++step) {
        int next = (start == -1) ? (delta > 0 ? 0 : count - 1) : (start + delta + count) % count;
        if (!items_[next].isSeparator && items_[next].enabled) {
            setHighlightedIndex(next, false);
            return;
        }
        start = next;
    }
}

void ContextMenu::activateItem(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        return;
    }
    auto& item = items_[index];
    if (item.isSeparator || !item.enabled) {
        return;
    }

    if (item.type == MenuItemType::Submenu) {
        openSubmenu(index);
        if (activeSubmenuWidget_ && context_) {
            context_->setFocusedWidget(activeSubmenuWidget_, FocusReason::Keyboard);
            activeSubmenuWidget_->moveHighlight(1);
        }
        return;
    }

    if (item.type == MenuItemType::Checkbox) {
        item.checked = !item.checked;
    } else if (item.type == MenuItemType::Radio) {
        for (int i = index - 1;
             i >= 0 && !items_[i].isSeparator && items_[i].type == MenuItemType::Radio; --i) {
            items_[i].checked = false;
        }
        for (size_t i = static_cast<size_t>(index) + 1;
             i < items_.size() && !items_[i].isSeparator && items_[i].type == MenuItemType::Radio;
             ++i) {
            items_[i].checked = false;
        }
        item.checked = true;
    }

    auto onTriggered = item.onTriggered;
    auto onToggled = item.onToggled;
    bool isChecked = item.checked;

    if (context_) {
        context_->dismissAllOverlays();
    }
    if (onTriggered) {
        onTriggered();
    }
    if (onToggled) {
        onToggled(isChecked);
    }
}

void ContextMenu::onMouseMove(WidgetMouseEvent& ev) {
    int idx = itemIndexAtY(ev.position().y);
    int nextHighlight = -1;
    if (idx >= 0 && idx < static_cast<int>(items_.size()) && !items_[idx].isSeparator &&
        items_[idx].enabled) {
        nextHighlight = idx;
    }
    if (highlightedIndex_ != nextHighlight) {
        setHighlightedIndex(nextHighlight, true);

        if (nextHighlight >= 0 && items_[nextHighlight].type == MenuItemType::Submenu) {
            if (activeSubmenuIndex_ != nextHighlight) {
                pendingSubmenuIndex_ = nextHighlight;
                submenuHoverTimer_ = 0.0f;
                if (context_) {
                    context_->registerAnimatedWidget(this);
                }
            }
        } else {
            pendingSubmenuIndex_ = -1;
            submenuHoverTimer_ = 0.0f;
            if (activeSubmenuWidget_ != nullptr) {
                closeSubmenu();
            }
        }
    }
    ev.accept();
}

void ContextMenu::onMouseLeave() {
    pendingSubmenuIndex_ = -1;
    submenuHoverTimer_ = 0.0f;
    if (pressedIndex_ != -1) {
        pressedIndex_ = -1;
        markPaintDirty();
    }
    if (activeSubmenuIndex_ == -1 && highlightedIndex_ != -1) {
        setHighlightedIndex(-1, true);
    }
}

void ContextMenu::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.isPrimaryButton()) {
        int idx = itemIndexAtY(ev.position().y);
        int nextPressed = -1;
        if (idx >= 0 && idx < static_cast<int>(items_.size()) && !items_[idx].isSeparator &&
            items_[idx].enabled) {
            nextPressed = idx;
        }
        if (pressedIndex_ != nextPressed) {
            pressedIndex_ = nextPressed;
            markPaintDirty();
        }
        ev.accept();
    }
}

void ContextMenu::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.isPrimaryButton()) {
        ev.accept();
        int idx = itemIndexAtY(ev.position().y);
        int pressed = pressedIndex_;
        if (pressedIndex_ != -1) {
            pressedIndex_ = -1;
            markPaintDirty();
        }
        if (pressed >= 0 && idx == pressed) {
            activateItem(idx);
        }
    }
}

void ContextMenu::onKeyDown(WidgetKeyEvent& ev) {
    if (ev.key() == platform::Key::Down) {
        moveHighlight(1);
        ev.accept();
        return;
    }
    if (ev.key() == platform::Key::Up) {
        moveHighlight(-1);
        ev.accept();
        return;
    }
    if (ev.key() == platform::Key::Right) {
        if (highlightedIndex_ >= 0 && items_[highlightedIndex_].type == MenuItemType::Submenu) {
            openSubmenu(highlightedIndex_);
            if (activeSubmenuWidget_ && context_) {
                context_->setFocusedWidget(activeSubmenuWidget_, FocusReason::Keyboard);
                activeSubmenuWidget_->moveHighlight(1);
            }
            ev.accept();
            return;
        }
        if (!parentMenu_ && onSiblingMenuNavigate_) {
            onSiblingMenuNavigate_(1);
            ev.accept();
            return;
        }
    }
    if (ev.key() == platform::Key::Left) {
        if (parentMenu_) {
            ContextMenu* p = parentMenu_;
            int pIdx = parentItemIndex_;
            p->closeSubmenu();
            if (p->context_) {
                p->context_->setFocusedWidget(p, FocusReason::Keyboard);
                p->setHighlightedIndex(pIdx, false);
            }
            ev.accept();
            return;
        }
        if (onSiblingMenuNavigate_) {
            onSiblingMenuNavigate_(-1);
            ev.accept();
            return;
        }
    }
    if (ev.key() == platform::Key::Escape) {
        if (parentMenu_) {
            ContextMenu* p = parentMenu_;
            int pIdx = parentItemIndex_;
            p->closeSubmenu();
            if (p->context_) {
                p->context_->setFocusedWidget(p, FocusReason::Keyboard);
                p->setHighlightedIndex(pIdx, false);
            }
            ev.accept();
            return;
        }
    }
    if (ev.key() == platform::Key::Home) {
        highlightedIndex_ = -1;
        moveHighlight(1);
        ev.accept();
        return;
    }
    if (ev.key() == platform::Key::End) {
        highlightedIndex_ = -1;
        moveHighlight(-1);
        ev.accept();
        return;
    }
    if (ev.key() == platform::Key::Enter || ev.key() == platform::Key::Space) {
        if (highlightedIndex_ >= 0) {
            ev.accept();
            activateItem(highlightedIndex_);
        }
        return;
    }
}

void ContextMenu::updateAnimations(float deltaSeconds) {
    bool needsPaint = false;
    for (auto& item : items_) {
        if (item.hoverAnim.isRunning()) {
            item.hoverAnim.update(deltaSeconds);
            needsPaint = true;
        }
    }
    if (pendingSubmenuIndex_ != -1) {
        submenuHoverTimer_ += deltaSeconds;
        if (submenuHoverTimer_ >= kSubmenuDelay) {
            int idx = pendingSubmenuIndex_;
            pendingSubmenuIndex_ = -1;
            submenuHoverTimer_ = 0.0f;
            openSubmenu(idx);
            needsPaint = true;
        }
    }
    if (needsPaint) {
        markPaintDirty();
    }
}

bool ContextMenu::hasActiveAnimations() const noexcept {
    if (pendingSubmenuIndex_ != -1) {
        return true;
    }
    for (const auto& item : items_) {
        if (item.hoverAnim.isRunning()) {
            return true;
        }
    }
    return false;
}

void ContextMenu::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float radius = theme::Metrics::CornerRadiusOverlay * scale;
    float radii[4] = {radius, radius, radius, radius};

    const auto& pal = colorPalette();
    ctx.drawBox(rect, pal.surfaceOverlay, pal.borderSubtle,
                theme::Metrics::BorderWidthHairline * scale, radii);
    ctx.pushClip(rect);

    bool gutter = hasLeadingGutter();
    float padX = kPadX * scale;
    float sepH = kSeparatorHeight * scale;
    float rowY = rect.origin.y + kPaddingY * scale;

    auto f = effectiveFont();
    auto sf = subtitleFont();

    for (size_t i = 0; i < items_.size(); ++i) {
        const auto& item = items_[i];
        if (item.isSeparator) {
            float sepY = rowY + sepH * 0.5f;
            ctx.drawLine(core::Point{rect.origin.x + padX, sepY},
                         core::Point{rect.origin.x + rect.size.width - padX, sepY},
                         pal.borderDefault, theme::Metrics::BorderWidthHairline * scale);
            rowY += sepH;
            continue;
        }

        float itemH = itemHeightFor(item);
        float pillInsetX = kPillInsetX * scale;
        float pillInsetY = kPillInsetY * scale;
        core::Rect itemRect{core::Point{rect.origin.x + pillInsetX, rowY + pillInsetY},
                            core::Size{std::max(0.0f, rect.size.width - pillInsetX * 2.0f),
                                       std::max(0.0f, itemH - pillInsetY * 2.0f)}};

        bool isHighlighted = (static_cast<int>(i) == highlightedIndex_) && item.enabled;
        bool isPressed = isHighlighted && (static_cast<int>(i) == pressedIndex_);
        float ht = std::clamp(item.hoverAnim.currentValue(), 0.0f, 1.0f);
        if (item.enabled && (isPressed || ht > 0.001f)) {
            float itemRadius =
                theme::resolveConcentricRadius(theme::Metrics::CornerRadiusOverlay, kPillInsetX) *
                scale;
            float itemRadii[4] = {itemRadius, itemRadius, itemRadius, itemRadius};
            float alpha = isPressed ? 0.85f : (0.55f * ht);
            core::Color pillColor = item.isDestructive
                                        ? pal.danger.withAlpha(isPressed ? 0.24f : (0.14f * ht))
                                        : pal.surfaceRaised.withAlpha(alpha);
            ctx.drawRoundedRect(itemRect, pillColor, itemRadii);
        }

        float centerY = rowY + itemH * 0.5f;
        float contentStartX = rect.origin.x + padX;

        core::Color itemCol;
        if (!item.enabled) {
            itemCol = pal.textDisabled;
        } else if (item.isDestructive) {
            itemCol = pal.danger;
        } else {
            itemCol = pal.textPrimary;
        }
        core::Color checkCol = item.enabled ? pal.accentDefault : pal.textDisabled;

        if (gutter) {
            float indicatorCenterX = contentStartX + 8.0f * scale;
            float indicatorCenterY = centerY;

            if (item.type == MenuItemType::Checkbox && item.checked) {
                core::Point p0{indicatorCenterX - 4.5f * scale, indicatorCenterY};
                core::Point p1{indicatorCenterX - 1.5f * scale, indicatorCenterY + 3.5f * scale};
                core::Point p2{indicatorCenterX + 4.5f * scale, indicatorCenterY - 3.5f * scale};
                ctx.drawLine(p0, p1, checkCol, 1.5f * scale);
                ctx.drawLine(p1, p2, checkCol, 1.5f * scale);
            } else if (item.type == MenuItemType::Radio && item.checked) {
                float dotRadius = 3.0f * scale;
                float dotRadii[4] = {dotRadius, dotRadius, dotRadius, dotRadius};
                ctx.drawRoundedRect(
                    core::Rect{{indicatorCenterX - dotRadius, indicatorCenterY - dotRadius},
                               {dotRadius * 2.0f, dotRadius * 2.0f}},
                    checkCol, dotRadii);
            } else if (iconPlacement_ == MenuIconPlacement::Leading && item.icon &&
                       item.icon->isValid()) {
                float iconSize = kIconSize * scale;
                float iconX = indicatorCenterX - iconSize * 0.5f;
                float iconY = indicatorCenterY - iconSize * 0.5f;
                ctx.drawSvg(*item.icon, core::Rect{{iconX, iconY}, {iconSize, iconSize}}, itemCol);
            }

            contentStartX += kGutterWidth * scale;
        }

        float rightEdgeX = rect.origin.x + rect.size.width - padX;
        if (item.type == MenuItemType::Submenu) {
            float chevX = rightEdgeX - 4.0f * scale;
            float chevY = centerY;
            ctx.drawLine(core::Point{chevX - 3.0f * scale, chevY - 4.0f * scale},
                         core::Point{chevX + 1.0f * scale, chevY}, itemCol, 1.5f * scale);
            ctx.drawLine(core::Point{chevX + 1.0f * scale, chevY},
                         core::Point{chevX - 3.0f * scale, chevY + 4.0f * scale}, itemCol,
                         1.5f * scale);
            rightEdgeX -= 16.0f * scale;
        } else if (iconPlacement_ == MenuIconPlacement::Trailing && item.icon &&
                   item.icon->isValid()) {
            float iconSize = kIconSize * scale;
            float iconX = rightEdgeX - iconSize;
            float iconY = centerY - iconSize * 0.5f;
            ctx.drawSvg(*item.icon, core::Rect{{iconX, iconY}, {iconSize, iconSize}}, itemCol);
            rightEdgeX -= (iconSize + 10.0f * scale);
        }

        if (f) {
            if (!item.label.empty()) {
                if (!item.cachedLabelLayout) {
                    item.cachedLabelLayout = text::TextLayout::create(item.label, f);
                }

                if (item.subtitle.empty()) {
                    float textY = theme::computeCenteredTextY(centerY, *f);
                    ctx.drawTextLayout(core::Point{contentStartX, textY}, *item.cachedLabelLayout,
                                       itemCol);
                } else {
                    float labelH = item.cachedLabelLayout->size().height;
                    float gap = 2.0f * scale;
                    float subH = 0.0f;
                    if (sf) {
                        if (!item.cachedSubtitleLayout) {
                            item.cachedSubtitleLayout = text::TextLayout::create(item.subtitle, sf);
                        }
                        subH = item.cachedSubtitleLayout->size().height;
                    }
                    float stackTop = rowY + (itemH - (labelH + gap + subH)) * 0.5f;
                    ctx.drawTextLayout(core::Point{contentStartX, stackTop},
                                       *item.cachedLabelLayout, itemCol);
                    if (sf && item.cachedSubtitleLayout) {
                        core::Color subCol = item.enabled ? pal.textSecondary : pal.textDisabled;
                        ctx.drawTextLayout(core::Point{contentStartX, stackTop + labelH + gap},
                                           *item.cachedSubtitleLayout, subCol);
                    }
                }
            }

            if (!item.shortcut.empty()) {
                if (!item.cachedShortcutLayout) {
                    item.cachedShortcutLayout = text::TextLayout::create(item.shortcut, f);
                }
                float scX = rightEdgeX - item.cachedShortcutLayout->size().width;
                float scY = theme::computeCenteredTextY(centerY, *f);
                core::Color scCol = (item.enabled ? pal.textTertiary : pal.textDisabled);
                ctx.drawTextLayout(core::Point{scX, scY}, *item.cachedShortcutLayout, scCol);
            }
        }

        if (i + 1 < items_.size() && !items_[i + 1].isSeparator &&
            static_cast<int>(i) != highlightedIndex_ &&
            static_cast<int>(i + 1) != highlightedIndex_) {
            float divY = rowY + itemH;
            ctx.drawLine(core::Point{rect.origin.x + padX, divY},
                         core::Point{rect.origin.x + rect.size.width - padX, divY},
                         pal.borderSubtle, theme::Metrics::BorderWidthHairline * scale);
        }

        rowY += itemH;
    }

    ctx.popClip();
}

} // namespace northern::widgets
