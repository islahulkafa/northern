#include "widgets/containers/menu_bar.hpp"
#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

MenuBar::MenuBar(std::shared_ptr<text::Font> font) : font_(std::move(font)) {
    setFocusable(true);
    updateDimensions();
}

MenuBar::~MenuBar() {
    if (context_) {
        context_->dismissOverlaysForOwner(this);
    }
}

ContextMenu* MenuBar::addMenu(std::string title) {
    auto menu = std::make_unique<ContextMenu>(font_);
    return addMenu(std::move(title), std::move(menu));
}

ContextMenu* MenuBar::addMenu(std::string title, std::unique_ptr<ContextMenu> menu) {
    if (!menu) {
        menu = std::make_unique<ContextMenu>(font_);
    }
    ContextMenu* raw = menu.get();
    menus_.push_back(MenuEntry{
        .title = std::move(title),
        .menu = std::move(menu),
        .layout = std::nullopt,
        .itemRect = core::Rect{},
    });
    markLayoutDirty();
    markPaintDirty();
    return raw;
}

const std::string& MenuBar::menuTitle(size_t index) const {
    static const std::string kEmpty{};
    if (index >= menus_.size()) {
        return kEmpty;
    }
    return menus_[index].title;
}

ContextMenu* MenuBar::menuAt(size_t index) const {
    if (index >= menus_.size()) {
        return nullptr;
    }
    return menus_[index].menu.get();
}

void MenuBar::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedDefaultFont_.reset();
        for (auto& m : menus_) {
            m.layout.reset();
            if (m.menu) {
                m.menu->setFont(font_);
            }
        }
        markLayoutDirty();
        markPaintDirty();
    }
}

std::shared_ptr<text::Font> MenuBar::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float pt = 10.5f * dpiScale_;
        resolvedDefaultFont_ = text::FontManager::instance().defaultFont(pt).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void MenuBar::setDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        resolvedDefaultFont_.reset();
        for (auto& m : menus_) {
            m.layout.reset();
        }
        updateDimensions();
        markPaintDirty();
    }
}

float MenuBar::preferredHeight() const noexcept {
    return 28.0f * dpiScale_;
}

void MenuBar::updateDimensions() noexcept {
    float h = preferredHeight();
    style().height = layout::Length::points(h);
    style().width = layout::Length::percent(100.0f);

    layoutNode_.setMeasureFunc([h](const layout::LayoutNode&, float availW, float) -> core::Size {
        return core::Size{availW, h};
    });
    markLayoutDirty();
}

void MenuBar::updateItemRects() const {
    auto f = effectiveFont();
    float startX = 6.0f * dpiScale_;
    float h = preferredHeight();
    float padX = 8.0f * dpiScale_;

    for (auto& m : menus_) {
        if (!m.layout && f && !m.title.empty()) {
            m.layout = text::TextLayout::create(m.title, f);
        }
        float textW = m.layout ? m.layout->size().width : 0.0f;
        float itemW = textW + padX * 2.0f;
        m.itemRect = core::Rect{core::Point{startX, 0.0f}, core::Size{itemW, h}};
        startX += itemW + 2.0f * dpiScale_;
    }
}

