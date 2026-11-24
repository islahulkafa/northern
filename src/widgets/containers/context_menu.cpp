#include "widgets/containers/context_menu.hpp"

#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

namespace {
constexpr float kPaddingY = 4.0f;
constexpr float kItemHeight = 28.0f;
constexpr float kSeparatorHeight = 9.0f;
constexpr float kMinWidth = 160.0f;
constexpr float kPadX = 12.0f;
constexpr float kGutterWidth = 24.0f;
constexpr float kShortcutGap = 20.0f;
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
    markPaintDirty();
    markLayoutDirty();
}

bool ContextMenu::hasLeadingGutter() const noexcept {
    for (const auto& item : items_) {
        if (item.isSeparator) {
            continue;
        }
        if (item.icon != nullptr || item.type == MenuItemType::Checkbox ||
            item.type == MenuItemType::Radio) {
            return true;
        }
    }
    return false;
}

void ContextMenu::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedDefaultFont_.reset();
        for (auto& item : items_) {
            item.cachedLabelLayout.reset();
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
        resolvedDefaultFont_ = text::FontManager::instance().defaultFont(10.5f).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
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

core::Size ContextMenu::preferredSize() const {
    float maxLabelW = 0.0f;
    float maxShortcutW = 0.0f;
    float totalH = kPaddingY * 2.0f;
    bool gutter = hasLeadingGutter();
    auto f = effectiveFont();

    for (const auto& item : items_) {
        if (item.isSeparator) {
            totalH += kSeparatorHeight;
            continue;
        }
        totalH += kItemHeight;

        float lw = static_cast<float>(item.label.size()) * 7.0f;
        float sw = item.shortcut.empty() ? 0.0f : static_cast<float>(item.shortcut.size()) * 6.5f;
        if (item.type == MenuItemType::Submenu) {
            sw = std::max(sw, 16.0f);
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
                sw = item.cachedShortcutLayout->size().width;
            }
        }
        maxLabelW = std::max(maxLabelW, lw);
        maxShortcutW = std::max(maxShortcutW, sw);
    }

    float contentW = kPadX * 2.0f + maxLabelW;
    if (gutter) {
        contentW += kGutterWidth;
    }
    if (maxShortcutW > 0.0f) {
        contentW += kShortcutGap + maxShortcutW;
    }

    return core::Size{std::max(kMinWidth, contentW),
                      std::max(kItemHeight + kPaddingY * 2.0f, totalH)};
}

ContextMenu* ContextMenu::showAt(WidgetContext& ctx, std::unique_ptr<ContextMenu> menu,
                                 const core::Point& screenPos, Widget* owner) {
    if (!menu) {
        return nullptr;
    }

    ctx.dismissAllOverlays();

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

    if (!child->font() && font_) {
        child->setFont(font_);
    }

    core::Size pref = child->preferredSize();
    child->style().width = layout::Length::points(pref.width);
    child->style().height = layout::Length::points(pref.height);
    child->layoutNode().setLayoutRect(core::Rect{core::Point{0.0f, 0.0f}, pref});

    float rowY = kPaddingY;
    for (int i = 0; i < index; ++i) {
        rowY += items_[i].isSeparator ? kSeparatorHeight : kItemHeight;
    }
    core::Rect sRect = screenRect();
    core::Rect itemScreenRect{core::Point{sRect.origin.x, sRect.origin.y + rowY},
                              core::Size{sRect.size.width, kItemHeight}};

    OverlayDesc desc{};
    desc.anchorRect = itemScreenRect;
    desc.preferredPlacement = OverlayPlacement::Right;
    desc.autoFlip = true;
    desc.dismissOnOutsideClick = true;
    desc.dismissOnEscape = true;
    desc.gap = -2.0f;

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
    float y = kPaddingY;
    for (size_t i = 0; i < items_.size(); ++i) {
        float rowH = items_[i].isSeparator ? kSeparatorHeight : kItemHeight;
        if (localY >= y && localY < y + rowH) {
            return static_cast<int>(i);
        }
        y += rowH;
    }
    return -1;
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
            highlightedIndex_ = next;
            markPaintDirty();
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
        highlightedIndex_ = nextHighlight;
        markPaintDirty();

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
    if (activeSubmenuIndex_ == -1 && highlightedIndex_ != -1) {
        highlightedIndex_ = -1;
        markPaintDirty();
    }
}

void ContextMenu::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        ev.accept();
    }
}

void ContextMenu::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        ev.accept();
        int idx = itemIndexAtY(ev.position().y);
        activateItem(idx);
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
                p->highlightedIndex_ = pIdx;
                p->markPaintDirty();
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
                p->highlightedIndex_ = pIdx;
                p->markPaintDirty();
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
        ev.accept();
        if (highlightedIndex_ >= 0) {
            activateItem(highlightedIndex_);
        }
        return;
    }
}

void ContextMenu::updateAnimations(float deltaSeconds) {
    if (pendingSubmenuIndex_ != -1) {
        submenuHoverTimer_ += deltaSeconds;
        if (submenuHoverTimer_ >= kSubmenuDelay) {
            int idx = pendingSubmenuIndex_;
            pendingSubmenuIndex_ = -1;
            submenuHoverTimer_ = 0.0f;
            openSubmenu(idx);
        }
    }
    markPaintDirty();
}

