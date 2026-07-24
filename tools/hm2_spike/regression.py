#!/usr/bin/env python3
"""Highway Mode 2 — OSRM /tree plugin regression harness.

Table-driven, committed replacement for the ad-hoc Stage 1–3 probes (probe.py /
walk_probe.py / tree_probe.py, kept for their raw evidence dumps). Encodes the known-good
NL behaviour the plugin must not regress, and exits nonzero on any failure. RUN IT after
every rebase of the fork and after every data refresh (a new Geofabrik extract can move a
snap or renumber a ref) — see README.md in this directory.

What it locks in (all against a live osrm-routed on the NL MLD dataset):
  * Motorway-to-motorway diverges QUALIFY, in order, with the right branch ref:
      A2 south  -> A27 then A15   (KP Everdingen, KP Deil; off-ramp diverges)
      A4 south  -> A9             (KP Badhoevedorp; off-ramp diverge)
      A15 east  -> A16            (KP Ridderkerk; a *fork* diverge, both arms Fork)
  * Ordinary local exits (afritten) NEVER qualify and never become branches:
      N320/N327 off the A2, N201/N207/N11 (Leiden ring) off the A4.
  * Concurrency: no qualifying junction ever points back at the road the segment is on
      (the A2;A67 unbundling regression) — checked to the full 200 km hard cap.
  * A10 Amsterdam ring terminates under a 50 km cap (cycle guard + budget).
  * Off-motorway snap -> HTTP 400 {"code":"NoSegment"} (the motorway post-filter).
  * Every junction name is "" (non-null) — the locked Contract-1 shape.

Probe coordinates are pinned; the qualifying cases were derived by the corridor method in
probe.py and re-verified against the live server. If a data refresh legitimately moves a
snap, update the coordinate here and re-verify — do not loosen an assertion to make it pass.

Usage: OSRM_URL=http://127.0.0.1:5050 python3 regression.py
       python3 regression.py [base_url]      (default $OSRM_URL or http://127.0.0.1:5050)
Exit:  0 = all green, 1 = one or more regressions (details printed).
"""
import json
import os
import sys
import urllib.error
import urllib.request

BASE = (sys.argv[1] if len(sys.argv) > 1
        else os.environ.get("OSRM_URL", "http://127.0.0.1:5050")).rstrip("/")

# --- pinned probes --------------------------------------------------------------------
# name, lng, lat, bearing, hard_cap_m, expected in-order qualifying branch refs,
# refs that must be SEEN-but-NOT-qualifying (afritten). Bearing tolerance is 45 throughout.
QUALIFY_CASES = [
    dict(name="A2 south (KP Everdingen -> KP Deil)",
         lng=5.06526, lat=52.02042, bearing=145, cap=60000,
         qualify_in_order=["A27", "A15"], afritten=["N320", "N327"]),
    dict(name="A4 south (KP Badhoevedorp; Leiden ring must not qualify)",
         lng=4.82557, lat=52.33885, bearing=268, cap=45000,
         qualify_in_order=["A9"], afritten=["N201", "N207", "N11"]),
    dict(name="A15 east (KP Ridderkerk fork A15 -> A16)",
         lng=4.37547, lat=51.87455, bearing=115, cap=20000,
         qualify_in_order=["A16"], afritten=[], fork_expected="A16"),
]

# A10 ring: must terminate under a tight cap.
RING_CASE = dict(name="A10 Amsterdam ring (cycle termination, 50 km cap)",
                 lng=4.84429, lat=52.38854, bearing=178, cap=50000)

# Concurrency: no toward-self qualifying junction, to the full hard cap.
CONCURRENCY_CASE = dict(name="A2/A67 concurrency — zero toward-self junctions (200 km)",
                        lng=5.06526, lat=52.02042, bearing=145, cap=200000)

# Breadth: best-first expansion must keep the root's near first-level branches even when the
# 200 km tree is truncated at MAX_SEGMENTS — regression against the depth-first bug where the
# first subtree exhausted the whole segment budget and the root collapsed to a single branch.
BREADTH_CASE = dict(name="A2 south breadth — root keeps first-level branches at 200 km",
                    lng=5.06526, lat=52.02042, bearing=145, cap=200000)

# Motorway containment: near KP Ypenburg / Prins Clausplein the A12 Utrechtsebaan degrades into Den
# Haag city streets and the A13 ends at Ypenburg. The walk used to escape onto ~7 km of local roads
# (the S107/Westvlietweg field report). Every segment's non_motorway_m (debug field) must be 0.
CONTAINMENT_CASE = dict(name="A4 @ Ypenburg — tree stays on motorways (no S-road escape)",
                        lng=4.31100, lat=52.00468, bearing=351, cap=40000)

# Same-ref suppression must cover ALL qualifying junction types (OffRamp AND Fork), not just forks:
# a branch must never be the same road as the segment it forks from (the opposite-carriageway /
# U-turn-loop spur). The A4 near Den Hoorn sits in a dense interchange cluster (Kethelplein A4/A20,
# KP Ypenburg A4/A13) that stresses this. Checked over the whole tree.
SAMEREF_CASE = dict(name="A4 Den Hoorn — no branch shares a ref component with its parent",
                    lng=4.32313, lat=51.99008, bearing=330, cap=40000)

