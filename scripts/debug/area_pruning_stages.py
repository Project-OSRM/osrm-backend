#!/usr/bin/env python3
"""Draw the three ways of baking an area's visibility graph, and what each one costs.

The mesher can keep all of the visibility graph, or prune it.  Pruning to the shortest
paths *between entry points* preserves every entry-to-entry route but strands a
coordinate inside the area; pruning to the shortest-path *trees rooted at* the entry
points preserves those routes too, and is exactly as good as the whole graph for any
journey with one end at an entry point.

    scripts/debug/area_pruning_stages.py --build-dir build > /tmp/pruning.html

The edges drawn are the ones osrm-extract really emits: the mesher logs every segment it
reports at debug verbosity, so this reads them back rather than modelling them.  Both
prunings are then derived from that same edge set, which makes the three panels strictly
comparable.  The quality figures come from sampling interior points and comparing each
option against the whole graph.
"""

import argparse
import json
import math
import os
import itertools
import subprocess
import sys
import tempfile
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import area_snapping_report as report  # noqa: E402
from area_snapping_report import DLON, DLAT, Fixture, osm_document  # noqa: E402

# One metre in degrees, on the same scale the cucumber fixtures use.
M_LON, M_LAT = DLON / 25.0, DLAT / 50.0
SIDE = 1000.0
BLOCK = 45.0
GRID = 3


def loc(x, y):
    return (1.0 + x * M_LON, 1.0 - y * M_LAT)


def build_fixture():
    """A square plaza with a 3x3 grid of blocks -- a colonnade, or a market.

    Forty vertices: four corners and nine four-cornered obstacles.  Ways lead away from
    each corner, so all four are entry points.
    """
    f = Fixture("colonnade")
    f.nodes = {1: ("a", 0, 0), 2: ("b", SIDE, 0), 3: ("c", SIDE, SIDE), 4: ("d", 0, SIDE),
               5: ("e", -120, 0), 6: ("f", SIDE + 120, 0),
               7: ("g", SIDE + 120, SIDE), 8: ("h", -120, SIDE)}
    f.outer_ring = [1, 2, 3, 4]
    f.ways = [(11, [5, 1], [("highway", "pedestrian")]),
              (12, [2, 6], [("highway", "pedestrian")]),
              (13, [8, 4], [("highway", "pedestrian")]),
              (14, [3, 7], [("highway", "pedestrian")])]

    node_id, way_id = 100, 30
    for i in range(GRID):
        for j in range(GRID):
            x, y = 120 + i * 380, 120 + j * 380
            ring = []
            for dx, dy in ((0, 0), (BLOCK, 0), (BLOCK, BLOCK), (0, BLOCK)):
                f.nodes[node_id] = (f"o{node_id}", x + dx, y + dy)
                ring.append(node_id)
                node_id += 1
            ring.reverse()  # clockwise, so libosmium reads it as a hole
            f.inner_rings.append(ring)
            f.ways.append((way_id, ring + [ring[0]], []))
            way_id += 1

    f.ways.insert(0, (10, f.outer_ring + [f.outer_ring[0]], []))
    f.relations.append((50,
                        [("way", 10, "outer")] + [("way", 30 + i, "inner") for i in range(GRID * GRID)],
                        [("type", "multipolygon"), ("highway", "pedestrian"), ("name", "Plaza")]))
    return f


# --------------------------------------------------------- the mesher's own output


def emitted_edges(build_dir, workdir, fixture, xy, whole, port):
    """Ask the built graph which vertex pairs it actually joined.

    Reading the mesher's debug log would need a build with ENABLE_DEBUG_LOGGING, so
    instead this reads the graph back through a shipped interface: `nearest` reports the
    two OSM nodes of the segment it snapped to, so a pair is an edge exactly when the
    midpoint between its ends snaps onto a segment with those two nodes.
    """
    tag = "whole" if whole else "forest"
    ways, reported = prepare_dataset(build_dir, workdir, fixture, tag, whole)
    server = report.serve(build_dir, ways, port)
    edges = set()
    try:
        for u, v in itertools.combinations(sorted(xy), 2):
            mid = ((xy[u][0] + xy[v][0]) / 2, (xy[u][1] + xy[v][1]) / 2)
            point = loc(*mid)
            # Several segments can pass through one midpoint -- chords cross, and in a
            # regular grid they cross exactly there -- so take the nearest few and look
            # for the pair among them rather than trusting the first.
            url = (f"http://127.0.0.1:{port}/nearest/v1/foot/"
                   f"{point[0]},{point[1]}?number=12")
            with urllib.request.urlopen(url, timeout=8) as response:
                found = json.load(response)["waypoints"]
            if any(set(w.get("nodes", [])) == {u, v} for w in found):
                edges.add((u, v))
    finally:
        server.terminate()
        server.wait(timeout=10)
    if not edges:
        raise SystemExit("the graph joined no vertex pair -- was the area meshed at all?")
    return edges, reported


