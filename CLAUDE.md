# CLAUDE.md — Claude-Specific Guidance for Northern

Read [AGENTS.md](AGENTS.md) first. This document extends it with operational guidelines tailored specifically for Claude.

---

## 1. Core Orientation and Mental Model

- The project is `northern`: a modern C++20 GPU-accelerated desktop UI framework built from scratch.
- Follow all constraints, invariants, module dependency rules, and architectural contracts set forth in [AGENTS.md](AGENTS.md).
- In any conflict between prompt intuition and [AGENTS.md](AGENTS.md), the architectural contracts in [AGENTS.md](AGENTS.md) take precedence.

---

## 2. Extended Thinking Discipline

### When to Invoke Sequential / Deep Thinking
Trigger deep analytical thinking before touching or proposing solutions in:
- **GPU Resource Lifetime & Synchronization**: generation-checked handles, timeline semaphores, frame fences, deferred destruction queues, swapchain resizing.
- **Multithreading & Frame Scheduling**: UI thread vs. Render thread boundaries, thread-safe command queues, snapshot extraction, frame pacing.
- **Signals and Slots Reentrancy**: callback disconnections during emission, cycle detection, reentrant signal emission safety.
- **Widget Lifecycle & Ownership**: parent-child teardown cascades, `deleteLater()` queues, focus restoration during reparenting.
- **Layout & Invalidation Engines**: Flexbox/Grid constraint solving, dirty subtree tracking, damage region coalescing.
- **Design System & Optical Geometry**: text cap-height baseline calculations (`computeCenteredTextY`), vector stroke rasterization, focus indicator offsets.

### When to Keep Execution Direct
- Routine implementation of leaf widgets conforming to established base classes.
- Straightforward unit test authoring with GoogleTest.
- CMake build configuration adjustments for new source files.
- Typo, documentation, or trivial refactoring passes.

---

## 3. Tool and Editing Discipline

- **Surgical Edits**: Use targeted editing tools (`edit_file`) on existing files. Never overwrite full files unless executing a previously approved complete rewrite.
- **Inspect Before Editing**: Always read the target header and implementation before editing to verify existing patterns, includes, and naming conventions.
- **No Stubs**: Write complete, compiling, and verifiable C++20 code. Never leave `// TODO`, `// FIXME`, or non-functional stubs in committed framework code.
- **Header Isolation**: Verify that internal or platform headers (`<windows.h>`, `<vulkan/vulkan.h>`, etc.) never leak into public module headers.

---

## 4. Context Window Management

- **Targeted Research**: When starting a task, read the specific module's interface headers under `src/<module>/` rather than loading the entire repository into context.
- **Header-First Navigation**: Read module interface headers first to understand contracts, followed by tests to understand usage expectations, before reading implementation files.
- **Command Output Redirection**: For long compiler or test outputs, pipe or inspect only the relevant failure snippets to avoid filling the context window.

---

## 5. Architectural Planning Gate

Before making structural changes, write a formal implementation plan and obtain user approval when:
1. Adding, renaming, or altering module boundaries under `src/`.
2. Changing the threading model or cross-thread data flow (UI, Worker, Render threads).
3. Modifying the base `Widget` lifecycle, ownership semantics, or `Painter` public APIs.
4. Introducing any new external third-party library or CMake package dependency.

---

## 6. Sensitive Subsystems Checklist

Prior to submitting code touching any of the following 5 subsystems, perform a targeted sanity check against these invariants:

1. **GPU Resource Manager (`northern::graphics`)**:
   - Are resource handles checked for generation validity?
   - Is destruction deferred until the GPU fence confirms completion?
2. **Threading & Scheduler (`northern::core`, `northern::renderer`)**:
   - Is widget access strictly restricted to the UI thread?
   - Are render-thread data structures fed exclusively via immutable snapshots?
3. **Signals and Slots (`northern::core`, `northern::widgets`)**:
   - Does disconnection during an active callback avoid iterator invalidation?
   - Are receiver lifetimes properly observed?
4. **Widget Tree & Lifecycle (`northern::widgets`)**:
   - Does parent destruction safely destroy all children recursively?
   - Does event dispatch protect against in-flight widget deletion?
5. **Event Dispatch & Focus (`northern::widgets`, `northern::platform`)**:
   - Does event propagation follow Capture -> Target -> Bubble?
   - Is pointer capture decoupled from keyboard focus?
6. **Design Tokens & UI Aesthetics (`northern::widgets`, `northern::theme`)**:
   - Are colors drawn strictly from `theme::ColorTokens` (Optical Zinc ramps, deliberate Cobalt accent)?
   - Are arbitrary purple/indigo tints (`#6366F1`), neon card borders, or floating drop-shadows completely absent?
   - Is text vertically centered using `theme::computeCenteredTextY` rather than arbitrary padding or raw `lineHeight`?
   - Are vector strokes (like checkmarks) drawn via `PaintContext::drawLine` rather than discrete pixel chunks?
   - Is the window title in `examples/windowing/main.cpp` maintained as strictly `"Windowing"`?
