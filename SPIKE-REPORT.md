# Highway Mode 2 — OSRM `/tree` plugin spike report

**Branch:** `feature/tree-plugin` (pinned at HEAD `6678ebc5a`, do not rebase for the spike)
**Platform:** macOS (Apple Silicon, arm64), Xcode 26.5 / Apple clang 21, CMake 4.2, vcpkg manifest mode
**Algorithm:** MLD (the plugin requires it — see §5)
**Verdict:** **The premise holds.** OSRM's edge-based graph exposes, at every NL knooppunt tested, (a) a turn classification that cleanly separates the motorway-to-motorway ramp/fork from the mainline continuation, and (b) populated `destination:ref` signage that names the branch motorway. Ordinary local exits (afritten) are equally cleanly rejected: their signage refs are N-roads, never `A`/`E`. Several spec-vs-reality mismatches were found (§7) — none fatal, all mechanical.

---

## 1. What was built

A skeletal `GET /tree` service alongside `route`/`nearest`/…. It snaps the input coordinate with the mandatory bearing filter, picks the carriageway whose onward heading matches the request, and from that edge-based node dumps **every outgoing edge** of the edge-based graph: the turn classification (`TurnType`), the target segment's road classes, name/ref, and the `GetDestinationsForID` signage — plus geometry endpoints for orientation.

It does **not** yet walk or branch a tree. That is deliberate (Stage 1 spike): the goal was only to prove the underlying data exists and is good enough, before writing the traversal (Stages 2–3).

---

## 2. Build — steps that worked

Dependencies are managed by **vcpkg in manifest mode** (`vcpkg.json`); there is no manual brew list to install. The only host tooling needed beyond the Xcode toolchain was **ninja** and a **bootstrapped vcpkg**:

```bash
brew install ninja
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
export VCPKG_ROOT=~/vcpkg

cd osrm-backend
cmake --preset ci-macos-arm64        # first run builds all deps from source (~long, one-time)
cmake --build --preset ci-macos-arm64
```

Notes:
- The repo ships a macOS preset **`ci-macos-arm64`** (triplet `arm64-osx`) — use it directly; the README only documents the Linux preset.
- The first `cmake --preset` compiles the whole dependency set (boost, tbb, expat, bzip2, lua, libosmium, protozero, flatbuffers, sol2, rapidjson, vtzero, …) from source into vcpkg's cache. Budget for this once; rebuilds reuse the binary cache.
- Build binaries land in `build/`: `osrm-extract`, `osrm-partition`, `osrm-customize`, `osrm-routed` (+ tools). The only build warnings were benign `ld: ignoring duplicate libraries` for libz/libbz2 — pre-existing, unrelated to the plugin.
- **CMake globs sources** (`file(GLOB … src/engine/**/*.cpp …)` without `CONFIGURE_DEPENDS`), so after adding new `.cpp` files you must **re-run `cmake --preset`** before `cmake --build`, or they won't compile.

---

## 3. Pipeline — MLD processing of the NL extract

Input: `data/netherlands-latest.osm.pbf` (Geofabrik NL, 1.39 GB).

```bash
./build/osrm-extract   -p profiles/car.lua data/netherlands-latest.osm.pbf
./build/osrm-partition  data/netherlands-latest.osrm
./build/osrm-customize  data/netherlands-latest.osrm
./build/osrm-routed --algorithm mld -p 5050 data/netherlands-latest.osrm
```

- Extract peak RAM ~5.3 GB; ~6.76 M edge-based edges; whole pipeline is a few minutes on this laptop.
- **Port 5000 is taken by macOS (AirPlay Receiver)** → serve on `5050`.
- Sanity route Amsterdam→Utrecht returns `code Ok`, 44.3 km / 44.7 min — MLD serving correctly across NL.

---

## 4. Files created / changed

**New (plugin + wiring):**
- `include/engine/api/tree_parameters.hpp` — `TreeParameters : BaseParameters` (coordinate + bearing).
- `include/engine/plugins/tree.hpp`, `src/engine/plugins/tree.cpp` — the plugin (the only file with real logic; ~180 lines).
- `include/server/api/tree_parameter_grammar.hpp` — reuses `base_grammar` options.
- `include/server/service/tree_service.hpp`, `src/server/service/tree_service.cpp` — request plumbing (mirror of `nearest_service`).

