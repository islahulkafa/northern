#include "widgets/views/tree_view.hpp"

#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/icons.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

TreeView::TreeView(ItemModel* model, std::shared_ptr<text::Font> font)
    : model_(model), delegate_(std::make_unique<ItemDelegate>()), font_(std::move(font)) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::Arrow);
    style().width = layout::Length::percent(100.0f);
    style().height = layout::Length::percent(100.0f);
    style().flexDirection = layout::FlexDirection::Column;

    header_ = addChild(std::make_unique<HeaderView>(font_));
    header_->style().width = layout::Length::percent(100.0f);
    header_->onSectionResized.connect([this](size_t, float) {
        clampScrollOffsets();
        markPaintDirty();
    });

    if (model_) {
        selectionModel_.setModel(model_);
        syncHeaderFromModel();
        rebuildVisibleNodes();
        model_->onModelReset.connect([this]() {
            syncHeaderFromModel();
            rebuildVisibleNodes();
        });
    }
}

void TreeView::setModel(ItemModel* model) {
    if (model_ != model) {
        model_ = model;
        selectionModel_.setModel(model_);
        expandedNodes_.clear();
        if (model_) {
            syncHeaderFromModel();
            rebuildVisibleNodes();
            model_->onModelReset.connect([this]() {
                syncHeaderFromModel();
                rebuildVisibleNodes();
            });
        } else {
            visibleNodes_.clear();
        }
        clampScrollOffsets();
        markPaintDirty();
    }
}

void TreeView::syncHeaderFromModel() {
    if (!model_ || !header_) {
        return;
    }
    header_->clearSections();
    int cols = model_->columnCount();
    for (int c = 0; c < cols; ++c) {
        std::string title = model_->headerData(c, Orientation::Horizontal);
        if (title.empty()) {
            title = (c == 0) ? "Name" : ("Column " + std::to_string(c + 1));
        }
        header_->addSection(std::move(title), (c == 0) ? 220.0f : 120.0f, 60.0f);
    }
}

void TreeView::rebuildVisibleNodes() {
    visibleNodes_.clear();
    if (model_) {
        buildNodesRecursive(ModelIndex{}, 0);
    }
    clampScrollOffsets();
    markPaintDirty();
}

void TreeView::buildNodesRecursive(const ModelIndex& parent, int depth) {
    if (!model_) {
        return;
    }
    int rows = model_->rowCount(parent);
    for (int r = 0; r < rows; ++r) {
        ModelIndex idx = model_->index(r, 0, parent);
        bool hasKids = model_->rowCount(idx) > 0;
        bool isExp = isExpanded(idx);

        visibleNodes_.push_back(TreeVisibleNode{
            .index = idx,
            .depth = depth,
            .hasChildren = hasKids,
            .isExpanded = isExp,
        });

        if (isExp && hasKids) {
            buildNodesRecursive(idx, depth + 1);
        }
    }
}

void TreeView::setDelegate(std::unique_ptr<ItemDelegate> delegate) {
    if (delegate) {
        delegate_ = std::move(delegate);
        markPaintDirty();
    }
}

void TreeView::setRowHeight(float height) noexcept {
    float h = std::max(14.0f, height);
    baseRowHeight_ = h / dpiScale_;
    if (rowHeight_ != h) {
        rowHeight_ = h;
        clampScrollOffsets();
        markPaintDirty();
    }
}

void TreeView::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedDefaultFont_.reset();
        if (header_) {
            header_->setFont(font_);
        }
        markPaintDirty();
    }
}

std::shared_ptr<text::Font> TreeView::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float pt = 10.0f * dpiScale_;
        resolvedDefaultFont_ = text::FontManager::instance().defaultFont(pt).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void TreeView::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiScale(scale);
}

void TreeView::applyDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        resolvedDefaultFont_.reset();
        rowHeight_ = std::max(14.0f, baseRowHeight_ * dpiScale_);
        indentPerLevel_ = std::max(8.0f, baseIndentPerLevel_ * dpiScale_);
        scrollbarThickness_ = 6.0f * dpiScale_;
        if (header_) {
            header_->setDpiScale(dpiScale_);
        }
        clampScrollOffsets();
        markPaintDirty();
    }
}

