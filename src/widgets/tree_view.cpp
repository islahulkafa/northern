#include "widgets/tree_view.hpp"

#include "renderer/paint_context.hpp"
#include "widgets/icons.hpp"
#include "widgets/theme.hpp"
#include "widgets/widget_context.hpp"

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
    if (rowHeight_ != h) {
        rowHeight_ = h;
        clampScrollOffsets();
        markPaintDirty();
    }
}

void TreeView::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        if (header_) {
            header_->setFont(font_);
        }
        markPaintDirty();
    }
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

    float thumbH = std::max(20.0f, gridH * (gridH / totalH));
    float thumbY = headerH + (scrollOffsetY_ / maxScroll) * (gridH - thumbH);
    float thumbX = layoutRect().size.width - scrollbarThickness_ - 2.0f;

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

    float thumbW = std::max(20.0f, viewW * (viewW / totalW));
    float thumbX = (scrollOffsetX_ / maxScroll) * (viewW - thumbW);
    float thumbY = layoutRect().size.height - scrollbarThickness_ - 2.0f;

    return core::Rect{core::Point{thumbX, thumbY}, {thumbW, scrollbarThickness_}};
}

void TreeView::paint(renderer::PaintContext& ctx) {
    core::Rect sRect = screenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return;
    }

    float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
    core::Rect gridRect{core::Point{sRect.origin.x, sRect.origin.y + headerH},
                        {sRect.size.width, sRect.size.height - headerH}};

    ctx.pushClip(gridRect);
    ctx.drawRect(gridRect, theme::ColorTokens::SurfaceCanvas);

    if (model_ && header_ && rowHeight_ > 0.0f && !visibleNodes_.empty()) {
        int rowCount = static_cast<int>(visibleNodes_.size());
        int colCount = model_->columnCount();

        int firstRow = std::max(0, static_cast<int>(std::floor(scrollOffsetY_ / rowHeight_)));
        int lastRow = std::min(
            rowCount - 1,
            static_cast<int>(std::ceil((scrollOffsetY_ + gridRect.size.height) / rowHeight_)));

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
                opt.font = font_ ? font_ : (context_ ? context_->tooltipFont() : nullptr);
                opt.textColor = opt.isSelected
                                    ? theme::ColorTokens::TextPrimary
                                    : (opt.isHovered ? theme::ColorTokens::TextPrimary
                                                     : theme::ColorTokens::TextSecondary);

                if (c == 0) {
                    float indent = static_cast<float>(node.depth) * indentPerLevel_;
                    opt.paddingLeft = indent + (node.hasChildren ? 20.0f : 8.0f);
                }

                if (delegate_) {
                    delegate_->paint(ctx, opt, cellIdx);
                }

                if (c == 0 && node.hasChildren) {
                    float indent = static_cast<float>(node.depth) * indentPerLevel_;
                    core::Color chevCol = isRowHov ? theme::ColorTokens::TextPrimary
                                                   : theme::ColorTokens::TextSecondary;
                    IconId iconId = node.isExpanded ? IconId::ChevronDown : IconId::ChevronRight;
                    if (auto doc = getSvgDocument(iconId)) {
                        float iconSize = 14.0f;
                        core::Rect chevRect{
                            core::Point{cellX + indent + 2.0f, y + (rowHeight_ - iconSize) * 0.5f},
                            core::Size{iconSize, iconSize}};
                        ctx.drawSvg(*doc, chevRect, chevCol);
                    }
                }
            }
        }
    }

    if (maxScrollOffsetY() > 0.0f) {
        core::Rect localThumb = verticalThumbRect();
        core::Rect screenThumb{
            core::Point{sRect.origin.x + localThumb.origin.x, sRect.origin.y + localThumb.origin.y},
            localThumb.size};
        core::Color thumbCol = isDraggingVThumb_
                                   ? theme::ColorTokens::TextSecondary
                                   : (isHoveringVThumb_ ? theme::ColorTokens::TextTertiary
                                                        : core::Color{1.0f, 1.0f, 1.0f, 0.18f});
        ctx.drawRoundedRect(screenThumb, thumbCol, 3.0f);
    }

    if (maxScrollOffsetX() > 0.0f) {
        core::Rect localThumb = horizontalThumbRect();
        core::Rect screenThumb{
            core::Point{sRect.origin.x + localThumb.origin.x, sRect.origin.y + localThumb.origin.y},
            localThumb.size};
        core::Color thumbCol = isDraggingHThumb_
                                   ? theme::ColorTokens::TextSecondary
                                   : (isHoveringHThumb_ ? theme::ColorTokens::TextTertiary
                                                        : core::Color{1.0f, 1.0f, 1.0f, 0.18f});
        ctx.drawRoundedRect(screenThumb, thumbCol, 3.0f);
    }

    ctx.popClip();
}

void TreeView::onMouseMove(WidgetMouseEvent& ev) {
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
    if (ev.button() == platform::MouseButton::Left) {
        core::Rect vThumb = verticalThumbRect();
        if (maxScrollOffsetY() > 0.0f && vThumb.contains(ev.position())) {
            isDraggingVThumb_ = true;
            dragStartMouseY_ = ev.position().y;
            dragStartScrollY_ = scrollOffsetY_;
            if (context_) {
                context_->setMouseGrabWidget(this);
            }
            ev.accept();
            markPaintDirty();
            return;
        }

        core::Rect hThumb = horizontalThumbRect();
        if (maxScrollOffsetX() > 0.0f && hThumb.contains(ev.position())) {
            isDraggingHThumb_ = true;
            dragStartMouseX_ = ev.position().x;
            dragStartScrollX_ = scrollOffsetX_;
            if (context_) {
                context_->setMouseGrabWidget(this);
            }
            ev.accept();
            markPaintDirty();
            return;
        }

        int r = visibleRowAt(ev.position().y);
        if (r >= 0 && r < static_cast<int>(visibleNodes_.size())) {
            setFocus(FocusReason::Pointer);
            const auto& node = visibleNodes_[static_cast<size_t>(r)];

            float indent = static_cast<float>(node.depth) * indentPerLevel_;
            float gridX = ev.position().x + scrollOffsetX_;
            if (node.hasChildren && gridX >= indent && gridX <= indent + 20.0f) {
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
    if (ev.button() == platform::MouseButton::Left && (isDraggingVThumb_ || isDraggingHThumb_)) {
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
    if (!model_ || visibleNodes_.empty()) {
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