**Edited (registration, mechanical — mirror of `nearest`):**
- `src/server/service_handler.cpp` — `service_map["tree"] = …`.
- `src/server/api/parameters_parser.cpp` — `parseParameters<TreeParameters>` specialization + grammar include.
- `include/engine/engine.hpp` — `Tree()` on the interface + `Engine` + a `TreePlugin` member.
- `include/osrm/osrm.hpp`, `src/osrm/osrm.cpp`, `include/osrm/osrm_fwd.hpp` — public `OSRM::Tree` facade methods (JSON + `ResultT`).

**Tooling / evidence:**
- `tools/hm2_spike/probe.py` — evidence collector (routes a corridor through each junction, places probes ~300/600/1000 m upstream of the diverge, hits `/tree`).
- `tools/hm2_spike/evidence/*.json` + `console_summary.txt` — raw dumps.

No commits; working tree left dirty as instructed. Node/Python bindings were intentionally **not** touched (the spike is served through `osrm-routed` only) to keep the surface against OSRM internals minimal.

---

## 5. How the plugin reaches the graph (the one non-obvious internal)

`RoutingAlgorithmsInterface::GetFacade()` returns only `BaseDataFacade`, which exposes node/edge **attributes** (name, ref, destinations, classes, geometry, turn instruction) but **not** graph adjacency. The edge-based graph accessors (`GetAdjacentEdgeRange`, `GetTarget`, `GetEdgeData`, `IsForwardEdge`) live on `AlgorithmDataFacade<MLD>`. The plugin therefore `dynamic_cast`s the base facade to the MLD algorithm facade (both are sibling bases of the concrete `ContiguousInternalMemoryDataFacade<MLD>`, so the cross-cast is valid) and fails loudly with `NotImplemented` on a CH dataset. This keeps the plugin from adding any new virtual to the routing interface — minimal rebase surface — at the cost of being MLD-only, which is what we run anyway.

Traversal per snapped node mirrors the MLD router's own edge loop (`routing_base_mld.hpp`):

```cpp
for (const auto edge : mld.GetAdjacentEdgeRange(node))
    if (mld.IsForwardEdge(edge)) {
        const auto turn_id = mld.GetEdgeData(edge).turn_id;   // key for guidance data
        const auto target  = mld.GetTarget(edge);             // edge-based node = a directed segment
        const auto turn    = facade.GetTurnInstructionForEdgeID(turn_id);
        // target attributes via GetNameIndex(target) -> GetRefForID / GetDestinationsForID / ...
    }
```

`GetOutDegree` counts both directions; `IsForwardEdge` filters the reverse/u-turn edges, which is why `out_degree` is 3–4 but only 2 forward edges surface at a diverge.

---

## 6. Evidence — per junction

Method: for each case a corridor is routed so OSRM goes mainline→branch through the junction; the diverge maneuver's location is taken as the gore; `/tree` is probed ~300/600/1000 m upstream on the mainline with the travel bearing. Full JSON in `tools/hm2_spike/evidence/`.

### Raw sample — KP Everdingen (A2 northbound, probe 467 m before the gore)

```json
{
  "code": "Ok",
  "snapped": { "requested_bearing": 328, "segment_bearing_forward": 327,
               "segment_bearing_reverse": 147, "chose_forward_direction": true },
  "start": { "ref": "A2", "classes": ["motorway"], "edge_based_node_id": 19768 },
  "out_degree": 4,
  "outgoing_edges": [
    { "turn_type": "off ramp", "turn_type_id": 6,  "turn_modifier": "slight right",
      "target": { "ref": "A2", "classes": ["motorway"],
                  "destinations": "A27: Almere, Utrecht" } },
    { "turn_type": "invalid",  "turn_type_id": 18, "turn_modifier": "straight",
      "target": { "ref": "A2", "classes": ["motorway"],
                  "destinations": "A2: Amsterdam, Utrecht-West, Vianen" } }
  ]
}
```

### Verdict table

