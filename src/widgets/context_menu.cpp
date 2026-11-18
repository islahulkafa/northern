#include "widgets/context_menu.hpp"

#include "renderer/paint_context.hpp"
#include "text/text_layout.hpp"
#include "widgets/theme.hpp"
#include "widgets/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

namespace {
constexpr float kPaddingY = 4.0f;
constexpr float kItemHeight = 28.0f;
constexpr float kSeparatorHeight = 9.0f;
constexpr float kMinWidth = 160.0f;
constexpr float kPadX = 12.0f;
constexpr float kShortcutGap = 24.0f;
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
        .isSeparator = false,
        .label = std::move(label),
        .shortcut = std::move(shortcut),
        .enabled = enabled,
        .onTriggered = std::move(onTriggered),
    });
    markPaintDirty();
}

void ContextMenu::addSeparator() {
    items_.push_back(ContextMenuItem{
        .isSeparator = true,
        .label = {},
        .shortcut = {},
        .enabled = false,
        .onTriggered = nullptr,
    });
    markPaintDirty();
}

void ContextMenu::clearItems() {
    items_.clear();
    highlightedIndex_ = -1;
    markPaintDirty();
}

void ContextMenu::setFont(std::shared_ptr<text::Font> font) {
    font_ = std::move(font);
    markPaintDirty();
}

core::Size ContextMenu::preferredSize() const {
    float maxLabelW = 0.0f;
    float maxShortcutW = 0.0f;
    float totalH = kPaddingY * 2.0f;

    for (const auto& item : items_) {
        if (item.isSeparator) {
            totalH += kSeparatorHeight;
            continue;
        }
        totalH += kItemHeight;

        float lw = static_cast<float>(item.label.size()) * 7.0f;
        float sw = item.shortcut.empty() ? 0.0f : static_cast<float>(item.shortcut.size()) * 6.5f;
        if (font_) {
            if (!item.label.empty()) {
                lw = text::TextLayout::create(item.label, font_).size().width;
            }
            if (!item.shortcut.empty()) {
                sw = text::TextLayout::create(item.shortcut, font_).size().width;
            }
        }
        maxLabelW = std::max(maxLabelW, lw);
        maxShortcutW = std::max(maxShortcutW, sw);
    }

    float contentW = kPadX * 2.0f + maxLabelW;
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
    menu->layoutNode().setLayoutRect(core::Rect{core::Point{0.0f, 0.0f}, pref});
    menu->openAnim_.start(0.0f, 1.0f, 0.12f, core::animation::EasingCurve::OutCubic);

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
    const auto& item = items_[index];
    if (item.isSeparator || !item.enabled) {
        return;
    }

    auto callback = item.onTriggered;
    if (context_) {
        context_->dismissTopOverlay();
    }
    if (callback) {
        callback();
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
    }
    ev.accept();
}

void ContextMenu::onMouseLeave() {
    if (highlightedIndex_ != -1) {
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
    openAnim_.update(deltaSeconds);
    markPaintDirty();
}

bool ContextMenu::hasActiveAnimations() const noexcept {
    return openAnim_.isRunning();
}

void ContextMenu::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    float t = openAnim_.currentValue();
    float slideOffset = (1.0f - t) * -4.0f;
    core::Rect animRect{core::Point{rect.origin.x, rect.origin.y + slideOffset}, rect.size};

    constexpr float kRadius = theme::Metrics::CornerRadiusControl;
    float radii[4] = {kRadius, kRadius, kRadius, kRadius};
    core::Color bg = theme::ColorTokens::SurfaceOverlay.withAlpha(t);
    core::Color border =
        theme::ColorTokens::BorderSubtle.withAlpha(theme::ColorTokens::BorderSubtle.a * t);

    ctx.drawBox(animRect, bg, border, 1.0f, radii);
    ctx.pushClip(animRect);

    float rowY = animRect.origin.y + kPaddingY;
    for (size_t i = 0; i < items_.size(); ++i) {
        const auto& item = items_[i];
        if (item.isSeparator) {
            float lineY = rowY + kSeparatorHeight * 0.5f;
            core::Color sepCol =
                theme::ColorTokens::BorderSubtle.withAlpha(theme::ColorTokens::BorderSubtle.a * t);
            ctx.drawLine(core::Point{animRect.origin.x + 6.0f, lineY},
                         core::Point{animRect.origin.x + animRect.size.width - 6.0f, lineY}, sepCol,
                         1.0f);
            rowY += kSeparatorHeight;
            continue;
        }

        core::Rect itemRect{core::Point{animRect.origin.x + 3.0f, rowY},
                            core::Size{std::max(0.0f, animRect.size.width - 6.0f), kItemHeight}};

        bool isHighlighted = (static_cast<int>(i) == highlightedIndex_) && item.enabled;
        if (isHighlighted) {
            float itemRadii[4] = {4.0f, 4.0f, 4.0f, 4.0f};
            ctx.drawRoundedRect(itemRect, core::Color{1.0f, 1.0f, 1.0f, 0.08f * t}, itemRadii);
        }

        if (font_) {
            float centerY = itemRect.origin.y + kItemHeight * 0.5f;
            float textY = theme::computeCenteredTextY(centerY, *font_);

            core::Color labelCol =
                (item.enabled ? theme::ColorTokens::TextPrimary : theme::ColorTokens::TextDisabled)
                    .withAlpha(t);
            if (!item.label.empty()) {
                auto labelLayout = text::TextLayout::create(item.label, font_);
                ctx.drawTextLayout(core::Point{animRect.origin.x + kPadX, textY}, labelLayout,
                                   labelCol);
            }

            if (!item.shortcut.empty()) {
                auto scLayout = text::TextLayout::create(item.shortcut, font_);
                float scX = animRect.origin.x + animRect.size.width - kPadX - scLayout.size().width;
                core::Color scCol = (item.enabled ? theme::ColorTokens::TextTertiary
                                                  : theme::ColorTokens::TextDisabled)
                                        .withAlpha(t);
                ctx.drawTextLayout(core::Point{scX, textY}, scLayout, scCol);
            }
        }

        rowY += kItemHeight;
    }

    ctx.popClip();
}

} // namespace northern::widgets