void TreeView::onContextAttached() {
    if (!hasCustomDpiScale_) {
        applyDpiScale(contextDpiScale());
    }
}

void TreeView::onThemeChanged() {
    resolvedDefaultFont_.reset();
    if (header_) {
        header_->onThemeChanged();
    }
    markPaintDirty();
}

void TreeView::onEnabledChanged() {
    if (!isEnabled()) {
        cancelActiveInteraction();
    }
    if (header_) {
        header_->setEnabled(isEnabled());
    }
    markPaintDirty();
}

void TreeView::onFocusGained() {
    markPaintDirty();
}

void TreeView::onFocusLost() {
    cancelActiveInteraction();
    markPaintDirty();
}

void TreeView::expand(const ModelIndex& index) {
    if (index.isValid() && !isExpanded(index)) {
        expandedNodes_.insert(index);
        rebuildVisibleNodes();
        onItemExpanded.emit(index);
    }
}

void TreeView::collapse(const ModelIndex& index) {
    if (index.isValid() && isExpanded(index)) {
        expandedNodes_.erase(index);
        rebuildVisibleNodes();
        onItemCollapsed.emit(index);
    }
}

void TreeView::toggle(const ModelIndex& index) {
    if (isExpanded(index)) {
        collapse(index);
    } else {
        expand(index);
    }
}

bool TreeView::isExpanded(const ModelIndex& index) const noexcept {
    if (!index.isValid()) {
        return false;
    }
    return expandedNodes_.find(index) != expandedNodes_.end();
}

void TreeView::expandAll() {
    if (!model_) {
        return;
    }
    std::function<void(const ModelIndex&)> expandSubtree = [&](const ModelIndex& parent) {
        int count = model_->rowCount(parent);
        for (int r = 0; r < count; ++r) {
            ModelIndex idx = model_->index(r, 0, parent);
            if (model_->rowCount(idx) > 0) {
                expandedNodes_.insert(idx);
                expandSubtree(idx);
            }
        }
    };
    expandSubtree(ModelIndex{});
    rebuildVisibleNodes();
}

void TreeView::collapseAll() {
    expandedNodes_.clear();
    rebuildVisibleNodes();
}

void TreeView::scrollTo(int visibleRowIndex) {
    if (visibleRowIndex < 0 || visibleRowIndex >= static_cast<int>(visibleNodes_.size())) {
        return;
    }
    float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
    float gridH = layoutRect().size.height - headerH;
    float rowTop = visibleRowIndex * rowHeight_;
    float rowBottom = rowTop + rowHeight_;

    if (rowTop < scrollOffsetY_) {
        scrollOffsetY_ = rowTop;
    } else if (rowBottom > scrollOffsetY_ + gridH) {
        scrollOffsetY_ = rowBottom - gridH;
    }
    clampScrollOffsets();
    markPaintDirty();
}

void TreeView::scrollBy(float deltaX, float deltaY) {
    scrollOffsetX_ += deltaX;
    scrollOffsetY_ += deltaY;
    clampScrollOffsets();
    markPaintDirty();
}

float TreeView::maxScrollOffsetY() const noexcept {
    float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
    float totalH = static_cast<float>(visibleNodes_.size()) * rowHeight_;
    float gridH = layoutRect().size.height - headerH;
    return std::max(0.0f, totalH - gridH);
}

float TreeView::maxScrollOffsetX() const noexcept {
    if (!header_) {
        return 0.0f;
    }
    float totalW = header_->totalWidth();
    float viewW = layoutRect().size.width;
    return std::max(0.0f, totalW - viewW);
}

void TreeView::clampScrollOffsets() noexcept {
    scrollOffsetY_ = std::clamp(scrollOffsetY_, 0.0f, maxScrollOffsetY());
    scrollOffsetX_ = std::clamp(scrollOffsetX_, 0.0f, maxScrollOffsetX());
    if (header_) {
        header_->setHorizontalOffset(scrollOffsetX_);
    }
}

