#include "widgets/controls/radio_group.hpp"

#include <algorithm>

namespace northern::widgets {

RadioGroup::RadioGroup(RadioGroupOrientation orientation) : orientation_(orientation) {
    setFocusable(false);
    setOrientation(orientation);
}

void RadioGroup::setOrientation(RadioGroupOrientation orientation) noexcept {
    orientation_ = orientation;
    if (orientation_ == RadioGroupOrientation::Horizontal) {
        style().flexDirection = layout::FlexDirection::Row;
        style().columnGap = 16.0f;
        style().rowGap = 0.0f;
    } else {
        style().flexDirection = layout::FlexDirection::Column;
        style().rowGap = 8.0f;
        style().columnGap = 0.0f;
    }
    markLayoutDirty();
}

RadioButton* RadioGroup::addRadioButton(std::unique_ptr<RadioButton> button) {
    if (!button) {
        return nullptr;
    }
    RadioButton* raw = button.get();
    buttons_.push_back(raw);
    int newIndex = static_cast<int>(buttons_.size()) - 1;

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

    bool preChecked = raw->isChecked();
    addChild(std::move(button));

    if (preChecked) {
        updatingSelection_ = true;
        for (size_t i = 0; i < buttons_.size(); ++i) {
            if (buttons_[i] != raw && buttons_[i]->isChecked()) {
                buttons_[i]->setChecked(false);
            }
        }
        selectedIndex_ = newIndex;
        updatingSelection_ = false;
    }

    return raw;
}

RadioButton* RadioGroup::addOption(std::string text, std::string value, RadioSize size) {
    auto btn = std::make_unique<RadioButton>(std::move(text), false);
    btn->setValue(std::move(value));
    btn->setSize(size);
    return addRadioButton(std::move(btn));
}

void RadioGroup::removeRadioButton(RadioButton* button) {
    auto it = std::find(buttons_.begin(), buttons_.end(), button);
    if (it == buttons_.end()) {
        return;
    }

    int removedIdx = static_cast<int>(std::distance(buttons_.begin(), it));
    buttons_.erase(it);
    removeChild(button);

    if (selectedIndex_ == removedIdx) {
        selectedIndex_ = -1;
        onSelectionChanged.emit(-1, nullptr);
    } else if (selectedIndex_ > removedIdx) {
        --selectedIndex_;
    }
}

void RadioGroup::clearRadioButtons() {
    buttons_.clear();
    bool hadSelection = selectedIndex_ != -1;
    selectedIndex_ = -1;
    clearChildren();
    if (hadSelection) {
        onSelectionChanged.emit(-1, nullptr);
    }
}

RadioButton* RadioGroup::selectedButton() const noexcept {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(buttons_.size())) {
        return buttons_[static_cast<size_t>(selectedIndex_)];
    }
    return nullptr;
}

std::string_view RadioGroup::selectedValue() const noexcept {
    if (auto* btn = selectedButton()) {
        return btn->value();
    }
    return {};
}

void RadioGroup::setSelectedIndex(int index) {
    if (index >= 0 && index < static_cast<int>(buttons_.size())) {
        RadioButton* target = buttons_[static_cast<size_t>(index)];
        if (!target->isChecked()) {
            target->setChecked(true);
        }
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

bool RadioGroup::setSelectedValue(std::string_view value) {
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (buttons_[i]->value() == value) {
            setSelectedIndex(static_cast<int>(i));
            return true;
        }
    }
    return false;
}

void RadioGroup::onKeyDown(WidgetKeyEvent& ev) {
    if (buttons_.empty()) {
        return;
    }
    int count = static_cast<int>(buttons_.size());
    int step = 0;
    if (ev.key() == platform::Key::Down || ev.key() == platform::Key::Right) {
        step = 1;
    } else if (ev.key() == platform::Key::Up || ev.key() == platform::Key::Left) {
        step = -1;
    } else {
        return;
    }

    int start = selectedIndex_;
    for (int offset = 1; offset <= count; ++offset) {
        int candidate = 0;
        if (start < 0) {
            candidate = (step > 0) ? (offset - 1) : (count - offset);
        } else {
            candidate = (start + step * offset % count + count) % count;
        }
        RadioButton* btn = buttons_[static_cast<size_t>(candidate)];
        if (btn && btn->isEnabled() && btn->isVisible()) {
            ev.accept();
            setSelectedIndex(candidate);
            btn->setFocus(FocusReason::Keyboard);
            return;
        }
    }
}

} // namespace northern::widgets