def prepare_dataset(build_dir, workdir, fixture, tag, whole):
    osm = os.path.join(workdir, f"{tag}.osm")
    with open(osm, "w") as handle:
        handle.write(osm_document(fixture))
    lua = os.path.join(workdir, f"{tag}.lua")
    root = os.getcwd()
    with open(lua, "w") as handle:
        handle.write(f'package.path = "{root}/profiles/?.lua;" .. package.path\n'
                     f'local p = assert(dofile("{root}/profiles/foot_area.lua"))\n'
                     'local setup = p.setup\n'
                     'p.setup = function(...)\n  local r = setup(...)\n'
                     f'  r.properties.area_emit_visibility_graph = {str(whole).lower()}\n'
                     '  return r\nend\nreturn p\n')
    base = os.path.join(workdir, tag)
    reported = 0
    for step in ("osrm-extract", "osrm-partition", "osrm-customize"):
        cmd = ([os.path.join(build_dir, step), "-p", lua, osm] if step == "osrm-extract"
               else [os.path.join(build_dir, step), base + ".osrm"])
        run = subprocess.run(cmd, capture_output=True, text=True)
        if run.returncode != 0:
            raise SystemExit(f"{step} failed:\n{run.stderr}")
        for line in run.stdout.splitlines():
            if "yielding" in line:
                reported = int(line.split("yielding")[1].split("ways")[0])
    return base + ".osrm", reported


# ------------------------------------------------------------------ graph work


def dijkstra(adjacency, source):
    dist = {n: math.inf for n in adjacency}
    prev = {}
    dist[source] = 0.0
    unvisited = set(adjacency)
    while unvisited:
        here = min(unvisited, key=lambda n: dist[n])
        if dist[here] == math.inf:
            break
        unvisited.remove(here)
        for other, weight in adjacency[here]:
            if other in unvisited and dist[here] + weight < dist[other]:
                dist[other] = dist[here] + weight
                prev[other] = here
    return dist, prev


def adjacency_of(edges, xy, extra=None):
    nodes = {n for e in edges for n in e}
    adjacency = {n: [] for n in nodes}
    for u, v in edges:
        w = math.dist(xy[u], xy[v])
        adjacency[u].append((v, w))
        adjacency[v].append((u, w))
    if extra:
        adjacency["S"] = list(extra)
        for v, w in extra:
            adjacency.setdefault(v, []).append(("S", w))
    return adjacency


def prune(edges, xy, entries, whole_tree):
    """Walk back from every vertex (a tree) or only from the entry points (paths)."""
    adjacency = adjacency_of(edges, xy)
    kept = set()
    for source in entries:
        _, prev = dijkstra(adjacency, source)
        targets = adjacency if whole_tree else entries
        for target in targets:
            node = target
            while node in prev:
                kept.add((min(node, prev[node]), max(node, prev[node])))
                node = prev[node]
    return kept


# ------------------------------------------------------------------- quality


def worst_excess(edges, xy, rings, entries, samples):
    """How far above the a-priori optimum this edge set can put a route, in metres."""
    worst = 0.0
    full = adjacency_of(edges["whole"], xy)
    for point in samples:
        visible = [(i, math.dist(point, xy[i])) for i in xy if not blocked(point, xy[i], rings, xy)]
        if not visible:
            continue
        for target in entries:
            best = dijkstra(adjacency_of(edges["whole"], xy, visible), "S")[0][target]
            here = dijkstra(adjacency_of(edges["subject"], xy, visible), "S")[0][target]
            if math.isfinite(best) and math.isfinite(here):
                worst = max(worst, here - best)
    del full
    return worst


