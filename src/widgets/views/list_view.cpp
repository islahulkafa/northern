#include "widgets/views/list_view.hpp"

#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/icons.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

ListView::ListView(ItemModel* model, std::shared_ptr<text::Font> font)
    : model_(model), delegate_(std::make_unique<ItemDelegate>()), font_(std::move(font)) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::Arrow);
    style().width = layout::Length::percent(100.0f);
    style().height = layout::Length::percent(100.0f);

    if (model_) {
        selectionModel_.setModel(model_);
        model_->onModelReset.connect([this]() {
            clampScrollOffset();
            markPaintDirty();
        });
    }
}

void ListView::setModel(ItemModel* model) {
    if (model_ != model) {
        model_ = model;
        selectionModel_.setModel(model_);
        if (model_) {
            model_->onModelReset.connect([this]() {
                clampScrollOffset();
                markPaintDirty();
            });
        }
        clampScrollOffset();
        markPaintDirty();
    }
}

void ListView::setDelegate(std::unique_ptr<ItemDelegate> delegate) {
    if (delegate) {
        delegate_ = std::move(delegate);
        markPaintDirty();
    }
}

void ListView::setRowHeight(float height) noexcept {
    float h = std::max(12.0f, height);
    baseRowHeight_ = h / dpiScale_;
    if (rowHeight_ != h) {
        rowHeight_ = h;
        clampScrollOffset();
        markPaintDirty();
    }
}

void ListView::setListStyle(ListStyle style) noexcept {
    if (listStyle_ != style) {
        listStyle_ = style;
        if (style == ListStyle::Plain) {
            showBorder_ = true;
            alternatingRowColors_ = true;
        } else if (style == ListStyle::Inset) {
            showBorder_ = true;
            alternatingRowColors_ = false;
        } else {
            showBorder_ = false;
            alternatingRowColors_ = false;
        }
        markPaintDirty();
    }
}

void ListView::setShowBorder(bool show) noexcept {
    if (showBorder_ != show) {
        showBorder_ = show;
        markPaintDirty();
    }
}

void ListView::setAlternatingRowColors(bool alternate) noexcept {
    if (alternatingRowColors_ != alternate) {
        alternatingRowColors_ = alternate;
        markPaintDirty();
    }
}

void ListView::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedDefaultFont_.reset();
        markPaintDirty();
    }
}

std::shared_ptr<text::Font> ListView::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float pt = 10.5f * dpiScale_;
        resolvedDefaultFont_ = text::FontManager::instance().defaultFont(pt).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void ListView::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiScale(scale);
}

void ListView::applyDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        resolvedDefaultFont_.reset();
        rowHeight_ = std::max(12.0f, baseRowHeight_ * dpiScale_);
        scrollbarThickness_ = 6.0f * dpiScale_;
        clampScrollOffset();
        markPaintDirty();
    }
}

void ListView::onContextAttached() {
    if (!hasCustomDpiScale_) {
        applyDpiScale(contextDpiScale());
    }
}

void ListView::onThemeChanged() {
    resolvedDefaultFont_.reset();
    markPaintDirty();
}

void ListView::onEnabledChanged() {
    if (!isEnabled()) {
        cancelActiveInteraction();
    }
    markPaintDirty();
}

void ListView::onFocusGained() {
    markPaintDirty();
}

void ListView::onFocusLost() {
    cancelActiveInteraction();
    markPaintDirty();
}

void ListView::scrollTo(int row) {
    if (!model_ || row < 0 || row >= model_->rowCount()) {
        return;
    }
    float viewH = layoutRect().size.height;
    float rowTop = row * rowHeight_;
    float rowBottom = rowTop + rowHeight_;

    if (rowTop < scrollOffsetY_) {
        scrollOffsetY_ = rowTop;
    } else if (rowBottom > scrollOffsetY_ + viewH) {
        scrollOffsetY_ = rowBottom - viewH;
    }
    clampScrollOffset();
    markPaintDirty();
}

void ListView::scrollBy(float deltaY) {
    scrollOffsetY_ += deltaY;
    clampScrollOffset();
    markPaintDirty();
}

float ListView::maxScrollOffset() const noexcept {
    if (!model_) {
        return 0.0f;
    }
    float totalH = model_->rowCount() * rowHeight_;
    float viewH = layoutRect().size.height;
    return std::max(0.0f, totalH - viewH);
}

