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
| `Divider` | [divider.hpp](../src/widgets/divider.hpp) | Horizontal or vertical hairline separator with optional centered text label |
| `SplitView` | [split_view.hpp](../src/widgets/split_view.hpp) | Proportional resizing container with draggable sash divider, min-size bounds, and `ResizeEW`/`ResizeNS` cursors |
| `TabBar` | [tab_view.hpp](../src/widgets/tab_view.hpp) | 32px desktop workbench tabs with 2px Cobalt indicator, vector close buttons, and arrow-key cycling |
| `TabView` | [tab_view.hpp](../src/widgets/tab_view.hpp) | Complete tabbed workbench hosting a `TabBar` and an automated page deck container |
| `CollapsibleSection` | [accordion.hpp](../src/widgets/accordion.hpp) | Smooth animated height collapse (`OutCubic`), rotating vector chevron, and clipped body |
| `Accordion` | [accordion.hpp](../src/widgets/accordion.hpp) | Stacked section container supporting independent or single-expanded mutual exclusivity |
| `HeaderView` | [header_view.hpp](../src/widgets/header_view.hpp) | Multi-column table header with draggable column resizing, `ResizeEW` cursor, and click-to-sort vector arrows |
| `ListView` | [list_view.hpp](../src/widgets/list_view.hpp) | High-performance virtualized 1D list rendering $O(1)$ visible slice with keyboard navigation and scrollbars |
| `TableView` | [table_view.hpp](../src/widgets/table_view.hpp) | 2D virtualized grid with integrated `HeaderView`, column resizing, alternating rows, and grid lines |
| `TreeView` | [tree_view.hpp](../src/widgets/tree_view.hpp) | Multi-column hierarchical tree-table with visible row flattening, depth indentation, and animated vector chevrons |
| `Icon` | [icon.hpp](../src/widgets/icon.hpp) | High-performance resolution-independent SVG vector icon widget with tinting and cached paths |
| `TitleBar` | [title_bar.hpp](../src/widgets/title_bar.hpp) | Frameless DWM title bar with app logo, customizable center search/action slots, and native SVG caption buttons |
| `Dialog` | [dialog.hpp](../src/widgets/dialog.hpp) | In-window modal dialog with dimming backdrop scrim, focus trapping, header icon, and action button bar |
| `MessageBox` | [message_box.hpp](../src/widgets/message_box.hpp) | Convenience dialog factory for `showInformation`, `showConfirm`, and `showWarning` with SVG status badges |
| `FileDialog` | [file_dialog.hpp](../src/platform/file_dialog.hpp) | Native OS file open/save and folder picker with COM shell integration |

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

## Navigation and multi-panel containers

### SplitView

[SplitView](../src/widgets/split_view.hpp) divides space horizontally or vertically with a live draggable sash divider:

```cpp
auto splitView = std::make_unique<SplitView>(SplitOrientation::Horizontal, 0.3f);
splitView->setMinPaneSize(180.0f);
splitView->setSashThickness(6.0f);
splitView->setPanes(std::move(sidebar), std::move(contentDeck));
splitView->onSplitChanged.connect([](float ratio) {
    // Proportional ratio update [0.01f, 0.99f]
});
```

- **Interactive sash**: Hovering over the divider dispatches `CursorShape::ResizeEW` (horizontal) or `CursorShape::ResizeNS` (vertical). Left-clicking acquires mouse grab (`WidgetContext::setMouseGrabWidget`), allowing dragging even if the cursor moves outside the split container.
- **Flexbox-native integration**: Resizes child pane styles proportionally via `flexGrow = ratio` and `flexGrow = 1.0f - ratio` within the flex layout engine.

### TabBar and TabView

[TabView](../src/widgets/tab_view.hpp) manages a tabbed workbench with page swapping:

```cpp
auto tabView = std::make_unique<TabView>(font);
tabView->addTab("Canvas", std::move(canvasPage), false);
tabView->addTab("Console", std::move(consolePage), true); // closable
tabView->onTabChanged.connect([](int index) { /* Active tab switched */ });
tabView->onTabClosed.connect([](int index) { /* Tab removed */ });
```

- **Desktop ergonomics**: Standard 32px tab height with active 2px Cobalt bottom underline indicator (`AccentDefault`).
- **Keyboard cycling**: `Left` and `Right` arrow keys cycle active tabs with wrap-around.
- **Vector close buttons**: 16×16px close buttons (`x`) rendered with continuous anti-aliased diagonal vector strokes.

### CollapsibleSection and Accordion

[Accordion](../src/widgets/accordion.hpp) manages stacked sections with animated height collapse:

```cpp
auto accordion = std::make_unique<Accordion>(true); // singleExpanded = true
accordion->addSection("Configuration", std::move(configControls), true);
accordion->addSection("Telemetry", std::move(metricsControls), false);
```

- **Smooth animation**: Heights animate from 0 to measured content size via `PropertyAnimation<float>` (0.15s `OutCubic`) with child clipping (`clipsChildren() == true`).
- **Continuous vector chevron**: Rotates smoothly by 90 degrees from right-pointing `>` when collapsed to downward `v` when expanded.
- **Single-expanded mode**: Automatically collapses adjacent sections when any section expands.

### Divider

[Divider](../src/widgets/divider.hpp) renders crisp 1px hairline optical boundaries:

```cpp
auto horizontalDiv = std::make_unique<Divider>("PROPERTIES", font);
auto verticalDiv = std::make_unique<Divider>(DividerOrientation::Vertical);
```

- **Labeled lines**: Centered text label with optical vertical centering (`theme::computeCenteredTextY`) and configurable gap clearance.

## Virtualized data and collection views

