# `/tree` plugin — maintainer cheat-sheet

Onboarding for a fresh engineer/agent maintaining the Highway-Mode-2 motorway-tree plugin
(`src/engine/plugins/tree.cpp`). This is the distilled "how it works and why"; the investigation
narrative and evidence live in `SPIKE-REPORT.md` (§ references throughout) and
`tools/hm2_spike/evidence/`. Read this first, then that.

**What the endpoint does.** `GET /tree/v1/driving/{lng},{lat}?bearings=<heading>,<tol>` snaps to the
motorway the driver is on and returns that motorway plus every motorway↔motorway junction
(knooppunt) reachable ahead, recursively, ignoring local exits (afritten). Response is a nested
tree of `{ref, start_offset_m, length_m, polyline, branches:[{junction, route}]}`. `hard_cap_m`
bounds total corridor length; `debug=true` adds `dropped_stubs[]`.

---

## 1. Architecture in brief

Four layers, all in `tree.cpp`:

- **`walkOne`** — walks ONE segment from a start node along the mainline until the ref genuinely
  ends, a cap/cycle stops it, or it reaches a knooppunt. Emits the route JSON, the child WorkItems
  it would spawn (qualifying junctions), and metadata (`bad_ending`, `rejoined_ancestor`,
  `ended_by_cycle`, `direction_forks`, `end_coord`). Contains the mainline-continuation and
  same-ref-suppression logic. Calls `traverseRamp` to probe each candidate branch's ramp.
- **`traverseRamp`** — from a junction, follows a ramp/connector (up to `MAX_RAMP_HOPS`) to the
  motorway it lands on, so `walkOne` can decide if the branch is a real crossing motorway. Returns
  the landing node + the arms there.
- **`expandSegment`** — wraps `walkOne` for one WorkItem: runs the primary walk; if it's a bad ramp
  landing, runs the fork-backtracking retry (keep-longest alternative); spawns crossing **direction
  siblings** from `direction_forks` with per-spawn dedup; then **probes `parallel_forks`** (same-road
  parallel carriageways) and lifts the motorway junctions the through carriageway skipped onto the
  segment (§S3-fix9).
