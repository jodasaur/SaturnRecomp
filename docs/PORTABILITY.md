# Portability inspection and implementation sequence

Inspection baseline: commit `26c9715`. This document records the first build
milestone; it does not claim native Apple Silicon or Windows verification.

## Current source and build structure

The checkout contains a game-independent disc/configuration CLI, one shared
SH-2 decoder, and a runtime with an SH-2 interpreter, dual-CPU scheduler/bus,
SCU DSP, BIOS support, CD block/ISO access, SMPC, VDP1/VDP2, MC68000 and SCSP.
There is no separate code-emission library to expose as a CMake target here.
`boot.c` and `window.c` are alternative entry points. Debug rendering and PNG
output have no SDL dependency. Settings persistence is likewise independent.
The Python desktop launcher and its packaging are separate from this scope.

`build.ps1` discovers MinGW, invokes GCC for the CLI and tests, compiles five
shaders with glslc, and invokes `tools/build_runtime.ps1`. The latter compiles
one set of runtime objects, filters out entry points and graphical objects
when linking, and supports selective GCC PGO. Both scripts use native CPU
optimization and LTO. The new CMake build reuses static libraries, supports
Ninja, makes the entire GUI optional, and uses ordinary CMake configurations
without imposing host-specific optimization or PGO.

## Platform findings

Windows dependencies include MinGW discovery, `mingw32`, `SDL2main`,
`vulkan-1`, PE stack reservation and SDL DLL copying. Runtime APIs include
headless `Sleep`, frontend `YieldProcessor` and process priority, settings
replacement with Windows file APIs, and dynamic mpg123 loading for MP3-backed
CUE tracks. Several already have POSIX alternatives or valid platform guards.
Raw BIN/CUE and ISO access do not require mpg123.

Unconditional `x86intrin.h` and `__rdtsc` occur in the boot harness and shared
bus/scheduler profiling. The window frontend calls the same intrinsic without
originally including its declaration. GCC/Clang-specific code includes
`__builtin_clz` in SCSP DSP and a `noinline` attribute in the interpreter;
these do not by themselves prevent ARM64 GCC/Clang builds, but exclude an
unmodified MSVC build.

macOS blockers are x86 timing, mandatory Vulkan dependencies in the current
frontend, backslashes in shader lookup, and Vulkan initialization enabling
only SDL's instance extensions and the swapchain device extension. There is
no portability-enumeration flag or portability-subset device extension yet.
SDL already provides graphical, audio, worker-thread and semaphore APIs.

On x86-64 Linux the baseline decoder and SH-2 semantics compile with GCC;
the semantics test passes 39 checks. Modern GCC additionally rejects the
boot harness's missing `_exit` declaration. The CMake milestone adds its
proper platform header and the missing window timing header. POSIX headless
throttling still ignores `SATURN_THROTTLE_MS`, and ARM Linux has the same
timing blocker as Apple Silicon.

## Proposed implementation/commit sequence

1. Add target-based CMake and CTest, optional GUI, shader generation, native
   dependency discovery, and build documentation. Preserve PowerShell and
   emulator behavior. Include the two missing declaration headers needed by
   current compilers.
2. Replace timestamp-counter profiling with one monotonic host clock using
   consistent units across all buckets. Provide POSIX headless sleeping and
   inspect frontend pacing. Keep guest clock accounting untouched.
3. Separate optional Vulkan linkage from SDL presentation. Build a software
   frontend without a Vulkan SDK or shader compiler; validate on native
   Apple Silicon with Homebrew SDL2. Make shader paths portable.
4. Enable Vulkan portability enumeration only when advertised, enable the
   portability-subset device extension when advertised, and validate MoltenVK
   plus existing Windows/Linux Vulkan behavior. Avoid changing shaders or
   renderer algorithms solely for platform convenience.

Core regressions gate frontend changes. Native macOS visible presentation,
input/audio checks and MinGW/PowerShell verification remain required before
claiming the full portability stage complete. No Panzer Dragoon analysis or
proprietary media belongs in these changes.

## Emulation correctness risks and audit follow-ups

- Host timing measures profiling/pacing; it must never replace SH-2, sound,
  DMA or scheduler cycle accounting. New profiling units change benchmark
  interpretation and must be documented.
- Disc and many bus accesses explicitly assemble bytes in guest order.
  This is encouraging but not a complete alignment/endian audit. Native
  structure snapshots and Vulkan buffer layouts need separate attention;
  raw snapshots are not a portable serialization format.
- File offsets use `long`/`fseek` in multiple places. LP64 Linux/macOS differ
  from Windows LLP64; large files need review rather than unconditional casts.
  Path resolution accepts both separators but compares separator pointers
  that may be null; this warrants a focused correction and regression check.
- SDL supplies worker threads and semaphores. Audio ring read/write positions
  and frontend snapshot ownership still merit a data-race audit. Replacing
  synchronization or memory layout without evidence could change behavior.
- Existing warnings include unused helpers and ambiguous expressions.
  They remain visible; broad suppression would hide useful portability
  diagnostics. Changing emulation expressions requires independent evidence.
- CD-DA currently relies on supplied media. GPU tests need a real SDL/Vulkan
  environment. A skip is not renderer or audio validation.
