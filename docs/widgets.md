# Widgets and UI systems reference

This reference documents Northern's retained widget catalog, keyboard focus traversal, cursor dispatch, top-level overlay layer, and animation subsystem in [src/widgets/](../src/widgets) and [src/core/animation/](../src/core/animation).

## Widget catalog

| Widget               | Header                                                         | Primary features                                                                                                          |
| :------------------- | :------------------------------------------------------------- | :------------------------------------------------------------------------------------------------------------------------ |
| `Container`          | [container.hpp](../src/widgets/containers/container.hpp)       | Flexbox layout container with `SurfaceRole`/`BorderRole` (`Subtle`, `Default`, `Strong`), per-corner radii, and interactive card mode |
| `Label`              | [label.hpp](../src/widgets/controls/label.hpp)                 | Shaped text display using `TextLayout`, optical vertical centering (`theme::computeCenteredTextY`), `contextDpiScale()` adoption, and theme/enabled invalidation |
| `Button`             | [button.hpp](../src/widgets/controls/button.hpp)               | `Secondary`, `Primary`, `Outline`, `Ghost`, and `Destructive` variants; `0.12s OutCubic` hover/press color fades paired with `0.97` spring press scale; mouse-grab release safety; loading spinner; `onThemeChanged`/`contextDpiScale()` sync; `Space`/`Enter` activation |
| `TextInput`          | [text_input.hpp](../src/widgets/controls/text_input.hpp)       | Single-line input with caret blinking, rounded selection highlight, leading icon, trailing clear button, synchronized border/ring fade, `IBeam` cursor, and always-on focus ring |
| `TextArea`           | [text_area.hpp](../src/widgets/controls/text_area.hpp)         | Multi-line text editor with word-wrap toggle, virtualized line rendering, scrollbars, undo/redo, clipboard, line-number gutter, and always-on focus ring |
| `Checkbox`           | [checkbox.hpp](../src/widgets/controls/checkbox.hpp)           | Animated vector checkmark/indeterminate stroke reveal (`checkAnim_`), `10.5pt * dpi` default font fallback with cached `TextLayout`, `contextDpiScale()` adoption, mouse-grab release safety, and `Space`/`Enter` toggling |
| `Switch`             | [switch.hpp](../src/widgets/controls/switch.hpp)               | Pure white thumb (Light-mode hairline border), soft capsule track, `0.16s OutCubic` slide, mouse-grab press stretch, `contextDpiScale()`, and `Widget::paintFocusRing` |
| `RadioButton`        | [radio_button.hpp](../src/widgets/controls/radio_button.hpp)   | Animated inner dot scale/fade and outer ring interpolation (`checkAnim_`), `contextDpiScale()` adoption, mouse-grab safety, and `Widget::paintFocusRing` |
| `RadioGroup`         | [radio_group.hpp](../src/widgets/controls/radio_group.hpp)     | Mutual exclusivity controller with DPI gap/child propagation, disabled guard, and `Up`/`Down`/`Left`/`Right` arrow navigation |
| `Slider`             | [slider.hpp](../src/widgets/controls/slider.hpp)               | Pure white knob with scaled hairline border, mouse-capture dragging outside bounds, `0.12s OutCubic` thumb hover scaling, `contextDpiScale()` adoption, `Widget::paintFocusRing`, and `Left`/`Right`/`Up`/`Down`/`Home`/`End`/`PageUp`/`PageDown` stepping |
| `ProgressBar`        | [progress_bar.hpp](../src/widgets/controls/progress_bar.hpp)   | Smooth `0.20s OutCubic` determinate progress and CPU-clipped indeterminate marquee, `contextDpiScale()` adoption, `ProgressRole` semantics, and disabled freeze/muting |
| `Dropdown`           | [dropdown.hpp](../src/widgets/containers/dropdown.hpp)         | Rotating chevron (`0.18s OutCubic`), top-level `10px` overlay popup with concentric `6px` `0.55`/`0.85` hover/press pills, scrollbar mouse grab, and `Up`/`Down`/`Enter`/`Space` navigation |
| `ContextMenu`        | [context_menu.hpp](../src/widgets/containers/context_menu.hpp) | Right-release `10px` popup menu supporting concentric `6px` hover pills (`0.12s OutCubic`), inset hairlines, subtitled items, submenus, shortcut badges, Cobalt check/radio groups, and trailing icons |
| `MenuBar`            | [menu_bar.hpp](../src/widgets/containers/menu_bar.hpp)         | Top-level desktop menu bar with `0.55`/`0.85` `0.12s OutCubic` pill fade, keyboard focus ring, cascading `ContextMenu` popups, keyboard accelerators, disabled state, and DPI scaling |
| `Tooltip`            | [tooltip.hpp](../src/widgets/controls/tooltip.hpp)             | Hover-delayed passive overlay with concentric shortcut keycap badge, `contextDpiScale()`, `onThemeChanged()`, and Flexbox measurement |
| `ScrollView`         | [scroll_view.hpp](../src/widgets/containers/scroll_view.hpp)   | Clipped viewport container with `SurfaceRole` theming, `ScrollbarVisibility` (`Automatic` 800ms fade, `AlwaysVisible`, `Never`), content-shrink clamping, hover expansion, thumb grab, and DPI scaling |
| `Divider`            | [divider.hpp](../src/widgets/controls/divider.hpp)             | Horizontal or vertical hairline separator (`Subtle`, `Default`, `Strong`) with optional aligned text label, `contextDpiScale()` adoption, cached `11pt * dpi` font fallback, and disabled muting |
| `SplitView`          | [split_view.hpp](../src/widgets/containers/split_view.hpp)     | `SplitStyle::Hairline` (`1px` divider + `4px` grab margin + hover grip pill) and `Classic`; `FixedFirst`/`FixedSecond`/`Proportional` sizing; `0.22s InOutCubic` collapse; `Widget::paintFocusRing`; and disabled state guards |
| `TabBar`             | [tab_view.hpp](../src/widgets/containers/tab_view.hpp)         | `Underline`, `Pill`, `EditorCard`, and `Segmented` tab styles with SVG icons, drag-to-reorder, close buttons, DPI scaling, and overflow scrolling |
| `TabView`            | [tab_view.hpp](../src/widgets/containers/tab_view.hpp)         | Complete tabbed workbench hosting a `TabBar` and an automated flex-filling page deck container                            |
| `CollapsibleSection` / `DisclosureGroup` | [accordion.hpp](../src/widgets/containers/accordion.hpp) | SwiftUI-inspired disclosure group with trailing Cobalt chevron, concentric `6px` hover/focus pill, `addRow()`, disabled state, and animated collapse |
| `Accordion`          | [accordion.hpp](../src/widgets/containers/accordion.hpp)       | Stacked section container supporting `Grouped` (Optical Zinc card), `Separated`, and `Flush` styles with visible-section dividers |
| `HeaderView`         | [header_view.hpp](../src/widgets/views/header_view.hpp)        | Multi-column table header with draggable column resizing, Cobalt divider highlight, `ResizeEW` cursor, and click-to-sort vector arrows |
| `ListView`           | [list_view.hpp](../src/widgets/views/list_view.hpp)            | Virtualized 1D list (`ListStyle::Inset`, `Sidebar`, `Plain`) rendering $O(1)$ visible slice with rounded selection pills and Dark/Light palette support |
| `TableView`          | [table_view.hpp](../src/widgets/views/table_view.hpp)          | 2D virtualized grid with integrated `HeaderView`, column resizing, optional border toggle, and Light/Dark `ColorPalette` cells |
| `TreeView`           | [tree_view.hpp](../src/widgets/views/tree_view.hpp)            | Multi-column hierarchical tree-table with visible row flattening, depth indentation, optional border toggle, and vector chevrons |
| `Icon`               | [icon.hpp](../src/widgets/controls/icon.hpp)                   | Resolution-independent 24x24 SVG vector icon widget with `IconRole` tinting, `contextDpiScale()` adoption, `SurfaceRaised` interactive pill, mouse-grab safety, and `Widget::paintFocusRing` |
| `TitleBar`           | [title_bar.hpp](../src/widgets/dialogs/title_bar.hpp)          | Frameless DWM title bar with app logo, customizable center search/action slots, and native SVG caption buttons            |
| `Dialog`             | [dialog.hpp](../src/widgets/dialogs/dialog.hpp)                | Centered modal overlay card (`SurfaceRole::Overlay`) with standard/custom button bar, `Escape`/`Enter`, and focus trap    |
| `MessageBox`         | [message_box.hpp](../src/widgets/dialogs/message_box.hpp)      | Standard `info`, `warning`, `error`, and `confirm` modal dialog factories with semantic icons for warnings/errors         |
| `FileDialog`         | [file_dialog.hpp](../src/platform/file_dialog.hpp)             | Native OS file open/save and folder picker with COM shell integration                                                     |