- **Best-first driver** (bottom of the plugin's `HandleRequest`) — a `priority_queue` keyed on
  `(priority_class, start_offset, id)`, **class-weighted** (§S3-fix9): class 0 = the root and every
  first-level spur head (guaranteed at any distance), class 1 = a spur's subtree within
  `SPUR_RESERVED_DEPTH_M` of its head, class 2 = deep fill nearest-first (the old behaviour; NOT
  depth-first — see §S3-fix). Pops WorkItems, calls `expandSegment`, enqueues its children and
  siblings (`MAX_SEGMENTS=400`, `MAX_DEPTH=10` caps), then reassembles the flat `SegmentResult` list
  into the nested tree via a `parent_id` map (`assemble` lambda), with the per-parent dedup pass.
  Parent-before-child holds structurally (a child is only enqueued after its parent is recorded).

Facade: the driver `dynamic_cast`s `BaseDataFacade` → `AlgorithmDataFacade<MLD>` (`MLDFacade`) for
graph adjacency. **MLD only** — see §3.

---

## 2. Behavioral rules (each with its WHY and regression case)

Regression cases live in `tools/hm2_spike/regression.py` (`run_*_case` fns). Run it after EVERY
change; extend it for EVERY fix.

- **Continuation by numeric TurnType** (`selectContinuation` L271, `isContinuationTurn` L64). A pure fork has NO `Continue` edge — the mainline leaves via a `Fork`/`Suppressed`/`NoTurn`
  arm. Rank: (1) a guidance continuation turn (`NoTurn`/`Suppressed`/`NewName`/`Continue`); (2) else
  the fork arm whose parsed ref component-matches the current ref; (3) else the ref genuinely ends →
  segment stops, both arms become children. Classify on the TurnType enum, never on the stringified
  instruction — `Suppressed`/`NoTurn` stringify to INVALID (see §3). Cases: `run_qualify_case`,
  `CONCURRENCY_CASE` (A2/A67, zero toward-self junctions).

- **Same-ref suppression — 3-way, component-wise** (in `walkOne`, the suppression block ~L459-471).
  A branch arm is suppressed (not emitted) if its signage ref shares a component with the mainline
  ref, split on `;` for concurrencies (`A4;E19` matches `A4`). Three checks: branch signage,
  current ref, continuation. WHY: parallel-carriageway splits and the not-taken direction of the
  current road would otherwise emit spurious "via A2" junctions. Case: `SAMEREF_CASE` (A4 Den Hoorn).
  **Do NOT gate on the arm's own EDGE ref** — the A27 off-ramp at Everdingen carries a *stale edge
  ref of `A2`* (Stage 1 finding 7.4), which would wrongly kill the real A27 branch. Signage only.
  See §S3-fix5 "Not shipped — #1". **The continuation check uses the continuation arm's RAW ref only,
  NEVER its destination signage** (§S3-fix8). At a knooppunt gore the through-lane's `destination`
  sign merges the mainline with the *crossing* road (A50's through-sign reads `A50, A1: Zwolle, …,
  Amsterdam`); folding those destination refs in makes the crossing motorway's own off-ramp look like
  a same-road split and drops one direction of the crossing (A1-east at KP Beekbergen, A28-north at KP
  Hattemerbroek). The raw ref identifies the road we stay on; destinations advertise reachable roads.
  Case: `CLOVERLEAF_CASE` (A50 north, both directions at both cloverleaf crossings).

- **Motorway containment guard** (`isMotorwayNode`/`isMotorwayRef`, L106/L180; enforced in the walk
  loop). Only continue onto motorway-class edges; a branch must never escape onto an N-road/city
  street. WHY: the tree is a motorway abstraction; leaking onto local roads corrupts the corridor.
  Case: `CONTAINMENT_CASE` (asserts `non_motorway_m == 0`).

- **Fork-backtracking landing retry** (`expandSegment`, `RETRY_MIN_M=5000`, `MAX_LANDING_RETRIES=6`).
  If the primary walk is a bad ramp landing (short, childless, at a complex interchange), re-walk
  from each alternative fork arm at the landing and keep the longest that reaches a real road. WHY:
  A12-east was amputated at Prins Clausplein because the first arm dead-ended. Case: `RETRY_CASE`
  (A12-east recovers >20 km). See §S3-fix4.

- **Multi-candidate root snap** (`MAX_ROOT_CANDIDATES=3`). The initial snap tries up to 3 candidate
  motorway edges; if the primary self-snaps badly (e.g. inside a knooppunt stack), retry from the
  next. WHY: a bad self-snap otherwise loses the whole corridor. Case: `ROOT_RETRY_CASE` (inside the
  Prins Clausplein stack, root still snaps through to a >20 km motorway). See §S3-fix4b.

- **Parallelbaan rejoin-drop** (`WalkOutput.rejoined_ancestor`; driver drops childless rejoins).
  A branch that stops because a forward motorway edge merges back onto an *ancestor* path, AND spawned
  no children, is a parallel carriageway of a road already in the tree — dropped (its stations are
  within metres of the kept polyline). Branches that spawned children stay. WHY: A4 parallelbaan
  sections showed same-road stubs ending at the rejoin. Case: `PARALLELBAAN_CASE` (stubs dropped, max
  stray < 1 km — over-drop guard). See §S3-fix5.

- **Crossing-directions spawn + two dedup guards** (§S3-fix6). `walkOne` records same-ref arms it
  suppresses within `CONNECTOR_M=3000` leading to an unvisited motorway node → `direction_forks`
  (these are the OTHER directions of a crossing road). For the PRIMARY branch only, `expandSegment`
  re-walks each and spawns a sibling if it's a genuine second direction (not cycle/rejoin, length
  ≥ `MIN_DIRECTION_M=1500`), `toward` from that arm's signage, capped at `MAX_CROSSING_DIRECTIONS=3`;
  siblings do not spawn siblings. WHY: at Prins Clausplein only A12-east was emitted; A12-west was
  swallowed by same-ref suppression. Both directions must show. **Dedup is essential** (un-deduped:
  x6/x8 identical branches that shred the segment budget recursively). Two guards, keyed on where a
  branch *ends* (`SAME_DIRECTION_M=500`; a real crossing's two directions end tens of km apart,
  duplicates end at the same node): (1) per-spawn — a candidate ending within 500 m of the primary or
  a kept sibling is skipped; (2) per-parent at assembly — a child dropped if a nearer same-ref child
  ends within 500 m (catches sibling-vs-child collisions across spawn paths). Case: `DIRECTIONS_CASE`
  (both A12 east lng>4.48 and west lng<4.36). Note: 6→7 root breadth on A2-south is the recovered
  A15-west at Deil — a coverage win, NOT drift (`BREADTH_CASE` asserts root ≥ 4).

- **Per-direction junction signage** (§S3-fix7). A branch's junction `toward` is captured at spawn
  from the mainline arm's signage. Where a knooppunt is entered via a **shared slip road** that splits
  into per-direction links (Prins Clausplein A4→A12: gantry `A12;Den Haag;Voorburg;Utrecht;Zoetermeer`
  before the split), that captured signage is the *union* of both directions. `walkOne` records
  `refined_toward` — the signage of the arm it actually follows at the first same-ref direction split
  (where a `direction_fork` is recorded) — and the driver overwrites the **primary** segment's junction
  `toward` with it (siblings already carry their own arm's signage from `df.second`). Each direction
  then shows its own destinations (east: Utrecht/Zoetermeer; west: Den Haag/Voorburg). Case:
  `SIGNAGE_CASE`. WHY: the merged union mislabels which cities a direction actually leads to.

- **Complete ancestor visited set — no parallelbaan re-expansion** (§S3-fix7). The per-path `visited`
  set a child is spawned with only covers the ancestor **up to the spawn junction**. Best-first walks
  every ancestor to completion before a child pops, so the driver completes each item's `visited` with
  every ancestor's FULL `path_nodes` before walking it. WHY: a parallel carriageway that leaves the
  mainline and rejoins it *past* the spawn point (the A4 parallelbaan mis-signed `A13` at KP Ypenburg —
  100% overlap with root A4, children duplicating root's) otherwise never sees the rejoin and
  re-expands a ~255 m-shifted copy of the whole tree; with the complete set it hits a visited ancestor
  node at the rejoin, stops childless, and is dropped by the rejoin rule. Only affects a branch
  rejoining **its own ancestor chain** — the same road via a *different* parent (design §12) is kept.
  Case: `DUP_CASE` part (a).

- **Same-direction overlap dedup — twin entry ramps** (§S3-fix7). The end-coordinate dedup
  (`SAME_DIRECTION_M`) misses two qualifying entry ramps onto ONE crossing direction (KP Muiderberg's
  twin A6 lanes): same-ref, coincident start, but ending km apart when the copies truncate
  differently. The per-parent assembly dedup also compares **arc-distance fingerprints** (`dir_samples`
  at 1/2/4/…/64 km via `sampleAlong`): same-ref children with ≥ `DIR_OVERLAP_FRAC` of shared samples
  within `DIR_MATCH_M` are one corridor, keep the nearer. Opposite directions diverge within 1 km (≈0
  overlap) and are both kept. Start-coordinate dedup is unusable — legit opposite directions share a
  start. Case: `DUP_CASE` part (b).

- **Parallelbaan junction lifting** (§S3-fix9). Where a motorway splits into a through **hoofdbaan**
  and a **parallelbaan** (both same ref — the NL parallelstructuur at Den Bosch, Eindhoven, Utrecht…),
  `selectContinuation` follows the hoofdbaan (destination-signed) and the parallelbaan arm is dropped
  (empty destination → non-qualifying; raw ref same → suppressed). But motorway junctions can sit
  **only on the parallelbaan** (the A59 at KP Empel/Hintham off the A2 through 's-Hertogenbosch), so
  they never surfaced from a distance. `walkOne` records a `parallel_fork` for a non-continuation
  `Fork`/`OffRamp` arm that does NOT qualify as a different motorway, leads to unvisited motorway, and
  shares the current road on its **raw ref** — gated on `!qualifies` so the "gore ref = mainline ref"
  stale-edge case (A27 off-ramp reads raw ref `A2` but destination `A27`) is excluded, and limited to
  **depth ≤ 1** (the current road + first spurs; bounds cost). `expandSegment` probes each from the
  split, seeded with the through walk's `path_nodes` so it stops at the rejoin (`PARALLELBAAN_PROBE_M`
  cap), and appends its qualifying **children** to the segment. The parallelbaan itself is NEVER
  emitted (same road → would break same-ref); only its crossing children are, de-duped at assembly
  against the through carriageway's own. Case: `HINTHAM_CASE` (A59-east off the A2 at any distance).

- **Class-weighted budget ordering** (§S3-fix9, the 2026-07-08 ruling "prioritise the current road and
  the first spurs"). The best-first heap orders by `(priority_class, start_offset)` not `start_offset`
  alone: class 0 = the root chain + every first-level spur head (a slot at ANY distance), class 1 = a
  spur's subtree within `SPUR_RESERVED_DEPTH_M` (25 km) of its head, class 2 = deep fill nearest-first.
  WHY: pure nearest-first sinks the 400-segment budget into near spurs' dense subtrees and drops far
  first-level spurs (A76/A79 in Limburg, ~120-130 km, off the A2 south). Mirrors the BE stage-A prune so
  the two budget layers agree. Parent-before-child is structural, unaffected. Case: `BUDGET_PRIORITY_CASE`.

- **`current_ref` adopts a Merge-renumber** (§S3-fix10, the ref-update site in `walkOne` ~L714-728). The
  segment's tracked road identity `current_ref` starts as `item.reported_ref` and is adopted from the
  continuation arm's RAW ref on **`NewName` OR `Merge`** — a `Merge` is a spur road ending and merging onto a
  differently-numbered motorway (A59 ending at KP Paalgraven and merging onto the A50 north), gated on the
  merged-onto ref being a motorway ref not already sharing `current_ref`. WHY: without it `current_ref` stays
  frozen at the snap ref (`A59`) after the road physically renumbers; guidance-turn continuations
  (`NoTurn`/`Suppressed`, ref-independent) hide it, but at a downstream same-ref carriageway split (a
  parallelstructuur/bridge `Fork` with no guidance arm) `selectContinuation`'s rank-2 ref match fails
  (`A59` ≠ `A50`), the ref "ends", and the driver's own road is demoted to a merged-signage first-level branch
  while the root dies. **Do NOT extend this to `NoTurn`/`Suppressed`**: those carry the *crossing* road's stale
  ref at a gore (§S3-fix8) and the *parent* mainline's stale ref on a branch's first nodes (the A27 off-ramp at
  Everdingen reads `A2`, §S3-fix5) — adopting them re-opens both. `Merge` is a distinct turn type from those.
  Case: `VALBURG_CASE` (root follows A59→A50 north past KP Valburg; no first-level A50; A15/A12 first-level).

Other cases: `RING_CASE` (cycle termination), `OFF_MOTORWAY_CASE` (→ `NoSegment`),
`run_junction_names` (every junction/branch name is `""`).

---

## 3. Sharp edges (each has burned someone)

| Edge | Reality |
|---|---|
| Merged destinations string | `GetDestinationsForID` returns ONE merged string per edge, mixing `ref`, `ref: name`, and no-colon forms (e.g. `A12;Utrecht;Den Haag`). Parse defensively; don't assume `ref: name`. |
| Gore ref = mainline ref | At a knooppunt gore, `NoTurn`/`Suppressed` continuations carry the *crossing* motorway's ref, and off-ramps can carry a stale ref of the road they left (Everdingen A27-ramp reads `A2`). Trust signage, not edge ref, for branch identity. |
| `Suppressed`/`NoTurn` stringify invalid | The stringified turn instruction for these is not a usable label; classify on the numeric `TurnType` enum only (`GetTurnInstructionForEdgeID`). |
| MLD-only facade cast | The driver `dynamic_cast`s to `AlgorithmDataFacade<MLD>`. Serve with `--algorithm mld`. A CH/other facade → null cast → the plugin can't read adjacency. |
| Connector = offset gap, not a node | NL connectors are effectively 0-length between edge nodes; a "connector" is an offset window (`CONNECTOR_M`), not a distinct ramp geometry. Direction-fork detection keys on the offset gap. |
| `hard_cap_m` is segment-granular | The cap stops *new segment expansion*; an in-flight branch that crosses the cap still completes. A branch with `start_offset + length ≈ cap` is cap-truncated, not a bug — exclude it when auditing for stray/rejoin stubs (bit the parallelbaan check once). |

---

## 4. Working on this code

**Build** (macOS arm64; deps build from source once into vcpkg cache):
```bash
export VCPKG_ROOT=~/vcpkg
cmake --preset ci-macos-arm64        # only needed once, or after ADDING new .cpp files (GLOB, no CONFIGURE_DEPENDS)
cmake --build build --target osrm-routed     # normal iterate: rebuild just the server
```

**Pipeline** (rebuild the NL graph only if the profile/extract changes — normally you don't):
```bash
./build/osrm-extract   -p profiles/car.lua data/netherlands-latest.osm.pbf
./build/osrm-partition  data/netherlands-latest.osrm
./build/osrm-customize  data/netherlands-latest.osrm
```

**Serve** (port 5000 is macOS AirPlay — use 5050):
```bash
./build/osrm-routed --algorithm mld -p 5050 data/netherlands-latest.osrm
```

**Test — non-negotiable loop:** after every change, `python3 tools/hm2_spike/regression.py` (must be
all-green; currently 51 assertions across the `run_*_case` fns). For every bug you fix, ADD a case
that fails before and passes after — that is how each rule above earned its line. Use `debug=true`
to inspect `dropped_stubs[]`.

**Evidence:** `tools/hm2_spike/evidence/` holds captured knooppunt/afrit JSON (`KP_*`, `Afrit_*`) and
stage snapshots — the ground-truth fixtures the investigation was built on. `SPIKE-REPORT.md` §§
S2–S3-fix6 carry the full reasoning behind every rule; cross-reference there rather than
re-deriving.

**Constants worth knowing** (top of `tree.cpp`): `MAX_SEGMENTS=400`, `MAX_DEPTH=10`,
`MAX_RAMP_HOPS=15`, `RETRY_MIN_M=5000`, `MAX_LANDING_RETRIES=6`, `MAX_ROOT_CANDIDATES=3`,
`MAX_CROSSING_DIRECTIONS=3`, `MIN_DIRECTION_M=1500`, `CONNECTOR_M=3000`, `SAME_DIRECTION_M=500`,
`DIR_MATCH_M=300`, `DIR_OVERLAP_FRAC=0.75` (arc-distance overlap dedup), `DIR_SAMPLE_M`
(1/2/4/…/64 km fingerprint distances), `MAX_PARALLEL_FORKS=12` / `PARALLELBAAN_PROBE_M=12000`
(parallelbaan junction lifting, §S3-fix9), `SPUR_RESERVED_DEPTH_M=25000` (class-weighted budget:
per-spur reserved subtree depth, §S3-fix9).

**Region note:** the motorway ref regex (`isMotorwayRef`) is tuned for NL/EU (`^[AE]\d+`). UK (`^M\d+`)
and other launch regions need per-region patterns — make it configurable before those launches
(open question #6). All rules above are validated against NL only; reconfirm signage density for
DE/UK (open question #9).