# Landing retry: the A12-east branch off the A4 at Prins Clausplein used to dead-end at Nootdorp
# (~3 km) because the ramp dropped onto a service loop; the through-lane is the other arm of a
# motorway fork ~1 km in. The fork-backtracking retry must recover it well past Nootdorp/Zoetermeer.
RETRY_CASE = dict(name="A4 @ Prins Clausplein — A12-east branch recovers past Nootdorp",
                  lng=4.31100, lat=52.00468, bearing=351, cap=70000)

# Root snap retry: snapping inside the Prins Clausplein stack, the nearest bearing-valid candidate
# here is a non-motorway surface road; the multi-candidate snap must fall through to a motorway
# alignment and produce a full corridor. Without it this returns NoSegment (worst-case: the whole
# corridor is lost). Asserts a long root.
ROOT_RETRY_CASE = dict(name="Inside Prins Clausplein stack — root snaps through to a motorway",
                       lng=4.36, lat=52.048, bearing=90, cap=60000)

# Parallel carriageway (parallelbaan) on the A4 Leiden–Hoofddorp: a same-road arm that splits and
# rejoins with no exits of its own is a stub, not a route option. It must be dropped, AND the drop
# must not lose a unique alignment (a dropped stub must hug the kept tree).
PARALLELBAAN_CASE = dict(name="A4 Leiden–Hoofddorp — parallel-carriageway stubs dropped, no over-drop",
                         lng=4.49431, lat=52.13088, bearing=53, cap=80000)

# Crossing-road directions: a single A4->A12 ramp at Prins Clausplein reaches both A12 directions
# via a fork; both must be emitted — A12 east (toward Utrecht, long) AND A12 west (Utrechtsebaan
# toward Den Haag, legitimately short). Assert existence of both, not length.
DIRECTIONS_CASE = dict(name="A4 @ Prins Clausplein — both A12 directions emitted",
                       lng=4.31100, lat=52.00468, bearing=351, cap=70000)

# Crossing-road signage refinement (S3-fix7). The A4->A12 ramp at Prins Clausplein is a shared
# slip road whose gantry carries the union of both directions' destinations
# (A12;Den Haag;Voorburg;Utrecht;Zoetermeer). The primary (east, toward Utrecht) branch used to
# inherit that whole union; each direction must instead carry its own post-split signage - east has
# Utrecht/Zoetermeer and NOT the west-only Voorburg; west keeps Den Haag/Voorburg.
SIGNAGE_CASE = dict(name="A4 @ Prins Clausplein — each A12 direction carries its own refined signage",
                    lng=4.31100, lat=52.00468, bearing=351, cap=70000)

# Duplicate-corridor guards (S3-fix7). Two ways the same physical corridor used to appear twice:
#  (a) a parallel carriageway that leaves the mainline and rejoins it further on (the A4 parallelbaan
#      mis-signed A13 at KP Ypenburg) re-expanded a ~255 m-shifted copy of the whole tree as a root
#      branch hugging root A4 for 39 km;
#  (b) two qualifying entry ramps onto one crossing direction (KP Muiderberg's twin A6 lanes) both
#      emitted as same-parent children, ending km apart so the end-coordinate de-dup missed them.
# Distinct from the legitimate cross-parent reachability (design decision #12): the same road reached
# via two different parent chains is two real routes and is kept. These guards are (a) no root branch
# hugs the root corridor, (b) no two same-ref children of ONE parent are the same corridor.
DUP_CASE = dict(name="A4 north — no duplicate corridor (parallelbaan re-expansion / twin ramps)",
                lng=4.3236797, lat=52.0268771, bearing=32, cap=200000)

# Crossing-road directions at a cloverleaf (S3-fix8). At a klaverblad the two directions of the
# crossing motorway leave the mainline as two SEPARATE off-ramps (unlike Prins Clausplein, where one
# shared ramp forks into both — that path is covered by S3-fix6/DIRECTIONS_CASE). The through-lane's
# gore sign merges the mainline ref with the crossing ref (A50's through-sign reads
# "A50, A1: Zwolle, ..., Amsterdam"); folding that destination signage into the same-ref suppression
# made the crossing's own off-ramp look like a same-road split and dropped exactly ONE direction of
# each crossing. Probing A50 north, KP Beekbergen (A1) and KP Hattemerbroek (A28) must each emit BOTH
# directions with the right per-direction signage. Pre-fix: only A1-west (Amsterdam) and A28-south
# (Amersfoort) appeared; A1-east (Deventer/Hengelo) and A28-north (Zwolle/Groningen) were missing.
CLOVERLEAF_CASE = dict(name="A50 north — both directions of each cloverleaf crossing (Beekbergen A1, Hattemerbroek A28)",
                       lng=5.94851, lat=52.08628, bearing=9, cap=70000)