## Focus traversal and focus-visible rules

[WidgetContext](../src/widgets/base/widget_context.hpp) tracks both the active `focusedWidget()` and the `FocusReason` (`Pointer`, `Keyboard`, or `Programmatic`) that granted focus:

- **Tab order**: Pressing `Tab` or `Shift+Tab` invokes `WidgetContext::focusNext(reverse)`, performing depth-first pre-order traversal across visible, enabled, focusable widgets and wrapping around cleanly.
- **Focus-visible rings**: Interactive controls (`Button`, `Checkbox`, `Switch`, `RadioButton`, `Slider`, `Dropdown`, `Accordion`) paint the 2px Cobalt ring (`Widget::paintFocusRing`) only when `hasKeyboardFocus()` is `true` (`FocusReason::Keyboard`). `TextInput` and `TextArea` render their focus ring whenever `hasFocus()` is `true`.
- **Context lifecycle & DPI hooks**: Attached widgets receive `onContextAttached()` (allowing default DPI sync via `contextDpiScale()`) and `onThemeChanged()` for live palette/layout updates.

## Retained display lists and viewport culling

`WidgetContext::render()` caches paint commands in a retained `PaintContext` instance:
- **Zero-cost static frames**: When neither widgets, layouts, nor active animations are dirty, recorded commands are reused directly via `PaintContext::append()`, completely skipping recursive widget tree traversal.
- **Viewport scissor culling**: `WidgetContext::renderWidget()` checks each widget's `screenRect()` against `visibleBounds`. Subtrees situated completely outside the viewport bounds are culled from display list recording.

