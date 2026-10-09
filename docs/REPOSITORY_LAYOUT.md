# Repository boundaries

The SaturnRecomp fork uses separate branches for upstream changes and analysis
tooling. Panzer Dragoon investigation lives in an independent sibling repository.

| Branch | Purpose | Initial boundary |
| --- | --- | --- |
| `main` | Local upstream baseline, tracking `upstream/main` | `26c9715` |
| `build-portability` | Build, portability and associated runtime fixes/tests | `bb8dd2d` |
| `analysis-tooling` | General tools built on portability | `545b6b4`, then tooling-only changes |
| `panzer-runtime` | Preserved experimental native implementation | `a723587` |

`cross-platform-build` is retained at `545b6b4` as the original branch.
No existing commits were rewritten. `origin` is the user's SaturnRecomp fork;
`upstream` is `sonsegajp/SaturnRecomp`. The portability branch contains seven
commits beyond the upstream baseline and excludes executable-analysis tooling
and Panzer native routines.

## Upstream contributions

Prepare build/portability PRs from `build-portability` against
`sonsegajp/SaturnRecomp:main`. Keep future upstream changes on that branch or
on focused branches based on upstream. Bring accepted portability changes into
`analysis-tooling` by merging or rebasing deliberately. Do not merge the
tooling or experimental branches into a portability PR.

Inspect the exact proposed scope with:

```sh
git log --oneline upstream/main..build-portability
git diff --stat upstream/main...build-portability
```

## Panzer Dragoon project

The independent project is `../PanzerDragoonRecomp`. It owns game-specific
scripts, handwritten verification models, findings and the experimental native
implementation as a patch. Its `dependencies.json` records the runtime revisions
needed for reproduction. Scripts select this sibling runtime by default or use
`SATURNRECOMP_ROOT` for another checkout.

The migration copied the original ignored `out/analysis/panzer-dragoon-usa`
workspace without removing it. The copy includes private local evidence, but
only explicitly reviewed source and notes are tracked by the Panzer repository.
Extracted game code, firmware, decompiler exports, captures and generated output
remain local. Neither repository's Git history backs up ignored evidence.

The native experiment remains available in `out/panzer-runtime`, a Git worktree
on `panzer-runtime`. Its build directory is `out/panzer-runtime-build`.
This keeps native validation independent of the active tooling checkout. The
Panzer-owned patch reproduces the experiment on its pinned tooling revision.
It is a transitional integration mechanism; there is no general native-plugin
interface yet.

Analysis scripts still tailored to Panzer remain in that project. Extract
reusable portions into the tooling branch as their inputs and interfaces become
independent of the title. A separate toolkit repository can be introduced later
without changing the portability PR boundary.
