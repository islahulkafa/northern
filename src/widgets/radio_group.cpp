#include "widgets/radio_group.hpp"

namespace northern::widgets {

RadioGroup::RadioGroup() {
    setFocusable(false);
    style().flexDirection = layout::FlexDirection::Column;
    style().rowGap = 8.0f;
}

RadioButton* RadioGroup::addRadioButton(std::unique_ptr<RadioButton> button) {
    if (!button) {
        return nullptr;
    }
    RadioButton* raw = button.get();
    buttons_.push_back(raw);

    raw->onSelected.connect([this, raw](RadioButton*) {
        if (updatingSelection_) {
            return;
        }
        updatingSelection_ = true;
        int newIdx = -1;
        for (size_t i = 0; i < buttons_.size(); ++i) {
            if (buttons_[i] == raw) {
                newIdx = static_cast<int>(i);
            } else if (buttons_[i]->isChecked()) {
                buttons_[i]->setChecked(false);
            }
        }
        selectedIndex_ = newIdx;
        updatingSelection_ = false;
        onSelectionChanged.emit(selectedIndex_, raw);
    });

    addChild(std::move(button));
    return raw;
}

RadioButton* RadioGroup::selectedButton() const noexcept {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(buttons_.size())) {
        return buttons_[static_cast<size_t>(selectedIndex_)];
    }
    return nullptr;
}

void RadioGroup::setSelectedIndex(int index) {
    if (index >= 0 && index < static_cast<int>(buttons_.size())) {
        buttons_[static_cast<size_t>(index)]->setChecked(true);
    } else if (index == -1) {
        updatingSelection_ = true;
        for (auto* btn : buttons_) {
            btn->setChecked(false);
        }
        selectedIndex_ = -1;
        updatingSelection_ = false;
        onSelectionChanged.emit(-1, nullptr);
    }
}

void RadioGroup::onKeyDown(WidgetKeyEvent& ev) {
    if (buttons_.empty()) {
        return;
    }
    if (ev.key() == platform::Key::Down || ev.key() == platform::Key::Right) {
        ev.accept();
        int next = (selectedIndex_ + 1) % static_cast<int>(buttons_.size());
        setSelectedIndex(next);
        if (selectedButton()) {
            selectedButton()->setFocus(FocusReason::Keyboard);
        }
    } else if (ev.key() == platform::Key::Up || ev.key() == platform::Key::Left) {
        ev.accept();
        int prev = selectedIndex_ <= 0 ? static_cast<int>(buttons_.size()) - 1 : selectedIndex_ - 1;
        setSelectedIndex(prev);
        if (selectedButton()) {
            selectedButton()->setFocus(FocusReason::Keyboard);
        }
    }
}

} // namespace northern::widgets
