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

For an SDL frontend without Vulkan or shader tooling:

```sh
sudo apt-get install libsdl2-dev
cmake -S . -B out/software -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DSATURNRECOMP_BUILD_GUI=ON -DSATURNRECOMP_ENABLE_VULKAN=OFF
cmake --build out/software
ctest --test-dir out/software --output-on-failure
```

The core still renders in software; SDL presents the resulting texture using
an available native backend, falling back to SDL software presentation.
`SATURN_RENDERER=software` explicitly selects SDL software presentation.
Requesting `SATURN_RENDERER=vulkan` in a build with Vulkan disabled reports an
error. Vulkan remains enabled by default for graphical Linux/Windows builds.

## macOS Apple Silicon

From a native ARM64 terminal (`uname -m` should report `arm64`), install the
Homebrew dependencies and build. If Apple's compiler tools are missing,
install them with `xcode-select --install` first.

```sh
brew install cmake ninja sdl2
cmake -S . -B out/macos -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_PREFIX_PATH="$(brew --prefix sdl2)" \
  -DSATURNRECOMP_BUILD_GUI=ON -DSATURNRECOMP_ENABLE_VULKAN=OFF
cmake --build out/macos
ctest --test-dir out/macos --output-on-failure
```

macOS defaults to Vulkan disabled. Neither a Vulkan SDK, MoltenVK nor glslc
is needed for this configuration. It builds the CLI, headless runtime, SDL
frontend and non-Vulkan tests. Python/Capstone remain optional as described
above; a skipped decoder oracle means that comparison has not been validated.
To enable it in an isolated Python environment:

```sh
python3 -m venv out/test-python
out/test-python/bin/python -m pip install capstone
cmake -S . -B out/macos -DPython3_EXECUTABLE="$PWD/out/test-python/bin/python"
ctest --test-dir out/macos --output-on-failure
```

The native Mac build/run is still to be verified on an Apple Silicon host;
these are setup instructions, not a claim of completed native validation.
With a local, user-supplied game configuration:

```sh
out/macos/saturnboot "games/<name>/game.toml" 1000000
out/macos/saturnwin "games/<name>/game.toml"
```

Check visible presentation and resizing/high-DPI scaling, keyboard/controller
input, pause/frame stepping and audible playback. In software mode F1 retains
the existing diagnostic display; the Vulkan settings overlay and interpolation
are unavailable. CTest's synthetic SDL run uses dummy video/audio drivers and
does not establish visible presentation or sound quality.

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

## Host timing and headless throttling

`SATURN_PROF=1` measures elapsed host time in nanoseconds with a monotonic
clock. All runtime/frontend profiling buckets use the same units;
`SATURN_RENDERBENCH=N` prints nanoseconds per software-rendered frame.
These values replace timestamp-counter ticks and cannot be compared directly
with old tick totals. Clock calls add measurement overhead, so use runs without
profiling when measuring throughput. Guest CPU, video and sound clocks are
unchanged.

`SATURN_THROTTLE_MS=N` sleeps after each scheduled headless field on Windows
and POSIX hosts. It is opt-in; unset or zero runs without sleeping. POSIX sleep
resumes after signals. The `SATURN_NOVID` diagnostic bypass retains its existing
unthrottled behavior. SDL presentation continues using its performance counter
and delay APIs with the existing short final spin.

The shared header uses QueryPerformanceCounter on Windows and
`clock_gettime(CLOCK_MONOTONIC)`/`nanosleep` on POSIX. CMake provides
`_POSIX_C_SOURCE=200809L`; manual POSIX builds must define it as well. Keeping
the helper header-only preserves the existing PowerShell test source lists.

## Portability work still required

The runtime no longer includes x86 timing intrinsics. An SDL software-only
configuration is available, and shader lookup uses portable separators.
MoltenVK extension handling is a subsequent milestone. MP3-backed CD tracks
are decoded only on Windows; raw BIN/CUE and ISO do not need that decoder.
The SDL audio ring now uses atomic publication and consumer-owned reset
handling; see [the audit](PORTABILITY.md). Native audio validation is still
required. Regenerate diagnostic structure snapshots after rebuilding, since
their native layout changes with the ring counters.

Native macOS and Windows validation must be performed on those hosts. Linux
compilation and synthetic tests do not establish visible presentation, audio
quality, or compatibility with any particular game. Raw diagnostic snapshots
contain native structure layouts and should not be treated as portable saves.
