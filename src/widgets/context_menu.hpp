#pragma once

#include "core/animation/property_animation.hpp"
#include "text/font.hpp"
#include "widgets/widget.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace northern::widgets {

class WidgetContext;

struct ContextMenuItem {
    bool isSeparator{false};
    std::string label{};
    std::string shortcut{};
    bool enabled{true};
    std::function<void()> onTriggered{nullptr};
};

class ContextMenu : public Widget {
public:
    explicit ContextMenu(std::shared_ptr<text::Font> font = nullptr);

    void addItem(std::string label, std::function<void()> onTriggered, std::string shortcut = {},
                 bool enabled = true);
    void addSeparator();
    void clearItems();

    [[nodiscard]] const std::vector<ContextMenuItem>& items() const noexcept { return items_; }
    [[nodiscard]] int highlightedIndex() const noexcept { return highlightedIndex_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    [[nodiscard]] core::Size preferredSize() const;

    static ContextMenu* showAt(WidgetContext& ctx, std::unique_ptr<ContextMenu> menu,
                               const core::Point& screenPos, Widget* owner = nullptr);

    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseLeave() override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    int itemIndexAtY(float localY) const noexcept;
    void moveHighlight(int delta) noexcept;
    void activateItem(int index);

    std::vector<ContextMenuItem> items_{};
    std::shared_ptr<text::Font> font_{nullptr};
    int highlightedIndex_{-1};
    core::animation::PropertyAnimation<float> openAnim_{1.0f};
};

} // namespace northern::widgets
