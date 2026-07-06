#!/usr/bin/env python3
"""Highway Mode 2 - Stage 3 (recursive tree) validation for the OSRM /tree plugin.

Checks the nested branches[] tree against known NL topology:
  (a) A2 south  - the A27 child exists, has its own geometry, start_offset = junction + connector.
  (b) A4 south  - the A9 child exists; no N-road (Leiden ring) ever becomes a branch.
  (c) A10 ring  - terminates under a 50 km cap (cycles bounded); repeats allowed.
  (d) A2/A67    - no qualifying junction points back at the road the segment is on (concurrency).
  (e) every junction in the whole tree carries name:"" (non-null, per the locked contract).

Usage: python3 tree_probe.py [base_url]   (default http://127.0.0.1:5050)
"""
import json
import os
import sys
import urllib.error
import urllib.request

BASE = sys.argv[1] if len(sys.argv) > 1 else "http://127.0.0.1:5050"
OUTDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "evidence")


def get(url):
    try:
        with urllib.request.urlopen(url) as r:
            return json.load(r), 200
    except urllib.error.HTTPError as e:
        return json.load(e), e.code


def iter_routes(route):
    """Yield every route node in the tree (root first)."""
    yield route
    for b in route.get("branches", []):
        yield from iter_routes(b["route"])


def iter_branches(route):
    """Yield (parent_route, branch) for every branch in the tree."""
    for b in route.get("branches", []):
        yield route, b
        yield from iter_branches(b["route"])


def child_refs(route):
    return [b["route"]["ref"] for b in route.get("branches", [])]


def save(name, obj):
    with open(os.path.join(OUTDIR, "stage3_" + name + ".json"), "w") as fh:
        json.dump(obj, fh, indent=2)


def main():
    os.makedirs(OUTDIR, exist_ok=True)
    results = []

    # (a) A2 south: A27 child exists with geometry and start_offset = junction + connector.
    a2, _ = get(f"{BASE}/tree/v1/driving/5.06526,52.02042?bearings=145,45&hard_cap_m=60000&debug=true")
    save("a2_south_tree", a2)
    a27 = next((b for _, b in iter_branches(a2) if b["route"]["ref"] == "A27"), None)
    ok_a = (a2["code"] == "Ok" and a27 is not None
            and a27["route"]["length_m"] > 1000 and len(a27["route"]["polyline"]) > 20
            and abs(a27["route"]["start_offset_m"]
                    - (a27["junction"]["at_offset_m"] + a27["junction"]["connector_length_m"])) < 1.0)
    print(f"(a) A2 south A27 child: {'PASS' if ok_a else 'FAIL'}"
          + (f"  start_offset={a27['route']['start_offset_m']:.0f} "
             f"= junction {a27['junction']['at_offset_m']:.0f} + connector "
             f"{a27['junction']['connector_length_m']:.0f}, len={a27['route']['length_m']/1000:.0f}km"
             if a27 else "  (A27 child missing)"))
    results.append(ok_a)

    # (b) A4 south: A9 child present; no N-road ever a branch (Leiden ring must not qualify).
    a4, _ = get(f"{BASE}/tree/v1/driving/4.82557,52.33885?bearings=268,45&hard_cap_m=45000&debug=true")
    save("a4_south_tree", a4)
    all_branch_refs = [b["route"]["ref"] for _, b in iter_branches(a4)]
    a9_present = "A9" in all_branch_refs
    n_road_branches = [r for r in all_branch_refs if r and r[0] == "N"]
    ok_b = a4["code"] == "Ok" and a9_present and not n_road_branches
    print(f"(b) A4 south A9 child + no N-road branches: {'PASS' if ok_b else 'FAIL'}"
          f"  A9_present={a9_present} n_road_branches={n_road_branches[:4]}")
    results.append(ok_b)

    # (c) A10 ring, 50 km cap: terminates.
    a10, code_c = get(f"{BASE}/tree/v1/driving/4.84429,52.38854?bearings=178,45&hard_cap_m=50000")
    save("a10_ring_tree", a10)
    ok_c = a10.get("code") == "Ok"
    print(f"(c) A10 ring cap 50km terminates: {'PASS' if ok_c else 'FAIL'}"
          f"  segments={a10.get('segment_count')} root_len={a10.get('length_m',0)/1000:.0f}km")
    results.append(ok_c)

    # (d) A2/A67: no qualifying junction toward the segment's own road (needs debug junctions).
    a2d, _ = get(f"{BASE}/tree/v1/driving/5.06526,52.02042?bearings=145,45&hard_cap_m=90000&debug=true")
    toward_self = []
    for r in iter_routes(a2d):
        for j in r.get("junctions", []):
            if j["qualifies"] and j["toward_ref"] == r["ref"]:
                toward_self.append((r["ref"], int(j["at_offset_m"])))
    ok_d = a2d["code"] == "Ok" and not toward_self
    print(f"(d) A2/A67 no toward-self qualifying junction: {'PASS' if ok_d else 'FAIL'}"
          f"  offenders={toward_self[:5]}")
    results.append(ok_d)

    # (e) every junction name is "" across all trees probed above.
    names = []
    for tree in (a2, a4, a10, a2d):
        for _, b in iter_branches(tree):
            names.append(b["junction"]["name"])
        for r in iter_routes(tree):
            for j in r.get("junctions", []):
                names.append(j["name"])
    ok_e = all(n == "" for n in names)
    print(f"(e) every junction name is \"\": {'PASS' if ok_e else 'FAIL'}"
          f"  checked={len(names)} distinct={sorted(set(names))[:3]}")
    results.append(ok_e)

    print(f"\n=== Stage 3 validation: {'ALL PASS' if all(results) else 'FAILURES ABOVE'} ===")
    sys.exit(0 if all(results) else 1)


if __name__ == "__main__":
    main()
