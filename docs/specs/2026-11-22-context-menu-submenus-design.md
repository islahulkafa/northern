# ContextMenu Cascading Submenus Design Specification

**Date**: 2026-11-22  
**Status**: Proposed  
**Author**: Antigravity  
**Target Module**: `src/widgets/containers/` & `src/widgets/base/`

---

## 1. Intent & The Why

`ContextMenu` currently supports rich item types (standard, checkbox, radio, separator, and destructive actions) within a single flat popover list. Complex desktop applications require hierarchical command organization (e.g. `View -> Zoom -> In / Out`, `Encoding -> UTF-8 / ASCII`, `Export -> PNG / SVG / PDF`). Cascading submenus allow menus to group related secondary actions without cluttering the primary menu list.

---

## 2. Goals, Non-Goals, and Anti-Goals

### Goals
- **Intuitive Desktop API**: Simple builder/factory or child-menu methods on `ContextMenu` (e.g. `addSubmenu(...)`).
- **Visual Distinction**: Continuous vector right-pointing chevron indicator (`>`) rendered on the trailing edge of submenu items, optical gutter alignment.
- **Cascading Hover Interaction**: Hovering over a submenu item for a debounce threshold (~150ms) automatically spawns the nested submenu overlay to the right (with horizontal auto-flip if near the viewport boundary). Moving hover to another sibling item closes open submenus of the parent.
- **Full Keyboard Navigation**:
  - `Right Arrow` or `Enter` on a focused submenu item opens and focuses the child submenu.
  - `Left Arrow` or `Escape` while focused inside a child submenu closes the child submenu and restores focus to the parent menu's activating item.
- **Unified Light Dismiss**: Activating any leaf action closes the entire hierarchy. Clicking outside all menu overlays dismisses the whole chain.

### Non-Goals
- Unlimited arbitrary widget tree nesting inside menus (submenus remain instances of `ContextMenu`).
- Drag-and-drop tear-off menus (floating palettes).

### Anti-Goals
- **No Immediate Hover Jitter**: Submenus must not immediately flash open and closed on fast pointer transit across rows.
- **No Stuck Overlays**: Dismissing a parent or clicking outside must never leave orphaned submenus floating on screen.

---

## 3. Approaches Considered & Trade-Offs

| Approach | Mechanics | Trade-Offs | Selected? |
| :--- | :--- | :--- | :--- |
| **Approach 1: Desktop Hover Cascading** | Submenus open on hover after ~150ms debounce or on `Right Arrow`. Parent stays open. `Left Arrow` retreats. | Standard desktop convention across Windows 11, macOS, and Linux. Requires hover timer tracking. | **Yes (User Selected)** |
| **Approach 2: Click-to-Open** | Submenu only opens when explicitly clicked. | Simpler state machine, but feels non-standard on desktop applications; slows down navigation. | No |

---

## 4. Technical Architecture & Implementation

### 4.1 Data Model Extensions (`ContextMenuItem`)
Extend `ContextMenuItem` in `src/widgets/containers/context_menu.hpp`:
```cpp
enum class MenuItemType {
    Standard,
    Checkbox,
    Radio,
    Separator,
    Submenu,
};

struct ContextMenuItem {
    MenuItemType type{MenuItemType::Standard};
    bool isSeparator{false};
    std::string label{};
    std::string shortcut{};
    bool enabled{true};
    bool checked{false};
    bool isDestructive{false};
    std::shared_ptr<renderer::SvgDocument> icon{nullptr};
    std::function<void()> onTriggered{nullptr};
    std::function<void(bool)> onToggled{nullptr};
    std::function<std::unique_ptr<ContextMenu>()> submenuBuilder{nullptr};
    // Cached TextLayouts
    mutable std::optional<text::TextLayout> cachedLabelLayout{std::nullopt};
    mutable std::optional<text::TextLayout> cachedShortcutLayout{std::nullopt};
};
```

### 4.2 API Additions
```cpp
ContextMenu* addSubmenu(std::string label,
                        std::function<std::unique_ptr<ContextMenu>()> builder,
                        std::shared_ptr<renderer::SvgDocument> icon = nullptr,
                        bool enabled = true);
ContextMenu* addSubmenu(IconId iconId, std::string label,
                        std::function<std::unique_ptr<ContextMenu>()> builder,
                        bool enabled = true);
```

### 4.3 Trailing Chevron Indicator
- When `item.type == MenuItemType::Submenu`, paint an anti-aliased continuous chevron `>` on the trailing edge (instead of shortcut text):
  ```cpp
  float chevX = animRect.origin.x + animRect.size.width - kPadX - 6.0f;
  float chevY = centerY;
  ctx.drawLine({chevX - 3.0f, chevY - 4.0f}, {chevX + 1.0f, chevY}, itemCol, 1.5f);
  ctx.drawLine({chevX + 1.0f, chevY}, {chevX - 3.0f, chevY + 4.0f}, itemCol, 1.5f);
  ```

### 4.4 Overlay Placement for Submenus (`OverlayPlacement::Right`)
Enhance `computeOverlayPosition` in `src/widgets/base/overlay.hpp`:
- Supports `OverlayPlacement::Right`:
  - `pos.x = desc.anchorRect.origin.x + desc.anchorRect.size.width + desc.gap;`
  - Horizontal auto-flip to the left if clipping viewport right edge.
  - Align Y with the anchor row origin minus menu padding so top items line up optically.
  - Clamps Y within viewport.

### 4.5 Submenu Lifecycle & Navigation
- **Opening**:
  - `ContextMenu` tracks `activeSubmenuIndex_` and `activeSubmenuWidget_` (pointer to the active child overlay).
  - Hovering a submenu item starts a timer (`submenuOpenDelay_ = 0.15f`). When fired (or when `Right Arrow` is pressed), instantiates the submenu via its builder and calls `ctx.showOverlay(std::move(childMenu), desc, this)`.
  - Child menu records `parentMenu_ = this` and `parentItemIndex_`.
- **Keyboard Traversal**:
  - `Right Arrow` on submenu item opens submenu and focuses its first valid item.
  - `Left Arrow` / `Escape` in child menu dismisses the child overlay and restores focus to `parentMenu_` at `parentItemIndex_`.
- **Dismissal**:
  - Changing hover to another item in the parent menu closes any open child submenu via `ctx.dismissOverlaysForOwner(this)`.
  - Clicking any action in child or parent calls `ctx.dismissAllOverlays()`.

---

## 5. Verification & Testing Plan
- **Unit Tests (`tests/unit/test_context_menu.cpp`)**:
  - `ContextMenuTest.SubmenuItemDefinitionAndChevron`: Verifies submenu items configure correctly with trailing chevron space.
  - `ContextMenuTest.SubmenuOpenOnHoverAndKeyboard`: Verifies child menu opens, registers in `overlayStack_`, and positions via `OverlayPlacement::Right`.
  - `ContextMenuTest.SubmenuLeftArrowAndEscapeBackToParent`: Verifies pressing Left Arrow or Escape dismisses child menu and focuses parent.
  - `ContextMenuTest.SubmenuActionDismissesAll`: Verifies activating a submenu action dismisses entire overlay chain and fires callback.
- **Interactive Verification**:
  - Update `examples/windowing/main.cpp` with a nested submenu (e.g., `View -> Zoom -> [100%, 150%, 200%]`, `Theme -> [Dark, Light]`).
