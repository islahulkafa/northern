#pragma once

#include "core/math/geometry.hpp"

#include <algorithm>
#include <functional>
#include <memory>

namespace northern::widgets {

class Widget;

enum class OverlayPlacement {
    Bottom, // Directly below anchor rect (standard for dropdowns)
    Top,    // Above anchor rect
    Right,  // Submenu placement
    Left
};

struct OverlayDesc {
    core::Rect anchorRect{};
    OverlayPlacement preferredPlacement{OverlayPlacement::Bottom};
    bool autoFlip{true};              // Flip Top <-> Bottom if clipping viewport
    bool dismissOnOutsideClick{true}; // Light-dismiss
    bool dismissOnEscape{true};       // Dismiss when Escape is pressed
    float gap{4.0f};                  // Spacing between anchor and overlay
    std::function<void()> onDismiss{nullptr};
};

struct OverlayEntry {
    std::unique_ptr<Widget> widget{nullptr};
    OverlayDesc desc{};
    Widget* owner{nullptr}; // Non-owning pointer to trigger/spawning widget
    core::Point computedPosition{0.0f, 0.0f};
};

[[nodiscard]] inline core::Point computeOverlayPosition(const core::Size& viewportSize,
                                                        const core::Size& overlaySize,
                                                        const OverlayDesc& desc) noexcept {
    core::Point pos{desc.anchorRect.origin.x, 0.0f};

    if (desc.preferredPlacement == OverlayPlacement::Bottom) {
        float bottomY = desc.anchorRect.origin.y + desc.anchorRect.size.height + desc.gap;
        if (desc.autoFlip && (bottomY + overlaySize.height > viewportSize.height)) {
            float topY = desc.anchorRect.origin.y - overlaySize.height - desc.gap;
            if (topY >= 0.0f || (topY > (viewportSize.height - (bottomY + overlaySize.height)))) {
                pos.y = topY;
            } else {
                pos.y = bottomY;
            }
        } else {
            pos.y = bottomY;
        }
    } else if (desc.preferredPlacement == OverlayPlacement::Top) {
        float topY = desc.anchorRect.origin.y - overlaySize.height - desc.gap;
        if (desc.autoFlip && (topY < 0.0f)) {
            float bottomY = desc.anchorRect.origin.y + desc.anchorRect.size.height + desc.gap;
            if (bottomY + overlaySize.height <= viewportSize.height) {
                pos.y = bottomY;
            } else {
                pos.y = topY;
            }
        } else {
            pos.y = topY;
        }
    } else {
        pos.y = desc.anchorRect.origin.y;
    }

    // Clamp horizontal alignment within viewport
    if (pos.x + overlaySize.width > viewportSize.width) {
        pos.x = std::max(0.0f, viewportSize.width - overlaySize.width);
    }
    if (pos.x < 0.0f) {
        pos.x = 0.0f;
    }

    return pos;
}

} // namespace northern::widgets