int MenuBar::itemIndexAtPoint(const core::Point& pos) const {
    updateItemRects();
    for (size_t i = 0; i < menus_.size(); ++i) {
        if (menus_[i].itemRect.contains(pos)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void MenuBar::openMenu(int index) {
    if (openMenuIndex_ == index && index >= 0) {
        return;
    }

    if (context_) {
        context_->dismissOverlaysForOwner(this);
    }

    if (index < 0 || index >= static_cast<int>(menus_.size())) {
        openMenuIndex_ = -1;
        markPaintDirty();
        return;
    }

    openMenuIndex_ = index;
    highlightedIndex_ = index;
    markPaintDirty();
    updateItemRects();

    auto* menu = menus_[index].menu.get();
    if (menu && context_) {
        if (!menu->font()) {
            menu->setFont(effectiveFont());
        }

        core::Rect sRect = screenRect();
        core::Rect anchorScreen{core::Point{sRect.origin.x + menus_[index].itemRect.origin.x,
                                            sRect.origin.y + sRect.size.height},
                                core::Size{menus_[index].itemRect.size.width, 0.0f}};

        OverlayDesc desc{};
        desc.anchorRect = anchorScreen;
        desc.preferredPlacement = OverlayPlacement::Bottom;
        desc.autoFlip = true;
        desc.gap = 1.0f;
        desc.dismissOnOutsideClick = true;
        desc.dismissOnEscape = true;
        desc.onDismiss = [this]() {
            openMenuIndex_ = -1;
            markPaintDirty();
        };

        context_->showOverlay(menu, desc, this);
        context_->setFocusedWidget(menu, FocusReason::Programmatic);

        menu->setOnSiblingMenuNavigate([this](int delta) {
            int count = static_cast<int>(menus_.size());
            if (count > 0) {
                int next = (openMenuIndex_ + delta + count) % count;
                openMenu(next);
            }
        });
    }
}

void MenuBar::closeMenu() {
    if (context_ && openMenuIndex_ >= 0) {
        context_->dismissOverlaysForOwner(this);
    }
    openMenuIndex_ = -1;
    markPaintDirty();
}

void MenuBar::registerAccelerators(WidgetContext& ctx) {
    for (const auto& m : menus_) {
        if (m.menu) {
            m.menu->registerAccelerators(ctx);
        }
    }
}

void MenuBar::onMouseEnter() {
    markPaintDirty();
}

void MenuBar::onMouseMove(WidgetMouseEvent& ev) {
    int nextHighlight = itemIndexAtPoint(ev.position());
    if (highlightedIndex_ != nextHighlight) {
        highlightedIndex_ = nextHighlight;
        markPaintDirty();

        if (openMenuIndex_ >= 0 && nextHighlight >= 0 && nextHighlight != openMenuIndex_) {
            openMenu(nextHighlight);
        }
    }
    ev.accept();
}

void MenuBar::onMouseLeave() {
    if (openMenuIndex_ < 0 && highlightedIndex_ != -1) {
        highlightedIndex_ = -1;
        markPaintDirty();
    }
}

void MenuBar::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        ev.accept();
        int idx = itemIndexAtPoint(ev.position());
        if (idx >= 0) {
            if (openMenuIndex_ == idx) {
                closeMenu();
            } else {
                openMenu(idx);
            }
        }
    }
}

void MenuBar::onKeyDown(WidgetKeyEvent& ev) {
    int count = static_cast<int>(menus_.size());
    if (count == 0) {
        return;
    }

    if (ev.key() == platform::Key::Left) {
        ev.accept();
        int next = (highlightedIndex_ <= 0) ? count - 1 : highlightedIndex_ - 1;
        highlightedIndex_ = next;
        markPaintDirty();
        if (openMenuIndex_ >= 0) {
            openMenu(next);
        }
        return;
    }

    if (ev.key() == platform::Key::Right) {
        ev.accept();
        int next =
            (highlightedIndex_ < 0 || highlightedIndex_ >= count - 1) ? 0 : highlightedIndex_ + 1;
        highlightedIndex_ = next;
        markPaintDirty();
        if (openMenuIndex_ >= 0) {
            openMenu(next);
        }
        return;
    }

    if (ev.key() == platform::Key::Down || ev.key() == platform::Key::Enter ||
        ev.key() == platform::Key::Space) {
        ev.accept();
        int idx = (highlightedIndex_ >= 0) ? highlightedIndex_ : 0;
        openMenu(idx);
        return;
    }

    if (ev.key() == platform::Key::Escape) {
        ev.accept();
        closeMenu();
        highlightedIndex_ = -1;
        markPaintDirty();
        return;
    }
}

void MenuBar::onFocusLost() {
    if (openMenuIndex_ < 0) {
        highlightedIndex_ = -1;
        markPaintDirty();
    }
}

void MenuBar::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    const auto& pal = colorPalette();
    ctx.drawRect(rect, pal.surfaceBase);

    float borderY = rect.origin.y + rect.size.height - 1.0f;
    ctx.drawLine(core::Point{rect.origin.x, borderY},
                 core::Point{rect.origin.x + rect.size.width, borderY}, pal.borderSubtle, 1.0f);

    updateItemRects();
    auto f = effectiveFont();
    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;

    for (size_t i = 0; i < menus_.size(); ++i) {
        const auto& m = menus_[i];
        core::Rect itemScreen{
            core::Point{rect.origin.x + m.itemRect.origin.x,
                        rect.origin.y + m.itemRect.origin.y + 2.0f * dpiScale_},
            core::Size{m.itemRect.size.width, m.itemRect.size.height - 4.0f * dpiScale_}};

        bool isOpen = (static_cast<int>(i) == openMenuIndex_);
        bool isHigh = (static_cast<int>(i) == highlightedIndex_);

        if (isOpen) {
            float radii[4] = {4.0f * dpiScale_, 4.0f * dpiScale_, 4.0f * dpiScale_,
                              4.0f * dpiScale_};
            ctx.drawRoundedRect(itemScreen, pal.surfaceRaised, radii);
        } else if (isHigh) {
            float radii[4] = {4.0f * dpiScale_, 4.0f * dpiScale_, 4.0f * dpiScale_,
                              4.0f * dpiScale_};
            ctx.drawRoundedRect(itemScreen, pal.surfaceRaised.withAlpha(0.6f), radii);
        }

        if (f && m.layout) {
            float textY = theme::computeCenteredTextY(targetCenterY, *f);
            float textX = itemScreen.origin.x + 8.0f * dpiScale_;
            core::Color textCol = (isOpen || isHigh) ? pal.textPrimary : pal.textSecondary;
            ctx.drawTextLayout(core::Point{textX, textY}, *m.layout, textCol);
        }
    }
}

} // namespace northern::widgets
