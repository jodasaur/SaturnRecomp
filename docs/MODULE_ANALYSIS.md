# Executable and overlay analysis

The first milestone is reproducible local extraction, followed by confirmation
of loading behavior. The runtime currently uses interpreted SH-2 execution;
module declarations and linear disassembly do not establish static recompilation.

## Phase 1: preserve the evidence

Use a built `saturnrecomp` CLI and Python 3 (standard library only). Keep the
original CUE and every referenced BIN together. The CLI disc reader handles
the track layout; the analysis tool does not implement a second CUE reader.

Inspect the selected disc to identify its first-read filename and load address.
Use the values reported by IP.BIN; inspect each title and revision independently.

From the repository root on the Mac:

```sh
python3 tools/analyze_modules.py \
  'games/example/disc.cue' \
  --cli out/macos/saturnrecomp \
  --first-read /1ST_READ.PRG \
  --out out/analysis/example
```

On Linux, substitute your CUE path and `out/cmake-headless/saturnrecomp`.
The output directory must be new. Failed extraction can leave partial output;
use a new directory after investigating the failure.

Outputs:

- `inspection.txt`: IP.BIN, track table and filesystem listing.
- `manifest.json`: complete file inventory, selected candidates, extracted-file
  SHA-256 hashes, CLI executable hash, and known versus unknown addresses.
- `modules/NNN.bin`: exact extracted files, mapped back to ISO paths in the manifest.
- `first-read-preview.txt`: 128 linearly decoded words at the IP.BIN load address.

Selection defaults to `.PRG` and `.TSK`, plus the explicitly selected first-read
file. Override it with repeated `--suffix` options. Extensions only select
candidates: they do not prove CPU architecture, compression, code boundaries,
or executable content. Unknown values remain null in the manifest. The CLI hash
identifies the analysis executable; extracted hashes identify the chosen files,
not the complete disc or audio tracks.

## Phase 2: establish module loading

Inspect the first-read startup and its file-loading calls. Other selected files
are candidates requiring architecture and format investigation. Do not assign
them the first-read address by default.

Record each claim with its evidence: file offset, decoded instruction address,
loader destination, source filename, compression handling, and observed runtime
transition. Distinguish static conclusions from runtime observations. Add modules
to the private `game.toml` only after their load behavior is established.

## Phase 3: Ghidra and execution checks

After confirming raw code and its base, import that module into Ghidra with a
matching SH-2 big-endian language. Confirm available language IDs in the installed
Ghidra version before scripting headless imports. Keep overlapping overlays in
separate programs until their lifetime and mapping are understood.

Preserve boot, title, menus, one reproducible gameplay route, loading transitions,
music/effects, and input as the runtime baseline. Compare generated execution
against that baseline when an AOT emitter exists. No native port is established
by the initial extraction workflow.

Keep extracted binaries, disassembly and Ghidra projects under ignored `out/`
or another private workspace. Public tooling contains no game or BIOS bytes.
