#pragma once

#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/base/widget.hpp"
#include "widgets/views/item_delegate.hpp"
#include "widgets/views/item_model.hpp"
#include "widgets/views/item_selection_model.hpp"

#include <memory>
#include <string>

namespace northern::widgets {

class ListView : public Widget {
public:
    static constexpr float kDefaultRowHeight = 28.0f;

    explicit ListView(ItemModel* model = nullptr, std::shared_ptr<text::Font> font = nullptr);
    ~ListView() override = default;

    ListView(ListView&&) noexcept = default;
    ListView& operator=(ListView&&) noexcept = default;

    void setModel(ItemModel* model);
    [[nodiscard]] ItemModel* model() const noexcept { return model_; }

    [[nodiscard]] ItemSelectionModel& selectionModel() noexcept { return selectionModel_; }
    [[nodiscard]] const ItemSelectionModel& selectionModel() const noexcept {
        return selectionModel_;
    }

    void setDelegate(std::unique_ptr<ItemDelegate> delegate);
    [[nodiscard]] ItemDelegate* itemDelegate() const noexcept { return delegate_.get(); }

    void setRowHeight(float height) noexcept;
    [[nodiscard]] float rowHeight() const noexcept { return rowHeight_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void scrollTo(int row);
    void scrollBy(float deltaY);
    [[nodiscard]] float scrollOffsetY() const noexcept { return scrollOffsetY_; }
    [[nodiscard]] core::Point scrollOffset() const noexcept override {
        return core::Point{0.0f, scrollOffsetY_};
    }
    [[nodiscard]] float maxScrollOffset() const noexcept;

    [[nodiscard]] int rowAt(float localY) const noexcept;
    [[nodiscard]] core::Rect rowRect(int row) const noexcept;

    core::Signal<const ModelIndex&> onItemClicked;
    core::Signal<const ModelIndex&> onItemDoubleClicked;
    core::Signal<const ModelIndex&> onItemActivated;

    void paint(renderer::PaintContext& ctx) override;

    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onMouseWheel(WidgetMouseEvent& ev) override;
    void onMouseLeave() override;
    void onKeyDown(WidgetKeyEvent& ev) override;

private:
    [[nodiscard]] core::Rect verticalThumbRect() const noexcept;
    void clampScrollOffset() noexcept;

    ItemModel* model_{nullptr};
    ItemSelectionModel selectionModel_{};
    std::unique_ptr<ItemDelegate> delegate_{nullptr};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    float dpiScale_{1.0f};
    int anchorRow_{-1};

    float rowHeight_{kDefaultRowHeight};
    float scrollOffsetY_{0.0f};
    float scrollbarThickness_{6.0f};

    int hoveredRow_{-1};
    bool isHoveringThumb_{false};
    bool isDraggingThumb_{false};
    float dragStartMouseY_{0.0f};
    float dragStartScrollY_{0.0f};
};

} // namespace northern::widgets
