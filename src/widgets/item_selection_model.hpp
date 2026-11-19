#pragma once

#include "core/signal/signal.hpp"
#include "widgets/item_model.hpp"

#include <cstdint>
#include <vector>

namespace northern::widgets {

enum class SelectionMode : uint8_t { None, Single, Multi };

enum class SelectionBehavior : uint8_t { SelectRows, SelectCells };

enum class SelectionFlag : uint8_t { Clear, Select, Deselect, Toggle, ClearAndSelect };

class ItemSelectionModel {
public:
    explicit ItemSelectionModel(ItemModel* model = nullptr);
    ~ItemSelectionModel() = default;

    ItemSelectionModel(const ItemSelectionModel&) = delete;
    ItemSelectionModel& operator=(const ItemSelectionModel&) = delete;
    ItemSelectionModel(ItemSelectionModel&&) noexcept = default;
    ItemSelectionModel& operator=(ItemSelectionModel&&) noexcept = default;

    void setModel(ItemModel* model);
    [[nodiscard]] ItemModel* model() const noexcept { return model_; }

    void setSelectionMode(SelectionMode mode) noexcept { mode_ = mode; }
    [[nodiscard]] SelectionMode selectionMode() const noexcept { return mode_; }

    void setSelectionBehavior(SelectionBehavior behavior) noexcept { behavior_ = behavior; }
    [[nodiscard]] SelectionBehavior selectionBehavior() const noexcept { return behavior_; }

    [[nodiscard]] bool isSelected(const ModelIndex& index) const;
    [[nodiscard]] bool isRowSelected(int row, const ModelIndex& parent = {}) const;

    [[nodiscard]] const std::vector<ModelIndex>& selectedIndexes() const noexcept {
        return selectedIndexes_;
    }
    [[nodiscard]] std::vector<int> selectedRows() const;

    void select(const ModelIndex& index, SelectionFlag flag = SelectionFlag::ClearAndSelect);
    void selectRange(const ModelIndex& from, const ModelIndex& to,
                     SelectionFlag flag = SelectionFlag::ClearAndSelect);
    void selectAll();
    void clearSelection();

    void setCurrentIndex(const ModelIndex& index);
    [[nodiscard]] const ModelIndex& currentIndex() const noexcept { return currentIndex_; }

    core::Signal<const std::vector<ModelIndex>&, const std::vector<ModelIndex>&> onSelectionChanged;
    core::Signal<const ModelIndex&, const ModelIndex&> onCurrentChanged;

private:
    ItemModel* model_{nullptr};
    SelectionMode mode_{SelectionMode::Single};
    SelectionBehavior behavior_{SelectionBehavior::SelectRows};
    ModelIndex currentIndex_{};
    ModelIndex anchorIndex_{};
    std::vector<ModelIndex> selectedIndexes_{};
};

} // namespace northern::widgets
