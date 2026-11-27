#pragma once

#include "core/signal/signal.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/controls/radio_button.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace northern::widgets {

enum class RadioGroupOrientation : uint8_t {
    Vertical,
    Horizontal,
};

class RadioGroup : public Container {
public:
    explicit RadioGroup(RadioGroupOrientation orientation = RadioGroupOrientation::Vertical);
    ~RadioGroup() override = default;

    core::Signal<int, RadioButton*> onSelectionChanged;

    RadioButton* addRadioButton(std::unique_ptr<RadioButton> button);
    RadioButton* addOption(std::string text, std::string value = {},
                           RadioSize size = RadioSize::Standard);
    void removeRadioButton(RadioButton* button);
    void clearRadioButtons();

    void setOrientation(RadioGroupOrientation orientation) noexcept;
    [[nodiscard]] RadioGroupOrientation orientation() const noexcept { return orientation_; }

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    [[nodiscard]] std::span<RadioButton* const> buttons() const noexcept { return buttons_; }
    [[nodiscard]] int selectedIndex() const noexcept { return selectedIndex_; }
    [[nodiscard]] RadioButton* selectedButton() const noexcept;
    [[nodiscard]] std::string_view selectedValue() const noexcept;

    void setSelectedIndex(int index);
    bool setSelectedValue(std::string_view value);

    void onKeyDown(WidgetKeyEvent& ev) override;
    void onContextAttached() override;

private:
    void applyDpiScale(float scale);

    std::vector<RadioButton*> buttons_{};
    RadioGroupOrientation orientation_{RadioGroupOrientation::Vertical};
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};
    int selectedIndex_{-1};
    bool updatingSelection_{false};
};

} // namespace northern::widgets