# Same-road parallel carriageway lifts skipped junctions (S3-fix9). Driving the A2 SOUTHBOUND ~25 km
# north of Knooppunt Hintham ('s-Hertogenbosch), the A2 splits into its hoofdbaan (through, signed
# "A2: Eindhoven") and its parallelbaan (empty destination) at KP Empel. The walk follows the
# hoofdbaan; the A59 interchanges (KP Empel A59-west, KP Hintham A59-east toward Oss/Nijmegen) sit
# only on the parallelbaan, so pre-fix A59 never appeared from a distance (it showed only when the
# driver snapped onto the parallelbaan within ~5 km, or deep via A50/A58 at 60 km+). The plugin now
# probes the parallelbaan and lifts its motorway junctions onto the mainline, so the A59-east group
# (toward Oss/Nijmegen) is a first-level branch at ANY distance. Pre-fix first-level was
# A15x2/A65/A50/A58/A67x2 with NO A59; post-fix A59 appears at ~16-19 km.
HINTHAM_CASE = dict(name="A2 south, 25 km north of KP Hintham — A59-east lifted off the parallelbaan",
                    lng=5.21045, lat=51.87636, bearing=148, cap=200000)

# Class-weighted budget keeps far first-level spurs (S3-fix9, the 2026-07-08 budget-priority ruling).
# Same A2-south probe at the 200 km cap (segment_count = 400, truncated). Pure nearest-first sinks the
# whole budget into the near spurs' dense subtrees and drops the far first-level spurs A76 (~119 km)
# and A79 (~130 km) in Limburg entirely. The class-weighted ordering makes every first-level spur head
# class 0 (guaranteed a slot at any distance), so A76 and A79 survive the truncation. Verified
# before/after on the same build: pure nearest-first first-level = A15x2/A59x2/A65/A50/A58/A67x2 (no
# A76/A79); class-weighted adds A76x2/A79.
BUDGET_PRIORITY_CASE = dict(name="A2 south 200 km — far first-level spurs A76/A79 survive the segment cap",
                            lng=5.21045, lat=51.87636, bearing=148, cap=200000)

# Spur-terminus renumber → continuation child (S3-fix11, productising S3-fix10). Driving the A59
# eastbound toward KP Paalgraven, the A59 ends there and MERGES onto the A50 north (Ravenstein -> KP
# Bankhoef -> KP Ewijk -> Waal bridge -> KP Valburg -> Grijsoord -> Apeldoorn). A segment's emitted
# ref must be the road it is physically on, so at the merge the A59 root ENDS and the drive continues
# as an A50 continuation child carrying the real crossings (A15 at KP Valburg, A12 at KP Grijsoord,
# ...) as its own first-level branches. (S3-fix10 kept the root on the driver's road but left the
# whole corridor mislabelled "A59"; S3-fix11 relabels it via the split.) The pre-fix10 bug this still
# guards against: the A50 corridor must NOT reappear as a mis-signed both-directions branch, and the
# root must NOT die at ~21 km - the A50 continuation reaches far north.
VALBURG_CASE = dict(name="A59 east -> A50 north — A59 root ends at KP Paalgraven, A50 continuation carries the corridor",
                    lng=5.555818, lat=51.733873, bearing=120, cap=200000,
                    root_ref="A59", max_root_m=12000, cont_ref="A50", min_cont_m=40000,
                    cont_crossings=("A15", "A12"))

# Universal label relabel — the emitted ref is always the road physically driven (S3-fix11, defect A).
# Driving the A5 south toward KP De Hoek, the A5 (a short spur near Schiphol) MERGES onto the A4; the
# root used to emit ref "A5" for its whole 70.6 km (≈60 km of physical A4 mislabelled A5). Post-fix
# the A5 root is short and an A4 continuation child carries the rest of the corridor.
DEHOEK_CASE = dict(name="A5 south -> A4 at KP De Hoek — A5 root is short, A4 continuation carries the corridor",
                   lng=4.774, lat=52.392, bearing=209, cap=200000,
                   root_ref="A5", max_root_m=20000, cont_ref="A4", min_cont_m=30000,
                   cont_crossings=("A44", "A13"))

# Concurrency un-bundle follows the driver's ref component (S3-fix12, defect B). Driving the A58 west
# near Breda, the A58 merges onto the A16 north as a signed concurrency (A16;A58) then peels off west
# to Vlissingen. The geometric mainline is the A16 north; the root used to follow it ~33 km to
# Moerdijk still labelled A58, and the real A58 peel-off died under same-ref suppression. Post-fix the
# root follows the driver's A58 component to the Vlissingen side, and BOTH A16 directions emit as
# branches (A16 south at the merge, A16 north at the peel).
GALDER_CASE = dict(name="A58 west @ Breda — root follows A58 to Vlissingen, both A16 directions branch",
                   lng=5.04984, lat=51.53935, bearing=279, cap=200000)

# Off-motorway (Amsterdam centrum local road) -> NoSegment.
OFF_MOTORWAY_CASE = dict(name="off-motorway -> NoSegment",
                         lng=4.89218, lat=52.37320, bearing=90)

failures = []


