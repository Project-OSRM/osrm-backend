#!/usr/bin/env python3
"""Highway Mode 2 - Stage 2 (linear walk) validation for the OSRM /tree plugin.

Drives the walk endpoint against known NL topology and checks that the qualifying
motorway-to-motorway junctions surface in the right order while ordinary exits (afritten,
N-roads) never qualify. Start points + bearings were derived by scanning a routed corridor
for the first vertex whose /tree snap reports the expected motorway ref (see probe.py for the
Stage 1 corridor method); they are pinned here so the check is reproducible.

Usage: python3 walk_probe.py [base_url]   (default http://127.0.0.1:5050)
"""
import json
import os
import sys
import urllib.error
import urllib.request

BASE = sys.argv[1] if len(sys.argv) > 1 else "http://127.0.0.1:5050"
OUTDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "evidence")

# name, lng,lat, bearing, hard_cap_m, expectations
CASES = [
    dict(name="A2 south (Everdingen then Deil)", lng=5.06526, lat=52.02042, bearing=145,
         cap=200000, expect_qualify_order=["A27", "A15"], expect_not_qualify=["N320", "N327"]),
    dict(name="A4 south (Badhoevedorp then Leiden N-ring)", lng=4.82557, lat=52.33885, bearing=268,
         cap=200000, expect_qualify_order=["A9", "A44"], expect_not_qualify=["N201", "N207", "N11"]),
    # hard_cap honoured: same A2 start, 20 km budget -> stops one segment past the cap (the walk
    # ends at the first segment whose *end* crosses hard_cap), far short of the 85 km uncapped walk.
    dict(name="A2 south, hard_cap 20km", lng=5.06526, lat=52.02042, bearing=145,
         cap=20000, expect_qualify_order=["A27", "A15"], expect_not_qualify=[], max_len_m=30000),
    # A10 Amsterdam ring: a cycle must terminate (visited-set + budget), length stays bounded.
    dict(name="A10 Amsterdam ring (cycle termination)", lng=4.84429, lat=52.38854, bearing=178,
         cap=200000, expect_qualify_order=[], expect_not_qualify=[], must_terminate=True),
]

# Off-motorway negative: the class post-filter must synthesize NoSegment (not snap to a local road).
NEGATIVE = dict(name="off-motorway (local road) -> NoSegment", lng=4.89218, lat=52.37320, bearing=90)


def get(url):
    try:
        with urllib.request.urlopen(url) as r:
            return json.load(r), 200
    except urllib.error.HTTPError as e:
        return json.load(e), e.code


def main():
    os.makedirs(OUTDIR, exist_ok=True)
    all_ok = True
    for c in CASES:
        # junctions[] is behind debug=true since Stage 3 made branches[] the primary output.
        url = (f"{BASE}/tree/v1/driving/{c['lng']},{c['lat']}"
               f"?bearings={c['bearing']},45&hard_cap_m={c['cap']}&debug=true")
        tree, _ = get(url)
        js = tree.get("junctions", [])
        qualify = [j["toward_ref"] for j in js if j["qualifies"]]
        notq = {j["toward_ref"] for j in js if not j["qualifies"] and j["toward_ref"]}
        length = tree.get("length_m", 0)

        print(f"\n### {c['name']}")
        print(f"    ref={tree.get('ref')} code={tree.get('code')} length_km={length/1000:.1f} "
              f"junctions={len(js)} qualifying={qualify[:10]}")

        ok = tree.get("code") == "Ok"
        # expected qualifiers appear in order (as a subsequence)
        it = iter(qualify)
        if not all(ref in it for ref in c["expect_qualify_order"]):
            ok = False
            print(f"    FAIL: expected qualifying order {c['expect_qualify_order']} not a subsequence")
        for ref in c["expect_not_qualify"]:
            if ref in qualify:
                ok = False
                print(f"    FAIL: {ref} qualified but must not")
            elif ref not in notq:
                print(f"    note: {ref} not seen on this branch (topology), skipping check")
        if "max_len_m" in c and length > c["max_len_m"]:
            ok = False
            print(f"    FAIL: length {length:.0f} exceeds cap expectation {c['max_len_m']}")
        if c.get("must_terminate") and tree.get("code") != "Ok":
            ok = False
            print("    FAIL: did not terminate cleanly")
        print("    PASS" if ok else "    *** CHECK FAILED ***")
        all_ok = all_ok and ok

        out = os.path.join(OUTDIR, "stage2_" + c["name"].split("(")[0].strip()
                           .replace(" ", "_").replace(",", "") + ".json")
        with open(out, "w") as fh:
            json.dump(dict(case=c, url=url, tree=tree), fh, indent=2)

    # negative
    nurl = f"{BASE}/tree/v1/driving/{NEGATIVE['lng']},{NEGATIVE['lat']}?bearings={NEGATIVE['bearing']},45"
    ntree, ncode = get(nurl)
    print(f"\n### {NEGATIVE['name']}")
    print(f"    http={ncode} code={ntree.get('code')} message={ntree.get('message','')}")
    neg_ok = ntree.get("code") == "NoSegment"
    print("    PASS" if neg_ok else "    *** CHECK FAILED ***")
    all_ok = all_ok and neg_ok
    with open(os.path.join(OUTDIR, "stage2_negative_off_motorway.json"), "w") as fh:
        json.dump(dict(case=NEGATIVE, url=nurl, http=ncode, tree=ntree), fh, indent=2)

    print(f"\n=== Stage 2 validation: {'ALL PASS' if all_ok else 'FAILURES ABOVE'} ===")
    sys.exit(0 if all_ok else 1)


if __name__ == "__main__":
    main()