| Junction | Snap (ref, class) | Branch edge — turn (id) | Branch `destinations` (→ `destination:ref`) | Continuation — turn (id) | `destination:ref` populated? | Ramp vs local distinguishable? |
|---|---|---|---|---|---|---|
| **KP Everdingen** A2→A27 | A2, motorway | off ramp (6) | `A27: Almere, Utrecht` → **A27** | invalid/Suppressed (18) straight | **Yes** | **Yes** |
| **KP Deil** A2→A15 | A2, motorway | off ramp (6) | `A15: Rotterdam, Gorinchem, Nijmegen, Tiel` → **A15** | invalid/Suppressed (18) straight | **Yes** | **Yes** |
| **KP Badhoevedorp** A4→A9 | A5, motorway¹ | off ramp (6) | `A9: Alkmaar, Haarlem, Amstelveen, Badhoevedorp` → **A9** | invalid/Suppressed (18) straight | **Yes** | **Yes** |
| **KP Ridderkerk** A15→A16 | A15, motorway | fork (7) | `RING A16` → **A16**² | fork (7), other arm `destinations:""` | **Yes** | **Yes** (both arms Fork) |
| Afrit Culemborg (A2) | A2, motorway | off ramp (6) | `N320: Culemborg` → **N320** | invalid/Suppressed (18) | n/a (correctly non-motorway) | **Yes — rejected** |
| Afrit Hoofddorp (A4) | A4, motorway | off ramp (6) | `N201: Hoofddorp, Aalsmeer` → **N201** | invalid/Suppressed (18) | n/a | **Yes — rejected** |
| Afrit Leiden (A4) | A4, motorway | fork (7) | `Ring N14: Leidschendam, Wassenaar (N44), Den Haag-Noord` → **N14** | fork (7) arm `A4: Amsterdam, Leiden` | n/a | **Yes — rejected** |

¹ Badhoevedorp is a multi-motorway stack; the mainline arm the router follows through it is signed **A5** (Verlengde Westrandweg spur), with the **A9** branch correctly signed. The interchange is identified regardless of which mainline arm you approach on — the branch's `destination:ref` is what matters, and it is present.
² Ridderkerk's A16 arm is signed `RING A16` (no colon) — see §7.4 for why the branch matcher must handle this format.

**Bottom line:** `destination:ref` coverage in NL is **excellent — 4/4 knooppunten populated**. Turn classification quality is **excellent — 7/7 cases** cleanly separate the motorway ramp/fork from the mainline continuation, and the three afritten are cleanly rejectable purely on their signage ref. The in-plugin ramp-walk fallback (spec §3.4) is **not needed for NL**.

---

## 7. Spec-vs-reality mismatches (flag every one — the spec was written from general OSRM knowledge)

**7.1 Query parameter is `bearings=`, not `bearing=`.**
Contract 1 / §3.1 write `?bearing={heading},{tolerance}`. Stock OSRM (and therefore this plugin, which reuses the base grammar) uses the **plural** `bearings={deg},{tol}` (semicolon-separated per coordinate). Working URL:
`GET /tree/v1/driving/{lng},{lat}?bearings=328,45`. Either update the contract, or add a `bearing=` alias in the tree grammar later. Kept as-is for the spike to minimise surface.

**7.2 There is no separate `destination:ref` field — `GetDestinationsForID` returns one merged string.**
The spec assumes distinct `destination` and `destination:ref`. In this OSRM revision the name table packs them into a **single string**, formatted (observed) as either `"<REF>: <place>, <place>"` (Everdingen, Deil, Badhoevedorp, the afritten) **or** a bare place `"Nijmegen"` **or** a ref-only `"RING A16"` / `"A15"`. The branch test (`^[AE]\d+`) must run on the **leading ref token** (split on the first `:`), and must also handle the no-colon `RING A16` form. `destination:ref` is therefore *derived*, not read directly. The plugin currently returns the raw string (`destinations`) and leaves parsing to the caller — recommend the traversal (Stage 2) own a small `parseDestinationRef()` helper.

**7.3 The mainline continuation is classified `NoTurn`/`Suppressed`, not `Continue`.**
§3.2 step 2 says "follow the edge classified Continue/Suppressed/stay-on-road." In reality the straight-through mainline edge at a diverge is **`Suppressed` (18)** or **`NoTurn` (17)** — both stringify to the external name **`"invalid"`**, which is *also* the string for a genuinely-invalid turn. **Do not branch on the external string.** The plugin additionally emits `turn_type_id` (the raw `TurnType::Enum`); the traversal should key on the numeric id (`OffRamp=6`, `Fork=7` qualify as branches; `Suppressed=18`/`NoTurn=17`/`NewName=1`/`Continue=2` are the continuation). `Continue (2)` was **not** observed at any diverge here.