def check(cond, label, detail=""):
    status = "PASS" if cond else "FAIL"
    print(f"  [{status}] {label}" + (f"  {detail}" if detail else ""))
    if not cond:
        failures.append(label)
    return cond


def get(path):
    """GET a /tree URL; return (json, http_status). NoSegment comes back as HTTP 400."""
    url = f"{BASE}{path}"
    try:
        with urllib.request.urlopen(url) as r:
            return json.load(r), r.status
    except urllib.error.HTTPError as e:
        return json.load(e), e.code


def tree_url(c):
    return (f"/tree/v1/driving/{c['lng']},{c['lat']}"
            f"?bearings={c['bearing']},45&hard_cap_m={c['cap']}&debug=true")


def iter_routes(route):
    yield route
    for b in route.get("branches", []):
        yield from iter_routes(b["route"])


def iter_branches(route):
    for b in route.get("branches", []):
        yield route, b
        yield from iter_branches(b["route"])


def all_junctions(tree):
    """Every debug junction across every segment in the tree."""
    for r in iter_routes(tree):
        for j in r.get("junctions", []):
            yield r, j


def run_qualify_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok" and http == 200,
                 f"{c['name']}: code Ok", f"http={http} code={tree.get('code')}"):
        return

    qualifying = [j["toward_ref"] for _, j in all_junctions(tree) if j["qualifies"]]
    # expected refs appear, in order, as a subsequence of the qualifying stream
    it = iter(qualifying)
    ordered = all(ref in it for ref in c["qualify_in_order"])
    check(ordered, f"{c['name']}: qualifiers {c['qualify_in_order']} in order",
          f"got {qualifying[:8]}")

    # fork diverges must be recognised as forks, not only off-ramps
    if c.get("fork_expected"):
        fork_ok = any(j["toward_ref"] == c["fork_expected"] and j["turn_type"] == "fork"
                      and j["qualifies"] for _, j in all_junctions(tree))
        check(fork_ok, f"{c['name']}: {c['fork_expected']} recognised as a qualifying fork")

    # afritten: seen (topology-permitting) but never qualifying, and never a branch ref
    branch_refs = [b["route"]["ref"] for _, b in iter_branches(tree)]
    n_road_branches = [r for r in branch_refs if r and r[0] == "N"]
    check(not n_road_branches, f"{c['name']}: no N-road ever becomes a branch",
          f"offenders={n_road_branches[:5]}")
    for ref in c["afritten"]:
        if ref in qualifying:
            check(False, f"{c['name']}: afrit {ref} must NOT qualify")


def run_ring_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    check(tree.get("code") == "Ok" and http == 200,
          f"{c['name']}: terminates cleanly",
          f"segments={tree.get('segment_count')} root_len_km={tree.get('length_m', 0)/1000:.0f}")


def run_concurrency_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok", f"{c['name']}: code Ok", f"http={http}"):
        return
    toward_self = [(r["ref"], int(j["at_offset_m"]))
                   for r, j in all_junctions(tree)
                   if j["qualifies"] and j["toward_ref"] == r["ref"]]
    check(not toward_self, f"{c['name']}: no qualifying junction toward its own road",
          f"offenders={toward_self[:5]}")


def run_breadth_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok", f"{c['name']}: code Ok", f"http={http}"):
        return
    check(len(tree.get("branches", [])) >= 4,
          f"{c['name']}: root has >= 4 first-level branches",
          f"root_branches={len(tree.get('branches', []))} segments={tree.get('segment_count')}")


def run_containment_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok", f"{c['name']}: code Ok", f"http={http}"):
        return
    escapes = [(r["ref"], round(r.get("non_motorway_m", 0)))
               for r in iter_routes(tree) if r.get("non_motorway_m", 0) > 1.0]
    check(not escapes,
          f"{c['name']}: every segment stays on motorway (non_motorway_m == 0)",
          f"escapes={escapes[:5]}")


def run_sameref_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok", f"{c['name']}: code Ok", f"http={http}"):
        return

    def comps(ref):
        return {x.strip().upper() for x in ref.split(";") if x.strip()}

    offenders = []

    def walk(r):
        pc = comps(r["ref"])
        for b in r["branches"]:
            if pc & comps(b["route"]["ref"]):
                offenders.append((r["ref"], b["route"]["ref"], round(b["junction"]["at_offset_m"])))
            walk(b["route"])

    walk(tree)
    check(not offenders,
          f"{c['name']}: no branch shares a ref component with its parent",
          f"offenders={offenders[:5]}")


def _decode_polyline(s, prec=1e5):
    coords, i, lat, lng = [], 0, 0, 0
    while i < len(s):
        for who in range(2):
            shift = res = 0
            while True:
                b = ord(s[i]) - 63
                i += 1
                res |= (b & 0x1F) << shift
                shift += 5
                if b < 0x20:
                    break
            d = ~(res >> 1) if res & 1 else res >> 1
            if who == 0:
                lat += d
            else:
                lng += d
        coords.append((lng / prec, lat / prec))
    return coords