## Mouse cursor dispatch

Each [Widget](../src/widgets/base/widget.hpp) exposes `setCursorShape(platform::CursorShape)` and `cursorShape()`:

- `TextInput` and `TextArea` default to `CursorShape::IBeam`.
- `Button`, `Checkbox`, `Switch`, `RadioButton`, `Slider`, and `Dropdown` default to `CursorShape::PointingHand`.
- Non-interactive containers, labels, and disabled widgets resolve to `CursorShape::Arrow`.
- `WidgetContext` emits `onCursorShapeChanged` whenever hover, mouse grab, or widget removal changes the effective cursor shape.

## Overlay layer, tooltips, and context menus

### Top-level overlays

[overlay.hpp](../src/widgets/base/overlay.hpp) defines `OverlayPlacement` (`Bottom`, `Top`, `Right`, `Left`), `OverlayDesc`, and the viewport boundary solver `computeOverlayPosition()`:

- Overlays submitted via `WidgetContext::showOverlay()` render above the root widget hierarchy, bypassing ancestor `pushClip()` regions.
- `autoFlip` flips placement (`Bottom` ↔ `Top`) near viewport edges and clamps horizontally within viewport bounds.
- `dismissOnOutsideClick` and `dismissOnEscape` provide built-in light-dismiss behavior.

### Delayed tooltips

Attach a tooltip and optional shortcut label to any widget with `widget->setTooltip("Apply changes", "Ctrl+Enter")`:

- After `500ms` of continuous hover (`setTooltipDelay`), `WidgetContext` positions a passive [Tooltip](../src/widgets/controls/tooltip.hpp) overlay via `computeOverlayPosition` and fades it in over `120ms`.
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

## Navigation and multi-panel containers

### SplitView

[SplitView](../src/widgets/containers/split_view.hpp) divides space horizontally or vertically with a live draggable sash divider:

```cpp
auto splitView = std::make_unique<SplitView>(SplitOrientation::Horizontal);
splitView->setDpiScale(dpi);
splitView->setSizeMode(SplitSizeMode::FixedFirst);
splitView->setFirstPaneSize(220.0f * dpi);
splitView->setMinFirstPaneSize(150.0f * dpi);
splitView->setPanes(std::move(sidebar), std::move(contentDeck));
splitView->onSplitChanged.connect([](float ratio) {
    // Proportional ratio update [0.01f, 0.99f]
});
```

