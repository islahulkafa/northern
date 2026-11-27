#pragma once

#include "core/animation/property_animation.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget.hpp"
#include "widgets/containers/context_menu.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace northern::widgets {

class MenuBar : public Widget {
public:
    explicit MenuBar(std::shared_ptr<text::Font> font = nullptr);
    ~MenuBar() override;

    ContextMenu* addMenu(std::string title);
    ContextMenu* addMenu(std::string title, std::unique_ptr<ContextMenu> menu);

    [[nodiscard]] size_t menuCount() const noexcept { return menus_.size(); }
    [[nodiscard]] const std::string& menuTitle(size_t index) const;
    [[nodiscard]] ContextMenu* menuAt(size_t index) const;

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    [[nodiscard]] float preferredHeight() const noexcept;

    void openMenu(int index);
    void closeMenu();
    [[nodiscard]] int openMenuIndex() const noexcept { return openMenuIndex_; }
    [[nodiscard]] int highlightedIndex() const noexcept { return highlightedIndex_; }

    void registerAccelerators(WidgetContext& ctx);

    void onContextAttached() override;
    void onThemeChanged() override;
    void onEnabledChanged() override;
    void onMouseEnter() override;
    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseLeave() override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;
    void onFocusLost() override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    struct MenuEntry {
        std::string title;
        std::unique_ptr<ContextMenu> menu;
        mutable std::optional<text::TextLayout> layout;
        mutable core::Rect itemRect{};
        core::animation::PropertyAnimation<float> hoverAnim{0.0f};
    };

    void applyDpiMetrics(float scale);
    void updateDimensions() noexcept;
    void updateItemRects() const;
    void updateHoverTargets();
    [[nodiscard]] int itemIndexAtPoint(const core::Point& pos) const;

    std::vector<MenuEntry> menus_{};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};
    mutable bool isItemsLayoutDirty_{true};
    int highlightedIndex_{-1};
    int openMenuIndex_{-1};
};

} // namespace northern::widgets