def run_retry_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok", f"{c['name']}: code Ok", f"http={http}"):
        return
    # an A12 branch that starts near Prins Clausplein (lng < 4.42) must reach east past Zoetermeer.
    best_east = None
    for r in iter_routes(tree):
        if r["ref"] != "A12":
            continue
        co = _decode_polyline(r["polyline"])
        if co and co[0][0] < 4.42:
            east = max(x[0] for x in co)
            best_east = east if best_east is None else max(best_east, east)
    check(best_east is not None and best_east > 4.48,
          f"{c['name']}: A12-east branch reaches past Zoetermeer (lng > 4.48)",
          f"max_east_lng={best_east}")


def run_root_retry_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok", f"{c['name']}: code Ok (root snapped to a motorway)",
                 f"http={http} code={tree.get('code')}"):
        return
    check(tree.get("length_m", 0) > 20000,
          f"{c['name']}: root corridor > 20 km",
          f"ref={tree.get('ref')} len_km={tree.get('length_m', 0) / 1000:.1f}")


def _haversine(a, b):
    import math
    R = 6371000.0
    la1, la2 = math.radians(a[1]), math.radians(b[1])
    dla, dlo = math.radians(b[1] - a[1]), math.radians(b[0] - a[0])
    h = math.sin(dla / 2) ** 2 + math.cos(la1) * math.cos(la2) * math.sin(dlo / 2) ** 2
    return 2 * R * math.asin(math.sqrt(h))


def run_parallelbaan_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok", f"{c['name']}: code Ok", f"http={http}"):
        return

    root = _decode_polyline(tree["polyline"])[::4]
    # (a) no KEPT childless branch hugs the root line (an undropped parallelbaan stub). Exclude
    # cap-truncated branches: a real branch cut off at hard_cap can run parallel to the root and be
    # childless without being a rejoin stub (it would have continued/branched given more budget).
    hugging = []
    for _, b in iter_branches(tree):
        r = b["route"]
        if r["branches"] or r["length_m"] > 4000:
            continue
        if r["start_offset_m"] + r["length_m"] >= c["cap"] - 3000:
            continue  # cap-truncated, not a rejoin stub
        co = _decode_polyline(r["polyline"])
        if not co:
            continue
        near = sum(1 for x in co if min(_haversine(x, rp) for rp in root) < 120) / len(co)
        if near > 0.6:
            hugging.append((r["ref"], round(r["length_m"])))
    check(not hugging, f"{c['name']}: no childless branch hugs the root (parallel stub survived)",
          f"offenders={hugging[:5]}")

    # (b) over-drop guard: every dropped stub stays close to the kept tree (no unique alignment lost).
    kept = [p for r in iter_routes(tree) for p in _decode_polyline(r["polyline"])[::3]]
    worst = 0
    for d in tree.get("dropped_stubs", []):
        co = _decode_polyline(d["polyline"])[::3]
        for x in co:
            worst = max(worst, min(_haversine(x, kp) for kp in kept))
    check(worst < 1000,
          f"{c['name']}: dropped stubs hug the kept tree (max stray < 1 km)",
          f"dropped={len(tree.get('dropped_stubs', []))} max_stray_m={round(worst)}")


def run_directions_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok", f"{c['name']}: code Ok", f"http={http}"):
        return
    east = west = False
    for r in iter_routes(tree):
        if r["ref"] != "A12":
            continue
        co = _decode_polyline(r["polyline"])
        if not co or co[0][0] > 4.42:
            continue  # only the branches spawned near Prins Clausplein
        if max(x[0] for x in co) > 4.48:
            east = True
        if min(x[0] for x in co) < 4.36:
            west = True
    check(east and west,
          f"{c['name']}: both A12 east (toward Utrecht) and A12 west (toward Den Haag) present",
          f"east={east} west={west}")


def _frac_within(a, b, thr=150.0):
    """Fraction of points in polyline a that lie within thr metres of any point in b."""
    if not a or not b:
        return 0.0
    return sum(1 for x in a if min(_haversine(x, y) for y in b) < thr) / len(a)


_DIR_SAMPLE_M = (1000, 2000, 4000, 8000, 16000, 32000, 64000)


def _sample_along(coords):
    """Points at each _DIR_SAMPLE_M arc distance (mirrors the plugin's sampleAlong)."""
    out, acc, nxt = [], 0.0, 0
    for i in range(1, len(coords)):
        acc += _haversine(coords[i - 1], coords[i])
        while nxt < len(_DIR_SAMPLE_M) and acc >= _DIR_SAMPLE_M[nxt]:
            out.append(coords[i])
            nxt += 1
    return out


def _same_direction(a, b, thr=350.0, frac=0.75):
    """Do two branches from a shared start run the SAME way (not opposite directions)?

    Compares their coordinates at equal arc distances (the plugin's dir_samples test). Two ways
    around a ring, or the two opposite directions of a crossing, share a start yet their same-arc
    points sit km apart within the first km - so this is False for them. A twin ramp / parallel
    carriageway duplicate runs together, so it is True. This is the discriminator the pure spatial
    overlap (_frac_within) lacks: on a ring the two directions trace the same loop and so overlap
    spatially, but they are not the same directed corridor.
    """
    sa, sb = _sample_along(a), _sample_along(b)
    n = min(len(sa), len(sb))
    if n < 1:
        return False
    matched = sum(1 for i in range(n) if _haversine(sa[i], sb[i]) < thr)
    return matched >= frac * n