int TreeView::visibleRowAt(float localY) const noexcept {
    float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
    if (localY < headerH || rowHeight_ <= 0.0f) {
        return -1;
    }
    float contentY = localY - headerH + scrollOffsetY_;
    if (contentY < 0.0f) {
        return -1;
    }
    int r = static_cast<int>(std::floor(contentY / rowHeight_));
    if (r >= 0 && r < static_cast<int>(visibleNodes_.size())) {
        return r;
    }
    return -1;
}

ModelIndex TreeView::indexAt(const core::Point& localPos) const noexcept {
    int r = visibleRowAt(localPos.y);
    if (r < 0 || !header_) {
        return {};
    }
    float gridX = localPos.x + scrollOffsetX_;
    int c = header_->sectionAt(gridX);
    if (c < 0 || !model_ || c >= model_->columnCount()) {
        return {};
    }

    ModelIndex nodeIdx = visibleNodes_[static_cast<size_t>(r)].index;
    return model_->index(nodeIdx.row, c, model_->parent(nodeIdx));
}

core::Rect TreeView::verticalThumbRect() const noexcept {
    float maxScroll = maxScrollOffsetY();
    if (maxScroll <= 0.0f) {
        return {};
    }
    float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
    float gridH = layoutRect().size.height - headerH;
    float totalH = static_cast<float>(visibleNodes_.size()) * rowHeight_;
    if (totalH <= 0.0f) {
        return {};
    }

    float thumbH = std::max(20.0f * dpiScale_, gridH * (gridH / totalH));
    float thumbY = headerH + (scrollOffsetY_ / maxScroll) * (gridH - thumbH);
    float thumbX = layoutRect().size.width - scrollbarThickness_ - 2.0f * dpiScale_;

    return core::Rect{core::Point{thumbX, thumbY}, {scrollbarThickness_, thumbH}};
}

core::Rect TreeView::horizontalThumbRect() const noexcept {
    float maxScroll = maxScrollOffsetX();
    if (maxScroll <= 0.0f || !header_) {
        return {};
    }
    float totalW = header_->totalWidth();
    float viewW = layoutRect().size.width;
    if (totalW <= 0.0f) {
        return {};
    }

    float thumbW = std::max(20.0f * dpiScale_, viewW * (viewW / totalW));
    float thumbX = (scrollOffsetX_ / maxScroll) * (viewW - thumbW);
    float thumbY = layoutRect().size.height - scrollbarThickness_ - 2.0f * dpiScale_;

    return core::Rect{core::Point{thumbX, thumbY}, {thumbW, scrollbarThickness_}};
}

