# AGENTS.md

Operational engineering guidelines for autonomous agents working in `northern`, a GPU-accelerated desktop UI framework written from scratch in modern C++20.

## 1. Tech Stack & Environment

- **Language & Standard**: Pure C++20 (`-std=c++20` / `/std:c++20`), `#pragma once`, no Qt MOC/codegen.
- **Compiler & Build**: MSYS2 UCRT64 `clang++` 16+ (or GCC 12+ / MSVC 19.30+), CMake 3.25+, Ninja.
- **Dependencies**: Vulkan SDK 1.3+, `harfbuzz`, `freetype`, `gtest` (pinned in [vcpkg.json](vcpkg.json); isolated behind internal wrappers).
- **Test Suite**: GoogleTest (`unit_tests.exe`) with 324 tests across 54 suites using headless `MockDevice`.

### Directory Landmarks

| Module | Path | Responsibility |
| :--- | :--- | :--- |
| `Core` | `src/core/` | `Result<T, Error>`, `Arena`/`ScopedArena`, `HandlePool`, `ThreadPool`, `TaskQueue`, `Signal`, `Animation` |
| `Platform` | `src/platform/` | `Application`, `Window`, `Display`, `Clipboard`, `FileDialog`, Win32 pImpl backend |
| `Graphics` | `src/graphics/` | GAL (`Device`, `CommandBuffer`, `Swapchain`), Vulkan 1.3+ (`BlockAllocator`, `StagingPool`), `MockDevice` |
| `Text` | `src/text/` | `FontManager`, `TextShaper` (HarfBuzz), `TextLayout`, `GlyphAtlas` (FreeType), `VectorRasterizer` |
| `Renderer` | `src/renderer/` | `BatchRenderer`, `PaintContext`, `SvgDocument`, 2D SDF shaders (`shaders/`) |
| `Layout` | `src/layout/` | `Style`, `LayoutNode`, zero-heap `FlexSolver` with `ArenaAllocator` and `aspectRatio` |
| `Widgets` | `src/widgets/` | `base/` (`Widget`, `WidgetContext`, `theme`), `containers/` (`MenuBar`, `TabView`, `SplitView`, `Dropdown`, `ContextMenu`, `ScrollView`, `Accordion`), `controls/`, `dialogs/` (`TitleBar`), `views/` (`ListView`, `TableView`, `TreeView`, `HeaderView`, `ItemModel`, `ItemDelegate`) |
| `Tests` | `tests/` | `unit/` and `integration/` GoogleTest suites |
| `Docs` | `docs/` | [architecture.md](docs/architecture.md), [widgets.md](docs/widgets.md), [design-languages.md](docs/design-languages.md), [getting-started.md](docs/getting-started.md) |

---

## 2. Key Commands

- **Configure (Ninja + Clang)**: `cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++`
- **Build All Targets**: `cmake --build build` (or `ninja -C build`)
- **Run Full Test Suite**: `.\build\tests\unit_tests.exe`
- **Run Single Test / Suite**: `.\build\tests\unit_tests.exe --gtest_filter=TabViewTest.*`
- **Launch Demo App**: `.\build\examples\example_windowing.exe`
- **Check Formatting**: `clang-format --dry-run --Werror src/widgets/containers/tab_view.hpp src/widgets/containers/tab_view.cpp`
- **Apply Formatting**: `clang-format -i <file1> <file2>`

---

## 3. Boundaries

### Always
- Write a failing test in `tests/unit/` or `tests/integration/` before implementing features or bugfixes.
- Run `ninja -C build`, `.\build\tests\unit_tests.exe`, and `clang-format --dry-run --Werror` on modified files before claiming completion.
- Use `northern::core::Result<T, Error>` for recoverable runtime errors; reserve `NORTHERN_ASSERT` for programmer invariants.
- Implement `setDpiScale(float)` and `effectiveFont()` (falling back to `FontManager::instance().defaultFont(pt * dpiScale_)`) on text-rendering widgets.
- Adhere strictly to [docs/design-languages.md](docs/design-languages.md) and `northern::theme`: Optical Zinc surfaces, Cobalt `#2563EB` accent, `theme::computeCenteredTextY()` optical centering, and continuous `PaintContext::drawLine` strokes.

