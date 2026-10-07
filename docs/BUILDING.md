# Native builds

CMake is an additional build system. The existing MinGW/PowerShell scripts
remain supported and unchanged. CMake requires version 3.20 or newer and GCC
or Clang; MSVC is not currently supported. Use a separate build directory for
each compiler or configuration.

## Linux

On Debian/Ubuntu:

```sh
sudo apt-get update
sudo apt-get install cmake ninja-build gcc python3-capstone
cmake -S . -B out/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build out/build
ctest --test-dir out/build --output-on-failure
```

The default build produces `out/build/saturnrecomp`, `out/build/saturnboot`,
and the existing hardware/runtime tests without SDL or Vulkan dependencies.
`-DSATURNRECOMP_BUILD_TESTS=OFF` omits tests. Python is optional; the decoder
comparison needs Capstone with SH-2 support and reports a skip if unavailable.
The comparison checks all 65,536 encodings with the existing allowlist and
assertions. Test assertions remain enabled in Release builds.

To also build the existing SDL/Vulkan frontend:

```sh
sudo apt-get install libsdl2-dev libvulkan-dev glslc
cmake -S . -B out/gui -G Ninja -DCMAKE_BUILD_TYPE=Release -DSATURNRECOMP_BUILD_GUI=ON
cmake --build out/gui
ctest --test-dir out/gui --output-on-failure
```

Shaders are compiled in the build directory and placed beside `saturnwin`.
GPU tests report skips when SDL cannot create a Vulkan window/device. CD-DA
playback currently requires user-supplied media and reports a skip without it.
Declared games under `games/*/game.toml` also receive the existing CLI module
validation when Python is available; missing discs are reported as skips.
No firmware or games are needed for the other C tests.

## Windows

Continue to use `build.ps1`, `tools/build_runtime.ps1`, and `tests/run_all.ps1`
for the established MinGW/PowerShell workflow, including PGO. Alternatively,
from an MSYS2 MINGW64 shell with GCC, CMake, Ninja, SDL2, Vulkan and shaderc:

```sh
cmake -S . -B out/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DSATURNRECOMP_BUILD_GUI=ON
cmake --build out/cmake
ctest --test-dir out/cmake --output-on-failure
```

Use native MinGW-w64 dependencies, not MSYS runtime libraries. CMake links
SDL2's startup target when provided, copies SDL2 beside the frontend, and
preserves the 64 MiB MinGW stack reservation. Other dependency DLLs must be on
PATH. CMake does not enable native-CPU optimization, LTO or PGO by default.

## Target organization

- `sh2_decoder`: shared decoder and instruction formatting.
- `saturn_disc`: sector/ISO access and game configuration.
- `saturn_core`: interpreter, scheduler/bus, video, sound and BIOS support.
- `saturn_settings`: host settings persistence.
- `saturnrecomp`, `saturnboot`: CLI and headless runtime.
- `saturn_overlay`, `saturn_vulkan`, `saturnwin`: optional graphical frontend.

Sources are listed once for each library; tests reuse those targets. Some GPU
tests intentionally include the renderer implementation for fault injection
and link the remaining dependencies separately.

## Portability work still required

The first CMake milestone retains the original x86 timing implementation.
Apple Silicon and ARM Linux require a portable host clock before these targets
can build. The graphical target still requires Vulkan; a software-only SDL
configuration and MoltenVK extension handling are subsequent milestones.
Shader lookup still contains Windows path separators. MP3-backed CD tracks
are decoded only on Windows; raw BIN/CUE and ISO do not need that decoder.

Native macOS and Windows validation must be performed on those hosts. Linux
compilation and synthetic tests do not establish visible presentation, audio
quality, or compatibility with any particular game. Raw diagnostic snapshots
contain native structure layouts and should not be treated as portable saves.
