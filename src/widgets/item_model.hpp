#pragma once

#include "core/signal/signal.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace northern::widgets {

struct ModelIndex {
    int row{-1};
    int column{-1};
    void* internalPointer{nullptr};

    [[nodiscard]] constexpr bool isValid() const noexcept { return row >= 0 && column >= 0; }

    constexpr bool operator==(const ModelIndex& other) const noexcept {
        return row == other.row && column == other.column &&
               internalPointer == other.internalPointer;
    }

    constexpr bool operator!=(const ModelIndex& other) const noexcept { return !(*this == other); }
};

struct ModelIndexHash {
    std::size_t operator()(const ModelIndex& idx) const noexcept {
        return std::hash<int>()(idx.row) ^ (std::hash<int>()(idx.column) << 1) ^
               (std::hash<void*>()(idx.internalPointer) << 2);
    }
};

enum class ItemDataRole : uint8_t {
    Display,
    Tooltip,
    Icon,
    TextAlignment,
    CheckState,
    Foreground,
    Background,
    UserData
};

enum class Orientation : uint8_t { Horizontal, Vertical };

class ItemModel {
public:
    virtual ~ItemModel() = default;

    [[nodiscard]] virtual int rowCount(const ModelIndex& parent = {}) const = 0;
    [[nodiscard]] virtual int columnCount(const ModelIndex& parent = {}) const = 0;

    [[nodiscard]] virtual std::string data(const ModelIndex& index,
                                           ItemDataRole role = ItemDataRole::Display) const = 0;
    virtual bool setData(const ModelIndex& index, const std::string& value,
                         ItemDataRole role = ItemDataRole::Display) {
        (void)index;
        (void)value;
        (void)role;
        return false;
    }

    [[nodiscard]] virtual std::string
    headerData(int section, Orientation orientation = Orientation::Horizontal) const {
        (void)section;
        (void)orientation;
        return {};
    }

    [[nodiscard]] virtual ModelIndex index(int row, int column,
                                           const ModelIndex& parent = {}) const {
        if (row < 0 || column < 0 || row >= rowCount(parent) || column >= columnCount(parent)) {
            return {};
        }
        return ModelIndex{row, column, nullptr};
    }

    [[nodiscard]] virtual ModelIndex parent(const ModelIndex& child) const {
        (void)child;
        return {};
    }

    core::Signal<const ModelIndex&, const ModelIndex&> onDataChanged;
    core::Signal<> onModelReset;
    core::Signal<const ModelIndex&, int, int> onRowsInserted;
    core::Signal<const ModelIndex&, int, int> onRowsRemoved;
};

// ============================================================================
// StringListModel
// ============================================================================

class StringListModel : public ItemModel {
public:
    explicit StringListModel(std::vector<std::string> strings = {});

    [[nodiscard]] int rowCount(const ModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const ModelIndex& parent = {}) const override;
    [[nodiscard]] std::string data(const ModelIndex& index,
                                   ItemDataRole role = ItemDataRole::Display) const override;
    bool setData(const ModelIndex& index, const std::string& value,
                 ItemDataRole role = ItemDataRole::Display) override;

    void setStringList(std::vector<std::string> strings);
    [[nodiscard]] const std::vector<std::string>& stringList() const noexcept { return strings_; }

    void append(std::string str);
    void insert(int row, std::string str);
    void remove(int row);
    void clear();

private:
    std::vector<std::string> strings_{};
};

// ============================================================================
// StandardTableModel
// ============================================================================

class StandardTableModel : public ItemModel {
public:
    StandardTableModel(int rows = 0, int columns = 0);
    StandardTableModel(std::vector<std::string> headers,
                       std::vector<std::vector<std::string>> cells = {});

    [[nodiscard]] int rowCount(const ModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const ModelIndex& parent = {}) const override;
    [[nodiscard]] std::string data(const ModelIndex& index,
                                   ItemDataRole role = ItemDataRole::Display) const override;
    bool setData(const ModelIndex& index, const std::string& value,
                 ItemDataRole role = ItemDataRole::Display) override;

    [[nodiscard]] std::string
    headerData(int section, Orientation orientation = Orientation::Horizontal) const override;
    void setHeaderData(int section, std::string text,
                       Orientation orientation = Orientation::Horizontal);

    void setHeaders(std::vector<std::string> headers);
    void appendRow(std::vector<std::string> row);
    void insertRow(int row, std::vector<std::string> rowData);
    void removeRow(int row);
    void setCell(int row, int column, std::string text);
    void clear();

private:
    std::vector<std::string> horizontalHeaders_{};
    std::vector<std::vector<std::string>> cells_{};
    int columnCount_{0};
};

// ============================================================================
// StandardTreeItem & StandardTreeModel
// ============================================================================

class StandardTreeItem {
public:
    explicit StandardTreeItem(std::vector<std::string> columns = {},
                              StandardTreeItem* parent = nullptr);
    ~StandardTreeItem() = default;

    StandardTreeItem(const StandardTreeItem&) = delete;
    StandardTreeItem& operator=(const StandardTreeItem&) = delete;
    StandardTreeItem(StandardTreeItem&&) noexcept = default;
    StandardTreeItem& operator=(StandardTreeItem&&) noexcept = default;

    [[nodiscard]] StandardTreeItem* parent() noexcept { return parent_; }
    [[nodiscard]] const StandardTreeItem* parent() const noexcept { return parent_; }

    StandardTreeItem* appendChild(std::unique_ptr<StandardTreeItem> child);
    void removeChild(int index);
    void clearChildren();

    [[nodiscard]] StandardTreeItem* child(int row) noexcept;
    [[nodiscard]] const StandardTreeItem* child(int row) const noexcept;
    [[nodiscard]] int childCount() const noexcept;
    [[nodiscard]] int row() const noexcept;

    [[nodiscard]] int columnCount() const noexcept;
    [[nodiscard]] std::string text(int column) const;
    void setText(int column, std::string text);

private:
    std::vector<std::string> columns_{};
    StandardTreeItem* parent_{nullptr};
    std::vector<std::unique_ptr<StandardTreeItem>> children_{};
};

class StandardTreeModel : public ItemModel {
public:
    explicit StandardTreeModel(std::vector<std::string> headers = {});

    [[nodiscard]] int rowCount(const ModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const ModelIndex& parent = {}) const override;
    [[nodiscard]] std::string data(const ModelIndex& index,
                                   ItemDataRole role = ItemDataRole::Display) const override;
    bool setData(const ModelIndex& index, const std::string& value,
                 ItemDataRole role = ItemDataRole::Display) override;

    [[nodiscard]] std::string
    headerData(int section, Orientation orientation = Orientation::Horizontal) const override;

    [[nodiscard]] ModelIndex index(int row, int column,
                                   const ModelIndex& parent = {}) const override;
    [[nodiscard]] ModelIndex parent(const ModelIndex& child) const override;

    [[nodiscard]] StandardTreeItem* rootItem() noexcept { return &rootItem_; }
    [[nodiscard]] const StandardTreeItem* rootItem() const noexcept { return &rootItem_; }

    void setHeaders(std::vector<std::string> headers);
    void clear();

private:
    [[nodiscard]] StandardTreeItem* itemFromIndex(const ModelIndex& index) const noexcept;

    std::vector<std::string> headers_{};
    StandardTreeItem rootItem_{};
};

} // namespace northern::widgets
