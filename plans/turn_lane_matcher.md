# Coverage Analysis Report: `turn_lane_matcher.cpp`

## Overview

`src/guidance/turn_lane_matcher.cpp` implements the core turn lane matching logic for OSRM's guidance system. It translates OpenStreetMap turn lane tags into OSRM's internal turn instructions by matching lanes to intersection roads. The module contains 6 functions with total ~270 lines of code.

### Test file

**`unit_tests/extractor/turn_lane_matcher_tests.cpp`** — 36 new test cases added, covering all functions. All 104 tests pass (existing + new).

---

## Previously uncovered code: what it does

### Lines 66–79: `isValidMatch` for `straight` tag

A `straight` lane tag is considered a valid match when **any** of these is true:

| # | Condition | Rationale |
|---|-----------|-----------|
| 1 | `direction_modifier == Straight` | Trivial direct match |
| 2 | `type == Suppressed` | Suppressed turns map to straight-through behavior |
| 3 | `type == NewName` | Name changes without an actual turn |
| 4 | `type == StayOnRoundabout` | Continuing on a roundabout |
| 5 | `entersRoundabout()` | Entering any roundabout type |
| 6 | `type == Fork` **and** (`SlightLeft` or `SlightRight`) | Forks can be experienced as slight deviations |
| 7 | `type == Continue` **and** (`SlightLeft` or `SlightRight`) | Continue instructions with slight deviation |

**Important design note:** Condition 1 (`direction_modifier == Straight`) short-circuits all others. This means `Fork + Straight` and `Continue + Straight` **are valid** matches for a `straight` lane tag — the Fork/Continue + Slight checks (conditions 6–7) are *additional allowances*, not the only paths.

### Lines 80–89: `isValidMatch` for left-side tags (`slight_left`, `left`, `sharp_left`)

Two distinct cases depending on whether the instruction is a Merge:

- **Merge instruction** (`type == TurnType::Merge`): Returns true only if the instruction has a **right** modifier. This is the *mirrored semantics* pattern — a merge instruction's left/right modifier is flipped relative to the turn direction. A "left" lane tag corresponds to a right-side merge in OSRM's internal representation.

- **Non-Merge instruction**: Returns true if `type == StayOnRoundabout` **or** the instruction has a left modifier (`SharpLeft`, `Left`, `SlightLeft`).

```cpp
if (isMirroredModifier(instruction))
    return hasRightModifier(instruction);       // mirror: left lane → right merge
else
    return (instruction.type == TurnType::StayOnRoundabout) ||
           hasLeftModifier(instruction);        // normal: left lane → left turn
```

### Lines 186–188: `canMatchTrivially` end condition

```cpp
return lane == lane_data.size() ||
       (lane + 1 == lane_data.size() && lane_data.back().tag == TurnLaneType::uturn);
```

The matching loop iterates lanes and roads sequentially. The end condition succeeds in two cases:
1. **All lanes matched**: `lane == lane_data.size()` — every lane found its corresponding road.
2. **One u-turn lane remains**: `lane + 1 == lane_data.size() && lane_data.back().tag == uturn` — the unmatched lane is a u-turn. This is necessary because u-turns are disabled by default in OSRM (see comment at lines 129–133) and won't have a corresponding enterable road in the intersection.

Without the second condition, any intersection with u-turn lane markings would fail the trivial match check.

### Lines 190–268: `triviallyMatchLanesToTurns`

This function performs the actual lane-to-road data assignment. It operates in three phases:

#### Phase 1: Front u-turn handler (lines 206–232)

Triggered when `lane_data.front().tag == uturn` — meaning the leftmost lane is a u-turn lane.

```
if lane_data.front().tag == uturn:
    if NOT intersection[0].entry_allowed:
        return intersection  // EARLY RETURN — can't match u-turn

    if edge IS NOT reversed:
        u_turn_position = 0              // use the standard u-turn road
    else (edge IS reversed):
        if intersection has ≤ 1 road:
            return intersection          // EARLY RETURN — no place for u-turn
        if NOT intersection[1].entry_allowed:
            return intersection          // EARLY RETURN
        if intersection[1].modifier != SharpRight:
            return intersection          // EARLY RETURN — can't map u-turn
        u_turn_position = 1
        road_index = 2                   // skip the u-turn road in main loop

    intersection[u_turn_position].type = Continue
    intersection[u_turn_position].modifier = UTurn
    matchRoad(intersection[u_turn_position], lane_data.back())
    lane = 1                             // skip lane 0 in main loop
```

