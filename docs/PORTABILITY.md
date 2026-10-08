# Portability inspection and implementation sequence

Inspection baseline: commit `26c9715`. The initial findings below describe that
baseline; the later audits record subsequent fixes. User-reported native
Apple Silicon validation is recorded below; Windows validation remains pending.

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

## Phase 2 implementation and audit

### Host timing and pacing

`runner/include/host_time.h` replaces every runtime/frontend `__rdtsc` call
and all unconditional x86 headers. It reports monotonic nanoseconds using
QueryPerformanceCounter on Windows and CLOCK_MONOTONIC on POSIX. No ARM
counter or guest-cycle conversion is introduced. Clock failure terminates
instead of returning a bogus timestamp. Helpers are inline to retain the
PowerShell scripts' explicit test source lists without new link dependencies.
There is no shared mutable clock initialization to synchronize.

Headless throttling now sleeps on POSIX and retries nanosleep after EINTR;
Windows keeps Sleep. The SDL frontend already uses SDL's portable performance
counter, frequency and delay APIs. Its last approximately three milliseconds
are deliberately a busy wait; Windows additionally issues YieldProcessor.
That hint is platform-guarded and is not needed for the timer to work on POSIX.
Cadence/deadline calculations and guest scheduler clocks are untouched.

### Filesystem and compiler assumptions

Both game configuration and CUE relative-path helpers compared a separator
pointer with a possibly null pointer. They now check null before comparing
pointers within the same string. Tests exercise bare paths, either separator,
parent-relative paths, POSIX absolute paths, drive paths and UNC paths, plus
actual synthetic CUE sector reads. Existing path spelling is retained; this
does not translate a Windows path into a POSIX filesystem path.

Disc offsets still use fseek/ftell and long. These are 64 bits on the target
LP64 Linux/macOS hosts; Windows LLP64/32-bit builds retain a large-file limit
and need a separately tested file-offset abstraction. Shader backslashes are
an identified GUI issue for Phase 3. Windows MP3 loading remains guarded;
raw sector access has no platform dependency. Settings replacement already
selects Windows file APIs or POSIX rename/unlink.

The remaining compiler-specific operations are GCC/Clang's noinline attribute
and guarded __builtin_clz (zero is handled before the intrinsic). These are
not x86 instructions and fit the supported compilers on ARM64. MSVC remains
outside the supported compiler set. No warnings are suppressed by this work.

### Byte order, alignment and synchronization

Disc/IP/ISO readers assemble little- and big-endian fields from bytes; core
bus and MC68000 memory access likewise use byte assembly rather than casting
guest byte arrays to host word pointers. PNG output writes its multibyte fields
explicitly. These paths do not require aligned guest addresses. SDL audio
casts its provided sample buffer to int16_t; Vulkan shader input comes from
allocated storage. Settings field offsets address actual int members.
Native Saturn structure dumps still include pointers, padding and host layout;
they are diagnostic snapshots, not cross-platform saves. Native sample/WAV
storage also warrants separate review for big-endian hosts, which are outside
the current little-endian x86-64/ARM64 targets. An alignment-sanitized Linux
test run is useful evidence, not proof for every unexercised memory path.

Field-worker handoff uses SDL semaphores: the main thread waits for completion
before reading guest state and posts work after capturing graphics inputs.
The audio callback concurrently drains the shared sound ring. Its snd_wp and
snd_rp indices are ordinary uint32_t fields in sound.c and window.c, with no
audio lock or acquire/release synchronization; sound reset can also modify
them. SDL atomics protect gain and diagnostic counters, not ring ownership.
This is an existing C data race, and x86 execution does not establish ARM64
correctness. Address ring publication, reset and snapshot reads as one focused
SDL/audio change before the Phase 3 graphical milestone. The current callback
tests cover sample behavior but do not validate concurrent access.

### Linux verification

- Release headless: 26 passed, 1 media-dependent skip, including all-opcode
  decoder conformance, 20,000 fastpath fuzz cases and the new timing/path tests.
- Release GUI build: 28 passed, 4 skips (CD-DA media and three Vulkan tests
  without a Vulkan display). Existing overlay and audio callback tests pass.
- Decoder output is byte-identical to the pre-CMake baseline.
- Debug headless with GCC alignment sanitizer: 26 passed, 1 media-dependent
  skip; no alignment violations reported.
- A temporary, entirely synthetic ISO with an SH-2 branch loop passes CLI
  inspection and headless execution. Profiling/renderbench print nanoseconds;
  0/40 ms throttling produces identical retired counts, guest registers and
  cycles, with the expected added host delay. No game or BIOS media is used.

Native ARM64 and MinGW execution remain host-validation tasks. These checks
establish Linux/headless behavior, not macOS presentation or real-game audio.

## Audio ring correctness follow-up

The Phase 2 ring race is fixed independently of renderer selection. Write/read
positions are now C11 atomic monotonic 64-bit counters. A producer release
store publishes both stereo samples; a consumer acquire load observes them.
The consumer releases its read position only after copying the samples, so
the producer cannot overwrite a slot still being read. Capacity and the
existing drop-on-full policy remain unchanged. Counters wrap after more than
13 million years at 44.1 kHz; storage slots still wrap at the ring capacity.