**7.4 The branch's target `ref` is still the *mainline* ref at the gore.**
At the immediate diverge the ramp's first edge-based node reports `ref="A2"` (or A5/A15) — the real branch ref (A27/A9/A16) only appears further down the ramp. So **raw ref-matching at the diverge does not identify the branch** — you must use the `destinations` signage (which already says `A27:…`), exactly as §3.2 step 2's "trust guidance continuation, not raw ref matching" anticipated. This also means the in-plugin fallback of "walk the ramp until road class is motorway and read its ref" (§3.4) is a *second* mechanism, only needed where signage is absent (not in NL).

**7.5 `destination:ref` can be a motorway ref with no place, or a place with no ref.**
Deil's branch is `"A15"` (ref only); some maneuvers sign only a city (`"Nijmegen"`, no ref). The qualifier must treat "no parseable A/E ref" as *non-qualifying* (correctly skips city-only local exits) and must not require a place to be present.

**7.6 Facade access is MLD-only via `dynamic_cast` (see §5).** Not a spec error, but a design constraint worth carrying forward: the traversal cannot use `GetFacade()` alone; it needs the algorithm facade. Fine for our single MLD instance.

**7.7 `snapping`/road-class restriction at snap time — and off-motorway ≠ `NoSegment`.**
§3.2 step 1 / Contract 1 want the snap *restricted to motorway class*, and Contract 1 maps snap failure to `400 NoSegment` → the app's off-motorway state. Two realities to reconcile:
- The R-tree snap API (`NearestPhantomNodes`) has **no road-class filter** — it filters by bearing/approach/radius only. The spike does a plain bearing snap and reads the class afterwards (`start.classes`). Every on-motorway probe snapped to the motorway, as expected.
- With **no radius set, OSRM never returns `NoSegment` for an off-network point** — it snaps to the *nearest* road at any distance. Probing a North Sea coordinate snapped ~50 km away to a local road (`classes: []`, `code: Ok`). So `NoSegment` alone will **not** detect "driver isn't on a motorway."

