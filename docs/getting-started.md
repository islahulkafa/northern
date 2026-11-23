# Get started with Northern

This guide walks you through installing build dependencies, compiling Northern with CMake and Ninja, running the headless test suite, and launching the interactive windowing demo.

## Prerequisites

Northern targets C++20 on Windows (`Win32`) with Vulkan 1.3+ rendering:

- **Compiler**: MSYS2 UCRT64 `clang++` 16+, `g++` 12+, or MSVC 19.30+
- **Build tools**: CMake 3.25+ and Ninja
- **SDK and libraries**:
  - Vulkan SDK 1.3+
  - `harfbuzz`, `freetype`, and `gtest` (via MSYS2 UCRT64 packages or [vcpkg.json](../vcpkg.json))

## Build from source

1. Clone the repository and enter the workspace root:

   ```powershell
   git clone https://github.com/islahulkafa/northern.git
   cd northern
   ```

2. Ensure your toolchain (`clang++`, `cmake`, `ninja`, and MSYS2 UCRT64 libraries) is on `PATH`:

   ```powershell
   $env:PATH = "C:\Dev\msys64\ucrt64\bin;" + $env:PATH
   ```

3. Configure and build all libraries, tests, and examples:

   ```powershell
   cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++
   cmake --build build
   ```

> [!NOTE]
> Compiler warnings are enforced as errors (`-Wall -Wextra -Wpedantic -Werror` for Clang/GCC, `/W4 /WX /permissive-` for MSVC) via [CompilerWarnings.cmake](../cmake/CompilerWarnings.cmake).

## Run the test suite

Execute the headless GoogleTest binary directly to verify all subsystems (`core`, `platform`, `graphics`, `renderer`, `text`, `layout`, and `widgets`):

```powershell
.\build\tests\unit_tests.exe
```

Expected output:

```text
[==========] Running 186 tests from 44 test suites.
...
[  PASSED  ] 186 tests.
```

> [!TIP]
> Headless CI runners and environments lacking Vulkan hardware acceleration run all tests using Northern's pure-CPU [MockDevice](../src/graphics/mock/mock_device.hpp).

## Launch the desktop demo

Run the interactive Vulkan windowing showcase to inspect focus traversal, cursor dispatch, animated controls, tooltips, dropdown overlays, and context menus:

```powershell
.\build\examples\example_windowing.exe
```

## Check code formatting

Verify formatting compliance against [.clang-format](../.clang-format) before opening a pull request:

```powershell
clang-format --dry-run --Werror src/widgets/**/*.hpp src/widgets/**/*.cpp
```
