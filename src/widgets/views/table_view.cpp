#include "widgets/views/table_view.hpp"

#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <numeric>

namespace northern::widgets {

TableView::TableView(ItemModel* model, std::shared_ptr<text::Font> font)
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
    header_->onSortChanged.connect(
        [this](size_t col, SortOrder order) { sortByColumn(static_cast<int>(col), order); });

    if (model_) {
        selectionModel_.setModel(model_);
        syncHeaderFromModel();
        model_->onModelReset.connect([this]() {
            syncHeaderFromModel();
            updateRowMapping();
            clampScrollOffsets();
            markPaintDirty();
        });
    }
}

void TableView::setModel(ItemModel* model) {
    if (model_ != model) {
        model_ = model;
        selectionModel_.setModel(model_);
        if (model_) {
            syncHeaderFromModel();
            model_->onModelReset.connect([this]() {
                syncHeaderFromModel();
                updateRowMapping();
                clampScrollOffsets();
                markPaintDirty();
            });
        }
        updateRowMapping();
        clampScrollOffsets();
        markPaintDirty();
    }
}

void TableView::syncHeaderFromModel() {
    if (!model_ || !header_) {
        return;
    }
    header_->clearSections();
    int cols = model_->columnCount();
    for (int c = 0; c < cols; ++c) {
        std::string title = model_->headerData(c, Orientation::Horizontal);
        if (title.empty()) {
            title = "Column " + std::to_string(c + 1);
        }
        header_->addSection(std::move(title), 120.0f, 50.0f);
    }
    if (sortColumn_ >= 0 && sortColumn_ < static_cast<int>(header_->sectionCount())) {
        header_->setSectionSortOrder(static_cast<size_t>(sortColumn_), sortOrder_);
    }
}

void TableView::sortByColumn(int column, SortOrder order) {
    sortColumn_ = column;
    sortOrder_ = order;
    if (header_ && column >= 0 && column < static_cast<int>(header_->sectionCount())) {
        if (header_->sectionSortOrder(static_cast<size_t>(column)) != order) {
            header_->setSectionSortOrder(static_cast<size_t>(column), order);
        }
    }
    updateRowMapping();
    markPaintDirty();
}

void TableView::updateRowMapping() {
    if (!model_ || sortColumn_ < 0 || sortOrder_ == SortOrder::None) {
        rowMapping_.clear();
        return;
    }

    int n = model_->rowCount();
    if (n <= 0 || sortColumn_ >= model_->columnCount()) {
        rowMapping_.clear();
        return;
    }

    rowMapping_.resize(static_cast<size_t>(n));
    std::iota(rowMapping_.begin(), rowMapping_.end(), 0);

    auto naturalLess = [](const std::string& a, const std::string& b) {
        char* endA = nullptr;
        char* endB = nullptr;
        double numA = std::strtod(a.c_str(), &endA);
        double numB = std::strtod(b.c_str(), &endB);
        if (endA != a.c_str() && endB != b.c_str()) {
            if (numA < numB) {
                return true;
            }
            if (numA > numB) {
                return false;
            }
            return std::strcmp(endA, endB) < 0;
        }

        size_t minLen = std::min(a.size(), b.size());
        for (size_t i = 0; i < minLen; ++i) {
            char ca = static_cast<char>(std::tolower(static_cast<unsigned char>(a[i])));
            char cb = static_cast<char>(std::tolower(static_cast<unsigned char>(b[i])));
            if (ca != cb) {
                return ca < cb;
            }
        }
        return a.size() < b.size();
    };

    if (sortOrder_ == SortOrder::Ascending) {
        std::stable_sort(rowMapping_.begin(), rowMapping_.end(), [&](int a, int b) {
            std::string valA = model_->data(model_->index(a, sortColumn_), ItemDataRole::Display);
            std::string valB = model_->data(model_->index(b, sortColumn_), ItemDataRole::Display);
            return naturalLess(valA, valB);
        });
    } else if (sortOrder_ == SortOrder::Descending) {
        std::stable_sort(rowMapping_.begin(), rowMapping_.end(), [&](int a, int b) {
            std::string valA = model_->data(model_->index(a, sortColumn_), ItemDataRole::Display);
            std::string valB = model_->data(model_->index(b, sortColumn_), ItemDataRole::Display);
            return naturalLess(valB, valA);
        });
    }
}