int ListView::rowAt(float localY) const noexcept {
    if (!model_ || rowHeight_ <= 0.0f) {
        return -1;
    }
    float contentY = localY + scrollOffsetY_;
    if (contentY < 0.0f) {
        return -1;
    }
    int r = static_cast<int>(std::floor(contentY / rowHeight_));
    if (r >= 0 && r < model_->rowCount()) {
        return r;
    }
    return -1;
}

core::Rect ListView::rowRect(int row) const noexcept {
    if (row < 0 || rowHeight_ <= 0.0f) {
        return {};
    }
    float y = (row * rowHeight_) - scrollOffsetY_;
    return core::Rect{core::Point{0.0f, y}, {layoutRect().size.width, rowHeight_}};
}

core::Rect ListView::verticalThumbRect() const noexcept {
    float maxScroll = maxScrollOffset();
    if (maxScroll <= 0.0f) {
        return {};
    }
    float viewH = layoutRect().size.height;
    float totalH = model_ ? model_->rowCount() * rowHeight_ : 0.0f;
    if (totalH <= 0.0f) {
        return {};
    }

    float thumbH = std::max(20.0f * dpiScale_, viewH * (viewH / totalH));
    float thumbY = (scrollOffsetY_ / maxScroll) * (viewH - thumbH);
    float thumbX = layoutRect().size.width - scrollbarThickness_ - 2.0f * dpiScale_;

    return core::Rect{core::Point{thumbX, thumbY}, {scrollbarThickness_, thumbH}};
}

void ListView::clampScrollOffset() noexcept {
    scrollOffsetY_ = std::clamp(scrollOffsetY_, 0.0f, maxScrollOffset());
}

void ListView::paint(renderer::PaintContext& ctx) {
    core::Rect sRect = screenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return;
    }

    const auto& pal = colorPalette();
    ctx.pushClip(sRect);
    if (listStyle_ != ListStyle::Sidebar) {
        ctx.drawRect(sRect, pal.surfaceCanvas);
    }

    if (model_ && rowHeight_ > 0.0f) {
        int count = model_->rowCount();
        if (count > 0) {
            int firstRow = std::max(0, static_cast<int>(std::floor(scrollOffsetY_ / rowHeight_)));
            int lastRow = std::min(
                count - 1,
                static_cast<int>(std::ceil((scrollOffsetY_ + sRect.size.height) / rowHeight_)));

            auto f = effectiveFont();
            for (int r = firstRow; r <= lastRow; ++r) {
                float y = sRect.origin.y + (r * rowHeight_) - scrollOffsetY_;
                core::Rect rBounds{core::Point{sRect.origin.x, y}, {sRect.size.width, rowHeight_}};
                ModelIndex idx = model_->index(r, 0);

                StyleOptionViewItem opt;
                opt.bounds = rBounds;
                opt.isSelected = selectionModel_.isSelected(idx);
                opt.isHovered = (r == hoveredRow_);
                opt.isAlternate = alternatingRowColors_ && (r % 2 == 1);
                opt.hasFocus = (hasKeyboardFocus() && selectionModel_.currentIndex() == idx);
                opt.text = model_->data(idx, ItemDataRole::Display);
                opt.font = f;
                opt.dpiScale = dpiScale_;
                opt.palette = &pal;
                opt.textColor = opt.isSelected
                                    ? pal.textPrimary
                                    : (opt.isHovered ? pal.textPrimary : pal.textSecondary);
                if (listStyle_ != ListStyle::Plain) {
                    opt.rowInsetX = 6.0f * dpiScale_;
                    opt.rowInsetY = 2.0f * dpiScale_;
                    opt.cornerRadius = theme::Metrics::CornerRadiusControl * dpiScale_;
                    opt.paddingLeft = 14.0f * dpiScale_;
                    opt.paddingRight = 14.0f * dpiScale_;
                }
                if (listStyle_ == ListStyle::Inset && r + 1 < count) {
                    bool nextHighlighted = selectionModel_.isSelected(model_->index(r + 1, 0)) ||
                                           (r + 1 == hoveredRow_);
                    opt.drawSeparator = !nextHighlighted;
                }

                // Optional roles from model
                std::string checkVal = model_->data(idx, ItemDataRole::CheckState);
                if (!checkVal.empty()) {
                    opt.showCheckbox = true;
                    opt.checkboxChecked = (checkVal == "1" || checkVal == "true");
                }
                opt.badgeText = model_->data(idx, ItemDataRole::BadgeText);
                if (!opt.badgeText.empty()) {
                    opt.badgeColor = pal.surfaceOverlay;
                    opt.badgeTextColor = pal.textPrimary;
                }

                if (delegate_) {
                    delegate_->paint(ctx, opt, idx);
                }
            }
        }
    }

    if (maxScrollOffset() > 0.0f) {
        core::Rect localThumb = verticalThumbRect();
        core::Rect screenThumb{
            core::Point{sRect.origin.x + localThumb.origin.x, sRect.origin.y + localThumb.origin.y},
            localThumb.size};
        core::Color restThumbCol = pal.mode == theme::ThemeMode::Dark
                                       ? core::Color{1.0f, 1.0f, 1.0f, 0.20f}
                                       : core::Color{0.0f, 0.0f, 0.0f, 0.20f};
        core::Color thumbCol = isDraggingThumb_
                                   ? pal.textSecondary
                                   : (isHoveringThumb_ ? pal.textTertiary : restThumbCol);
        float r[4] = {scrollbarThickness_ * 0.5f, scrollbarThickness_ * 0.5f,
                      scrollbarThickness_ * 0.5f, scrollbarThickness_ * 0.5f};
        ctx.drawRoundedRect(screenThumb, thumbCol, r);
    }

    if (showBorder_) {
        ctx.drawBorder(sRect, pal.borderSubtle, std::max(1.0f, std::round(1.0f * dpiScale_)));
    }
    ctx.popClip();

    if (hasKeyboardFocus() && isEnabled()) {
        Widget::paintFocusRing(ctx, sRect, 0.0f);
    }
}

