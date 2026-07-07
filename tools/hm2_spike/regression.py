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