bool ContextMenu::hasActiveAnimations() const noexcept {
    return pendingSubmenuIndex_ != -1;
}

void ContextMenu::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    constexpr float kRadius = theme::Metrics::CornerRadiusControl;
    float radii[4] = {kRadius, kRadius, kRadius, kRadius};

    const auto& pal = colorPalette();
    ctx.drawBox(rect, pal.surfaceOverlay, pal.borderSubtle, theme::Metrics::BorderWidthHairline,
                radii);
    ctx.pushClip(rect);

    bool gutter = hasLeadingGutter();
    float rowY = rect.origin.y + kPaddingY;

    for (size_t i = 0; i < items_.size(); ++i) {
        const auto& item = items_[i];
        if (item.isSeparator) {
            float lineY = rowY + kSeparatorHeight * 0.5f;
            ctx.drawLine(core::Point{rect.origin.x + 6.0f, lineY},
                         core::Point{rect.origin.x + rect.size.width - 6.0f, lineY},
                         pal.borderSubtle, theme::Metrics::BorderWidthHairline);
            rowY += kSeparatorHeight;
            continue;
        }

        core::Rect itemRect{core::Point{rect.origin.x + 3.0f, rowY},
                            core::Size{std::max(0.0f, rect.size.width - 6.0f), kItemHeight}};

        bool isHighlighted = (static_cast<int>(i) == highlightedIndex_) && item.enabled;
        if (isHighlighted) {
            float itemRadii[4] = {4.0f, 4.0f, 4.0f, 4.0f};
            core::Color highlightBg =
                item.isDestructive ? pal.danger.withAlpha(0.12f) : pal.surfaceRaised;
            ctx.drawRoundedRect(itemRect, highlightBg, itemRadii);
        }

        float centerY = itemRect.origin.y + kItemHeight * 0.5f;
        float contentStartX = rect.origin.x + kPadX;

        core::Color itemCol;
        if (!item.enabled) {
            itemCol = pal.textDisabled;
        } else if (item.isDestructive) {
            itemCol = pal.danger;
        } else {
            itemCol = pal.textPrimary;
        }

        // Gutter rendering (Checkbox checkmark, Radio dot, or Vector icon)
        if (gutter) {
            float indicatorCenterX = contentStartX + 8.0f;
            float indicatorCenterY = centerY;

            if (item.type == MenuItemType::Checkbox && item.checked) {
                core::Point p0{indicatorCenterX - 4.5f, indicatorCenterY};
                core::Point p1{indicatorCenterX - 1.5f, indicatorCenterY + 3.5f};
                core::Point p2{indicatorCenterX + 4.5f, indicatorCenterY - 3.5f};
                ctx.drawLine(p0, p1, itemCol, 1.5f);
                ctx.drawLine(p1, p2, itemCol, 1.5f);
            } else if (item.type == MenuItemType::Radio && item.checked) {
                float dotRadius = 3.0f;
                float dotRadii[4] = {dotRadius, dotRadius, dotRadius, dotRadius};
                ctx.drawRoundedRect(
                    core::Rect{{indicatorCenterX - dotRadius, indicatorCenterY - dotRadius},
                               {dotRadius * 2.0f, dotRadius * 2.0f}},
                    itemCol, dotRadii);
            } else if (item.icon && item.icon->isValid()) {
                float iconX = indicatorCenterX - kIconSize * 0.5f;
                float iconY = indicatorCenterY - kIconSize * 0.5f;
                ctx.drawSvg(*item.icon, core::Rect{{iconX, iconY}, {kIconSize, kIconSize}},
                            itemCol);
            }

            contentStartX += kGutterWidth;
        }

        auto f = effectiveFont();
        if (f) {
            float textY = theme::computeCenteredTextY(centerY, *f);

            if (!item.label.empty()) {
                if (!item.cachedLabelLayout) {
                    item.cachedLabelLayout = text::TextLayout::create(item.label, f);
                }
                ctx.drawTextLayout(core::Point{contentStartX, textY}, *item.cachedLabelLayout,
                                   itemCol);
            }

            if (!item.shortcut.empty()) {
                if (!item.cachedShortcutLayout) {
                    item.cachedShortcutLayout = text::TextLayout::create(item.shortcut, f);
                }
                float scX = rect.origin.x + rect.size.width - kPadX -
                            item.cachedShortcutLayout->size().width;
                core::Color scCol = (item.enabled ? pal.textTertiary : pal.textDisabled);
                ctx.drawTextLayout(core::Point{scX, textY}, *item.cachedShortcutLayout, scCol);
            }
        }

        if (item.type == MenuItemType::Submenu) {
            float chevX = rect.origin.x + rect.size.width - kPadX - 4.0f;
            float chevY = centerY;
            ctx.drawLine(core::Point{chevX - 3.0f, chevY - 4.0f}, core::Point{chevX + 1.0f, chevY},
                         itemCol, 1.5f);
            ctx.drawLine(core::Point{chevX + 1.0f, chevY}, core::Point{chevX - 3.0f, chevY + 4.0f},
                         itemCol, 1.5f);
        }

        rowY += kItemHeight;
    }

    ctx.popClip();
}

} // namespace northern::widgets
