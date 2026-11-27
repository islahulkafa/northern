#pragma once

#include "core/animation/property_animation.hpp"
#include "renderer/svg/svg_document.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/widget.hpp"
#include "widgets/controls/icons.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace northern::widgets {

class WidgetContext;

enum class MenuItemType { Standard, Checkbox, Radio, Separator, Submenu };

enum class MenuIconPlacement : uint8_t { Trailing, Leading };

struct ContextMenuItem {
    MenuItemType type{MenuItemType::Standard};
    bool isSeparator{false};
    std::string label{};
    std::string subtitle{};
    std::string shortcut{};
    bool enabled{true};
    bool checked{false};
    bool isDestructive{false};
    std::shared_ptr<renderer::SvgDocument> icon{nullptr};
    std::function<void()> onTriggered{nullptr};
    std::function<void(bool)> onToggled{nullptr};
    std::function<std::unique_ptr<ContextMenu>()> submenuBuilder{nullptr};

    mutable std::optional<text::TextLayout> cachedLabelLayout{std::nullopt};
    mutable std::optional<text::TextLayout> cachedSubtitleLayout{std::nullopt};
    mutable std::optional<text::TextLayout> cachedShortcutLayout{std::nullopt};
    core::animation::PropertyAnimation<float> hoverAnim{0.0f};
};

class ContextMenu : public Widget {
public:
    explicit ContextMenu(std::shared_ptr<text::Font> font = nullptr);

    void addItem(std::string label, std::function<void()> onTriggered, std::string shortcut = {},
                 bool enabled = true);
    void addItem(std::shared_ptr<renderer::SvgDocument> icon, std::string label,
                 std::function<void()> onTriggered, std::string shortcut = {}, bool enabled = true);
    void addItem(IconId iconId, std::string label, std::function<void()> onTriggered,
                 std::string shortcut = {}, bool enabled = true);

    void addSubtitledItem(std::string label, std::string subtitle,
                          std::function<void()> onTriggered, std::string shortcut = {},
                          bool enabled = true);
    void addSubtitledItem(IconId iconId, std::string label, std::string subtitle,
                          std::function<void()> onTriggered, std::string shortcut = {},
                          bool enabled = true);

    void addCheckItem(std::string label, bool checked, std::function<void(bool)> onToggled,
                      std::string shortcut = {}, bool enabled = true);
    void addRadioItem(std::string label, bool checked, std::function<void()> onSelected,
                      std::string shortcut = {}, bool enabled = true);
    void addDestructiveItem(std::string label, std::function<void()> onTriggered,
                            std::string shortcut = {}, bool enabled = true);
    void addDestructiveItem(IconId iconId, std::string label, std::function<void()> onTriggered,
                            std::string shortcut = {}, bool enabled = true);

    void addSubmenu(std::string label, std::function<std::unique_ptr<ContextMenu>()> builder,
                    std::shared_ptr<renderer::SvgDocument> icon = nullptr, bool enabled = true);
    void addSubmenu(IconId iconId, std::string label,
                    std::function<std::unique_ptr<ContextMenu>()> builder, bool enabled = true);

    void addSeparator();
    void clearItems();

    [[nodiscard]] const std::vector<ContextMenuItem>& items() const noexcept { return items_; }
    [[nodiscard]] std::vector<ContextMenuItem>& items() noexcept { return items_; }
    [[nodiscard]] int highlightedIndex() const noexcept { return highlightedIndex_; }
    [[nodiscard]] bool hasLeadingGutter() const noexcept;

    void setIconPlacement(MenuIconPlacement placement) noexcept;
    [[nodiscard]] MenuIconPlacement iconPlacement() const noexcept { return iconPlacement_; }

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;
    [[nodiscard]] std::shared_ptr<text::Font> subtitleFont() const;

    void setOnSiblingMenuNavigate(std::function<void(int delta)> cb) {
        onSiblingMenuNavigate_ = std::move(cb);
    }

    void registerAccelerators(WidgetContext& ctx);

    [[nodiscard]] core::Size preferredSize() const;

    static ContextMenu* showAt(WidgetContext& ctx, std::unique_ptr<ContextMenu> menu,
                               const core::Point& screenPos, Widget* owner = nullptr);

    void openSubmenu(int index);
    void closeSubmenu();

    [[nodiscard]] ContextMenu* activeSubmenu() const noexcept { return activeSubmenuWidget_; }
    [[nodiscard]] ContextMenu* parentMenu() const noexcept { return parentMenu_; }
    [[nodiscard]] int parentItemIndex() const noexcept { return parentItemIndex_; }

    void onContextAttached() override;
    void onThemeChanged() override;
    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseLeave() override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

private:
    void applyDpiMetrics(float scale);
    void setHighlightedIndex(int nextHighlight, bool animated);
    [[nodiscard]] float itemHeightFor(const ContextMenuItem& item) const noexcept;
    int itemIndexAtY(float localY) const noexcept;
    void moveHighlight(int delta) noexcept;
    void activateItem(int index);

    std::vector<ContextMenuItem> items_{};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedSubtitleFont_{nullptr};
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};
    MenuIconPlacement iconPlacement_{MenuIconPlacement::Trailing};

    std::function<void(int delta)> onSiblingMenuNavigate_{nullptr};
    int highlightedIndex_{-1};
    int pressedIndex_{-1};

    ContextMenu* parentMenu_{nullptr};
    int parentItemIndex_{-1};
    int activeSubmenuIndex_{-1};
    ContextMenu* activeSubmenuWidget_{nullptr};
    int pendingSubmenuIndex_{-1};
    float submenuHoverTimer_{0.0f};
    static constexpr float kSubmenuDelay = 0.15f;
};

} // namespace northern::widgets