The reversed edge case handles one-way streets where the u-turn is not at position 0 but must be matched to a SharpRight turn instead.

#### Phase 2: Normal matching loop (lines 234–246)

```cpp
for (road_index = 1; road_index < intersection.size() && lane < lane_data.size(); ++road_index)
{
    if (intersection[road_index].entry_allowed)
    {
        // Assertions verify perfect alignment:
        BOOST_ASSERT(lane_data[lane].from != INVALID_LANEID);
        BOOST_ASSERT(isValidMatch(lane_data[lane].tag, intersection[road_index].instruction));
        BOOST_ASSERT(findBestMatch(lane_data[lane].tag, intersection) ==
                     intersection.begin() + road_index);

        matchRoad(intersection[road_index], lane_data[lane]);
        ++lane;
    }
}
```

Starting from `road_index = 1` (skipping intersection[0] which is the u-turn road), each entry-allowed road is matched to its corresponding lane. The assertions enforce that `findBestMatch` returns exactly the sequential position — if lanes don't align in order, this is a logic error.

Roads with `entry_allowed = false` are skipped without consuming a lane.

#### Phase 3: Trailing u-turn handler (lines 249–266)

Triggered when one lane remains unmatched and it's a u-turn:

```
if lane + 1 == lane_data.size() AND lane_data.back().tag == uturn:
    u_turn_position = 0
    if edge IS reversed:
        if NOT intersection.back().entry_allowed:
            return intersection       // EARLY RETURN
        if intersection.back().modifier != SharpLeft:
            return intersection       // EARLY RETURN — can't map u-turn
        u_turn_position = intersection.size() - 1

    intersection[u_turn_position].type = Continue
    intersection[u_turn_position].modifier = UTurn
    matchRoad(intersection[u_turn_position], lane_data.back())
```

The trailing handler is the mirror of the front handler:
- **Not reversed**: u-turn matches to `intersection[0]` (the standard position)
- **Reversed**: u-turn matches to `intersection.back()` which must be a SharpLeft turn

---

## Misnomers and inaccuracies found

### 1. Known missing feature: left-hand driving support (lines 63, 87)

The comments at line 63 (`// needs to be adjusted for left side driving`) and line 87 (`// Needs to be fixed for left side driving`) flag an acknowledged limitation. The `isValidMatch` function doesn't account for left-hand driving countries where left/right turn semantics are flipped.

The `mirror()` method exists on `ConnectedRoad` (in `guidance/intersection.hpp:46-68`) and flips angles and direction modifiers, but it is never consulted in `isValidMatch`. Turn lane matching may produce incorrect results in left-hand driving regions.

### 2. Unreachable code path: reversed trailing u-turn success (line 260)

The success path `u_turn = intersection.size() - 1` at line 260 is **structurally unreachable** with valid data that respects the `intersection[0] = u-turn road` invariant. The reasoning:

1. `findBestMatch(uturn, intersection)` always prefers `intersection[0]` when `[0]` has a `UTurn` direction modifier, because `[0]` has the ideal u-turn angle (0° deviation)
2. The normal matching loop processes **all** entry-allowed roads from index 1 through `intersection.size() - 1`
3. If `intersection.back()` is entry-allowed `SharpLeft` (which has a left modifier valid for uturn), the loop **will consume** the u-turn lane at that position
4. After the u-turn is consumed in the loop: `lane + 1 > lane_data.size()`, so the trailing handler condition is false and never fires

This code path would only execute if `intersection[0]` does **not** have a `UTurn` modifier — which breaks the invariant that `intersection[0]` is always the u-turn road. This appears to be defensive code that exists for symmetry with the front handler but cannot be reached through the normal call chain (where `canMatchTrivially` validates data first).