void TreeView::paint(renderer::PaintContext& ctx) {
    core::Rect sRect = screenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return;
    }

    const auto& pal = colorPalette();
    float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
    core::Rect gridRect{core::Point{sRect.origin.x, sRect.origin.y + headerH},
                        {sRect.size.width, sRect.size.height - headerH}};

    ctx.pushClip(gridRect);
    ctx.drawRect(gridRect, pal.surfaceCanvas);

    if (model_ && header_ && rowHeight_ > 0.0f && !visibleNodes_.empty()) {
        int rowCount = static_cast<int>(visibleNodes_.size());
        int colCount = model_->columnCount();

        int firstRow = std::max(0, static_cast<int>(std::floor(scrollOffsetY_ / rowHeight_)));
        int lastRow = std::min(
            rowCount - 1,
            static_cast<int>(std::ceil((scrollOffsetY_ + gridRect.size.height) / rowHeight_)));

        auto f = effectiveFont();
        for (int r = firstRow; r <= lastRow; ++r) {
            float y = gridRect.origin.y + (r * rowHeight_) - scrollOffsetY_;
            const auto& node = visibleNodes_[static_cast<size_t>(r)];
            bool isRowSel = selectionModel_.isSelected(node.index);
            bool isRowHov = (r == hoveredVisibleRow_);

            for (int c = 0; c < colCount; ++c) {
                float colX = header_->sectionPosition(static_cast<size_t>(c));
                float colW = header_->sectionWidth(static_cast<size_t>(c));

                if (colX + colW < scrollOffsetX_ || colX > scrollOffsetX_ + gridRect.size.width) {
                    continue;
                }

                float cellX = gridRect.origin.x + colX - scrollOffsetX_;
                core::Rect cellBounds{core::Point{cellX, y}, {colW, rowHeight_}};
                ModelIndex cellIdx =
                    (c == 0) ? node.index
                             : model_->index(node.index.row, c, model_->parent(node.index));

                StyleOptionViewItem opt;
                opt.bounds = cellBounds;
                opt.isSelected = isRowSel;
                opt.isHovered = isRowHov;
                opt.isAlternate = alternatingRowColors_ && (r % 2 == 1);
                opt.hasFocus = (hasKeyboardFocus() && selectionModel_.currentIndex() == cellIdx);
                opt.text = model_->data(cellIdx, ItemDataRole::Display);
                opt.font = f;
                opt.dpiScale = dpiScale_;
                opt.palette = &pal;
                opt.textColor = opt.isSelected
                                    ? pal.textPrimary
                                    : (opt.isHovered ? pal.textPrimary : pal.textSecondary);

                std::string checkVal = model_->data(cellIdx, ItemDataRole::CheckState);
                if (!checkVal.empty()) {
                    opt.showCheckbox = true;
                    opt.checkboxChecked = (checkVal == "1" || checkVal == "true");
                }
                opt.badgeText = model_->data(cellIdx, ItemDataRole::BadgeText);
                if (!opt.badgeText.empty()) {
                    opt.badgeColor = pal.surfaceOverlay;
                    opt.badgeTextColor = pal.textPrimary;
                }

                if (c == 0) {
                    for (int d = 0; d < node.depth; ++d) {
                        float lineX =
                            cellX + (static_cast<float>(d) * indentPerLevel_) + 9.0f * dpiScale_;
                        ctx.drawLine(core::Point{lineX, y}, core::Point{lineX, y + rowHeight_},
                                     pal.borderSubtle, 1.0f);
                    }
                    float indent = static_cast<float>(node.depth) * indentPerLevel_;
                    opt.paddingLeft =
                        indent + (node.hasChildren ? (20.0f * dpiScale_) : (8.0f * dpiScale_));
                }

                if (delegate_) {
                    delegate_->paint(ctx, opt, cellIdx);
                }

                if (c == 0 && node.hasChildren) {
                    float indent = static_cast<float>(node.depth) * indentPerLevel_;
                    core::Color chevCol = isRowHov ? pal.textPrimary : pal.textSecondary;
                    IconId iconId = node.isExpanded ? IconId::ChevronDown : IconId::ChevronRight;
                    if (auto doc = getSvgDocument(iconId)) {
                        float iconSize = 14.0f * dpiScale_;
                        core::Rect chevRect{core::Point{cellX + indent + 2.0f * dpiScale_,
                                                        y + (rowHeight_ - iconSize) * 0.5f},
                                            core::Size{iconSize, iconSize}};
                        ctx.drawSvg(*doc, chevRect, chevCol);
                    }
                }
            }
        }
    }

    core::Color restThumbCol = pal.mode == theme::ThemeMode::Dark
                                   ? core::Color{1.0f, 1.0f, 1.0f, 0.20f}
                                   : core::Color{0.0f, 0.0f, 0.0f, 0.20f};

    if (maxScrollOffsetY() > 0.0f) {
        core::Rect localThumb = verticalThumbRect();
        core::Rect screenThumb{
            core::Point{sRect.origin.x + localThumb.origin.x, sRect.origin.y + localThumb.origin.y},
            localThumb.size};
        core::Color thumbCol = isDraggingVThumb_
                                   ? pal.textSecondary
                                   : (isHoveringVThumb_ ? pal.textTertiary : restThumbCol);
        float rV[4] = {scrollbarThickness_ * 0.5f, scrollbarThickness_ * 0.5f,
                       scrollbarThickness_ * 0.5f, scrollbarThickness_ * 0.5f};
        ctx.drawRoundedRect(screenThumb, thumbCol, rV);
    }

    if (maxScrollOffsetX() > 0.0f) {
        core::Rect localThumb = horizontalThumbRect();
        core::Rect screenThumb{
            core::Point{sRect.origin.x + localThumb.origin.x, sRect.origin.y + localThumb.origin.y},
            localThumb.size};
        core::Color thumbCol = isDraggingHThumb_
                                   ? pal.textSecondary
                                   : (isHoveringHThumb_ ? pal.textTertiary : restThumbCol);
        float rH[4] = {scrollbarThickness_ * 0.5f, scrollbarThickness_ * 0.5f,
                       scrollbarThickness_ * 0.5f, scrollbarThickness_ * 0.5f};
        ctx.drawRoundedRect(screenThumb, thumbCol, rH);
    }

    if (showBorder_) {
        ctx.drawBorder(sRect, pal.borderSubtle, std::max(1.0f, std::round(1.0f * dpiScale_)));
    }
    ctx.popClip();

    if (hasKeyboardFocus() && isEnabled()) {
        Widget::paintFocusRing(ctx, sRect, 0.0f);
    }
}

