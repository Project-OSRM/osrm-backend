#!/usr/bin/env python3
"""Highway Mode 2 spike - Stage 1 evidence collector for the OSRM /tree plugin.

NOTE: this documents the Stage 1 per-diverge dump (start + outgoing_edges). The live /tree
endpoint has since become the Stage 2 linear walk (root segment + junctions[]); use
walk_probe.py for the current endpoint. The Stage 1 dumps are preserved in evidence/*.json.


For each hand-picked NL junction (knooppunt) and ordinary exit (afrit) this:
  1. Routes a corridor that the router takes mainline -> branch through the junction,
     so the diverge maneuver (off ramp / fork) is exposed in the /route response.
  2. Locates that diverge on the mainline geometry and places a probe a few hundred
     metres upstream, with the travel bearing along the corridor.
  3. Calls the spike /tree endpoint at the probe and records the raw JSON.

The whole point is to inspect, at the diverge edge-based node, the turn classification
and the destination:ref signage of each outgoing edge - the two signals the tree
traversal (Stages 2-3) will branch on.

Usage: python3 probe.py [base_url]   (default http://127.0.0.1:5050)
"""
import json
import math
import os
import sys
import urllib.request

BASE = sys.argv[1] if len(sys.argv) > 1 else "http://127.0.0.1:5050"
OUTDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "evidence")

# Each case routes from->to so the router goes mainline then onto `branch` at the junction.
# `branch` is matched (case-insensitive substring) against each off-ramp/fork maneuver's
# ref + destinations to locate the diverge. afrit cases end by taking the local exit, so
# their final off-ramp is the exit itself (branch text = the town/road it signs).
CASES = [
    # --- knooppunten: motorway-to-motorway (positive cases) ---
    dict(name="KP Everdingen (A2->A27)", kind="knooppunt",
         frm=(5.3037, 51.6978), to=(5.1214, 52.0907), branch="A27"),
    dict(name="KP Deil (A2->A15)", kind="knooppunt",
         frm=(5.1214, 52.0907), to=(5.4300, 51.8860), branch="A15"),
    dict(name="KP Badhoevedorp (A4->A9)", kind="knooppunt",
         frm=(4.4800, 51.9200), to=(4.7500, 52.6300), branch="A9"),
    dict(name="KP Ridderkerk (A15->A16)", kind="knooppunt",
         frm=(4.1200, 51.9500), to=(4.7760, 51.5860), branch="A16"),
    # --- afritten: ordinary local exits (negative cases) ---
    dict(name="Afrit Culemborg off A2", kind="afrit",
         frm=(5.3037, 51.6978), to=(5.2270, 51.9560), branch="Culemborg"),
    dict(name="Afrit Hoofddorp off A4", kind="afrit",
         frm=(4.4800, 51.9200), to=(4.6900, 52.3020), branch="Hoofddorp"),
    dict(name="Afrit Leiden off A4", kind="afrit",
         frm=(4.4800, 51.9200), to=(4.4970, 52.1600), branch="Leiden"),
]

BACKOFFS = (300, 600, 1000)   # metres upstream of the diverge to probe
BEARING_TOL = 45


def get(url):
    with urllib.request.urlopen(url) as r:
        return json.load(r)


def haversine(a, b):
    R = 6371000.0
    la1, la2 = math.radians(a[1]), math.radians(b[1])
    dla, dlo = math.radians(b[1] - a[1]), math.radians(b[0] - a[0])
    h = math.sin(dla / 2) ** 2 + math.cos(la1) * math.cos(la2) * math.sin(dlo / 2) ** 2
    return 2 * R * math.asin(math.sqrt(h))


def bearing(a, b):
    la1, la2 = math.radians(a[1]), math.radians(b[1])
    dlo = math.radians(b[0] - a[0])
    y = math.sin(dlo) * math.cos(la2)
    x = math.cos(la1) * math.sin(la2) - math.sin(la1) * math.cos(la2) * math.cos(dlo)
    return (math.degrees(math.atan2(y, x)) + 360) % 360


def find_diverge(route, branch):
    """Return the (lon,lat) of the off-ramp/fork maneuver whose ref+destinations
    contains `branch`, else the last off-ramp/fork in the route."""
    fallback = None
    b = branch.lower()
    for leg in route["legs"]:
        for step in leg["steps"]:
            if step["maneuver"]["type"] not in ("off ramp", "fork"):
                continue
            fallback = step["maneuver"]["location"]
            hay = (step.get("ref", "") + " " + step.get("destinations", "")).lower()
            if b in hay:
                return step["maneuver"]["location"]
    return fallback


def probe_points(coords, gore):
    gi = min(range(len(coords)), key=lambda i: haversine(coords[i], gore))
    pts = []
    for off in BACKOFFS:
        d, i = 0.0, gi
        while i > 0 and d < off:
            d += haversine(coords[i], coords[i - 1])
            i -= 1
        nxt = coords[i + 1] if i + 1 < len(coords) else gore
        pts.append((coords[i], bearing(coords[i], nxt), round(d)))
    return pts


def summarize(tree):
    if tree.get("code") != "Ok":
        return f"  <{tree.get('code')}: {tree.get('message','')}>"
    s = tree["start"]
    lines = [f"  START ref={s['ref']!r} classes={s['classes']} out_degree={tree['out_degree']}"]
    for e in tree["outgoing_edges"]:
        t = e["target"]
        lines.append(
            f"    turn={e['turn_type']:<9} mod={e['turn_modifier']:<8} | "
            f"tgt ref={t['ref']!r:<8} classes={str(t['classes']):<14} "
            f"dest={t['destinations']!r}"
        )
    return "\n".join(lines)


def main():
    os.makedirs(OUTDIR, exist_ok=True)
    for case in CASES:
        f, t = case["frm"], case["to"]
        url = (f"{BASE}/route/v1/driving/{f[0]},{f[1]};{t[0]},{t[1]}"
               "?overview=full&geometries=geojson&steps=true")
        route = get(url)["routes"][0]
        coords = route["geometry"]["coordinates"]
        gore = find_diverge(route, case["branch"])
        print(f"\n########## {case['name']}  [{case['kind']}]  gore={gore}")
        record = dict(case=case, gore=gore, probes=[])
        for (pt, brg, actual_off) in probe_points(coords, gore):
            turl = f"{BASE}/tree/v1/driving/{pt[0]:.6f},{pt[1]:.6f}?bearings={brg:.0f},{BEARING_TOL}"
            tree = get(turl)
            print(f"--- probe {pt[0]:.5f},{pt[1]:.5f} bearing={brg:.0f} (~{actual_off}m upstream)")
            print(summarize(tree))
            record["probes"].append(dict(point=pt, bearing=brg, offset_m=actual_off,
                                         url=turl, tree=tree))
        out = os.path.join(OUTDIR, case["name"].replace(" ", "_").replace("(", "").replace(")", "")
                           .replace("->", "to").replace("/", "_") + ".json")
        with open(out, "w") as fh:
            json.dump(record, fh, indent=2)


if __name__ == "__main__":
    main()