### Ask First
- Adding new external dependencies to `CMakeLists.txt` or `vcpkg.json`.
- Modifying module dependency boundaries in `src/CMakeLists.txt`.
- Changing public framework headers in `src/core/`, `src/graphics/`, or `src/platform/` that affect ABI or cross-module contracts.

### Never
- Never expose native OS headers (`<windows.h>`, `<shellapi.h>`, `<wayland-client.h>`, `<X11/Xlib.h>`) or graphics headers (`<vulkan/vulkan.h>`, `<d3d11.h>`) in public headers; isolate via pImpl or opaque handles.
- Never introduce Qt, GTK, Electron, SDL, GLFW, or C++23 `std::expected` in public headers.
- Never mutate widget state across threads; dispatch worker results back to the UI thread via `TaskQueue`.
- Never destroy GPU resources immediately while in-flight; retire via timeline-synchronized deferred destruction queues.
- Never trigger `markLayoutDirty()` for purely visual updates (color, opacity, hover tint); call `markPaintDirty()`.
- Never introduce circular module dependencies; follow `Core` -> `Platform`/`Graphics`/`Layout` -> `Text` -> `Renderer` -> `Widgets`.
- Never use AI-slop visuals (`#6366F1`, `#8B5CF6`, glowing borders, colored drop shadows) or disjoint pixel blocks for vector strokes.
- Never run `git add` or `git commit` without explicit user confirmation.

---

## 4. Code & Architecture Patterns

### Naming & Ownership
- **Files**: `.hpp` for headers (`#pragma once`), `.cpp` for sources, co-located per module directory.
- **Identifiers**: `PascalCase` types/enums, `camelCase` methods, `camelCase_` trailing-underscore members, `kPascalCase` constants, `NORTHERN_UPPER_CASE` macros.
- **Namespace**: `northern::` (alias `nrth`), partitioned by module (`northern::core`, `northern::widgets`).
- **Widget Ownership**: Parent owns children via `std::unique_ptr<Widget>`. Raw `Widget*` pointers are non-owning observers; use `deleteLater()` when deleting during event dispatch.

### Layout vs. Paint & Retained Display Lists
```cpp
// Good: visual state change invalidates paint only; layout change invalidates layout
void Button::onMouseEnter() {
    isHovered_ = true;
    markPaintDirty();
}

// Bad: triggering full Flexbox pass on hover tint
void Button::onMouseEnter() {
    isHovered_ = true;
    markLayoutDirty();
}
```

### Font Fallback & DPI Scaling
```cpp
// Good: explicit font with DPI-scaled default fallback
std::shared_ptr<text::Font> TabBar::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        resolvedDefaultFont_ =
            text::FontManager::instance().defaultFont(10.0f * dpiScale_).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

// Bad: skipping text rendering when caller passes font == nullptr
if (font_ && itm.cachedLayout.has_value()) {
    ctx.drawTextLayout(pos, *itm.cachedLayout, fgCol);
}
```

### Key Architectural Invariants
- **Zero-Heap Layout**: `FlexSolver` uses thread-local `core::Arena` + `core::ScopedArena` + `ArenaAllocator<T>`, with dual-level layout/measure caching on `LayoutNode`.
- **Retained Display Lists & Culling**: `WidgetContext` caches `retainedPaintContext_` for static frames and culls off-screen subtrees against `visibleBounds`.
- **GPU Suballocation & Staging**: `VulkanMemoryAllocator` (64MB device-local / 16MB host-visible blocks), `VulkanStagingPool` (timeline semaphores), and `VkPipelineCache` (`pipeline_cache.bin`).
- **Idle Power Discipline**: Main loop uses `Application::waitEvents(timeoutMs)` (`MsgWaitForMultipleObjectsEx`) when idle.

---

## 5. Verification Protocol

Before reporting any task complete:
1. Build with zero warnings (`-Wall -Wextra -Wpedantic -Werror`): `ninja -C build`
2. Pass all headless unit tests: `.\build\tests\unit_tests.exe`
3. Verify formatting on all modified files: `clang-format --dry-run --Werror <modified_files>`
4. When commits are requested, use a single unified commit with a single-line imperative header and no body.