void TreeView::cancelActiveInteraction() {
    if (context_ && context_->mouseGrabWidget() == this) {
        context_->releaseMouseGrab();
    }
    bool wasActive = isDraggingVThumb_ || isDraggingHThumb_ || isHoveringVThumb_ ||
                     isHoveringHThumb_ || hoveredVisibleRow_ >= 0 || hoveredColumn_ >= 0;
    isDraggingVThumb_ = false;
    isDraggingHThumb_ = false;
    isHoveringVThumb_ = false;
    isHoveringHThumb_ = false;
    hoveredVisibleRow_ = -1;
    hoveredColumn_ = -1;
    if (wasActive) {
        markPaintDirty();
    }
}

void TreeView::onMouseMove(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (isDraggingVThumb_) {
        float maxScroll = maxScrollOffsetY();
        float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
        float gridH = layoutRect().size.height - headerH;
        float totalH = static_cast<float>(visibleNodes_.size()) * rowHeight_;
        float thumbH = std::max(20.0f, gridH * (gridH / totalH));
        float trackH = gridH - thumbH;

        if (trackH > 0.0f) {
            float deltaY = ev.position().y - dragStartMouseY_;
            float scrollDelta = (deltaY / trackH) * maxScroll;
            scrollOffsetY_ = dragStartScrollY_ + scrollDelta;
            clampScrollOffsets();
            markPaintDirty();
        }
        ev.accept();
        return;
    }

    if (isDraggingHThumb_) {
        float maxScroll = maxScrollOffsetX();
        float viewW = layoutRect().size.width;
        float totalW = header_ ? header_->totalWidth() : 0.0f;
        float thumbW = std::max(20.0f, viewW * (viewW / totalW));
        float trackW = viewW - thumbW;

        if (trackW > 0.0f) {
            float deltaX = ev.position().x - dragStartMouseX_;
            float scrollDelta = (deltaX / trackW) * maxScroll;
            scrollOffsetX_ = dragStartScrollX_ + scrollDelta;
            clampScrollOffsets();
            markPaintDirty();
        }
        ev.accept();
        return;
    }

    int r = visibleRowAt(ev.position().y);
    if (r != hoveredVisibleRow_) {
        hoveredVisibleRow_ = r;
        markPaintDirty();
    }
}

