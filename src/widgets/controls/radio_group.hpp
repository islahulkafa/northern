#pragma once

#include "core/signal/signal.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/controls/radio_button.hpp"

#include <vector>

namespace northern::widgets {

class RadioGroup : public Container {
public:
    explicit RadioGroup();
    ~RadioGroup() override = default;

    core::Signal<int, RadioButton*> onSelectionChanged;

    RadioButton* addRadioButton(std::unique_ptr<RadioButton> button);

    [[nodiscard]] int selectedIndex() const noexcept { return selectedIndex_; }
    [[nodiscard]] RadioButton* selectedButton() const noexcept;
    void setSelectedIndex(int index);

    void onKeyDown(WidgetKeyEvent& ev) override;

private:
    std::vector<RadioButton*> buttons_{};
    int selectedIndex_{-1};
    bool updatingSelection_{false};
};

} // namespace northern::widgets
