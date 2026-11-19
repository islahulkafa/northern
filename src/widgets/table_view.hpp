#pragma once

#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/header_view.hpp"
#include "widgets/item_delegate.hpp"
#include "widgets/item_model.hpp"
#include "widgets/item_selection_model.hpp"
#include "widgets/widget.hpp"

#include <memory>
#include <string>

namespace northern::widgets {

class TableView : public Widget {
public:
    static constexpr float kDefaultRowHeight = 28.0f;

    explicit TableView(ItemModel* model = nullptr, std::shared_ptr<text::Font> font = nullptr);
    ~TableView() override = default;

    TableView(TableView&&) noexcept = default;
    TableView& operator=(TableView&&) noexcept = default;

    void setModel(ItemModel* model);
    [[nodiscard]] ItemModel* model() const noexcept { return model_; }

    [[nodiscard]] HeaderView* horizontalHeader() noexcept { return header_; }
    [[nodiscard]] const HeaderView* horizontalHeader() const noexcept { return header_; }

    [[nodiscard]] ItemSelectionModel& selectionModel() noexcept { return selectionModel_; }
    [[nodiscard]] const ItemSelectionModel& selectionModel() const noexcept {
        return selectionModel_;
    }

    void setDelegate(std::unique_ptr<ItemDelegate> delegate);
    [[nodiscard]] ItemDelegate* itemDelegate() const noexcept { return delegate_.get(); }

    void setRowHeight(float height) noexcept;
    [[nodiscard]] float rowHeight() const noexcept { return rowHeight_; }

    void setAlternatingRowColors(bool alternate) noexcept { alternatingRowColors_ = alternate; }
    [[nodiscard]] bool alternatingRowColors() const noexcept { return alternatingRowColors_; }

    void setShowGridLines(bool show) noexcept { showGridLines_ = show; }
    [[nodiscard]] bool showGridLines() const noexcept { return showGridLines_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    void sortByColumn(int column, SortOrder order = SortOrder::Ascending);
    [[nodiscard]] int sortColumn() const noexcept { return sortColumn_; }
    [[nodiscard]] SortOrder sortOrder() const noexcept { return sortOrder_; }
    [[nodiscard]] int toSourceRow(int visualRow) const noexcept;
    [[nodiscard]] int toVisualRow(int sourceRow) const noexcept;

    void scrollTo(int row, int column = 0);
    void scrollBy(float deltaX, float deltaY);
    [[nodiscard]] float scrollOffsetX() const noexcept { return scrollOffsetX_; }
    [[nodiscard]] float scrollOffsetY() const noexcept { return scrollOffsetY_; }

    [[nodiscard]] float maxScrollOffsetY() const noexcept;
    [[nodiscard]] float maxScrollOffsetX() const noexcept;

    [[nodiscard]] ModelIndex indexAt(const core::Point& localPos) const noexcept;
    [[nodiscard]] core::Rect cellRect(int row, int column) const noexcept;

    core::Signal<const ModelIndex&> onCellClicked;
    core::Signal<const ModelIndex&> onCellDoubleClicked;
    core::Signal<const ModelIndex&> onCellActivated;

    void paint(renderer::PaintContext& ctx) override;

    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onMouseWheel(WidgetMouseEvent& ev) override;
    void onMouseLeave() override;
    void onKeyDown(WidgetKeyEvent& ev) override;

private:
    [[nodiscard]] core::Rect verticalThumbRect() const noexcept;
    [[nodiscard]] core::Rect horizontalThumbRect() const noexcept;
    void clampScrollOffsets() noexcept;
    void syncHeaderFromModel();
    void updateRowMapping();

    ItemModel* model_{nullptr};
    HeaderView* header_{nullptr};
    ItemSelectionModel selectionModel_{};
    std::unique_ptr<ItemDelegate> delegate_{nullptr};
    std::shared_ptr<text::Font> font_{nullptr};

    int sortColumn_{-1};
    SortOrder sortOrder_{SortOrder::None};
    std::vector<int> rowMapping_{};

    float rowHeight_{kDefaultRowHeight};
    float scrollOffsetX_{0.0f};
    float scrollOffsetY_{0.0f};
    float scrollbarThickness_{6.0f};
    bool alternatingRowColors_{true};
    bool showGridLines_{true};

    int hoveredRow_{-1};
    int hoveredColumn_{-1};
    bool isHoveringVThumb_{false};
    bool isDraggingVThumb_{false};
    float dragStartMouseY_{0.0f};
    float dragStartScrollY_{0.0f};

    bool isHoveringHThumb_{false};
    bool isDraggingHThumb_{false};
    float dragStartMouseX_{0.0f};
    float dragStartScrollX_{0.0f};
};

} // namespace northern::widgets