void TreeView::onMouseDown(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.isPrimaryButton()) {
        if (maxScrollOffsetY() > 0.0f) {
            float trackX = layoutRect().size.width - scrollbarThickness_ - 6.0f;
            if (ev.position().x >= trackX) {
                core::Rect vThumb = verticalThumbRect();
                if (vThumb.contains(ev.position())) {
                    isDraggingVThumb_ = true;
                    dragStartMouseY_ = ev.position().y;
                    dragStartScrollY_ = scrollOffsetY_;
                    if (context_) {
                        context_->setMouseGrabWidget(this);
                    }
                } else {
                    float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
                    float gridH = layoutRect().size.height - headerH;
                    if (ev.position().y < vThumb.origin.y) {
                        scrollBy(0.0f, -gridH * 0.8f);
                    } else {
                        scrollBy(0.0f, gridH * 0.8f);
                    }
                }
                ev.accept();
                markPaintDirty();
                return;
            }
        }

        if (maxScrollOffsetX() > 0.0f) {
            float trackY = layoutRect().size.height - scrollbarThickness_ - 6.0f;
            if (ev.position().y >= trackY) {
                core::Rect hThumb = horizontalThumbRect();
                if (hThumb.contains(ev.position())) {
                    isDraggingHThumb_ = true;
                    dragStartMouseX_ = ev.position().x;
                    dragStartScrollX_ = scrollOffsetX_;
                    if (context_) {
                        context_->setMouseGrabWidget(this);
                    }
                } else {
                    float viewW = layoutRect().size.width;
                    if (ev.position().x < hThumb.origin.x) {
                        scrollBy(-viewW * 0.8f, 0.0f);
                    } else {
                        scrollBy(viewW * 0.8f, 0.0f);
                    }
                }
                ev.accept();
                markPaintDirty();
                return;
            }
        }

        int r = visibleRowAt(ev.position().y);
        if (r >= 0 && r < static_cast<int>(visibleNodes_.size())) {
            setFocus(FocusReason::Pointer);
            const auto& node = visibleNodes_[static_cast<size_t>(r)];

            float indent = static_cast<float>(node.depth) * indentPerLevel_;
            float gridX = ev.position().x + scrollOffsetX_;
            if (node.hasChildren && gridX >= indent && gridX <= indent + 20.0f * dpiScale_) {
                toggle(node.index);
                ev.accept();
                markPaintDirty();
                return;
            }

            selectionModel_.select(node.index, SelectionFlag::ClearAndSelect);
            selectionModel_.setCurrentIndex(node.index);
            onItemClicked.emit(node.index);
            ev.accept();
            markPaintDirty();
        }
    }
}

void TreeView::onMouseUp(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        cancelActiveInteraction();
        return;
    }
    if (ev.isPrimaryButton() && (isDraggingVThumb_ || isDraggingHThumb_)) {
        isDraggingVThumb_ = false;
        isDraggingHThumb_ = false;
        if (context_) {
            context_->releaseMouseGrab();
        }
        ev.accept();
        markPaintDirty();
    }
}

void TreeView::onMouseWheel(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (maxScrollOffsetY() > 0.0f || maxScrollOffsetX() > 0.0f) {
        scrollBy(0.0f, -ev.wheelDelta().y * 24.0f);
        ev.accept();
    }
}

void TreeView::onMouseLeave() {
    if (!isDraggingVThumb_ && !isDraggingHThumb_) {
        hoveredVisibleRow_ = -1;
        hoveredColumn_ = -1;
        markPaintDirty();
    }
}