Sound reset publishes the current write position as a discard boundary. Only
the consumer moves the read position, including when acknowledging resets.
The callback acknowledges that boundary before checking its priming threshold,
so a reset of a full ring cannot stall subsequent production. Samples published
after the boundary remain available. A reset arriving during a callback can
leave old samples in that already-started callback; subsequent callbacks discard
the old queue. No mutex or spin lock is added to the callback.

Queue diagnostics load the atomic positions, and graphics snapshots no longer
copy audio indices. Native diagnostic state capture copies atomic fields with
atomic loads while guest execution is stopped. Its structure layout changes;
old captures must be regenerated. These files remain local diagnostics rather
than a versioned save format.

The SDL threaded test exercises production, draining, repeated storage wraps
and concurrent guest sound resets. It passes with GCC ThreadSanitizer when run
with process ASLR disabled via `setarch x86_64 -R` (the ordinary invocation
cannot initialize ThreadSanitizer in this worker's address layout). Callback
tests additionally cover discard boundaries, post-reset samples and full-ring
priming. Native Mac/Windows audio testing is still required.

## Phase 3 software frontend preparation

`SATURNRECOMP_ENABLE_VULKAN` controls the graphical backend independently of
`SATURNRECOMP_BUILD_GUI`. It defaults off on macOS and on elsewhere. With it
disabled, CMake discovers only SDL2 for the frontend, does not discover Vulkan
or glslc, and does not build shaders or GPU tests. A small disabled-backend
implementation satisfies the existing opaque Vulkan API without SDK types,
keeping one frontend implementation. It reports unavailable operations and
does not alter the core's VDP1 sink functions or rendering algorithms.

The frontend chooses SDL presentation by default in that configuration;
an explicit Vulkan request fails clearly before window creation. Conditional
SDL Vulkan includes/drawable queries remove Vulkan entry points from the
software executable. Existing Vulkan builds and PowerShell scripts retain
their default renderer selection. Shader paths now use forward slashes,
which work on Windows and POSIX.

The Linux software configuration passes its core, overlay, callback and
threaded-ring tests. Its CTest smoke test generates a minimal ISO and executes
three SH-2-loop fields through the real frontend with SDL dummy video/audio.
It verifies default software selection and rejects an explicit unavailable
Vulkan request. The same smoke test selects software explicitly in a Vulkan
build. No proprietary data is required. Undefined-symbol inspection finds no
Vulkan or SDL Vulkan references in the software executable.

Subsequent user-reported Mac validation is recorded below. Windows regression
testing and MoltenVK support remain pending. The build commands and native
checks are in BUILDING.md.

## User-reported Apple Silicon validation — 2026-10-08

Source snapshot: `c4c2847`. The user configured a Release ARM64 build with
AppleClang 21.0.0.21000101, Homebrew SDL2 and Vulkan disabled. The CLI, headless
runtime, graphical runner and tests compiled successfully. After installing
Capstone 5.0.9 in a Python environment, CTest reported 30 passes, zero failures
and one skip: the media-dependent cdda_play test. Decoder conformance, core
CPU/video/audio checks, host timing, threaded audio-ring and dummy-driver SDL
frontend tests all passed. Compiler warnings remained visible.

The user then launched a locally supplied retail title through the software
frontend and reported normal-looking behavior, working controls, music and
sound effects. This establishes the initial native Mac presentation/input/audio
smoke milestone. It does not establish comprehensive game compatibility or
replace the still-skipped CD-DA test. The user also reported successful window resizing and normal-looking output
on the laptop Retina display. Transitions between displays with different scale
factors remain untested. macOS version, SDL2 version and extended gameplay
were not reported. No game/BIOS data or
title-specific code changes are included in this validation record.

## Phase 4 MoltenVK preparation

Vulkan instance setup now queries advertised extensions and opts into
VK_KHR_portability_enumeration with its matching instance flag when available.
The extension is not duplicated if SDL already requests it. Enumeration errors
stop initialization with a diagnostic, and incomplete lists are retried. Device
creation enables VK_KHR_portability_subset only when the chosen GPU advertises
it; native Vulkan devices retain their existing swapchain-only request. No
shader or renderer algorithm changes are made for Metal.

A display-independent fault-injection test exercises the actual instance setup
with portability absent/present, an SDL-supplied duplicate, an incomplete
extension list and an enumeration error. Native MoltenVK presentation and the
existing GPU regression tests still require a Mac. BUILDING.md documents the
Homebrew loader, headers, MoltenVK and shaderc packages, an explicit driver
manifest and matching SDL loader selection. Windows validation remains pending.

Linux Release Vulkan build verification: 31 tests passed, zero failed and four
skipped (CD-DA media and three GPU tests without a display). After adding retry
handling for device extension discovery, the focused portability, SDL frontend
and callback checks passed again; GPU checks retained their display-related
skips. The portability test also checks device extension presence, incomplete
enumeration retry and error handling. macOS continues to default to the tested
software configuration unless Vulkan is explicitly enabled at configure time.