def run_signage_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok", f"{c['name']}: code Ok", f"http={http}"):
        return
    east_toward = west_toward = None
    for _, b in iter_branches(tree):
        if b["route"]["ref"] != "A12":
            continue
        co = _decode_polyline(b["route"]["polyline"])
        if not co or co[0][0] > 4.42:
            continue  # only the two directions spawned at the interchange
        toward = b["junction"].get("toward", [])
        if max(x[0] for x in co) > 4.48:
            east_toward = toward
        if min(x[0] for x in co) < 4.36:
            west_toward = toward
    check(east_toward is not None and "Utrecht" in east_toward and "Voorburg" not in east_toward,
          f"{c['name']}: A12-east carries its own refined signage (Utrecht, not the shared Voorburg)",
          f"east_toward={east_toward}")
    check(west_toward is not None and "Voorburg" in west_toward,
          f"{c['name']}: A12-west keeps its Den Haag/Voorburg signage",
          f"west_toward={west_toward}")


def run_dup_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok", f"{c['name']}: code Ok", f"http={http}"):
        return
    # (a) no root-level branch re-expands a parallel copy of the root corridor.
    root = _decode_polyline(tree["polyline"])[::3]
    reexpanders = [(b["route"]["ref"], round(b["route"]["length_m"] / 1000))
                   for b in tree.get("branches", [])
                   if _frac_within(_decode_polyline(b["route"]["polyline"])[::3], root) > 0.6]
    check(not reexpanders,
          f"{c['name']}: no root branch re-expands the root corridor (parallelbaan drop)",
          f"offenders={reexpanders[:5]}")

    # (b) no two same-ref children of one parent are the same physical corridor. A duplicate both
    # overlaps spatially AND runs the same direction: the twin entry ramps / parallel carriageways
    # this guards against share a start and run together. Two genuinely different directions from one
    # junction - the two ways around the A10 ring at KP Watergraafsmeer (Ring Noord vs Ring Oost),
    # surfaced once the spur-terminus splits (§S3-fix11) let a branch reach the ring - overlap
    # spatially on the loop but diverge in direction, so they are NOT duplicates (the plugin's own
    # arc-distance dedup keeps them, exactly as it keeps both directions of any crossing).
    dups = []
    for parent in iter_routes(tree):
        kids = parent.get("branches", [])
        decoded = [_decode_polyline(k["route"]["polyline"])[::4] for k in kids]
        for i in range(len(kids)):
            for j in range(i + 1, len(kids)):
                if kids[i]["route"]["ref"] != kids[j]["route"]["ref"]:
                    continue
                if _frac_within(decoded[i], decoded[j]) > 0.75 and _same_direction(
                        decoded[i], decoded[j]):
                    dups.append((parent["ref"], kids[i]["route"]["ref"]))
    check(not dups,
          f"{c['name']}: no same-ref sibling duplicates a corridor (overlap de-dup)",
          f"offenders={dups[:5]}")


def run_cloverleaf_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok" and tree.get("ref") == "A50",
                 f"{c['name']}: root snaps to A50", f"http={http} ref={tree.get('ref')}"):
        return

    # Places on the direct A50-root children of a given crossing ref (one set per emitted direction).
    def toward_sets(ref):
        return [set(b["junction"].get("toward", []))
                for b in tree.get("branches", []) if b["route"]["ref"] == ref]

    a1 = toward_sets("A1")
    a1_east = any({"Deventer", "Hengelo"} & p for p in a1)   # KP Beekbergen, eastbound
    a1_west = any("Amsterdam" in p for p in a1)              # westbound
    check(a1_east and a1_west,
          f"{c['name']}: both A1 directions at KP Beekbergen (east Deventer/Hengelo + west Amsterdam)",
          f"a1_toward={a1}")

    a28 = toward_sets("A28")
    a28_north = any({"Zwolle", "Groningen"} & p for p in a28)  # KP Hattemerbroek, northbound
    a28_south = any("Amersfoort" in p for p in a28)            # southbound
    check(a28_north and a28_south,
          f"{c['name']}: both A28 directions at KP Hattemerbroek (north Zwolle/Groningen + south Amersfoort)",
          f"a28_toward={a28}")


def run_hintham_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok" and tree.get("ref") == "A2",
                 f"{c['name']}: root snaps to A2", f"http={http} ref={tree.get('ref')}"):
        return
    # A59 must be a FIRST-LEVEL branch (a direct child of the root), not buried deep via A50/A58.
    a59 = [b for b in tree.get("branches", []) if b["route"]["ref"] == "A59"]
    check(bool(a59), f"{c['name']}: A59 is a first-level branch off the A2",
          f"first_level={[b['route']['ref'] for b in tree.get('branches', [])]}")
    # ... and the east direction (toward Oss / Nijmegen) is present with its own signage.
    a59_east = [b for b in a59 if {"Oss", "Nijmegen"} & set(b["junction"].get("toward", []))]
    check(bool(a59_east),
          f"{c['name']}: A59-east (toward Oss/Nijmegen) present as a first-level branch",
          f"a59_toward={[b['junction'].get('toward') for b in a59]}")