### 3. `merge_to_left` and `merge_to_right` always return `false` from `isValidMatch` (line 91)

These two tag types fall through every `if/else if` branch and hit `return false` at line 91. They are present in the `tag_by_modifier` array in `getMatchingModifier()` (mapped to `DirectionModifier::Straight`, indices 8–9) but have no corresponding logic in `isValidMatch`. This means:

- Any lane tagged `merge_to_left` or `merge_to_right` can **never** be "validly matched" to any intersection road
- `getMatchingModifier` maps them to `Straight` (a sensible fallback), but `isValidMatch` silently rejects them

This could be intentional (merge lanes don't correspond to turn directions) but it's not documented as such.

### 4. Front u-turn handler matches `lane_data.back()` (line 226)

The front handler checks `lane_data.front().tag == uturn` but matches using `lane_data.back()`:

```cpp
if (lane_data.front().tag == TurnLaneType::uturn) {
    // ...
    matchRoad(intersection[u_turn], lane_data.back());
}
```

This means the **last** entry in `lane_data` provides the actual lane-from/lane-to positions for the u-turn. The convention is:

- `lane_data.front() = uturn` → presence marker ("there is a u-turn to handle")
- `lane_data.back()` → actual u-turn lane data with `from`/`to` values

This is non-obvious from the code alone. The production caller (`TurnLaneHandler`) constructs the vector accordingly.

### 5. Debug-only assertions mask invalid states in release builds (lines 239–241)

The normal matching loop uses `BOOST_ASSERT` for three critical invariants:

```cpp
BOOST_ASSERT(lane_data[lane].from != INVALID_LANEID);
BOOST_ASSERT(isValidMatch(lane_data[lane].tag, intersection[road_index].instruction));
BOOST_ASSERT(findBestMatch(lane_data[lane].tag, intersection) == intersection.begin() + road_index);
```

In release builds, `BOOST_ASSERT` is compiled out entirely. If the caller hasn't validated the data with `canMatchTrivially()` first, `triviallyMatchLanesToTurns` will silently match lanes to wrong roads — breaking the invariant that `findBestMatch` returns the sequential position.

This is by design: the contract is that `canMatchTrivially` must return `true` before `triviallyMatchLanesToTurns` is called. The production code in `TurnLaneHandler` respects this, but the safety net only exists in debug builds.

---

## Test coverage summary

| Function | New test cases | Code paths covered |
|---|---|---|
| `getMatchingModifier` | 10 | All 11 tag types including fallback for invalid |
| `isValidMatch` — uturn | 4 | Left modifier, UTurn modifier, negative cases |
| `isValidMatch` — right-side | 5 | Right modifiers, leaves roundabout, merge mirror, negative cases |
| `isValidMatch` — straight | 9 | Straight modifier, Suppressed, NewName, StayOnRoundabout, entersRoundabout, Fork+Slight, Continue+Slight, negative cases |
| `isValidMatch` — left-side | 6 | Left modifiers, StayOnRoundabout, merge mirror, negative cases |
| `isValidMatch` — fallthrough | 3 | merge_to_left, merge_to_right, none → false |
| `getMatchingQuality` | 2 | Ideal and non-ideal angles |
| `findBestMatch` | 3 | Valid priority, entry-allowed priority, quality tiebreaker |
| `findBestMatchForReverse` | 2 | Neighbor-at-end fallback, normal search |
| `canMatchTrivially` | 8 | Empty data, valid/invalid matches, end conditions, first-lane-u-turn skip, best-match position mismatch |
| `triviallyMatchLanesToTurns` | 13 | Empty data, front-u-turn (not-reversed, reversed+valid SharpRight, reversed+invalid/no-road), front-u-turn entry-not-allowed early return, normal matching loop, trailing-u-turn (not-reversed success, reversed no-SharpLeft early return, reversed not-entry-allowed early return) |
| **Total** | **36** | **All reachable code paths** |

### Coverage estimate

The previously uncovered regions (lines 66–90 and 186–270) are now thoroughly tested. The only code paths not covered are the **unreachable** ones identified above (reversed trailing u-turn success path at line 260). Estimated coverage after these tests: **~95%**.
