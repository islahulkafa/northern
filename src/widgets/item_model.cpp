#include "widgets/item_model.hpp"

#include <algorithm>

namespace northern::widgets {

// ============================================================================
// StringListModel
// ============================================================================

StringListModel::StringListModel(std::vector<std::string> strings) : strings_(std::move(strings)) {}

int StringListModel::rowCount(const ModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(strings_.size());
}

int StringListModel::columnCount(const ModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return 1;
}

std::string StringListModel::data(const ModelIndex& index, ItemDataRole role) const {
    if (!index.isValid() || index.row < 0 || index.row >= static_cast<int>(strings_.size())) {
        return {};
    }
    if (role == ItemDataRole::Display || role == ItemDataRole::Tooltip) {
        return strings_[static_cast<size_t>(index.row)];
    }
    return {};
}

bool StringListModel::setData(const ModelIndex& index, const std::string& value,
                              ItemDataRole role) {
    if (!index.isValid() || index.row < 0 || index.row >= static_cast<int>(strings_.size())) {
        return false;
    }
    if (role == ItemDataRole::Display) {
        strings_[static_cast<size_t>(index.row)] = value;
        onDataChanged.emit(index, index);
        return true;
    }
    return false;
}

void StringListModel::setStringList(std::vector<std::string> strings) {
    strings_ = std::move(strings);
    onModelReset.emit();
}

void StringListModel::append(std::string str) {
    int row = static_cast<int>(strings_.size());
    onRowsInserted.emit(ModelIndex{}, row, row);
    strings_.push_back(std::move(str));
    onModelReset.emit();
}

void StringListModel::insert(int row, std::string str) {
    int clamped = std::clamp(row, 0, static_cast<int>(strings_.size()));
    onRowsInserted.emit(ModelIndex{}, clamped, clamped);
    strings_.insert(strings_.begin() + clamped, std::move(str));
    onModelReset.emit();
}

void StringListModel::remove(int row) {
    if (row >= 0 && row < static_cast<int>(strings_.size())) {
        onRowsRemoved.emit(ModelIndex{}, row, row);
        strings_.erase(strings_.begin() + row);
        onModelReset.emit();
    }
}

void StringListModel::clear() {
    strings_.clear();
    onModelReset.emit();
}

// ============================================================================
// StandardTableModel
// ============================================================================

StandardTableModel::StandardTableModel(int rows, int columns) : columnCount_(columns) {
    if (rows > 0 && columns > 0) {
        cells_.resize(static_cast<size_t>(rows),
                      std::vector<std::string>(static_cast<size_t>(columns)));
    }
}

StandardTableModel::StandardTableModel(std::vector<std::string> headers,
                                       std::vector<std::vector<std::string>> cells)
    : horizontalHeaders_(std::move(headers)), cells_(std::move(cells)) {
    columnCount_ = static_cast<int>(horizontalHeaders_.size());
    for (const auto& row : cells_) {
        columnCount_ = std::max(columnCount_, static_cast<int>(row.size()));
    }
}

int StandardTableModel::rowCount(const ModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(cells_.size());
}

int StandardTableModel::columnCount(const ModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return std::max(columnCount_, static_cast<int>(horizontalHeaders_.size()));
}

std::string StandardTableModel::data(const ModelIndex& index, ItemDataRole role) const {
    if (!index.isValid() || index.row < 0 || index.row >= static_cast<int>(cells_.size())) {
        return {};
    }
    const auto& row = cells_[static_cast<size_t>(index.row)];
    if (index.column < 0 || index.column >= static_cast<int>(row.size())) {
        return {};
    }
    if (role == ItemDataRole::Display || role == ItemDataRole::Tooltip) {
        return row[static_cast<size_t>(index.column)];
    }
    return {};
}

bool StandardTableModel::setData(const ModelIndex& index, const std::string& value,
                                 ItemDataRole role) {
    if (!index.isValid() || index.row < 0 || index.row >= static_cast<int>(cells_.size())) {
        return false;
    }
    auto& row = cells_[static_cast<size_t>(index.row)];
    if (index.column < 0) {
        return false;
    }
    if (index.column >= static_cast<int>(row.size())) {
        row.resize(static_cast<size_t>(index.column + 1));
        columnCount_ = std::max(columnCount_, static_cast<int>(row.size()));
    }
    if (role == ItemDataRole::Display) {
        row[static_cast<size_t>(index.column)] = value;
        onDataChanged.emit(index, index);
        return true;
    }
    return false;
}

std::string StandardTableModel::headerData(int section, Orientation orientation) const {
    if (orientation == Orientation::Horizontal) {
        if (section >= 0 && section < static_cast<int>(horizontalHeaders_.size())) {
            return horizontalHeaders_[static_cast<size_t>(section)];
        }
    }
    return {};
}

void StandardTableModel::setHeaderData(int section, std::string text, Orientation orientation) {
    if (orientation == Orientation::Horizontal) {
        if (section >= static_cast<int>(horizontalHeaders_.size())) {
            horizontalHeaders_.resize(static_cast<size_t>(section + 1));
        }
        horizontalHeaders_[static_cast<size_t>(section)] = std::move(text);
        columnCount_ = std::max(columnCount_, static_cast<int>(horizontalHeaders_.size()));
        onModelReset.emit();
    }
}

void StandardTableModel::setHeaders(std::vector<std::string> headers) {
    horizontalHeaders_ = std::move(headers);
    columnCount_ = std::max(columnCount_, static_cast<int>(horizontalHeaders_.size()));
    onModelReset.emit();
}

void StandardTableModel::appendRow(std::vector<std::string> row) {
    columnCount_ = std::max(columnCount_, static_cast<int>(row.size()));
    int r = static_cast<int>(cells_.size());
    onRowsInserted.emit(ModelIndex{}, r, r);
    cells_.push_back(std::move(row));
    onModelReset.emit();
}

void StandardTableModel::insertRow(int row, std::vector<std::string> rowData) {
    int clamped = std::clamp(row, 0, static_cast<int>(cells_.size()));
    columnCount_ = std::max(columnCount_, static_cast<int>(rowData.size()));
    onRowsInserted.emit(ModelIndex{}, clamped, clamped);
    cells_.insert(cells_.begin() + clamped, std::move(rowData));
    onModelReset.emit();
}

void StandardTableModel::removeRow(int row) {
    if (row >= 0 && row < static_cast<int>(cells_.size())) {
        onRowsRemoved.emit(ModelIndex{}, row, row);
        cells_.erase(cells_.begin() + row);
        onModelReset.emit();
    }
}

void StandardTableModel::setCell(int row, int column, std::string text) {
    if (row < 0 || column < 0) {
        return;
    }
    if (row >= static_cast<int>(cells_.size())) {
        cells_.resize(static_cast<size_t>(row + 1));
    }
    auto& r = cells_[static_cast<size_t>(row)];
    if (column >= static_cast<int>(r.size())) {
        r.resize(static_cast<size_t>(column + 1));
        columnCount_ = std::max(columnCount_, static_cast<int>(r.size()));
    }
    r[static_cast<size_t>(column)] = std::move(text);
    ModelIndex idx{row, column, nullptr};
    onDataChanged.emit(idx, idx);
}

void StandardTableModel::clear() {
    cells_.clear();
    onModelReset.emit();
}

// ============================================================================
// StandardTreeItem
// ============================================================================

StandardTreeItem::StandardTreeItem(std::vector<std::string> columns, StandardTreeItem* parent)
    : columns_(std::move(columns)), parent_(parent) {}

StandardTreeItem* StandardTreeItem::appendChild(std::unique_ptr<StandardTreeItem> child) {
    if (!child) {
        return nullptr;
    }
    child->parent_ = this;
    StandardTreeItem* raw = child.get();
    children_.push_back(std::move(child));
    return raw;
}

void StandardTreeItem::removeChild(int index) {
    if (index >= 0 && index < static_cast<int>(children_.size())) {
        children_.erase(children_.begin() + index);
    }
}

void StandardTreeItem::clearChildren() {
    children_.clear();
}

StandardTreeItem* StandardTreeItem::child(int row) noexcept {
    if (row >= 0 && row < static_cast<int>(children_.size())) {
        return children_[static_cast<size_t>(row)].get();
    }
    return nullptr;
}

const StandardTreeItem* StandardTreeItem::child(int row) const noexcept {
    if (row >= 0 && row < static_cast<int>(children_.size())) {
        return children_[static_cast<size_t>(row)].get();
    }
    return nullptr;
}

int StandardTreeItem::childCount() const noexcept {
    return static_cast<int>(children_.size());
}

int StandardTreeItem::row() const noexcept {
    if (parent_) {
        for (size_t i = 0; i < parent_->children_.size(); ++i) {
            if (parent_->children_[i].get() == this) {
                return static_cast<int>(i);
            }
        }
    }
    return 0;
}

int StandardTreeItem::columnCount() const noexcept {
    return static_cast<int>(columns_.size());
}

std::string StandardTreeItem::text(int column) const {
    if (column >= 0 && column < static_cast<int>(columns_.size())) {
        return columns_[static_cast<size_t>(column)];
    }
    return {};
}

void StandardTreeItem::setText(int column, std::string text) {
    if (column < 0) {
        return;
    }
    if (column >= static_cast<int>(columns_.size())) {
        columns_.resize(static_cast<size_t>(column + 1));
    }
    columns_[static_cast<size_t>(column)] = std::move(text);
}

// ============================================================================
// StandardTreeModel
// ============================================================================

StandardTreeModel::StandardTreeModel(std::vector<std::string> headers)
    : headers_(std::move(headers)) {}

StandardTreeItem* StandardTreeModel::itemFromIndex(const ModelIndex& index) const noexcept {
    if (index.isValid() && index.internalPointer) {
        return static_cast<StandardTreeItem*>(index.internalPointer);
    }
    return const_cast<StandardTreeItem*>(&rootItem_);
}

int StandardTreeModel::rowCount(const ModelIndex& parent) const {
    StandardTreeItem* parentItem = itemFromIndex(parent);
    return parentItem ? parentItem->childCount() : 0;
}

int StandardTreeModel::columnCount(const ModelIndex&) const {
    if (!headers_.empty()) {
        return static_cast<int>(headers_.size());
    }
    return std::max(1, rootItem_.columnCount());
}

std::string StandardTreeModel::data(const ModelIndex& index, ItemDataRole role) const {
    if (!index.isValid()) {
        return {};
    }
    StandardTreeItem* item = itemFromIndex(index);
    if (!item) {
        return {};
    }
    if (role == ItemDataRole::Display || role == ItemDataRole::Tooltip) {
        return item->text(index.column);
    }
    return {};
}

bool StandardTreeModel::setData(const ModelIndex& index, const std::string& value,
                                ItemDataRole role) {
    if (!index.isValid()) {
        return false;
    }
    StandardTreeItem* item = itemFromIndex(index);
    if (!item) {
        return false;
    }
    if (role == ItemDataRole::Display) {
        item->setText(index.column, value);
        onDataChanged.emit(index, index);
        return true;
    }
    return false;
}

std::string StandardTreeModel::headerData(int section, Orientation orientation) const {
    if (orientation == Orientation::Horizontal) {
        if (section >= 0 && section < static_cast<int>(headers_.size())) {
            return headers_[static_cast<size_t>(section)];
        }
    }
    return {};
}

ModelIndex StandardTreeModel::index(int row, int column, const ModelIndex& parent) const {
    if (row < 0 || column < 0) {
        return {};
    }
    StandardTreeItem* parentItem = itemFromIndex(parent);
    if (!parentItem) {
        return {};
    }
    StandardTreeItem* childItem = parentItem->child(row);
    if (childItem) {
        return ModelIndex{row, column, childItem};
    }
    return {};
}

ModelIndex StandardTreeModel::parent(const ModelIndex& child) const {
    if (!child.isValid()) {
        return {};
    }
    StandardTreeItem* childItem = itemFromIndex(child);
    if (!childItem) {
        return {};
    }
    StandardTreeItem* parentItem = childItem->parent();
    if (!parentItem || parentItem == &rootItem_) {
        return {};
    }
    return ModelIndex{parentItem->row(), 0, parentItem};
}

void StandardTreeModel::setHeaders(std::vector<std::string> headers) {
    headers_ = std::move(headers);
    onModelReset.emit();
}

void StandardTreeModel::clear() {
    rootItem_.clearChildren();
    onModelReset.emit();
}

} // namespace northern::widgets