def run_budget_priority_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok", f"{c['name']}: code Ok", f"http={http}"):
        return
    # At the 200 km cap the tree truncates (segment_count == MAX_SEGMENTS). Every first-level spur
    # head must survive regardless of distance - the far Limburg spurs A76 (~119 km) and A79
    # (~130 km) are the ones pure nearest-first dropped by sinking the budget into near subtrees.
    first_level = {b["route"]["ref"]: b["junction"]["at_offset_m"] for b in tree.get("branches", [])}
    check(tree.get("segment_count") == 400,
          f"{c['name']}: tree is truncated at the segment cap (so ordering matters)",
          f"segment_count={tree.get('segment_count')}")
    for ref in ("A76", "A79"):
        check(ref in first_level and first_level[ref] > 100000,
              f"{c['name']}: far first-level spur {ref} (>100 km) survives the cap",
              f"first_level={ {r: round(o/1000) for r, o in first_level.items()} }")


def _continuation_child(route, ref):
    """A direct child branch of `route` that is a renumber continuation onto `ref`: it carries that
    ref and attaches by a continuation junction (exit_ref null, no ramp connector), not a diverge.
    Returns the whole branch ({junction, route}) so callers can inspect the junction marker."""
    for b in route.get("branches", []):
        j = b["junction"]
        if (b["route"]["ref"] == ref and j.get("exit_ref") is None and
                (j.get("connector_length_m") or 0) == 0):
            return b
    return None


def run_renumber_split_case(c):
    """Defect A (S3-fix11): the root's emitted ref is only the road physically driven up to the
    renumber; the rest of the corridor is an equal-or-longer continuation child on the new ref."""
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok" and tree.get("ref") == c["root_ref"],
                 f"{c['name']}: root snaps to {c['root_ref']}",
                 f"http={http} ref={tree.get('ref')}"):
        return
    # The mislabel bug emitted the snap ref for the whole corridor; the root segment must now be short.
    check(tree.get("length_m", 0) < c["max_root_m"],
          f"{c['name']}: {c['root_ref']} root segment is short (< {c['max_root_m']/1000:.0f} km), not the whole corridor",
          f"root_len_km={tree.get('length_m', 0)/1000:.1f}")
    cont_branch = _continuation_child(tree, c["cont_ref"])
    if not check(cont_branch is not None,
                 f"{c['name']}: a {c['cont_ref']} continuation child carries the corridor past the renumber",
                 f"first_level={[b['route']['ref'] for b in tree.get('branches', [])]}"):
        return
    cont = cont_branch["route"]
    # The continuation junction carries the explicit marker the BE keys kRoot/quota off (it is the
    # same drive relabelled, not a diverge - its ref differs from the parent's).
    check(cont_branch["junction"].get("continuation") is True,
          f"{c['name']}: the {c['cont_ref']} continuation junction carries continuation==true",
          f"junction={ {k: v for k, v in cont_branch['junction'].items() if k != 'toward'} }")
    check(cont.get("length_m", 0) > c["min_cont_m"],
          f"{c['name']}: the {c['cont_ref']} continuation reaches far (> {c['min_cont_m']/1000:.0f} km)",
          f"cont_len_km={cont.get('length_m', 0)/1000:.1f}")
    # The real crossings past the renumber hang off the continuation (its own first-level branches),
    # and the continuation's own ref is never re-emitted as a diverge off it (same-road suppression).
    cont_first_level = [b["route"]["ref"] for b in cont.get("branches", [])]
    check(c["cont_ref"] not in cont_first_level,
          f"{c['name']}: the driver's own {c['cont_ref']} is not a branch off its own continuation",
          f"cont_first_level={cont_first_level}")
    for ref in c["cont_crossings"]:
        check(ref in cont_first_level,
              f"{c['name']}: {ref} crossing is a first-level branch off the {c['cont_ref']} continuation",
              f"cont_first_level={cont_first_level}")


