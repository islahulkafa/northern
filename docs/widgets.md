# Widgets and UI systems reference

This reference documents Northern's retained widget catalog, keyboard focus traversal, cursor dispatch, top-level overlay layer, and animation subsystem in [src/widgets/](../src/widgets) and [src/core/animation/](../src/core/animation).

## Widget catalog

| Widget | Header | Primary features |
| :--- | :--- | :--- |
| `Container` | [container.hpp](../src/widgets/container.hpp) | Flexbox layout container with background, border, and corner radius styling |
| `Label` | [label.hpp](../src/widgets/label.hpp) | Shaped text display using `TextLayout` and optical vertical centering |
| `Button` | [button.hpp](../src/widgets/button.hpp) | `Secondary`, `Primary`, `Ghost`, and `Destructive` variants; animated hover/press transitions; `Space`/`Enter` activation |
| `TextInput` | [text_input.hpp](../src/widgets/text_input.hpp) | Single-line input with caret blinking, selection highlight, clipboard shortcuts, `IBeam` cursor, and always-on focus ring |
| `Checkbox` | [checkbox.hpp](../src/widgets/checkbox.hpp) | Animated vector checkmark stroke reveal (`checkAnim_`) and `Space`/`Enter` toggling |
| `Switch` | [switch.hpp](../src/widgets/switch.hpp) | Animated thumb slide and track color interpolation (`thumbAnim_`) |
| `RadioButton` | [radio_button.hpp](../src/widgets/radio_button.hpp) | Animated inner dot scale/fade and outer ring interpolation (`checkAnim_`) |
| `RadioGroup` | [radio_group.hpp](../src/widgets/radio_group.hpp) | Mutual exclusivity controller with `Up`/`Down`/`Left`/`Right` arrow navigation |
| `Slider` | [slider.hpp](../src/widgets/slider.hpp) | Mouse-capture dragging outside bounds, thumb hover scaling, `Left`/`Right`/`Up`/`Down`/`Home`/`End` stepping |
| `ProgressBar` | [progress_bar.hpp](../src/widgets/progress_bar.hpp) | Smooth determinate progress transitions and continuous indeterminate marquee |
| `Dropdown` | [dropdown.hpp](../src/widgets/dropdown.hpp) | Top-level overlay popup, continuous vector chevron morphing, and `Up`/`Down`/`Enter`/`Space` navigation |
| `ContextMenu` | [context_menu.hpp](../src/widgets/context_menu.hpp) | Right-release popup menu supporting action items, shortcut badges, disabled rows, separators, and keyboard navigation |
| `Tooltip` | [tooltip.hpp](../src/widgets/tooltip.hpp) | Hover-delayed passive overlay with optional keyboard shortcut badge |
| `ScrollView` | [scroll_view.hpp](../src/widgets/scroll_view.hpp) | Clipped viewport container with mouse-wheel scrolling |

## Focus traversal and focus-visible rules

[WidgetContext](../src/widgets/widget_context.hpp) tracks both the active `focusedWidget()` and the `FocusReason` (`Pointer`, `Keyboard`, or `Programmatic`) that granted focus:

- **Tab order**: Pressing `Tab` or `Shift+Tab` invokes `WidgetContext::focusNext(reverse)`, performing depth-first pre-order traversal across visible, enabled, focusable widgets and wrapping around cleanly.
- **Focus-visible rings**: Interactive controls (`Button`, `Checkbox`, `Switch`, `RadioButton`, `Slider`, `Dropdown`) paint the 2px Cobalt ring (`theme::paintFocusRing`) only when `hasKeyboardFocus()` is `true` (`FocusReason::Keyboard`). `TextInput` renders its focus ring whenever `hasFocus()` is `true`.

## Mouse cursor dispatch

Each [Widget](../src/widgets/widget.hpp) exposes `setCursorShape(platform::CursorShape)` and `cursorShape()`:

- `TextInput` defaults to `CursorShape::IBeam`.
- `Button`, `Checkbox`, `Switch`, `RadioButton`, `Slider`, and `Dropdown` default to `CursorShape::PointingHand`.
- Non-interactive containers, labels, and disabled widgets resolve to `CursorShape::Arrow`.
- `WidgetContext` emits `onCursorShapeChanged` whenever hover, mouse grab, or widget removal changes the effective cursor shape.

## Overlay layer, tooltips, and context menus

### Top-level overlays

[overlay.hpp](../src/widgets/overlay.hpp) defines `OverlayPlacement` (`Bottom`, `Top`, `Right`, `Left`), `OverlayDesc`, and the viewport boundary solver `computeOverlayPosition()`:

- Overlays submitted via `WidgetContext::showOverlay()` render above the root widget hierarchy, bypassing ancestor `pushClip()` regions.
- `autoFlip` flips placement (`Bottom` ↔ `Top`) near viewport edges and clamps horizontally within viewport bounds.
- `dismissOnOutsideClick` and `dismissOnEscape` provide built-in light-dismiss behavior.

### Delayed tooltips

Attach a tooltip and optional shortcut label to any widget with `widget->setTooltip("Apply changes", "Ctrl+Enter")`:

- After `500ms` of continuous hover (`setTooltipDelay`), `WidgetContext` positions a passive [Tooltip](../src/widgets/tooltip.hpp) overlay via `computeOverlayPosition` and fades it in over `120ms`.
- Tooltips do not enter `hitTest()`, allowing clicks to pass directly through to the hovered control.

### Context menus

Attach a right-click menu builder to any widget using `widget->setContextMenuBuilder(...)` or override `Widget::onContextMenu`:

```cpp
card->setContextMenuBuilder([font]() {
    auto menu = std::make_unique<ContextMenu>(font);
    menu->addItem("Reset Controls", []() { /* action */ }, "Ctrl+R");
    menu->addSeparator();
    menu->addItem("Export Trace", nullptr, "Ctrl+E", false);
    return menu;
});
```

`WidgetContext` triggers `onContextMenu` only when `MouseButton::Right` is both pressed and released within the target widget's screen bounds.

## Animation subsystem

[src/core/animation/](../src/core/animation) provides frame-delta motion primitives advanced by `WidgetContext::advanceAnimations(deltaSeconds)`:

- `EasingCurve` ([easing.hpp](../src/core/animation/easing.hpp)): `Linear`, `InQuad`, `OutQuad`, `InOutQuad`, `InCubic`, `OutCubic`, `InOutCubic`, `OutExpo`, `InOutExpo`, `OutElastic`, and `cubicBezier()`.
- `Spring` ([spring.hpp](../src/core/animation/spring.hpp)): Closed-form analytical damped harmonic oscillator for under-damped, critically damped, and over-damped motion.
- `PropertyAnimation<T>` ([property_animation.hpp](../src/core/animation/property_animation.hpp)): Dual-mode easing/spring interpolator for `float`, `core::Color`, and `core::Point`.
- `AnimationManager` ([animation_manager.hpp](../src/core/animation/animation_manager.hpp)): Advances active animations and prunes settled instances automatically.