- **Hairline & Classic styles**: Defaults to `SplitStyle::Hairline` (`1px * dpiScale_` visual divider with an invisible `4px * dpiScale_` `grabMargin_` and hover/drag-revealed pill grip); `SplitStyle::Classic` renders a persistent `6px` sash bar.
- **Collapse transitions**: `setCollapsed()` and `toggleCollapse(true)` animate smoothly over `0.22s InOutCubic`.
- **Interactive sash**: Hovering dispatches `CursorShape::ResizeEW` / `ResizeNS` and acquires mouse grab (`WidgetContext::setMouseGrabWidget`).

### TabBar and TabView

[TabView](../src/widgets/containers/tab_view.hpp) manages a tabbed workbench with page swapping:

```cpp
auto tabView = std::make_unique<TabView>();
tabView->setDpiScale(dpi);
tabView->setTabBarStyle(TabBarStyle::Underline);
tabView->addTab("Table View", std::move(tableView));
tabView->addTab("Console", std::move(consolePage), true); // closable
tabView->onTabChanged.connect([](int index) { /* Active tab switched */ });
tabView->onTabClosed.connect([](int index) { /* Tab removed */ });
```

- **Styles & DPI**: Supports `TabBarStyle::Underline`, `TabBarStyle::Pill`, `TabBarStyle::EditorCard`, and `TabBarStyle::Segmented`, with automatic `contextDpiScale()` adoption, explicit `setDpiScale()`/`setFont()` overrides, `onThemeChanged()` layout invalidation, and `effectiveFont()` falling back to `FontManager::instance().defaultFont()`.
- **Keyboard cycling, selection & reordering**: `Left`/`Right`/`Home`/`End` and `Ctrl+Tab`/`Ctrl+Shift+Tab` cycle enabled tabs with wrap-around, `Ctrl+W`/`Delete` request close, removing or disabling the active tab automatically falls back to the nearest enabled tab, and `setReorderable(true)` enables drag-to-reorder.
- **Icons & close buttons**: Supports optional `IconId` or `SvgDocument` leading icons on both `addTab()` and `insertTab()`, modified dot indicators, per-tab `0.12s OutCubic` hover pills, and vector close buttons.

### MenuBar

[MenuBar](../src/widgets/containers/menu_bar.hpp) provides a top-level desktop menu bar with cascading `ContextMenu` popups and global keyboard accelerators:

```cpp
auto menuBar = std::make_unique<MenuBar>();
menuBar->setDpiScale(dpi);
auto* fileMenu = menuBar->addMenu("File");
fileMenu->addItem("Save", []() { /* save */ }, "Ctrl+S");
menuBar->registerAccelerators(uiContext);
```

### CollapsibleSection (DisclosureGroup) and Accordion

[Accordion](../src/widgets/containers/accordion.hpp) and `CollapsibleSection` (aliased as `DisclosureGroup`) manage grouped disclosure sections with animated height collapse:

```cpp
auto accordion = std::make_unique<Accordion>(false, SectionHeaderSize::Standard, AccordionStyle::Grouped);
accordion->setDpiScale(dpi);
auto* section = accordion->addSection("Graphics & Rendering", nullptr, true);
section->addRow("Backend", std::make_unique<Label>("Vulkan 1.3+"));
```

- **Grouping styles**: `AccordionStyle::Grouped` (default) renders a unified Optical Zinc `SurfaceDefault` card with `10px` radius and `16px` inset hairline dividers between visible sections; `AccordionStyle::Separated` renders individual cards; `AccordionStyle::Flush` renders edge-to-edge panels with full-width `1px` hairline dividers.
- **Trailing Cobalt chevron & concentric header pill**: Defaults to `IndicatorPlacement::Trailing`, rotating `>` to `v` across `0.25s InOutCubic` in Cobalt `accentDefault` when expanded or hovered (`textDisabled` when disabled), with a concentric `4px`-inset (`6px` radius) hover pill (`0.12s OutCubic`) and matching keyboard focus ring.
- **Indented rows & nested groups**: `addRow()` creates `36px` vertically centered rows (`11pt` label, inset row dividers) and supports nested `DisclosureGroup` sections inside `body()`.

### Divider