def run_galder_case(c):
    """Defect B (S3-fix12): at a concurrency un-bundle the root follows the driver's ref component,
    not the geometric mainline, and both directions of the abandoned mainline emit as branches."""
    print(f"\n# {c['name']}")
    tree, http = get(tree_url(c))
    if not check(tree.get("code") == "Ok" and tree.get("ref") == "A58",
                 f"{c['name']}: root snaps to A58", f"http={http} ref={tree.get('ref')}"):
        return
    # The whole root chain (A58 + any continuation) must carry the A58 component to the Vlissingen
    # side, not run off north to Moerdijk on the A16 mislabelled A58. The Vlissingen end is far west
    # (lng < 3.9); the pre-fix bug ran the root to ~4.65,51.80 (Moerdijk, well east).
    def chain_refs_and_west(route):
        refs = {x.strip() for x in route["ref"].split(";")}
        west = min((x[0] for x in _decode_polyline(route["polyline"])), default=99)
        for b in route.get("branches", []):
            if b["junction"].get("exit_ref") is None and (b["junction"].get("connector_length_m") or 0) == 0 \
                    and "A58" in {x.strip() for x in b["route"]["ref"].split(";")}:
                r2, w2 = chain_refs_and_west(b["route"])
                refs |= r2
                west = min(west, w2)
        return refs, west
    refs, west = chain_refs_and_west(tree)
    check("A58" in refs and west < 3.9,
          f"{c['name']}: root chain holds the A58 component to the Vlissingen side (lng < 3.9)",
          f"chain_refs={sorted(refs)} westmost_lng={west:.3f}")
    # Both A16 directions must appear as branches: south (toward Antwerpen) and north (toward
    # Rotterdam) - the north one is the abandoned mainline the fix now spawns.
    a16_toward = [b["junction"].get("toward", []) for _, b in iter_branches(tree)
                  if b["route"]["ref"] == "A16"]
    a16_north = any("Rotterdam" in t for t in a16_toward)
    a16_south = any("Antwerpen" in t for t in a16_toward)
    check(a16_north and a16_south,
          f"{c['name']}: both A16 directions branch (north toward Rotterdam + south toward Antwerpen)",
          f"a16_toward={a16_toward[:6]}")


def run_continuation_marker_case():
    """The `continuation:true` junction marker (S3-fix11) is additive and continuation-only: it
    appears only on renumber continuations (never emitted as false, never on a diverge). A diverge
    is reached by a ramp connector or carries an exit_ref; a continuation has neither."""
    print("\n# continuation marker only on renumber continuations")
    seen = 0
    offenders = []
    for c in QUALIFY_CASES + [VALBURG_CASE, DEHOEK_CASE, GALDER_CASE, DUP_CASE, HINTHAM_CASE]:
        tree, _ = get(tree_url(c))
        for _, b in iter_branches(tree):
            j = b["junction"]
            if "continuation" not in j:
                continue
            seen += 1
            # Present ⇒ exactly True, and on a continuation-shaped junction (no exit, no connector).
            if (j["continuation"] is not True or j.get("exit_ref") is not None or
                    (j.get("connector_length_m") or 0) != 0):
                offenders.append((c["name"], j.get("continuation"), j.get("exit_ref"),
                                  j.get("connector_length_m")))
    check(seen > 0 and not offenders,
          f"continuation marker seen ({seen}) and only ever true on a continuation junction",
          f"offenders={offenders[:3]}")


def run_off_motorway_case(c):
    print(f"\n# {c['name']}")
    tree, http = get(f"/tree/v1/driving/{c['lng']},{c['lat']}?bearings={c['bearing']},45")
    check(http == 400 and tree.get("code") == "NoSegment",
          f"{c['name']}: HTTP 400 NoSegment", f"http={http} code={tree.get('code')}")


def run_junction_names():
    print("\n# junction names are all \"\" (non-null)")
    names = []
    for c in QUALIFY_CASES + [RING_CASE, CONCURRENCY_CASE]:
        tree, _ = get(tree_url(c))
        for _, b in iter_branches(tree):
            names.append(b["junction"]["name"])
        for _, j in all_junctions(tree):
            names.append(j["name"])
    check(names and all(n == "" for n in names),
          "every junction/branch name is the empty string",
          f"checked={len(names)} distinct={sorted(set(names))[:3]}")


def main():
    print(f"OSRM /tree regression harness against {BASE}")
    for c in QUALIFY_CASES:
        run_qualify_case(c)
    run_ring_case(RING_CASE)
    run_concurrency_case(CONCURRENCY_CASE)
    run_breadth_case(BREADTH_CASE)
    run_containment_case(CONTAINMENT_CASE)
    run_sameref_case(SAMEREF_CASE)
    run_retry_case(RETRY_CASE)
    run_root_retry_case(ROOT_RETRY_CASE)
    run_parallelbaan_case(PARALLELBAAN_CASE)
    run_directions_case(DIRECTIONS_CASE)
    run_signage_case(SIGNAGE_CASE)
    run_dup_case(DUP_CASE)
    run_cloverleaf_case(CLOVERLEAF_CASE)
    run_hintham_case(HINTHAM_CASE)
    run_budget_priority_case(BUDGET_PRIORITY_CASE)
    run_renumber_split_case(VALBURG_CASE)
    run_renumber_split_case(DEHOEK_CASE)
    run_galder_case(GALDER_CASE)
    run_continuation_marker_case()
    run_off_motorway_case(OFF_MOTORWAY_CASE)
    run_junction_names()

    print()
    if failures:
        print(f"=== REGRESSION: {len(failures)} check(s) failed ===")
        for f in failures:
            print(f"  - {f}")
        sys.exit(1)
    print("=== all checks passed ===")
    sys.exit(0)


if __name__ == "__main__":
    main()