Production `/tree` must therefore drive the off-motorway state from a **class post-filter**: request N candidates (or read the snapped node's `classes`) and, if none is `motorway`, return `NoSegment` itself. Optionally also pass a `radiuses=` cap so far-away snaps don't masquerade as valid. The class data is available; the snap-time filter is not.

---

## 8. Recommendation

Proceed to Stage 2 (linear walk). The data is there and clean for NL. Carry forward: key branch decisions on `turn_type_id` + a parsed leading ref from `destinations` (not target `ref`, not the external turn string); add motorway post-filtering at snap; treat `Fork` and `OffRamp` identically as §3.2 already says. Reconfirm `destination:ref` density for DE/UK before those launches, but the NER fallback ramp-walk is unnecessary for NL.

---

# Stage 2 — linear walk (no branching)

**Status: DONE, all validations pass.** The `/tree` endpoint now walks the mainline continuation forward from the snapped node up to `hard_cap_m`, accumulates geometry, and reports the qualifying motorway-to-motorway junctions passed — without traversing them yet.

## S2.1 What the endpoint returns now

Contract 1 §3.3 root segment (`ref`, `polyline` = polyline5 of the accumulated mainline geometry, `start_offset_m: 0`, `length_m` = accumulated great-circle length, `branches: []`) plus a Stage-2 `junctions[]` debug array and a `snapped` block. Each junction: `at_offset_m` (driving distance from the snap to the diverge), `turn_type`, `toward` (parsed refs + places), `toward_ref` (parsed leading ref or null), `qualifies` (`toward_ref` matches `^[AE]\d+`), `name` (usually null — see S2.4).

```
GET /tree/v1/driving/5.06526,52.02042?bearings=145,45&hard_cap_m=200000
{ "code":"Ok", "ref":"A2", "start_offset_m":0, "length_m":84864, "polyline":"sfo|H{h|]…",
  "branches":[], "junctions":[
    { "at_offset_m":4547, "turn_type":"off ramp", "toward_ref":"A27",
      "toward":["A27","Breda","Gorinchem"], "qualifies":true, "name":null }, … ] }
```

## S2.2 The walk algorithm (as built, amended by findings)

Per node: gather forward outgoing edges (`IsForwardEdge`), read each target's turn type + `destinations` → parsed `branch_ref`. Then:

- **Continuation** (the edge to follow) is chosen by rank: (3) a `Suppressed/NoTurn/NewName/Continue` turn; else (2) an arm whose `branch_ref` equals the current road ref; else (1) an arm whose `branch_ref` is empty. Highest rank wins; none ⇒ the road ends, stop.
- **Junctions recorded**: every *non-continuation* `OffRamp`/`Fork` edge, except those whose `branch_ref` equals the current ref (parallel-carriageway splits — see S2.4). `qualifies` iff `branch_ref` matches `^[AE]\d+`.
- **Stop** on: no continuation, `at_offset_m ≥ hard_cap_m` (checked after the current segment, so the walk overshoots by at most one segment), a already-visited node (cycle guard), or graph end.
- `parseDestinationRef()` owns the format zoo from finding 7.2: takes the pre-`:` head and pulls the first `[A-Za-z]{1,3}\s?\d+` token — so `"A27: Almere"`→`A27`, `"RING A16"`→`A16`, `"Ring N14: … (N44) …"`→`N14` (not the parenthesised N44), `"Nijmegen"`→none.

## S2.3 Validation results (`tools/hm2_spike/walk_probe.py`, all PASS)

| Case | Result |
|---|---|
| **A2 south** (start N of Everdingen, hdg 145) | ref A2, 84.9 km, 27 junctions. Qualifiers in order: **A27 @4.5 km → A15 @18.2 km** → A65 → A50 → A58 → A67 → (A2/A67 fork). Afritten N320/N327/N322 present, **not** qualifying. ✅ matches "Everdingen then Deil". |
| **A4 south** (start N of Badhoevedorp, hdg 268) | ref A4, 67.5 km, 22 junctions. Qualifiers: **A9 @1.6 km** → A44 → A12 → A13 → A20 → A15. Leiden ring **N11/N207 present, not qualifying**. ✅ matches "Badhoevedorp then Leiden (must NOT qualify)". |
| **A2 south, `hard_cap_m=20000`** | Stops at 24.5 km (one segment past the cap) vs 84.9 km uncapped — cap honoured at segment granularity. ✅ |
| **A10 Amsterdam ring (cycle)** | Terminates cleanly at 6.0 km (visited-set + budget); no infinite loop. ✅ |
| **Off-motorway point** (Amsterdam centrum local road) | `400 {"code":"NoSegment"}` from the motorway post-filter — snaps to a local road but is rejected, giving the app its off-motorway state. ✅ |

Raw dumps: `tools/hm2_spike/evidence/stage2_*.json` + `stage2_console_summary.txt`.

## S2.4 New internals findings (Stage 2)

1. **Pure forks have no `Suppressed` continuation — the mainline leaves via a `Fork` arm.** At Ridderkerk (A15/A16) and the A4/Leiden-ring split, *both* arms are `Fork` and there is no `Suppressed`/`NoTurn` edge. Keying continuation only on `{17,18,1,2}` (as the Stage-2 brief literally specified) would dead-end the walk there. The rank-2 fallback (arm whose `branch_ref` == current ref) handles it and was necessary for the A4 walk to get past Leiden. **Amends** the brief's continuation rule.
2. **Parallel-carriageway splits masquerade as branches toward your own road.** At Oudenrijn the A2 through/local lanes split; the non-followed A2 arm is a `Fork` with `toward_ref == "A2"`. Recording it would emit a spurious "junction toward A2". Suppressed by the `branch_ref == current_ref` skip. (This is also why raw ref-matching for *branch* detection is unsafe, complementing finding 7.4.)
3. **`hard_cap_m` overshoots by up to one segment** by design — the walk stops at the first segment whose *end* passes the cap. Motorway segments run to a few km, so the JS layer should treat `length_m`/`searchedToM` as "walked at least this far", not an exact cap.
4. **Junction `name` is almost always empty.** OSRM exposes no knooppunt name via the facade; `GetNameForID` on the ramp target is `""`. The junction is identified by its `toward` signage, not a name. Contract 1's `junction.name` ("Knooppunt Everdingen") is **not derivable in-plugin** — drop it, or have the JS/BE layer reverse-geocode the diverge coordinate. **Flag for Contract 1.**
5. **Distances are honest and match reality**: A27 @4.5 km, A15 @18 km on the A2; A9 @1.6 km on the A4 — all consistent with the real motorway geometry, confirming the accumulated-haversine offset model.

## S2.5 Files (Stage 2 delta, working tree still dirty, no commits)

- `src/engine/plugins/tree.cpp` — rewritten: the walk, `parseDestinationRef`/`isMotorwayRef`/`towardArray`, `selectContinuation`, motorway post-filter.
- `include/engine/plugins/tree.hpp` — header comment updated to Stage 2.
- `include/engine/api/tree_parameters.hpp` — added `hard_cap_m` (default 200000).
- `include/server/api/tree_parameter_grammar.hpp` — parse `hard_cap_m=`.
- `tools/hm2_spike/walk_probe.py` + `tools/hm2_spike/evidence/stage2_*.json` — Stage 2 validation.

## S2.6 Recommendation

Proceed to Stage 3 (recursive tree). The walk, offset model, ramp/local discrimination, and cycle/cap termination all hold on real NL topology. For Stage 3: reuse `selectContinuation`'s branch set (the non-continuation qualifying `OffRamp`/`Fork` edges) as the recursion points; carry the connector geometry into the child branch (§3.2 step 4); dedupe by per-path visited set (already in place). Fold the Contract 1 fixes above (junction `name` undrivable; `hard_cap_m` is segment-granular).

---

# Stage 3 — recursive tree

**Status: DONE, all validations pass.** `/tree` now recurses into every qualifying motorway-to-motorway junction and returns the full Contract 1 §3.3 nested tree.

## S3.1 What the endpoint returns now

The nested schema: each `route` is `{ref, polyline, start_offset_m, length_m, branches[]}`; each entry of `branches[]` is `{junction:{name, exit_ref, at_offset_m, connector_length_m, toward}, route:{…recursive}}`. `debug=true` adds the Stage-2 `junctions[]` array to every segment; the response also carries `segment_count` (total segments emitted). Example (A2 root, first branch trimmed):

```json
{ "code":"Ok", "ref":"A2", "start_offset_m":0, "length_m":37701, "polyline":"sfo|H…", "segment_count":4,
  "branches":[
    { "junction":{ "name":"", "exit_ref":null, "at_offset_m":4547, "connector_length_m":0,
                   "toward":["A27","Breda","Gorinchem"] },
      "route":{ "ref":"A27", "start_offset_m":4547, "length_m":28203, "polyline":"upg|H…", "branches":[…] } } ] }
```

## S3.2 The recursion (as built)

`walkSegment` walks a mainline segment (Stage 2 walk), and at every non-continuation `OffRamp`/`Fork` edge that qualifies (`^[AE]\d+`), it: traverses the ramp connector (`traverseRamp` follows the ramp's continuation until a motorway-class node, ≤15 hops), then recurses from that node with `reported_ref` = the branch's first motorway ref, `start_offset_m` = `junction.at_offset_m + connector_length_m`, and remaining budget `hard_cap_m − child_start_offset`. Per-path `visited` set is copied into each child (a node reachable via two parents legitimately appears twice; a node on the current path is not re-entered). Safety bounds: `MAX_SEGMENTS`=400, `MAX_DEPTH`=10, `MAX_RAMP_HOPS`=15.

**Same-road suppression (the bug the reviewer caught + two more it exposed).** A branch is skipped when its ref shares a component with any of three ref sets, each split on `;` and matched component-wise (never string-equality — OSRM stores concurrency as `A2;A67`):
1. `current_ref` — the evolving road identity (catches the A2;A67 concurrency unbundling at Eindhoven; this was the reviewer's regression).
2. the **continuation arm's** ref — the arm we're actually following (catches carriageway/direction splits locally).
3. `reported_ref` — the segment's committed road identity, which never drifts (stable backstop at depth).

Two internal bugs surfaced building this (both fixed, see S3.4): `current_ref` must be seeded from the branch ref and only updated on `NewName`, and the branch's identity is its first *motorway* ref, not its first token.

## S3.3 Validation results (`tools/hm2_spike/tree_probe.py`, all PASS)

| Check | Result |
|---|---|
| **(a) A2 south, A27 child** | A27 child present, `length_m` 55 km, `start_offset_m` 4547 = junction 4547 + connector 0. ✅ |
| **(b) A4 south, A9 child + no N-road branches** | A9 child present; **zero** N-road branches anywhere in the tree (the Leiden ring never becomes a branch). ✅ |
| **(c) A10 ring, `hard_cap_m=50000`** | Terminates, 46 segments, root 6 km; cycles bounded by the per-path visited set + budget. ✅ |
| **(d) A2/A67 concurrency regression** | **Zero** qualifying junctions toward the segment's own road, at 90 km and at the full 200 km. ✅ (was: A2 + A67 both flagged toward the A2/A67 concurrency). |
| **(e) every junction `name:""`** | 1126 junctions/branches checked across all four trees, all `""` (non-null). ✅ |

Off-motorway snap still returns `400 {"code":"NoSegment"}`. Raw trees in `tools/hm2_spike/evidence/stage3_*.json`.

## S3.4 New internals findings (Stage 3)

1. **`current_ref` must not be overwritten on plain continuations.** Seeding the child from the branch ref (`A15`) then updating `current_ref = continuation.ref` on every hop re-broke it: at a knooppunt gore the continuation segment carries the *crossing* motorway's stale ref (finding 7.4), so the child's `current_ref` flipped `A15 → A2` and the same-road fork suppression failed (a single A15 entry exploded into a self-branching sub-tree — 90 segments where 24 were real). Fix: update `current_ref` **only on a `NewName` turn** (OSRM's genuine-renumbering signal); keep it through `NoTurn`/`Suppressed`. This alone cut the A2 tree from 90 → 24 segments.
2. **A branch's identity is its first *motorway* ref, not its first token.** A ramp signed `"N201; A9"` qualifies via A9 but `branch_refs.front()` is `N201`; labelling the child `N201` produced a spurious N-road child. Fixed with `firstMotorwayRef()`. (The reverse — an N-road signed with your own motorway ref, e.g. `"N201: … (A4)"` — is caught by the same-road suppression, so those never recurse.)
3. **`current_ref` still drifts at extreme depth**, so component-matching against it alone is insufficient; the `reported_ref` (never-drifting) backstop was needed to reach zero toward-self junctions at the full 200 km.
4. **NL knooppunt connectors are motorway-tagged, so `connector_length_m` is ~0.** The ramp geometry at these interchanges is `highway=motorway`, not `motorway_link`, so `traverseRamp` returns the branch's first node immediately and `start_offset_m` = `junction.at_offset_m`. `connector_length_m` will be non-zero where ramps are tagged `motorway_link` (some regions/exits); the field is emitted regardless.
5. **A 200 km recursive tree is combinatorially large in the Randstad** — it hits `MAX_SEGMENTS`=400. Budget subtraction (`child_budget = hard_cap − distance_driven`) bounds every root-to-leaf path but not breadth. This is expected and is exactly what the JS quota layer (plan §4.3) exists to tame; for the plugin, the segment cap is the safety net. Practical PoC caps (30–90 km) yield 4–50 segments.

## S3.5 Files (Stage 3 delta, working tree still dirty, no commits)

- `src/engine/plugins/tree.cpp` — recursion (`walkSegment` now recursive), `traverseRamp`, `firstMotorwayRef`, component-wise same-road suppression (three ref sets), `NewName`-gated `current_ref` update, safety bounds.
- `include/engine/plugins/tree.hpp` — header comment updated to Stage 3.
- `include/engine/api/tree_parameters.hpp` — added `debug`.
- `include/server/api/tree_parameter_grammar.hpp` — parse `debug=`.
- `tools/hm2_spike/tree_probe.py` + `tools/hm2_spike/evidence/stage3_*.json` — Stage 3 validation.

## S3.6 Open items for the reviewer / Contract 1

- **`start_offset_m` vs connector**: implemented per the brief (`junction.at_offset_m + connector_length_m`), and `connector_length_m` is emitted on the junction. With NL connectors at 0 this equals `at_offset_m`; when connectors are non-zero the BE must treat child polyline point distance as `start_offset_m + along_polyline` (the connector is *not* in the child polyline — it is represented as the offset gap, which keeps the §3.3 "start_offset + along = distance-from-me" invariant intact). Flagging the one deviation from "connector geometry in the child polyline": it is represented as distance, not geometry.
- **Combinatorial breadth at large `hard_cap_m`**: recommend the JS layer drive expansion by quota, or the plugin gain a `max_branches`/quota param, before running trees at 150–200 km in dense regions.
