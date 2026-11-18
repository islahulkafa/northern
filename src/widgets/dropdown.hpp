#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/theme.hpp"
#include "widgets/widget.hpp"

#include <memory>
#include <string>
#include <vector>

namespace northern::widgets {

class DropdownPopup;

class Dropdown : public Widget {
public:
    explicit Dropdown(std::vector<std::string> items = {}, int selectedIndex = -1,
                      std::shared_ptr<text::Font> font = nullptr);
    ~Dropdown() override;

    core::Signal<int, const std::string&> onSelectionChanged;

    void setItems(std::vector<std::string> items);
    void addItem(std::string item);
    void clearItems();
    [[nodiscard]] const std::vector<std::string>& items() const noexcept { return items_; }
    [[nodiscard]] size_t itemCount() const noexcept { return items_.size(); }

    void setSelectedIndex(int index);
    [[nodiscard]] int selectedIndex() const noexcept { return selectedIndex_; }
    [[nodiscard]] std::string selectedText() const;

    void setPlaceholder(std::string placeholder);
    [[nodiscard]] const std::string& placeholder() const noexcept { return placeholder_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }

    void open();
    void close();
    void toggle();
    [[nodiscard]] bool isOpen() const noexcept { return isOpen_; }

    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;
    void onFocusLost() override;

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

    // Internal hook for popup selection callback
    void selectAndClose(int index);
    void onPopupDismissed();

private:
    void updateTargetColors(bool animated = true);

    std::vector<std::string> items_{};
    int selectedIndex_{-1};
    std::string placeholder_{"Select an option..."};
    std::shared_ptr<text::Font> font_{nullptr};

    bool isOpen_{false};
    bool isHovered_{false};
    bool isPressed_{false};

    core::animation::PropertyAnimation<float> popupAnim_{0.0f};

    core::Color bgColor_{core::Color{0.122f, 0.122f, 0.137f, 1.0f}};
    core::Color hoverBgColor_{core::Color{0.153f, 0.153f, 0.165f, 1.0f}};
    core::Color idleBorderColor_{core::Color{1.0f, 1.0f, 1.0f, 0.12f}};
    core::Color hoverBorderColor_{core::Color{1.0f, 1.0f, 1.0f, 0.18f}};
    core::Color textColor_{core::Color{0.980f, 0.980f, 0.980f, 1.0f}};
    core::Color placeholderColor_{core::Color{0.443f, 0.443f, 0.478f, 1.0f}};
    core::Color chevronColor_{core::Color{0.631f, 0.631f, 0.667f, 1.0f}};

    float borderWidth_{1.0f};
    float cornerRadii_[4]{6.0f, 6.0f, 6.0f, 6.0f};
};

} // namespace northern::widgets
