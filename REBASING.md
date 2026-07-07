# Fork maintenance / rebasing policy — OSRM `/tree` plugin (Highway Mode 2)

This is a **thin fork** of upstream OSRM: one new engine plugin plus its registration
plumbing. The value of the fork is the plugin; the cost of the fork is keeping it building
against upstream. This document is the policy for paying that cost as rarely and as safely
as possible.

**Default stance: stay pinned.** We do not track upstream `master`. We pin to a known-good
base commit and only rebase when a concrete trigger (below) forces it.

## Pinned base

```
6678ebc5a  feat(tools): add --output option to osrm-contract, osrm-customize, osrm-partition (#7646)
```

The `feature/tree-plugin` branch is exactly this base + one commit (`Added a tree plugin for
first version of HM2 PoC.`). Confirm before any rebase:

```
git merge-base --is-ancestor 6678ebc5a HEAD && echo "base is an ancestor"
git log --oneline 6678ebc5a..HEAD        # should be only the plugin commit(s)
```

## What the plugin touches

Keep this list current — it is the entire rebase surface. A rebase conflict can only occur
in one of these files.

**New files (no upstream counterpart — never conflict, but must still compile):**
- `include/engine/api/tree_parameters.hpp` — `TreeParameters : BaseParameters` (coordinate, bearing, `hard_cap_m`, `debug`).
- `include/engine/plugins/tree.hpp` — plugin header.
- `src/engine/plugins/tree.cpp` — **the only file with real logic** (~600 lines: bearing snap + motorway post-filter, recursive walk, `selectContinuation`, `traverseRamp`, `parseDestinationRef`/`firstMotorwayRef`/`isMotorwayRef`, component-wise same-road suppression).
- `include/server/api/tree_parameter_grammar.hpp` — request grammar (reuses `base_grammar`; parses `hard_cap_m=`, `debug=`).
- `include/server/service/tree_service.hpp`, `src/server/service/tree_service.cpp` — request plumbing (mirror of `nearest_service`).

**Edited upstream files (mechanical registration — the likely conflict sites):**
- `src/server/service_handler.cpp` — `service_map["tree"] = …`.
- `src/server/api/parameters_parser.cpp` — `parseParameters<TreeParameters>` specialization + grammar include.
- `include/engine/engine.hpp` — `Tree()` on the interface + `Engine` + a `TreePlugin` member.
- `include/osrm/osrm.hpp`, `src/osrm/osrm.cpp`, `include/osrm/osrm_fwd.hpp` — public `OSRM::Tree` facade (JSON + `ResultT`).

Node/Python bindings are deliberately **not** touched — the plugin is served through
`osrm-routed` only, minimising surface.

## Known internals coupling (what an upstream refactor can silently break)

The plugin reaches past `BaseDataFacade` into OSRM internals in three places. Upstream can
change any of these without a compile error, so the **regression harness is the real safety
net**, not the build.

1. **MLD-only facade access via `dynamic_cast` (SPIKE-REPORT §5).** Graph adjacency
   (`GetAdjacentEdgeRange`, `GetTarget`, `GetEdgeData`, `IsForwardEdge`) lives on
   `AlgorithmDataFacade<MLD>`, not the base facade, so the plugin cross-casts the base facade
   to the MLD algorithm facade and fails loudly (`NotImplemented`) on a CH dataset. If upstream
   reorganizes the facade class hierarchy the cross-cast can start returning null → every
   `/tree` call degrades to `NotImplemented`. The service is therefore **MLD-only by design**;
   never build/serve this with `--algorithm ch`.
2. **`TurnType` numeric ids (SPIKE-REPORT §7.3).** Branch/continuation decisions key on the
   raw `TurnType::Enum` integers, not the stringified turn (`OffRamp=6`, `Fork=7` qualify;
   `Suppressed=18`/`NoTurn=17`/`NewName=1`/`Continue=2` are continuation). Upstream renumbering
   or reordering the enum would silently mis-classify diverges. Re-check these ids after any
   guidance/turn-instruction change upstream.
3. **`GetDestinationsForID` merged-string format (SPIKE-REPORT §7.2).** There is no separate
   `destination:ref`; the name table packs it into one string (`"A27: Almere, Utrecht"`,
   `"RING A16"`, or a bare place). `parseDestinationRef()` owns that format zoo. If upstream
   changes how destinations are stored/serialised, this parser is where it breaks.

## Rebase procedure

Only when a trigger below fires. Do it deliberately, in order:

1. **Pick the new base.** The smallest upstream commit that carries the fix/format change you
   need — not `master` HEAD. Read its changelog for touches to `engine/plugins`, guidance /
   `TurnType`, the data facade hierarchy, and the name/destinations table.
2. **Rebase the plugin commit** onto it:
   ```
   git rebase --onto <new-base> 6678ebc5a feature/tree-plugin
   ```
   Resolve conflicts only in the mechanical registration files listed above. Then update the
   pinned base commit at the top of this file and in `docker/Dockerfile.hm2`'s comments.
3. **Rebuild the fork** (vcpkg manifest mode; SPIKE-REPORT §2). Re-run `cmake --preset` before
   the build — CMake GLOBs sources without `CONFIGURE_DEPENDS`, so the new `.cpp` is only seen
   after a fresh configure:
   ```
   cmake --preset ci-linux && cmake --build --preset ci-linux   # or ci-macos-arm64 locally
   ```
   Also re-verify the three coupling points above compile *and* behave (a clean build proves
   nothing about items 2 and 3, which are runtime).
4. **Re-run the data pipeline on NL** (a new base can change the `.osrm` on-disk format, so the
   old dataset may be unreadable):
   ```
   REGION=europe/netherlands OSRM_DATA_DIR=./data OSRM_PROFILE=profiles/car.lua docker/pipeline.sh
   # or locally: osrm-extract -p profiles/car.lua … && osrm-partition … && osrm-customize …
   ```
5. **Run the regression harness against the rebuilt server — this is the gate:**
   ```
   ./build/osrm-routed --algorithm mld -p 5050 data/current/data.osrm &
   OSRM_URL=http://127.0.0.1:5050 python3 tools/hm2_spike/regression.py
   ```
   It must exit 0. A green build with a red harness means an internals-coupling break
   (items 1–3) — do not ship. If a data refresh legitimately moved a snap, update the pinned
   coordinate in `regression.py` and re-verify; never loosen an assertion to pass.

## When to rebase vs stay pinned

**Rebase (accept the cost):**
- **Upstream security fix** in a code path we actually run (routing engine, HTTP server,
  osmium/extract, a vendored parser). Track upstream security advisories; a CVE in the serving
  path is the main reason we'd move.
- **OSM data-format change** that our pinned `osrm-extract` can no longer parse, or that
  upstream fixed in a newer extractor (rare, but pbf/tag-schema shifts happen).
- A **vcpkg baseline / toolchain** move we can't avoid (e.g. a dependency dropped from the
  pinned baseline). Prefer bumping only `vcpkg-configuration.json` first; a full rebase is a
  last resort.

**Stay pinned (ignore upstream):**
- New OSRM features, performance work, or refactors we don't need. Every rebase risks the
  three coupling points; churn we don't benefit from is pure downside.
- Upstream changes to Node/Python bindings, CH, or profiles we don't ship.
- Cosmetic / CI-only upstream changes.

Rule of thumb: **the plugin is small and the coupling is sharp — rebase for security and data
compatibility, not for features.** Every rebase ends with a green `regression.py`, or it isn't
finished.