void ListView::cancelActiveInteraction() {
    if (context_ && context_->mouseGrabWidget() == this) {
        context_->releaseMouseGrab();
    }
    bool wasActive = isDraggingThumb_ || isHoveringThumb_ || hoveredRow_ >= 0;
    isDraggingThumb_ = false;
    isHoveringThumb_ = false;
    hoveredRow_ = -1;
    if (wasActive) {
        markPaintDirty();
    }
}

void ListView::onMouseMove(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (isDraggingThumb_) {
        float maxScroll = maxScrollOffset();
        float viewH = layoutRect().size.height;
        float totalH = model_ ? model_->rowCount() * rowHeight_ : 0.0f;
        float thumbH = std::max(20.0f, viewH * (viewH / totalH));
        float trackH = viewH - thumbH;

        if (trackH > 0.0f) {
            float deltaY = ev.position().y - dragStartMouseY_;
            float scrollDelta = (deltaY / trackH) * maxScroll;
            scrollOffsetY_ = dragStartScrollY_ + scrollDelta;
            clampScrollOffset();
            markPaintDirty();
        }
        ev.accept();
        return;
    }

    core::Rect thumb = verticalThumbRect();
    bool hoveringThumb = maxScrollOffset() > 0.0f && thumb.contains(ev.position());
    if (hoveringThumb != isHoveringThumb_) {
        isHoveringThumb_ = hoveringThumb;
        markPaintDirty();
    }

    int r = rowAt(ev.position().y);
    if (r != hoveredRow_) {
        hoveredRow_ = r;
        markPaintDirty();
    }
}