int TableView::toSourceRow(int visualRow) const noexcept {
    if (rowMapping_.empty() || visualRow < 0 || visualRow >= static_cast<int>(rowMapping_.size())) {
        return visualRow;
    }
    return rowMapping_[static_cast<size_t>(visualRow)];
}

int TableView::toVisualRow(int sourceRow) const noexcept {
    if (rowMapping_.empty()) {
        return sourceRow;
    }
    for (size_t i = 0; i < rowMapping_.size(); ++i) {
        if (rowMapping_[i] == sourceRow) {
            return static_cast<int>(i);
        }
    }
    return sourceRow;
}

void TableView::setDelegate(std::unique_ptr<ItemDelegate> delegate) {
    if (delegate) {
        delegate_ = std::move(delegate);
        markPaintDirty();
    }
}

void TableView::setRowHeight(float height) noexcept {
    float h = std::max(12.0f, height);
    if (rowHeight_ != h) {
        rowHeight_ = h;
        clampScrollOffsets();
        markPaintDirty();
    }
}

void TableView::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        if (header_) {
            header_->setFont(font_);
        }
        markPaintDirty();
    }
}

void TableView::scrollTo(int row, int column) {
    if (!model_ || row < 0 || row >= model_->rowCount()) {
        return;
    }
    int vRow = toVisualRow(row);
    float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
    float gridH = layoutRect().size.height - headerH;
    float rowTop = vRow * rowHeight_;
    float rowBottom = rowTop + rowHeight_;

    if (rowTop < scrollOffsetY_) {
        scrollOffsetY_ = rowTop;
    } else if (rowBottom > scrollOffsetY_ + gridH) {
        scrollOffsetY_ = rowBottom - gridH;
    }

    if (header_ && column >= 0 && column < static_cast<int>(header_->sectionCount())) {
        float colLeft = header_->sectionPosition(static_cast<size_t>(column));
        float colRight = colLeft + header_->sectionWidth(static_cast<size_t>(column));
        float viewW = layoutRect().size.width;

        if (colLeft < scrollOffsetX_) {
            scrollOffsetX_ = colLeft;
        } else if (colRight > scrollOffsetX_ + viewW) {
            scrollOffsetX_ = colRight - viewW;
        }
    }

    clampScrollOffsets();
    markPaintDirty();
}

void TableView::scrollBy(float deltaX, float deltaY) {
    scrollOffsetX_ += deltaX;
    scrollOffsetY_ += deltaY;
    clampScrollOffsets();
    markPaintDirty();
}

float TableView::maxScrollOffsetY() const noexcept {
    if (!model_) {
        return 0.0f;
    }
    float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
    float totalH = model_->rowCount() * rowHeight_;
    float gridH = layoutRect().size.height - headerH;
    return std::max(0.0f, totalH - gridH);
}

float TableView::maxScrollOffsetX() const noexcept {
    if (!header_) {
        return 0.0f;
    }
    float totalW = header_->totalWidth();
    float viewW = layoutRect().size.width;
    return std::max(0.0f, totalW - viewW);
}

void TableView::clampScrollOffsets() noexcept {
    scrollOffsetY_ = std::clamp(scrollOffsetY_, 0.0f, maxScrollOffsetY());
    scrollOffsetX_ = std::clamp(scrollOffsetX_, 0.0f, maxScrollOffsetX());
    if (header_) {
        header_->setHorizontalOffset(scrollOffsetX_);
    }
}

ModelIndex TableView::indexAt(const core::Point& localPos) const noexcept {
    if (!model_ || !header_) {
        return {};
    }
    float headerH = header_->layoutRect().size.height;
    if (localPos.y < headerH) {
        return {};
    }

    float gridY = localPos.y - headerH + scrollOffsetY_;
    if (gridY < 0.0f) {
        return {};
    }
    int r = static_cast<int>(std::floor(gridY / rowHeight_));
    if (r < 0 || r >= model_->rowCount()) {
        return {};
    }

    float gridX = localPos.x + scrollOffsetX_;
    int c = header_->sectionAt(gridX);
    if (c < 0 || c >= model_->columnCount()) {
        return {};
    }

    int sourceRow = toSourceRow(r);
    return model_->index(sourceRow, c);
}

core::Rect TableView::cellRect(int row, int column) const noexcept {
    if (!header_ || row < 0 || column < 0) {
        return {};
    }
    int vRow = toVisualRow(row);
    float headerH = header_->layoutRect().size.height;
    float y = headerH + (vRow * rowHeight_) - scrollOffsetY_;
    float x = header_->sectionPosition(static_cast<size_t>(column)) - scrollOffsetX_;
    float w = header_->sectionWidth(static_cast<size_t>(column));
    return core::Rect{core::Point{x, y}, {w, rowHeight_}};
}

