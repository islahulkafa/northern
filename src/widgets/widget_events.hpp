#pragma once

#include "core/math/geometry.hpp"
#include "platform/key_codes.hpp"
#include "platform/mouse_types.hpp"

namespace northern::widgets {

class WidgetMouseEvent {
public:
    WidgetMouseEvent(core::Point position, platform::MouseButton button,
                     platform::KeyModifier modifiers,
                     core::Point wheelDelta = core::Point{0.0f, 0.0f}) noexcept
        : position_(position), button_(button), modifiers_(modifiers), wheelDelta_(wheelDelta) {}

    [[nodiscard]] core::Point position() const noexcept { return position_; }
    [[nodiscard]] platform::MouseButton button() const noexcept { return button_; }
    [[nodiscard]] platform::KeyModifier modifiers() const noexcept { return modifiers_; }
    [[nodiscard]] core::Point wheelDelta() const noexcept { return wheelDelta_; }

    void accept() noexcept { accepted_ = true; }
    [[nodiscard]] bool isAccepted() const noexcept { return accepted_; }

private:
    core::Point position_{};
    platform::MouseButton button_{platform::MouseButton::None};
    platform::KeyModifier modifiers_{platform::KeyModifier::None};
    core::Point wheelDelta_{0.0f, 0.0f};
    bool accepted_{false};
};

class WidgetKeyEvent {
public:
    WidgetKeyEvent(platform::Key key, bool isDown, platform::KeyModifier modifiers,
                   uint32_t textCodepoint = 0) noexcept
        : key_(key), isDown_(isDown), modifiers_(modifiers), textCodepoint_(textCodepoint) {}

    [[nodiscard]] platform::Key key() const noexcept { return key_; }
    [[nodiscard]] bool isDown() const noexcept { return isDown_; }
    [[nodiscard]] platform::KeyModifier modifiers() const noexcept { return modifiers_; }
    [[nodiscard]] uint32_t textCodepoint() const noexcept { return textCodepoint_; }

    void accept() noexcept { accepted_ = true; }
    [[nodiscard]] bool isAccepted() const noexcept { return accepted_; }

private:
    platform::Key key_{platform::Key::Unknown};
    bool isDown_{false};
    platform::KeyModifier modifiers_{platform::KeyModifier::None};
    uint32_t textCodepoint_{0};
    bool accepted_{false};
};

class WidgetFocusEvent {
public:
    explicit WidgetFocusEvent(bool gained) noexcept : gained_(gained) {}

    [[nodiscard]] bool gained() const noexcept { return gained_; }
    [[nodiscard]] bool lost() const noexcept { return !gained_; }

private:
    bool gained_{false};
};

} // namespace northern::widgets