Northern provides high-performance collection views in [src/widgets/](../src/widgets) decoupled through a Model-View-Delegate architecture. Large datasets (10,000+ items) render at 60+ FPS using $O(1)$ visible viewport slicing with zero per-frame heap allocations.

### Role-based data models

[ItemModel](../src/widgets/item_model.hpp) defines an abstract interface returning typed cell data across [ItemDataRole](../src/widgets/item_model.hpp):

```cpp
enum class ItemDataRole {
    Display,        // Primary cell text (std::string)
    Decoration,     // Color tint or icon identifier (core::Color)
    ToolTip,        // Hover tooltip string
    CheckState,     // Checkbox boolean state
    TextAlignment,  // Text alignment (Left, Center, Right)
    Foreground,     // Custom text color (core::Color)
    Background      // Custom cell background color (core::Color)
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

[ItemSelectionModel](../src/widgets/item_selection_model.hpp) manages selections independently of view representations:

- **Selection modes**: `NoSelection`, `SingleSelection`, and `MultiSelection`.
- **Keyboard navigation**: `Up`, `Down`, `Home`, and `End` adjust the current index; holding `Shift` creates contiguous selection ranges; holding `Ctrl` toggles individual items.
- **Signals**: `onSelectionChanged` and `onCurrentChanged` notify listeners of state updates.

### Fast-path item delegate

[ItemDelegate](../src/widgets/item_delegate.hpp) paints individual cells directly through `PaintContext`:

- Honors `selected`, `hovered`, and `focused` visual states.
- Paints Optical Zinc backgrounds (`SurfaceSubtle`, `SurfaceOverlay`, or Cobalt selection tints).
- Uses `theme::computeCenteredTextY` to guarantee optical vertical text centering.

### HeaderView

[HeaderView](../src/widgets/header_view.hpp) manages resizable column headers:

- **Live resizing**: Dragging column dividers resizes columns interactively, dispatching `CursorShape::ResizeEW` and acquiring mouse grab.
- **Click-to-sort**: Clicking a section header cycles sort order (`None` -> `Ascending` -> `Descending`), rendering continuous vector arrow indicators.

### Virtualized collection widgets

#### ListView

[ListView](../src/widgets/list_view.hpp) virtualizes 1D item lists:

```cpp
auto listView = std::make_unique<ListView>(stringModel, font);
listView->setRowHeight(28.0f);
listView->selectionModel()->setSelectionMode(SelectionMode::Single);
listView->onItemClicked.connect([](const ModelIndex& idx) {
    // Handle item selection
});
```

- **$O(1)$ Slicing**: Calculates `firstRow = std::max(0, static_cast<int>(scrollOffsetY / rowHeight))` and paints only visible rows, supporting millions of entries with constant frame time.

#### TableView

[TableView](../src/widgets/table_view.hpp) renders 2D grids with an integrated `HeaderView`:

```cpp
auto tableView = std::make_unique<TableView>(tableModel, font);
tableView->setRowHeight(28.0f);
tableView->setShowGridLines(true);
tableView->setAlternatingRowColors(true);
tableView->headerView()->setSectionWidth(0, 180.0f);
```

- **Synchronized scrolling**: Horizontal scrolling smoothly syncs between header sections and table grid cells.
- **Cell focus & row selection**: Highlights selected rows with subtle Cobalt accent fills and active focus outlines.

#### TreeView

[TreeView](../src/widgets/tree_view.hpp) renders hierarchical tree-tables:

```cpp
auto treeView = std::make_unique<TreeView>(treeModel, font);
treeView->setRowHeight(28.0f);
treeView->setIndentation(18.0f);
treeView->expandAll();
```

- **Visible-row flattening**: Flattens expanded hierarchical nodes into a linear list of `TreeVisibleNode` entries, enabling $O(1)$ virtualized rendering for deeply nested trees.
- **Rotating vector chevrons**: Renders continuous 1.5px vector strokes that rotate between collapsed rightward `>` and expanded downward `v` states.

## Desktop window shell and modal dialogs

### TitleBar and DWM frameless windowing

[TitleBar](../src/widgets/title_bar.hpp) implements a complete DWM-integrated custom caption bar:

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

[SvgDocument](../src/renderer/svg/svg_document.hpp) and [Icon](../src/widgets/icon.hpp) provide resolution-independent vector rendering:

```cpp
auto icon = std::make_unique<Icon>(IconId::FolderOpen, 16.0f, theme::ColorTokens::AccentDefault);
```

- **De Casteljau curve decomposition**: Uses NanoSVG to parse SVG paths and evaluates cubic Bezier curves into anti-aliased 2D line primitives via adaptive subdivision.
- **Standard icon catalog**: [icons.hpp](../src/widgets/icons.hpp) provides a built-in catalog of 24x24 desktop icons conforming to standard geometric design grids.

### Modal Dialogs and MessageBox

[Dialog](../src/widgets/dialog.hpp) and [MessageBox](../src/widgets/message_box.hpp) provide modal overlays with backdrop dimming:

```cpp
MessageBox::showConfirm(
    uiContext, "Unsaved Changes",
    "Are you sure you want to discard your unsaved modifications?",
    font,
    [](bool confirmed) {
        if (confirmed) {
            // Handle discard
        }
    });
```

- **Backdrop scrim**: Darkens underlying content with `rgba(0, 0, 0, 0.60)` and blocks pointer clicks from reaching background widgets.
- **Focus trapping**: Traps `Tab` and `Shift+Tab` focus cycling strictly inside the dialog content.
- **Keyboard accelerators**: Automatically triggers the primary action on `Enter` and cancellation on `Escape`.

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