core::Rect TableView::verticalThumbRect() const noexcept {
    float maxScroll = maxScrollOffsetY();
    if (maxScroll <= 0.0f) {
        return {};
    }
    float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
    float gridH = layoutRect().size.height - headerH;
    float totalH = model_ ? model_->rowCount() * rowHeight_ : 0.0f;
    if (totalH <= 0.0f) {
        return {};
    }

    float thumbH = std::max(20.0f, gridH * (gridH / totalH));
    float thumbY = headerH + (scrollOffsetY_ / maxScroll) * (gridH - thumbH);
    float thumbX = layoutRect().size.width - scrollbarThickness_ - 2.0f;

    return core::Rect{core::Point{thumbX, thumbY}, {scrollbarThickness_, thumbH}};
}

core::Rect TableView::horizontalThumbRect() const noexcept {
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

void TableView::paint(renderer::PaintContext& ctx) {
    core::Rect sRect = screenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return;
    }

    float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
    core::Rect gridRect{core::Point{sRect.origin.x, sRect.origin.y + headerH},
                        {sRect.size.width, sRect.size.height - headerH}};

    ctx.pushClip(gridRect);
    ctx.drawRect(gridRect, theme::ColorTokens::SurfaceCanvas);

    if (model_ && header_ && rowHeight_ > 0.0f) {
        int rowCount = model_->rowCount();
        int colCount = model_->columnCount();

        if (rowCount > 0 && colCount > 0) {
            int firstRow = std::max(0, static_cast<int>(std::floor(scrollOffsetY_ / rowHeight_)));
            int lastRow = std::min(
                rowCount - 1,
                static_cast<int>(std::ceil((scrollOffsetY_ + gridRect.size.height) / rowHeight_)));

            for (int r = firstRow; r <= lastRow; ++r) {
                int sourceRow = toSourceRow(r);
                float y = gridRect.origin.y + (r * rowHeight_) - scrollOffsetY_;
                bool isRowSel = selectionModel_.isRowSelected(sourceRow);
                bool isRowHov = (sourceRow == hoveredRow_);

                for (int c = 0; c < colCount; ++c) {
                    float colX = header_->sectionPosition(static_cast<size_t>(c));
                    float colW = header_->sectionWidth(static_cast<size_t>(c));

                    if (colX + colW < scrollOffsetX_ ||
                        colX > scrollOffsetX_ + gridRect.size.width) {
                        continue;
                    }

                    float cellX = gridRect.origin.x + colX - scrollOffsetX_;
                    core::Rect cellBounds{core::Point{cellX, y}, {colW, rowHeight_}};
                    ModelIndex idx = model_->index(sourceRow, c);

                    StyleOptionViewItem opt;
                    opt.bounds = cellBounds;
                    opt.isSelected =
                        (selectionModel_.selectionBehavior() == SelectionBehavior::SelectRows)
                            ? isRowSel
                            : selectionModel_.isSelected(idx);
                    opt.isHovered = isRowHov;
                    opt.isAlternate = alternatingRowColors_ && (r % 2 == 1);
                    opt.hasFocus = (hasKeyboardFocus() && selectionModel_.currentIndex() == idx);
                    opt.text = model_->data(idx, ItemDataRole::Display);
                    opt.font = font_ ? font_ : (context_ ? context_->tooltipFont() : nullptr);
                    opt.textColor = opt.isSelected
                                        ? theme::ColorTokens::TextPrimary
                                        : (opt.isHovered ? theme::ColorTokens::TextPrimary
                                                         : theme::ColorTokens::TextSecondary);

                    if (delegate_) {
                        delegate_->paint(ctx, opt, idx);
                    }
                }

                if (showGridLines_) {
                    ctx.drawLine(
                        core::Point{gridRect.origin.x, y + rowHeight_ - 0.5f},
                        core::Point{gridRect.origin.x + gridRect.size.width, y + rowHeight_ - 0.5f},
                        theme::ColorTokens::BorderSubtle, 1.0f);
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

void TableView::onMouseMove(WidgetMouseEvent& ev) {
    if (isDraggingVThumb_) {
        float maxScroll = maxScrollOffsetY();
        float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
        float gridH = layoutRect().size.height - headerH;
        float totalH = model_ ? model_->rowCount() * rowHeight_ : 0.0f;
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

    core::Rect vThumb = verticalThumbRect();
    bool hovV = maxScrollOffsetY() > 0.0f && vThumb.contains(ev.position());
    if (hovV != isHoveringVThumb_) {
        isHoveringVThumb_ = hovV;
        markPaintDirty();
    }

    core::Rect hThumb = horizontalThumbRect();
    bool hovH = maxScrollOffsetX() > 0.0f && hThumb.contains(ev.position());
    if (hovH != isHoveringHThumb_) {
        isHoveringHThumb_ = hovH;
        markPaintDirty();
    }

    ModelIndex idx = indexAt(ev.position());
    if (idx.row != hoveredRow_ || idx.column != hoveredColumn_) {
        hoveredRow_ = idx.row;
        hoveredColumn_ = idx.column;
        markPaintDirty();
    }
}

void TableView::onMouseDown(WidgetMouseEvent& ev) {
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

        ModelIndex idx = indexAt(ev.position());
        if (idx.isValid()) {
            setFocus(FocusReason::Pointer);

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

            selectionModel_.setCurrentIndex(idx);
            onCellClicked.emit(idx);
            ev.accept();
            markPaintDirty();
        }
    }
}

void TableView::onMouseUp(WidgetMouseEvent& ev) {
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

void TableView::onMouseWheel(WidgetMouseEvent& ev) {
    if (maxScrollOffsetY() > 0.0f || maxScrollOffsetX() > 0.0f) {
        scrollBy(0.0f, -ev.wheelDelta().y * 24.0f);
        ev.accept();
    }
}

void TableView::onMouseLeave() {
    if (!isDraggingVThumb_ && !isDraggingHThumb_) {
        hoveredRow_ = -1;
        hoveredColumn_ = -1;
        isHoveringVThumb_ = false;
        isHoveringHThumb_ = false;
        markPaintDirty();
    }
}

void TableView::onKeyDown(WidgetKeyEvent& ev) {
    if (!model_ || model_->rowCount() == 0 || model_->columnCount() == 0) {
        return;
    }

    int curSourceRow =
        selectionModel_.currentIndex().isValid() ? selectionModel_.currentIndex().row : 0;
    int curVisualRow = toVisualRow(curSourceRow);
    int curCol =
        selectionModel_.currentIndex().isValid() ? selectionModel_.currentIndex().column : 0;

    int targetVisual = curVisualRow;
    int targetCol = curCol;
    int rowCount = model_->rowCount();
    int colCount = model_->columnCount();
    float headerH = header_ ? header_->layoutRect().size.height : 0.0f;
    int pageItems =
        std::max(1, static_cast<int>((layoutRect().size.height - headerH) / rowHeight_));

    bool ctrl = (static_cast<uint8_t>(ev.modifiers()) &
                 static_cast<uint8_t>(platform::KeyModifier::Control)) != 0;

    if (ctrl && ev.key() == platform::Key::A) {
        selectionModel_.selectAll();
        markPaintDirty();
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Up) {
        targetVisual = std::max(0, curVisualRow - 1);
    } else if (ev.key() == platform::Key::Down) {
        targetVisual = std::min(rowCount - 1, curVisualRow + 1);
    } else if (ev.key() == platform::Key::Left) {
        targetCol = std::max(0, curCol - 1);
    } else if (ev.key() == platform::Key::Right) {
        targetCol = std::min(colCount - 1, curCol + 1);
    } else if (ev.key() == platform::Key::PageUp) {
        targetVisual = std::max(0, curVisualRow - pageItems);
    } else if (ev.key() == platform::Key::PageDown) {
        targetVisual = std::min(rowCount - 1, curVisualRow + pageItems);
    } else if (ev.key() == platform::Key::Home) {
        targetVisual = 0;
    } else if (ev.key() == platform::Key::End) {
        targetVisual = rowCount - 1;
    } else if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        if (selectionModel_.currentIndex().isValid()) {
            onCellActivated.emit(selectionModel_.currentIndex());
        }
        ev.accept();
        return;
    } else {
        return;
    }

    int targetRow = toSourceRow(targetVisual);

    ModelIndex newIdx = model_->index(targetRow, targetCol);
    selectionModel_.select(newIdx, SelectionFlag::ClearAndSelect);
    selectionModel_.setCurrentIndex(newIdx);
    scrollTo(targetRow, targetCol);
    markPaintDirty();
    ev.accept();
}

} // namespace northern::widgets