void TreeView::onKeyDown(WidgetKeyEvent& ev) {
    if (!isEnabled() || !ev.isDown() || !model_ || visibleNodes_.empty()) {
        return;
    }

    int curRow = -1;
    if (selectionModel_.currentIndex().isValid()) {
        for (size_t i = 0; i < visibleNodes_.size(); ++i) {
            if (visibleNodes_[i].index == selectionModel_.currentIndex()) {
                curRow = static_cast<int>(i);
                break;
            }
        }
    }
    if (curRow < 0) {
        curRow = 0;
    }

    float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
    int pageItems =
        std::max(1, static_cast<int>((layoutRect().size.height - headerH) / rowHeight_));

    if (ev.key() == platform::Key::Up) {
        int next = std::max(0, curRow - 1);
        ModelIndex idx = visibleNodes_[static_cast<size_t>(next)].index;
        selectionModel_.select(idx, SelectionFlag::ClearAndSelect);
        selectionModel_.setCurrentIndex(idx);
        scrollTo(next);
        markPaintDirty();
        ev.accept();
    } else if (ev.key() == platform::Key::Down) {
        int next = std::min(static_cast<int>(visibleNodes_.size()) - 1, curRow + 1);
        ModelIndex idx = visibleNodes_[static_cast<size_t>(next)].index;
        selectionModel_.select(idx, SelectionFlag::ClearAndSelect);
        selectionModel_.setCurrentIndex(idx);
        scrollTo(next);
        markPaintDirty();
        ev.accept();
    } else if (ev.key() == platform::Key::PageUp) {
        int next = std::max(0, curRow - pageItems);
        ModelIndex idx = visibleNodes_[static_cast<size_t>(next)].index;
        selectionModel_.select(idx, SelectionFlag::ClearAndSelect);
        selectionModel_.setCurrentIndex(idx);
        scrollTo(next);
        markPaintDirty();
        ev.accept();
    } else if (ev.key() == platform::Key::PageDown) {
        int next = std::min(static_cast<int>(visibleNodes_.size()) - 1, curRow + pageItems);
        ModelIndex idx = visibleNodes_[static_cast<size_t>(next)].index;
        selectionModel_.select(idx, SelectionFlag::ClearAndSelect);
        selectionModel_.setCurrentIndex(idx);
        scrollTo(next);
        markPaintDirty();
        ev.accept();
    } else if (ev.key() == platform::Key::Home) {
        int next = 0;
        ModelIndex idx = visibleNodes_[static_cast<size_t>(next)].index;
        selectionModel_.select(idx, SelectionFlag::ClearAndSelect);
        selectionModel_.setCurrentIndex(idx);
        scrollTo(next);
        markPaintDirty();
        ev.accept();
    } else if (ev.key() == platform::Key::End) {
        int next = static_cast<int>(visibleNodes_.size()) - 1;
        ModelIndex idx = visibleNodes_[static_cast<size_t>(next)].index;
        selectionModel_.select(idx, SelectionFlag::ClearAndSelect);
        selectionModel_.setCurrentIndex(idx);
        scrollTo(next);
        markPaintDirty();
        ev.accept();
    } else if (ev.key() == platform::Key::Left) {
        const auto& node = visibleNodes_[static_cast<size_t>(curRow)];
        if (node.isExpanded) {
            collapse(node.index);
        } else {
            ModelIndex parentIdx = model_->parent(node.index);
            if (parentIdx.isValid()) {
                selectionModel_.select(parentIdx, SelectionFlag::ClearAndSelect);
                selectionModel_.setCurrentIndex(parentIdx);
                for (size_t i = 0; i < visibleNodes_.size(); ++i) {
                    if (visibleNodes_[i].index == parentIdx) {
                        scrollTo(static_cast<int>(i));
                        break;
                    }
                }
            }
        }
        markPaintDirty();
        ev.accept();
    } else if (ev.key() == platform::Key::Right) {
        const auto& node = visibleNodes_[static_cast<size_t>(curRow)];
        if (node.hasChildren) {
            if (!node.isExpanded) {
                expand(node.index);
            } else if (curRow + 1 < static_cast<int>(visibleNodes_.size())) {
                ModelIndex childIdx = visibleNodes_[static_cast<size_t>(curRow + 1)].index;
                selectionModel_.select(childIdx, SelectionFlag::ClearAndSelect);
                selectionModel_.setCurrentIndex(childIdx);
                scrollTo(curRow + 1);
            }
        }
        markPaintDirty();
        ev.accept();
    } else if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        const auto& node = visibleNodes_[static_cast<size_t>(curRow)];
        if (node.hasChildren) {
            toggle(node.index);
        }
        onItemActivated.emit(node.index);
        markPaintDirty();
        ev.accept();
    }
}

} // namespace northern::widgets
