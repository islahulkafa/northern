#pragma once

#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/base/widget.hpp"
#include "widgets/views/header_view.hpp"
#include "widgets/views/item_delegate.hpp"
#include "widgets/views/item_model.hpp"
#include "widgets/views/item_selection_model.hpp"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace northern::widgets {

struct TreeVisibleNode {
    ModelIndex index{};
    int depth{0};
    bool hasChildren{false};
    bool isExpanded{false};
};

class TreeView : public Widget {
public:
    static constexpr float kDefaultRowHeight = 28.0f;
    static constexpr float kDefaultIndentPerLevel = 16.0f;

    explicit TreeView(ItemModel* model = nullptr, std::shared_ptr<text::Font> font = nullptr);
    ~TreeView() override = default;

    TreeView(TreeView&&) noexcept = default;
    TreeView& operator=(TreeView&&) noexcept = default;

    void setModel(ItemModel* model);
    [[nodiscard]] ItemModel* model() const noexcept { return model_; }

    [[nodiscard]] HeaderView* header() noexcept { return header_; }
    [[nodiscard]] const HeaderView* header() const noexcept { return header_; }

    [[nodiscard]] ItemSelectionModel& selectionModel() noexcept { return selectionModel_; }
    [[nodiscard]] const ItemSelectionModel& selectionModel() const noexcept {
        return selectionModel_;
    }

    void setDelegate(std::unique_ptr<ItemDelegate> delegate);
    [[nodiscard]] ItemDelegate* itemDelegate() const noexcept { return delegate_.get(); }

    void setRowHeight(float height) noexcept;
    [[nodiscard]] float rowHeight() const noexcept { return rowHeight_; }

    void setIndentPerLevel(float indent) noexcept { indentPerLevel_ = std::max(8.0f, indent); }
    [[nodiscard]] float indentPerLevel() const noexcept { return indentPerLevel_; }

    void setAlternatingRowColors(bool alternate) noexcept { alternatingRowColors_ = alternate; }
    [[nodiscard]] bool alternatingRowColors() const noexcept { return alternatingRowColors_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void expand(const ModelIndex& index);
    void collapse(const ModelIndex& index);
    void toggle(const ModelIndex& index);
    [[nodiscard]] bool isExpanded(const ModelIndex& index) const noexcept;
    void expandAll();
    void collapseAll();

    void scrollTo(int visibleRowIndex);
    void scrollBy(float deltaX, float deltaY);
    [[nodiscard]] float scrollOffsetX() const noexcept { return scrollOffsetX_; }
    [[nodiscard]] float scrollOffsetY() const noexcept { return scrollOffsetY_; }

    [[nodiscard]] float maxScrollOffsetY() const noexcept;
    [[nodiscard]] float maxScrollOffsetX() const noexcept;

    [[nodiscard]] int visibleRowAt(float localY) const noexcept;
    [[nodiscard]] ModelIndex indexAt(const core::Point& localPos) const noexcept;

    core::Signal<const ModelIndex&> onItemClicked;
    core::Signal<const ModelIndex&> onItemDoubleClicked;
    core::Signal<const ModelIndex&> onItemActivated;
    core::Signal<const ModelIndex&> onItemExpanded;
    core::Signal<const ModelIndex&> onItemCollapsed;

    void paint(renderer::PaintContext& ctx) override;

    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onMouseWheel(WidgetMouseEvent& ev) override;
    void onMouseLeave() override;
    void onKeyDown(WidgetKeyEvent& ev) override;

private:
    void rebuildVisibleNodes();
    void buildNodesRecursive(const ModelIndex& parent, int depth);
    void syncHeaderFromModel();
    void clampScrollOffsets() noexcept;

    [[nodiscard]] core::Rect verticalThumbRect() const noexcept;
    [[nodiscard]] core::Rect horizontalThumbRect() const noexcept;

    ItemModel* model_{nullptr};
    HeaderView* header_{nullptr};
    ItemSelectionModel selectionModel_{};
    std::unique_ptr<ItemDelegate> delegate_{nullptr};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    float dpiScale_{1.0f};

    std::vector<TreeVisibleNode> visibleNodes_{};
    std::unordered_set<ModelIndex, ModelIndexHash> expandedNodes_{};

    float rowHeight_{kDefaultRowHeight};
    float indentPerLevel_{kDefaultIndentPerLevel};
    float scrollOffsetX_{0.0f};
    float scrollOffsetY_{0.0f};
    float scrollbarThickness_{6.0f};
    bool alternatingRowColors_{false};

    int hoveredVisibleRow_{-1};
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
