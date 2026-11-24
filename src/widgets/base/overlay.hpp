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
    Left,
    Center // Centered in viewport (standard for modal dialogs)
};

struct OverlayDesc {
    core::Rect anchorRect{};
    OverlayPlacement preferredPlacement{OverlayPlacement::Bottom};
    bool autoFlip{true};              // Flip Top <-> Bottom if clipping viewport
    bool dismissOnOutsideClick{true}; // Light-dismiss
    bool dismissOnEscape{true};       // Dismiss when Escape is pressed
    bool isModal{false};              // Dim background scrim and block background clicks
    float gap{4.0f};                  // Spacing between anchor and overlay
    std::function<void()> onDismiss{nullptr};
};

struct OverlayEntry {
    std::unique_ptr<Widget> widget{nullptr};
    OverlayDesc desc{};
    Widget* owner{nullptr}; // Non-owning pointer to trigger/spawning widget
    core::Point computedPosition{0.0f, 0.0f};
    bool isOwned{true};
};

[[nodiscard]] inline core::Point computeOverlayPosition(const core::Size& viewportSize,
                                                        const core::Size& overlaySize,
                                                        const OverlayDesc& desc) noexcept {
    if (desc.preferredPlacement == OverlayPlacement::Center) {
        float cx = std::max(0.0f, (viewportSize.width - overlaySize.width) * 0.5f);
        float cy = std::max(0.0f, (viewportSize.height - overlaySize.height) * 0.5f);
        return core::Point{cx, cy};
    }

    core::Point pos{desc.anchorRect.origin.x, 0.0f};

    if (desc.preferredPlacement == OverlayPlacement::Right) {
        float rightX = desc.anchorRect.origin.x + desc.anchorRect.size.width + desc.gap;
        if (desc.autoFlip && (rightX + overlaySize.width > viewportSize.width)) {
            float leftX = desc.anchorRect.origin.x - overlaySize.width - desc.gap;
            if (leftX >= 0.0f || leftX > (viewportSize.width - (rightX + overlaySize.width))) {
                pos.x = leftX;
            } else {
                pos.x = rightX;
            }
        } else {
            pos.x = rightX;
        }

        float y = desc.anchorRect.origin.y - 4.0f;
        if (y + overlaySize.height > viewportSize.height) {
            y = std::max(0.0f, viewportSize.height - overlaySize.height - 4.0f);
        }
        if (y < 0.0f) {
            y = 0.0f;
        }
        pos.y = y;

        // Clamp horizontal alignment within viewport
        if (pos.x + overlaySize.width > viewportSize.width) {
            pos.x = std::max(0.0f, viewportSize.width - overlaySize.width);
        }
        if (pos.x < 0.0f) {
            pos.x = 0.0f;
        }
        return pos;
    }

    if (desc.preferredPlacement == OverlayPlacement::Left) {
        float leftX = desc.anchorRect.origin.x - overlaySize.width - desc.gap;
        if (desc.autoFlip && (leftX < 0.0f)) {
            float rightX = desc.anchorRect.origin.x + desc.anchorRect.size.width + desc.gap;
            if (rightX + overlaySize.width <= viewportSize.width) {
                pos.x = rightX;
            } else {
                pos.x = leftX;
            }
        } else {
            pos.x = leftX;
        }

        float y = desc.anchorRect.origin.y - 4.0f;
        if (y + overlaySize.height > viewportSize.height) {
            y = std::max(0.0f, viewportSize.height - overlaySize.height - 4.0f);
        }
        if (y < 0.0f) {
            y = 0.0f;
        }
        pos.y = y;

        // Clamp horizontal alignment within viewport
        if (pos.x + overlaySize.width > viewportSize.width) {
            pos.x = std::max(0.0f, viewportSize.width - overlaySize.width);
        }
        if (pos.x < 0.0f) {
            pos.x = 0.0f;
        }
        return pos;
    }

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