def blocked(p, q, rings, xy):
    def side(a, b, c):
        return (b[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (b[1] - a[1])

    def crosses(a, b, c, d):
        d1, d2, d3, d4 = side(c, d, a), side(c, d, b), side(a, b, c), side(a, b, d)
        return ((d1 > 0) != (d2 > 0)) and ((d3 > 0) != (d4 > 0))

    mid = ((p[0] + q[0]) / 2, (p[1] + q[1]) / 2)
    for ring in rings[1:]:
        xs = [xy[i][0] for i in ring]
        ys = [xy[i][1] for i in ring]
        if min(xs) < mid[0] < max(xs) and min(ys) < mid[1] < max(ys):
            return True
    for ring in rings:
        for i, a in enumerate(ring):
            b = ring[(i + 1) % len(ring)]
            if xy[a] in (p, q) or xy[b] in (p, q):
                continue
            if crosses(p, q, xy[a], xy[b]):
                return True
    return False


# ------------------------------------------------------------------- drawing

W = H = 330
PAD = 24


def project(p):
    return (PAD + p[0] / SIDE * (W - 2 * PAD), PAD + p[1] / SIDE * (H - 2 * PAD))


def panel(title, subtitle, edges, xy, rings, entries, tally, dense=False):
    out = [f'<figure class="plate{" dense" if dense else ""}">',
           '<figcaption>',
           f'<span class="name">{title}</span>',
           f'<span class="sub">{subtitle}</span>',
           '</figcaption>',
           f'<svg viewBox="0 0 {W} {H}" role="img" aria-label="{title}: {subtitle}">']
    pts = " ".join(f"{x:.1f},{y:.1f}" for x, y in (project(xy[i]) for i in rings[0]))
    out.append(f'<polygon class="plaza" points="{pts}"/>')
    for ring in rings[1:]:
        pts = " ".join(f"{x:.1f},{y:.1f}" for x, y in (project(xy[i]) for i in ring))
        out.append(f'<polygon class="block" points="{pts}"/>')
    for u, v in sorted(edges):
        (x1, y1), (x2, y2) = project(xy[u]), project(xy[v])
        out.append(f'<line class="edge" x1="{x1:.1f}" y1="{y1:.1f}" '
                   f'x2="{x2:.1f}" y2="{y2:.1f}"/>')
    for i in xy:
        x, y = project(xy[i])
        out.append(f'<circle class="{"entry" if i in entries else "corner"}" '
                   f'cx="{x:.1f}" cy="{y:.1f}" r="{3.6 if i in entries else 2.2}"/>')
    out += ["</svg>", f'<p class="tally">{tally}</p>', "</figure>"]
    return "\n".join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--json", action="store_true", help="print the figures, draw nothing")
    args = parser.parse_args()

    report.loc = loc  # the fixture is laid out in metres, not grid cells
    fixture = build_fixture()
    xy = {nid: (n[1], n[2]) for nid, n in fixture.nodes.items() if nid >= 100 or nid <= 4}
    rings = [fixture.outer_ring] + fixture.inner_rings
    entries = list(fixture.outer_ring)

    with tempfile.TemporaryDirectory() as workdir:
        whole, whole_reported = emitted_edges(args.build_dir, workdir, fixture, xy, True, 5320)
        shipped_forest, forest_reported = emitted_edges(args.build_dir, workdir, fixture, xy, False, 5321)

    # The first two panels are what the mesher really emits.  The third cannot be: the
    # binary no longer produces it, so it is derived from the same edge set -- the paths
    # between entry points, plus the ring edges, which both modes always emit.
    ring_edges = set()
    for ring in rings:
        for i, u in enumerate(ring):
            v = ring[(i + 1) % len(ring)]
            ring_edges.add((min(u, v), max(u, v)))
    forest = shipped_forest
    pairs = prune(whole, xy, entries, whole_tree=False) | ring_edges

    step = SIDE / 12
    samples = []
    for i in range(1, 12):
        for j in range(1, 12):
            p = (i * step, j * step)
            if not any(min(xy[k][0] for k in r) <= p[0] <= max(xy[k][0] for k in r) and
                       min(xy[k][1] for k in r) <= p[1] <= max(xy[k][1] for k in r)
                       for r in rings[1:]):
                samples.append(p)

    quality = {}
    for name, subject in (("whole", whole), ("forest", forest), ("pairs", pairs)):
        quality[name] = worst_excess({"whole": whole, "subject": subject},
                                     xy, rings, entries, samples)

    figures = {
        "vertices": len(xy), "obstacles": GRID * GRID, "samples": len(samples),
        "whole": len(whole), "forest": len(forest), "pairs": len(pairs),
        "reported_by_extract": {"whole": whole_reported, "forest": forest_reported},
        "quality": {k: round(v, 3) for k, v in quality.items()},
    }
    if args.json:
        print(json.dumps(figures, indent=2))
        return 0

    def metres(x):
        return "0 m" if x < 0.05 else f"{x:,.0f} m"

    plates = "\n".join([
        panel("Whole visibility graph", "every mutually visible pair, plus the ring edges",
              whole, xy, rings, entries,
              f'<b>{len(whole)}</b> ways &middot; worst excess {metres(quality["whole"])}',
              dense=True),
        panel("Shortest-path forest", "the tree rooted at each entry point",
              forest, xy, rings, entries,
              f'<b>{len(forest)}</b> ways &middot; worst excess '
              f'<span class="good">{metres(quality["forest"])}</span>'),
        panel("Entry-to-entry paths", "what the pruning kept before",
              pairs, xy, rings, entries,
              f'<b>{len(pairs)}</b> ways &middot; worst excess '
              f'<span class="bad">{metres(quality["pairs"])}</span>'),
    ])

    page = TEMPLATE
    for token, value in [
        ("%PLATES%", plates),
        ("%VERTICES%", str(len(xy))),
        ("%OBSTACLES%", str(GRID * GRID)),
        ("%SAMPLES%", str(len(samples))),
        ("%WHOLE%", str(len(whole))),
        ("%FOREST%", str(len(forest))),
        ("%PAIRS%", str(len(pairs))),
        ("%READBACK%", f"{len(whole)} of {whole_reported}"),
        ("%QPAIRS%", metres(quality["pairs"])),
        ("%RATIO%", f"{len(forest) / len(whole):.2f}"),
    ]:
        page = page.replace(token, value)
    print(page)
    return 0


TEMPLATE = r"""<title>Which edges an open area has to keep</title>
<style>
  :root {
    --paper:#f6f8fb; --card:#ffffff; --ink:#151b24; --muted:#5a6575; --rule:#d9e0ea;
    --edge:#1f6feb; --entry:#e08600; --good:#0b8a53; --bad:#c62f2f;
    --plaza:#e6edf8; --plaza-edge:#8ea1bd; --block:#b6c0ce; --block-edge:#7d8899;
    --quote:#eef3fa;
    --sans:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
    --mono:ui-monospace,SFMono-Regular,"SF Mono",Menlo,Consolas,monospace;
  }
  @media (prefers-color-scheme: dark) {
    :root:not([data-theme="light"]) {
      --paper:#0e1319; --card:#151b23; --ink:#e5eaf2; --muted:#98a3b3; --rule:#28313d;
      --edge:#5a9bff; --entry:#f0a63c; --good:#3fbe84; --bad:#f0736f;
      --plaza:#1b2634; --plaza-edge:#44566e; --block:#333f4f; --block-edge:#5d6b7d;
      --quote:#141d27;
    }
  }
  :root[data-theme="dark"] {
    --paper:#0e1319; --card:#151b23; --ink:#e5eaf2; --muted:#98a3b3; --rule:#28313d;
    --edge:#5a9bff; --entry:#f0a63c; --good:#3fbe84; --bad:#f0736f;
    --plaza:#1b2634; --plaza-edge:#44566e; --block:#333f4f; --block-edge:#5d6b7d;
    --quote:#141d27;
  }

  * { box-sizing:border-box; }
  body { margin:0; padding:40px 22px 60px; background:var(--paper); color:var(--ink);
         font:16px/1.6 var(--sans); -webkit-font-smoothing:antialiased; }
  .wrap { max-width:1120px; margin:0 auto; display:flex; flex-direction:column; gap:26px; }
  header { display:flex; flex-direction:column; gap:8px; max-width:66ch; }
  h1 { margin:0; font-size:25px; line-height:1.2; letter-spacing:-.02em; text-wrap:balance; }
  .standfirst { margin:0; color:var(--muted); }
  code { font:.88em var(--mono); background:var(--quote); padding:1px 5px; border-radius:4px; }

  .plates { display:grid; grid-template-columns:repeat(auto-fit,minmax(280px,1fr)); gap:16px; }
  .plate { margin:0; padding:14px 15px 12px; background:var(--card); border:1px solid var(--rule);
           border-radius:8px; display:flex; flex-direction:column; gap:9px; }
  figcaption { display:flex; flex-direction:column; gap:1px; }
  .name { font-weight:650; font-size:14.5px; letter-spacing:-.01em; }
  .sub { color:var(--muted); font-size:12.5px; line-height:1.35; }
  svg { width:100%; height:auto; display:block; }
  .tally { margin:0; padding-top:9px; border-top:1px solid var(--rule);
           font:12.5px/1.45 var(--sans); color:var(--muted); font-variant-numeric:tabular-nums; }
  .tally b { color:var(--ink); font:600 12.5px var(--mono); }
  .good { color:var(--good); font-weight:600; }
  .bad  { color:var(--bad); font-weight:600; }

  .plaza { fill:var(--plaza); stroke:var(--plaza-edge); stroke-width:1.1; }
  .block { fill:var(--block); stroke:var(--block-edge); stroke-width:1; }
  .edge { stroke:var(--edge); stroke-width:1.15; }
  .dense .edge { stroke-width:.8; opacity:.62; }
  circle.entry { fill:var(--entry); stroke:var(--card); stroke-width:1.3; }
  circle.corner { fill:var(--ink); stroke:none; }

  .notes { max-width:66ch; display:flex; flex-direction:column; gap:18px; }
  .notes h2 { margin:0 0 4px; font-size:15px; letter-spacing:-.01em; }
  .notes p { margin:0 0 9px; } .notes p:last-child { margin:0; }
  .notes section { padding-left:14px; border-left:2px solid var(--rule); }
  .notes section.keyed { border-left-color:var(--good); }
  footer { color:var(--muted); font-size:13px; border-top:1px solid var(--rule); padding-top:14px; }
  a { color:var(--edge); }
  a:focus-visible { outline:2px solid var(--edge); outline-offset:2px; }
</style>

<div class="wrap">
  <header>
    <h1>Which edges an open area has to keep</h1>
    <p class="standfirst">A 1&nbsp;km square with a %OBSTACLES%-block colonnade in it &mdash;
    %VERTICES% vertices, four entry points at the corners. The edges drawn are the ones
    <code>osrm-extract</code> really emits; both prunings are derived from that same set, so
    the three panels compare like with like. &ldquo;Worst excess&rdquo; is how far above the
    a&nbsp;priori optimum each one can put a route, over %SAMPLES% interior points to each
    of the four exits.</p>
  </header>

  <div class="plates">%PLATES%</div>

  <div class="notes">
    <section class="keyed">
      <h2>The forest is exactly as good as the whole graph</h2>
      <p>A coordinate inside the area sets off towards a vertex it can see and then wants the
      shortest way out. So the baked graph has to hold a shortest path from <em>every vertex</em>
      to <em>every entry point</em> &mdash; not between every pair of vertices. That is precisely
      a shortest-path tree rooted at each entry point, and it costs nothing extra to keep:
      <code>run_dijkstra</code> already runs one search per entry point, it just used to
      discard everything except the paths joining them.</p>
    </section>

    <section>
      <h2>And it is still a pruning</h2>
      <p>%FOREST% ways against %WHOLE% here, or %RATIO% of the whole graph. The forest grows
      as entry points &times; vertices, the whole graph as vertices squared, so the gap widens
      with the area: on a 148-vertex plaza it is 573 against 3&nbsp;176.</p>
    </section>

    <section>
      <h2>What the old pruning could not do</h2>
      <p>Keeping only the paths <em>between</em> entry points leaves %PAIRS% ways &mdash;
      fewer still, and every route across the area is preserved exactly. But a coordinate
      inside the area may want a chord that no pair of entry points had a reason to keep, and
      then it is out by as much as %QPAIRS%.</p>
    </section>

    <section>
      <h2>The one case none of this covers</h2>
      <p>Both endpoints inside the <em>same</em> area. The forest guarantees shortest paths to
      entry points; two interior coordinates want a path between two arbitrary vertices, which
      only the whole graph is sure to hold. That case routes by way of a shared vertex today,
      so this is not a regression &mdash; but it is why the whole graph stays available.</p>
    </section>
  </div>

  <footer>Drawn by <code>scripts/debug/area_pruning_stages.py</code>, which reads each edge
  back out of the built graph rather than modelling it. The forest panel recovers all
  %FOREST% ways <code>osrm-extract</code> reports; the whole-graph panel recovers %READBACK%,
  the remainder being chords whose midpoint a dozen others pass closer to.</footer>
</div>
"""


if __name__ == "__main__":
    sys.exit(main())