void ListView::onMouseDown(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.isPrimaryButton()) {
        core::Rect thumb = verticalThumbRect();
        if (maxScrollOffset() > 0.0f) {
            float trackX = layoutRect().size.width - scrollbarThickness_ - 6.0f;
            if (ev.position().x >= trackX) {
                if (thumb.contains(ev.position())) {
                    isDraggingThumb_ = true;
                    dragStartMouseY_ = ev.position().y;
                    dragStartScrollY_ = scrollOffsetY_;
                    if (context_) {
                        context_->setMouseGrabWidget(this);
                    }
                } else {
                    float viewH = layoutRect().size.height;
                    if (ev.position().y < thumb.origin.y) {
                        scrollBy(-viewH * 0.8f);
                    } else {
                        scrollBy(viewH * 0.8f);
                    }
                }
                ev.accept();
                markPaintDirty();
                return;
            }
        }

        int r = rowAt(ev.position().y);
        if (r >= 0 && model_) {
            setFocus(FocusReason::Pointer);
            ModelIndex idx = model_->index(r, 0);

            bool ctrl = (static_cast<uint8_t>(ev.modifiers()) &
                         static_cast<uint8_t>(platform::KeyModifier::Control)) != 0;
            bool shift = (static_cast<uint8_t>(ev.modifiers()) &
                          static_cast<uint8_t>(platform::KeyModifier::Shift)) != 0;

            if (shift && selectionModel_.currentIndex().isValid()) {
                selectionModel_.selectRange(selectionModel_.currentIndex(), idx,
                                            SelectionFlag::ClearAndSelect);
            } else if (ctrl) {
                selectionModel_.select(idx, SelectionFlag::Toggle);
            } else {
                selectionModel_.select(idx, SelectionFlag::ClearAndSelect);
            }

            anchorRow_ = r;
            selectionModel_.setCurrentIndex(idx);
            onItemClicked.emit(idx);
            ev.accept();
            markPaintDirty();
        }
    }
}

void ListView::onMouseUp(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        cancelActiveInteraction();
        return;
    }
    if (ev.isPrimaryButton() && isDraggingThumb_) {
        isDraggingThumb_ = false;
        if (context_) {
            context_->releaseMouseGrab();
        }
        ev.accept();
        markPaintDirty();
    }
}

void ListView::onMouseWheel(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (maxScrollOffset() > 0.0f) {
        scrollBy(-ev.wheelDelta().y * 24.0f);
        ev.accept();
    }
}

void ListView::onMouseLeave() {
    if (!isDraggingThumb_) {
        hoveredRow_ = -1;
        isHoveringThumb_ = false;
        markPaintDirty();
    }
}

void ListView::onKeyDown(WidgetKeyEvent& ev) {
    if (!isEnabled() || !ev.isDown() || !model_ || model_->rowCount() == 0) {
        return;
    }

    int current = selectionModel_.currentIndex().isValid() ? selectionModel_.currentIndex().row : 0;
    int target = current;
    int count = model_->rowCount();
    int pageItems = std::max(1, static_cast<int>(layoutRect().size.height / rowHeight_));

    bool ctrl = (static_cast<uint8_t>(ev.modifiers()) &
                 static_cast<uint8_t>(platform::KeyModifier::Control)) != 0;
    bool shift = (static_cast<uint8_t>(ev.modifiers()) &
                  static_cast<uint8_t>(platform::KeyModifier::Shift)) != 0;

    if (ctrl && ev.key() == platform::Key::A) {
        selectionModel_.selectAll();
        markPaintDirty();
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Up) {
        target = std::max(0, current - 1);
    } else if (ev.key() == platform::Key::Down) {
        target = std::min(count - 1, current + 1);
    } else if (ev.key() == platform::Key::PageUp) {
        target = std::max(0, current - pageItems);
    } else if (ev.key() == platform::Key::PageDown) {
        target = std::min(count - 1, current + pageItems);
    } else if (ev.key() == platform::Key::Home) {
        target = 0;
    } else if (ev.key() == platform::Key::End) {
        target = count - 1;
    } else if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        if (selectionModel_.currentIndex().isValid()) {
            onItemActivated.emit(selectionModel_.currentIndex());
        }
        ev.accept();
        return;
    } else {
        return;
    }

    ModelIndex newIdx = model_->index(target, 0);
    if (anchorRow_ < 0 || anchorRow_ >= count) {
        anchorRow_ = current;
    }

    if (shift) {
        ModelIndex anchorIdx = model_->index(anchorRow_, 0);
        selectionModel_.selectRange(anchorIdx, newIdx, SelectionFlag::ClearAndSelect);
    } else {
        anchorRow_ = target;
        selectionModel_.select(newIdx, SelectionFlag::ClearAndSelect);
    }
    selectionModel_.setCurrentIndex(newIdx);
    scrollTo(target);
    markPaintDirty();
    ev.accept();
}

} // namespace northern::widgets