[Divider](../src/widgets/controls/divider.hpp) renders crisp 1px hairline optical boundaries:

```cpp
auto horizontalDiv = std::make_unique<Divider>("PROPERTIES", font);
auto verticalDiv = std::make_unique<Divider>(DividerOrientation::Vertical);
```

- **Labeled lines & DPI**: Centered/start/end-aligned text label with optical vertical centering (`theme::computeCenteredTextY`), `11pt * dpiScale_` cached font fallback via `effectiveFont()`, configurable gap clearance, automatic `contextDpiScale()` adoption (preserving explicit thickness/gap/offset overrides), and muted disabled styling.

## Virtualized data and collection views

Northern provides high-performance collection views in [src/widgets/](../src/widgets) decoupled through a Model-View-Delegate architecture. Large datasets (10,000+ items) render at 60+ FPS using $O(1)$ visible viewport slicing with zero per-frame heap allocations.

### Role-based data models

[ItemModel](../src/widgets/views/item_model.hpp) defines an abstract interface returning typed cell data across [ItemDataRole](../src/widgets/views/item_model.hpp):

```cpp
enum class ItemDataRole : uint8_t {
    Display,        // Primary cell text (std::string)
    Tooltip,        // Hover tooltip string
    Icon,           // Leading SVG icon (IconId)
    TextAlignment,  // Text alignment (Left, Center, Right)
    CheckState,     // Checkbox boolean state
    Foreground,     // Custom text color (core::Color)
    Background,     // Custom cell background color (core::Color)
    BadgeText,      // Inline pill badge text (std::string)
    BadgeColor      // Custom badge background color (core::Color)
};
```

Standard model implementations include:
- `StringListModel`: 1D string list for `ListView` and simple lists.
- `StandardTableModel`: 2D tabular model with configurable column header labels.
- `StandardTreeModel`: Multi-column hierarchical tree with expandable `StandardTreeItem` nodes.

```cpp
// 2D Table Model
auto tableModel = std::make_shared<StandardTableModel>(10000, 4);
tableModel->setHeaderLabels({"Subsystem", "Status", "Allocated", "Latency"});
tableModel->setItem(0, 0, "Vulkan GAL");
tableModel->setItem(0, 1, "Optimal");
tableModel->setItem(0, 2, "124 MB");
tableModel->setItem(0, 3, "0.14 ms");

// Hierarchical Tree Model
auto treeModel = std::make_shared<StandardTreeModel>(3);
treeModel->setHeaderLabels({"Resource", "Type", "Size"});
auto* rootItem = treeModel->invisibleRootItem();
auto* meshBranch = rootItem->appendChild({"Models", "Directory", ""});
meshBranch->appendChild({"character.gltf", "Mesh", "14.2 MB"});
```

### Item selection model

[ItemSelectionModel](../src/widgets/views/item_selection_model.hpp) manages selections independently of view representations:

- **Selection modes**: `NoSelection`, `SingleSelection`, and `MultiSelection`.
- **Keyboard navigation**: `Up`, `Down`, `Home`, and `End` adjust the current index; holding `Shift` creates contiguous selection ranges; holding `Ctrl` toggles individual items.
- **Signals**: `onSelectionChanged` and `onCurrentChanged` notify listeners of state updates.

### Fast-path item delegate

[ItemDelegate](../src/widgets/views/item_delegate.hpp) paints individual cells directly through `PaintContext`:

- Honors `isSelected`, `isHovered`, and `hasFocus` visual states.
- Paints Optical Zinc row backgrounds (`surfaceRaised` on hover, Cobalt selection tints) and subtle `3.0f * dpiScale` rounded `BadgeText` pills (`surfaceOverlay` with 1px `borderDefault` outline).
- Uses `theme::computeCenteredTextY` to guarantee optical vertical text centering.

### HeaderView

[HeaderView](../src/widgets/views/header_view.hpp) manages resizable column headers:

- **Live resizing**: Dragging column dividers resizes columns interactively, dispatching `CursorShape::ResizeEW` and acquiring mouse grab.
- **Click-to-sort**: Clicking a section header cycles sort order (`None` -> `Ascending` -> `Descending`), rendering continuous vector arrow indicators.

### Virtualized collection widgets

#### ListView

[ListView](../src/widgets/views/list_view.hpp) virtualizes 1D item lists:

```cpp
auto listView = std::make_unique<ListView>(&stringModel, font);
listView->setDpiScale(dpi);
listView->setListStyle(ListStyle::Inset);
listView->setRowHeight(32.0f * dpi);
listView->selectionModel().setSelectionMode(SelectionMode::Single);
listView->onItemClicked.connect([](const ModelIndex& idx) {
    // Handle item selection
});
```

- **$O(1)$ Slicing & Styles**: Calculates `firstRow` from `scrollOffsetY_` and paints only visible rows. Supports `ListStyle::Inset` (default, rounded pills with inset hairlines), `ListStyle::Sidebar` (transparent borderless sidebar pills), and `ListStyle::Plain`.

#### TableView

[TableView](../src/widgets/views/table_view.hpp) renders 2D grids with an integrated `HeaderView`:

```cpp
auto tableView = std::make_unique<TableView>(&tableModel, font);
tableView->setDpiScale(dpi);
tableView->setRowHeight(28.0f * dpi);
tableView->setShowGridLines(true);
tableView->setShowBorder(true);
tableView->horizontalHeader()->setSectionWidth(0, 180.0f * dpi);
```

- **Synchronized scrolling**: Horizontal scrolling smoothly syncs between header sections and table grid cells.
- **Cell focus & row selection**: Highlights selected rows with Cobalt accent tints across both Dark and Light `ColorPalette` themes.

#### TreeView

[TreeView](../src/widgets/views/tree_view.hpp) renders hierarchical tree-tables:

```cpp
auto treeView = std::make_unique<TreeView>(&treeModel, font);
treeView->setDpiScale(dpi);
treeView->setRowHeight(28.0f * dpi);
treeView->setIndentPerLevel(16.0f * dpi);
treeView->expandAll();
```

- **Visible-row flattening**: Flattens expanded hierarchical nodes into a linear list of `TreeVisibleNode` entries, enabling $O(1)$ virtualized rendering for deeply nested trees.
- **Rotating vector chevrons**: Renders continuous 1.5px vector strokes that rotate between collapsed rightward `>` and expanded downward `v` states.

## Desktop window shell and native dialogs

### TitleBar and DWM frameless windowing

[TitleBar](../src/widgets/dialogs/title_bar.hpp) implements a complete DWM-integrated custom caption bar:

```cpp
auto titleBar = std::make_unique<TitleBar>("Northern Workbench", font, window.get());
titleBar->setCenterWidget(std::move(searchBox));
root->addChild(std::move(titleBar));
```

- **Frameless window integration**: Setting `WindowDesc::customTitleBar = true` extends the client area across the entire window frame via `WM_NCCALCSIZE` while `DwmExtendFrameIntoClientArea` preserves native DWM shadows and maximize animations.
- **Hit testing**: `TitleBar::hitTestPoint()` translates local coordinates into `HitTestResult::Caption`, `CloseBtn`, `MinimizeBtn`, `MaximizeBtn`, or `Client`.
- **Windows 11 Snap Layouts**: Returning `HTMAXBUTTON` on maximize button hover activates native Windows 11 snap flyout menus.
- **SVG caption buttons**: Renders crisp vector Minimize (`_`), Maximize (`[]`), Restore, and Close (`X`) buttons with native hover and press tinting.

### SVG vector icon subsystem

[SvgDocument](../src/renderer/svg/svg_document.hpp) and [Icon](../src/widgets/controls/icon.hpp) provide resolution-independent vector rendering:

```cpp
auto icon = std::make_unique<Icon>(IconId::FolderOpen, 16.0f, theme::ColorTokens::AccentDefault);
```

- **De Casteljau curve decomposition**: Uses NanoSVG to parse SVG paths and evaluates cubic Bezier curves into anti-aliased 2D line primitives via adaptive subdivision.
- **Standard icon catalog**: [icons.hpp](../src/widgets/controls/icons.hpp) provides a built-in catalog of 24x24 desktop icons conforming to standard geometric design grids.

### Dialog and MessageBox

[Dialog](../src/widgets/dialogs/dialog.hpp) and [MessageBox](../src/widgets/dialogs/message_box.hpp) provide non-blocking in-window modal dialogs rendered over `WidgetContext::showOverlay` with `OverlayPlacement::Center` and `isModal = true`:

```cpp
MessageBox::confirm(uiContext, "Exit Application", "Are you sure you want to close Northern?",
    [&](DialogResult res) {
        if (res == DialogResult::Accepted) {
            app.quit(0);
        }
    },
    dpi);
```

- **Modal scrim & focus trap**: Blocks background pointer events with a `0.14s OutCubic` faded `0.55f` backdrop scrim (`dismissOnOutsideClick = false`) and traps `Tab` / `Shift+Tab` inside the dialog controls.
- **Keyboard actions**: `Escape` rejects/closes the dialog (`DialogResult::Rejected`); `Enter` activates the primary/default button (`DialogResult::Accepted`).
- **Semantic icon restraint**: `MessageBox::info` and `MessageBox::confirm` omit icons for a clean layout, while `MessageBox::warning` and `MessageBox::error` display semantic `24px` warning/error icons.

### Platform FileDialog

[FileDialog](../src/platform/file_dialog.hpp) provides native OS file and folder picker dialogs:

```cpp
FileDialogDesc desc;
desc.title = "Open Document";
desc.filters = {{"Supported Files (*.txt, *.json)", "*.txt;*.json"}, {"All Files (*.*)", "*.*"}};
auto res = FileDialog::openFile(window.get(), desc);
if (res.isOk() && !res.value().empty()) {
    std::string path = res.value();
}
```

- **COM shell integration**: Utilizes `IFileOpenDialog` and `IFileSaveDialog` on Windows with modal parent window attachment and customizable file type filters.

## Animation subsystem

[src/core/animation/](../src/core/animation) provides frame-delta motion primitives advanced by `WidgetContext::advanceAnimations(deltaSeconds)`:

- `EasingCurve` ([easing.hpp](../src/core/animation/easing.hpp)): `Linear`, `InQuad`, `OutQuad`, `InOutQuad`, `InCubic`, `OutCubic`, `InOutCubic`, `OutExpo`, `InOutExpo`, `OutElastic`, and `cubicBezier()`.
- `Spring` ([spring.hpp](../src/core/animation/spring.hpp)): Closed-form analytical damped harmonic oscillator for under-damped, critically damped, and over-damped motion.
- `PropertyAnimation<T>` ([property_animation.hpp](../src/core/animation/property_animation.hpp)): Dual-mode easing/spring interpolator for `float`, `core::Color`, and `core::Point`.
- `AnimationManager` ([animation_manager.hpp](../src/core/animation/animation_manager.hpp)): Advances active animations and prunes settled instances automatically.

## Media & status widgets

### ImageView

[ImageView](../src/widgets/controls/image_view.hpp) displays decoded `renderer::Image` assets ([image.hpp](../src/renderer/image/image.hpp)) with SDF rounded corners, optional hairline borders, and automatic GPU texture caching:

- **Fit modes (`ImageFit`)**: `Contain`, `Cover` (centered UV crop), `Fill`, `ScaleDown`, and `None`.
- **Sampling (`graphics::Filter`)**: `Linear` (photos) or `Nearest` (pixel art / crisp UI sprites).
- **Empty placeholder**: Renders a `SurfaceDefault` rounded frame with centered `placeholderIcon_` when no image is assigned.

### Avatar

[Avatar](../src/widgets/controls/avatar.hpp) renders circular or rounded-square user portraits (`Small = 24px`, `Medium = 32px`, `Large = 40px`, `XLarge = 56px`):

- **Image & initials fallback**: Crops `renderer::Image` via `ImageFit::Cover` or extracts 1–2 uppercase initials centered with `theme::computeCenteredTextY()`.
- **Presence status**: Optional `AvatarStatus` (`Online`, `Busy`, `Away`, `Offline`) indicator dot with a surface-matched cutout ring.

### Badge

[Badge](../src/widgets/controls/badge.hpp) renders pill labels and interactive tags (`Compact = 20px`, `Standard = 24px`):

- **Variants & roles**: `Subtle`, `Solid`, and `Outline` across `Neutral`, `Accent`, `Success`, `Warning`, `Danger`, and `Info` semantic roles.
- **Leading indicators & dismissible tags**: Supports optional status dots (`setShowDot`), leading `IconId` glyphs (`setIcon`), and interactive close buttons (`setDismissible`, emitting `onDismissed` on click or `Delete`/`Backspace`/`Enter`).
