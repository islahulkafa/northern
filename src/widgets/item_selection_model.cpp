#include "widgets/item_selection_model.hpp"

#include <algorithm>

namespace northern::widgets {

ItemSelectionModel::ItemSelectionModel(ItemModel* model) : model_(model) {
    if (model_) {
        model_->onModelReset.connect([this]() { clearSelection(); });
    }
}

void ItemSelectionModel::setModel(ItemModel* model) {
    if (model_ != model) {
        clearSelection();
        model_ = model;
        if (model_) {
            model_->onModelReset.connect([this]() { clearSelection(); });
        }
    }
}

bool ItemSelectionModel::isSelected(const ModelIndex& index) const {
    if (!index.isValid()) {
        return false;
    }
    return std::find(selectedIndexes_.begin(), selectedIndexes_.end(), index) !=
           selectedIndexes_.end();
}

bool ItemSelectionModel::isRowSelected(int row, const ModelIndex& parent) const {
    if (row < 0) {
        return false;
    }
    for (const auto& idx : selectedIndexes_) {
        if (idx.row == row && idx.internalPointer == parent.internalPointer) {
            return true;
        }
    }
    return false;
}

std::vector<int> ItemSelectionModel::selectedRows() const {
    std::vector<int> rows;
    for (const auto& idx : selectedIndexes_) {
        if (std::find(rows.begin(), rows.end(), idx.row) == rows.end()) {
            rows.push_back(idx.row);
        }
    }
    std::sort(rows.begin(), rows.end());
    return rows;
}

void ItemSelectionModel::select(const ModelIndex& index, SelectionFlag flag) {
    if (mode_ == SelectionMode::None || !index.isValid()) {
        if (flag == SelectionFlag::Clear) {
            clearSelection();
        }
        return;
    }

    std::vector<ModelIndex> newlySelected;
    std::vector<ModelIndex> newlyDeselected;

    if (flag == SelectionFlag::Clear) {
        newlyDeselected = selectedIndexes_;
        selectedIndexes_.clear();
    } else if (flag == SelectionFlag::ClearAndSelect) {
        newlyDeselected = selectedIndexes_;
        selectedIndexes_.clear();
        selectedIndexes_.push_back(index);
        newlySelected.push_back(index);
        anchorIndex_ = index;
    } else if (flag == SelectionFlag::Select) {
        if (!isSelected(index)) {
            if (mode_ == SelectionMode::Single) {
                newlyDeselected = selectedIndexes_;
                selectedIndexes_.clear();
            }
            selectedIndexes_.push_back(index);
            newlySelected.push_back(index);
            anchorIndex_ = index;
        }
    } else if (flag == SelectionFlag::Deselect) {
        auto it = std::find(selectedIndexes_.begin(), selectedIndexes_.end(), index);
        if (it != selectedIndexes_.end()) {
            newlyDeselected.push_back(*it);
            selectedIndexes_.erase(it);
        }
    } else if (flag == SelectionFlag::Toggle) {
        if (isSelected(index)) {
            auto it = std::find(selectedIndexes_.begin(), selectedIndexes_.end(), index);
            if (it != selectedIndexes_.end()) {
                newlyDeselected.push_back(*it);
                selectedIndexes_.erase(it);
            }
        } else {
            if (mode_ == SelectionMode::Single) {
                newlyDeselected = selectedIndexes_;
                selectedIndexes_.clear();
            }
            selectedIndexes_.push_back(index);
            newlySelected.push_back(index);
            anchorIndex_ = index;
        }
    }

    if (!newlySelected.empty() || !newlyDeselected.empty()) {
        onSelectionChanged.emit(newlySelected, newlyDeselected);
    }
}

void ItemSelectionModel::selectRange(const ModelIndex& from, const ModelIndex& to,
                                     SelectionFlag flag) {
    if (mode_ != SelectionMode::Multi || !model_ || !from.isValid() || !to.isValid()) {
        select(to, flag);
        return;
    }

    int startRow = std::min(from.row, to.row);
    int endRow = std::max(from.row, to.row);
    int startCol =
        (behavior_ == SelectionBehavior::SelectRows) ? 0 : std::min(from.column, to.column);
    int endCol = (behavior_ == SelectionBehavior::SelectRows)
                     ? std::max(0, model_->columnCount() - 1)
                     : std::max(from.column, to.column);

    std::vector<ModelIndex> newlySelected;
    std::vector<ModelIndex> newlyDeselected;

    if (flag == SelectionFlag::ClearAndSelect) {
        newlyDeselected = selectedIndexes_;
        selectedIndexes_.clear();
    }

    ModelIndex parentIdx = model_->parent(from);
    for (int r = startRow; r <= endRow; ++r) {
        for (int c = startCol; c <= endCol; ++c) {
            ModelIndex idx = model_->index(r, c, parentIdx);
            if (idx.isValid() && !isSelected(idx)) {
                selectedIndexes_.push_back(idx);
                newlySelected.push_back(idx);
            }
        }
    }

    if (!newlySelected.empty() || !newlyDeselected.empty()) {
        onSelectionChanged.emit(newlySelected, newlyDeselected);
    }
}

void ItemSelectionModel::selectAll() {
    if (mode_ != SelectionMode::Multi || !model_) {
        return;
    }

    int rows = model_->rowCount();
    int cols = model_->columnCount();
    if (rows <= 0 || cols <= 0) {
        return;
    }

    std::vector<ModelIndex> newlySelected;
    for (int r = 0; r < rows; ++r) {
        int endC = (behavior_ == SelectionBehavior::SelectRows) ? 0 : cols - 1;
        for (int c = 0; c <= endC; ++c) {
            ModelIndex idx = model_->index(r, c);
            if (idx.isValid() && !isSelected(idx)) {
                selectedIndexes_.push_back(idx);
                newlySelected.push_back(idx);
            }
        }
    }

    if (!newlySelected.empty()) {
        onSelectionChanged.emit(newlySelected, {});
    }
}

void ItemSelectionModel::clearSelection() {
    if (!selectedIndexes_.empty()) {
        auto old = selectedIndexes_;
        selectedIndexes_.clear();
        onSelectionChanged.emit({}, old);
    }
}

void ItemSelectionModel::setCurrentIndex(const ModelIndex& index) {
    if (currentIndex_ != index) {
        ModelIndex old = currentIndex_;
        currentIndex_ = index;
        onCurrentChanged.emit(currentIndex_, old);
    }
}

} // namespace northern::widgets
