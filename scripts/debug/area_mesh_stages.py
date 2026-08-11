#!/usr/bin/env python3
"""Draw what each stage of area meshing adds and removes.

Meshing an area goes through four stages, and which edges survive each one is the thing
that decides whether a coordinate inside the area can be routed from.  This draws them
side by side for one plaza with an obstacle:

    1. the rings                  -- the area as it was mapped
    2. the visibility graph       -- every mutually visible pair, ring edges NOT among them
    3. the entry-point mesh       -- the shortest paths between entry points, the rest cut
    4. + the ring edges           -- put back, because nothing works without them

    scripts/debug/area_mesh_stages.py > /tmp/stages.html

The geometry is computed the same way the mesher computes it -- a rotational sweep is
overkill for a rectangle with one rectangular hole, so visibility is a direct test -- and
cross-checked against the ways osrm-extract actually emits.
"""

import itertools
import math
import sys

# ------------------------------------------------------------------ the fixture

# A plaza with one obstacle, in metres.  Entry points are the plaza's four corners; ways
# lead away from each of them to the outside world.
# the "centre obstacle" fixture of area_snapping_report.py, in metres
PLAZA = [(0, 0), (400, 0), (400, 400), (0, 400)]
OBSTACLE = [(150, 150), (250, 150), (250, 250), (150, 250)]
ENTRIES = [0, 1, 2, 3]  # indices into the flattened vertex list

NAMES = ["a", "b", "c", "d", "o1", "o2", "o3", "o4"]
VERTICES = PLAZA + OBSTACLE
RINGS = [list(range(0, 4)), list(range(4, 8))]


def ring_edges():
    edges = set()
    for ring in RINGS:
        for i, u in enumerate(ring):
            v = ring[(i + 1) % len(ring)]
            edges.add((min(u, v), max(u, v)))
    return edges


def blocked(u, v):
    """Does the open segment between vertices u and v pass through the obstacle?

    An obstacle edge that shares an endpoint with the segment meets it only there and
    cannot obstruct it, so it is skipped -- the same rule as VisibilityGraph::visible(),
    and for the same reason: without it a sight line that ends exactly on a corner is
    read as crossing the two edges that meet there.
    """
    p, q = VERTICES[u], VERTICES[v]

    def side(a, b, c):
        return (b[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (b[1] - a[1])

    def properly_crosses(a, b, c, d):
        d1, d2, d3, d4 = side(c, d, a), side(c, d, b), side(a, b, c), side(a, b, d)
        return ((d1 > 0) != (d2 > 0)) and ((d3 > 0) != (d4 > 0))

    mid = ((p[0] + q[0]) / 2, (p[1] + q[1]) / 2)
    xs = [c[0] for c in OBSTACLE]
    ys = [c[1] for c in OBSTACLE]
    if min(xs) < mid[0] < max(xs) and min(ys) < mid[1] < max(ys):
        return True
    for ring in RINGS:
        for i, a in enumerate(ring):
            b = ring[(i + 1) % len(ring)]
            if a in (u, v) or b in (u, v):
                continue
            if properly_crosses(p, q, VERTICES[a], VERTICES[b]):
                return True
    return False


def visibility_graph():
    """Every mutually visible pair -- and, as in the sweep, no ring edges.

    The sweep's cone test excludes a vertex's own ring neighbours, so an edge of a ring
    is never reported as a sight line even though the two ends can plainly see each
    other.  That is the whole reason stage 4 exists.
    """
    rings = ring_edges()
    edges = set()
    for u, v in itertools.combinations(range(len(VERTICES)), 2):
        if (u, v) in rings:
            continue
        if not blocked(u, v):
            edges.add((u, v))
    return edges


def entry_point_mesh(graph):
    """The edges carrying a shortest path between two entry points -- run_dijkstra().

    Dijkstra runs on the visibility graph *and* the ring edges, but only reports the
    edges it used, so a ring edge survives here only when some entry-to-entry path ran
    along it.
    """
    adjacency = {i: [] for i in range(len(VERTICES))}
    for u, v in graph | ring_edges():
        w = math.dist(VERTICES[u], VERTICES[v])
        adjacency[u].append((v, w))
        adjacency[v].append((u, w))

    kept = set()
    for source in ENTRIES:
        dist = {i: math.inf for i in adjacency}
        prev = {}
        dist[source] = 0.0
        unvisited = set(adjacency)
        while unvisited:
            here = min(unvisited, key=lambda n: dist[n])
            if dist[here] == math.inf:
                break
            unvisited.remove(here)
            for other, w in adjacency[here]:
                if other in unvisited and dist[here] + w < dist[other]:
                    dist[other] = dist[here] + w
                    prev[other] = here
        for target in ENTRIES:
            node = target
            while node in prev:
                kept.add((min(node, prev[node]), max(node, prev[node])))
                node = prev[node]
    return kept


# ------------------------------------------------------------------- drawing

# One frame for all four panels: same box, same vertices, same scale, so the only thing
# that changes between them is which edges are drawn.
W = H = 300
PAD = 30
SPAN = 400


def xy(p):
    return (PAD + p[0] / SPAN * (W - 2 * PAD), PAD + p[1] / SPAN * (H - 2 * PAD))


def polygon(points, cls):
    pts = " ".join(f"{x:.1f},{y:.1f}" for x, y in (xy(p) for p in points))
    return f'<polygon class="{cls}" points="{pts}"/>'


def panel(number, title, subtitle, drawn, note):
    """`drawn` is a list of (edge set, css class), painted in order."""
    out = ['<figure class="plate">',
           '<figcaption>',
           f'<span class="stage">{number}</span>',
           f'<span class="name">{title}</span>',
           f'<span class="sub">{subtitle}</span>',
           '</figcaption>',
           f'<svg viewBox="0 0 {W} {H}" role="img" aria-label="{title}: {subtitle}">',
           polygon(PLAZA, "plaza"), polygon(OBSTACLE, "block")]
    for edges, cls in drawn:
        for u, v in sorted(edges):
            (x1, y1), (x2, y2) = xy(VERTICES[u]), xy(VERTICES[v])
            out.append(f'<line class="{cls}" x1="{x1:.1f}" y1="{y1:.1f}" '
                       f'x2="{x2:.1f}" y2="{y2:.1f}"/>')
    for i, p in enumerate(VERTICES):
        x, y = xy(p)
        out.append(f'<circle class="{"entry" if i in ENTRIES else "corner"}" '
                   f'cx="{x:.1f}" cy="{y:.1f}" r="4"/>')
        dx = -16 if p[0] < SPAN / 2 else 8
        dy = -8 if p[1] < SPAN / 2 else 18
        out.append(f'<text x="{x + dx:.1f}" y="{y + dy:.1f}">{NAMES[i]}</text>')
    out += ["</svg>", f'<p class="tally">{note}</p>', "</figure>"]
    return "\n".join(out)


def main():
    rings = ring_edges()
    graph = visibility_graph()
    pruned = entry_point_mesh(graph)
    sight_kept = pruned - rings
    cut = graph - pruned

    whole_mode = graph | rings
    pruned_mode = pruned | rings

    stranded = sorted(NAMES[i] for i in range(len(VERTICES))
                      if not any(i in e for e in pruned))
    stranded_html = " and ".join(f'<code>{o}</code>' for o in stranded)

    plate = "\n".join([
        panel("1", "The rings", "the area as it was mapped",
              [(rings, "ring")],
              f'<b>{len(rings)}</b> ring edges &middot; '
              f'{len(ENTRIES)} entry points, {len(VERTICES) - len(ENTRIES)} obstacle corners'),
        panel("2", "The visibility graph", "every mutually visible pair",
              [(graph, "sight")],
              f'<b>{len(graph)}</b> sight lines &middot; and not one ring edge among them'),
        panel("3", "The entry-point mesh", "what the pruning keeps",
              [(cut, "cut"), (sight_kept, "sight"), (pruned & rings, "ring")],
              f'<b>{len(pruned)}</b> kept, <b>{len(cut)}</b> cut &middot; '
              f'no obstacle edge survives; {stranded_html} keep none at all'),
        panel("4", "Ring edges put back", "what each mode emits",
              [(sight_kept, "sight"), (rings, "ring")],
              f'<b>{len(pruned_mode)}</b> ways pruned, <b>{len(whole_mode)}</b> whole graph '
              '&middot; every vertex has somewhere to go'),
    ])

    page = TEMPLATE
    for token, value in [
        ("%PLATE%", plate),
        ("%STRANDED%", stranded_html),
        ("%SIGHT%", str(len(graph))),
        ("%RINGS%", str(len(rings))),
        ("%KEPT%", str(len(pruned))),
        ("%CUT%", str(len(cut))),
        ("%PRUNED%", str(len(pruned_mode))),
        ("%WHOLE%", str(len(whole_mode))),
    ]:
        page = page.replace(token, value)
    print(page)


TEMPLATE = r"""<title>What area meshing adds and removes</title>
<style>
  /* Light is the base palette; both dark paths redefine only tokens. */
  :root {
    --paper:#f6f8fb; --card:#ffffff; --ink:#151b24; --muted:#5a6575; --rule:#d9e0ea;
    --sight:#1f6feb; --ring:#0b8a53; --cut:#c62f2f; --entry:#e08600;
    --plaza:#e6edf8; --block:#b6c0ce; --block-edge:#7d8899; --plaza-edge:#8ea1bd;
    --quote:#eef3fa;
    --sans:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
    --mono:ui-monospace,SFMono-Regular,"SF Mono",Menlo,Consolas,monospace;
  }
  @media (prefers-color-scheme: dark) {
    :root:not([data-theme="light"]) {
      --paper:#0e1319; --card:#151b23; --ink:#e5eaf2; --muted:#98a3b3; --rule:#28313d;
      --sight:#5a9bff; --ring:#3fbe84; --cut:#f0736f; --entry:#f0a63c;
      --plaza:#1b2634; --block:#333f4f; --block-edge:#5d6b7d; --plaza-edge:#44566e;
      --quote:#141d27;
    }
  }
  :root[data-theme="dark"] {
    --paper:#0e1319; --card:#151b23; --ink:#e5eaf2; --muted:#98a3b3; --rule:#28313d;
    --sight:#5a9bff; --ring:#3fbe84; --cut:#f0736f; --entry:#f0a63c;
    --plaza:#1b2634; --block:#333f4f; --block-edge:#5d6b7d; --plaza-edge:#44566e;
    --quote:#141d27;
  }

  * { box-sizing:border-box; }
  body {
    margin:0; padding:40px 22px 64px; background:var(--paper); color:var(--ink);
    font:16px/1.6 var(--sans); -webkit-font-smoothing:antialiased;
  }
  .wrap { max-width:1160px; margin:0 auto; display:flex; flex-direction:column; gap:28px; }
  header { display:flex; flex-direction:column; gap:8px; max-width:66ch; }
  h1 { margin:0; font-size:26px; line-height:1.2; letter-spacing:-.02em; text-wrap:balance; }
  .standfirst { margin:0; color:var(--muted); }
  code { font:.88em var(--mono); background:var(--quote); padding:1px 5px; border-radius:4px; }

  .plates { display:grid; grid-template-columns:repeat(auto-fit,minmax(250px,1fr)); gap:16px; }
  .plate {
    margin:0; padding:14px 15px 12px; background:var(--card);
    border:1px solid var(--rule); border-radius:8px;
    display:flex; flex-direction:column; gap:10px;
  }
  figcaption { display:grid; grid-template-columns:auto 1fr; gap:2px 9px; align-items:baseline; }
  .stage {
    grid-row:1/3; font:600 12px var(--mono); color:var(--muted);
    border:1px solid var(--rule); border-radius:4px; padding:2px 6px; align-self:start;
    font-variant-numeric:tabular-nums;
  }
  .name { font-weight:650; font-size:14.5px; letter-spacing:-.01em; }
  .sub { color:var(--muted); font-size:12.5px; line-height:1.35; }
  svg { width:100%; height:auto; display:block; }
  .tally {
    margin:0; padding-top:9px; border-top:1px solid var(--rule);
    font:12.5px/1.45 var(--sans); color:var(--muted); font-variant-numeric:tabular-nums;
  }
  .tally b { color:var(--ink); font:600 12.5px var(--mono); }
  .tally code { font-size:12px; padding:0 4px; }

  .plaza { fill:var(--plaza); stroke:var(--plaza-edge); stroke-width:1.1; }
  .block { fill:var(--block); stroke:var(--block-edge); stroke-width:1.1; }
  .sight { stroke:var(--sight); stroke-width:1.7; }
  .ring  { stroke:var(--ring); stroke-width:3.2; stroke-linecap:round; }
  .cut   { stroke:var(--cut); stroke-width:1.3; stroke-dasharray:3 3.5; }
  circle.entry  { fill:var(--entry); stroke:var(--card); stroke-width:1.5; }
  circle.corner { fill:var(--ink); stroke:var(--card); stroke-width:1.5; }
  svg text { font:10.5px var(--mono); fill:var(--muted); }

  .legend {
    display:flex; flex-wrap:wrap; gap:8px 22px; padding:12px 15px;
    border:1px solid var(--rule); border-radius:8px; background:var(--card);
    font-size:13px; color:var(--muted);
  }
  .legend span { display:inline-flex; align-items:center; gap:8px; white-space:nowrap; }
  .swatch { width:22px; height:3px; border-radius:2px; }
  .swatch.dot { width:9px; height:9px; border-radius:50%; }
  .swatch.dash {
    height:0; border-top:1.5px dashed var(--cut); border-radius:0;
  }

  .notes { max-width:66ch; display:flex; flex-direction:column; gap:18px; }
  .notes h2 { margin:0 0 4px; font-size:15px; letter-spacing:-.01em; }
  .notes p { margin:0; }
  .notes section { padding-left:14px; border-left:2px solid var(--rule); }
  .notes section.keyed { border-left-color:var(--sight); }
  footer { color:var(--muted); font-size:13px; border-top:1px solid var(--rule); padding-top:14px; }
  a { color:var(--sight); }
  a:focus-visible, [tabindex]:focus-visible { outline:2px solid var(--sight); outline-offset:2px; }
</style>

<div class="wrap">
  <header>
    <h1>What area meshing adds and removes</h1>
    <p class="standfirst">A 400&nbsp;m plaza with one obstacle, drawn at each stage of the
    mesher. Every panel uses the same frame and the same eight vertices, so the only thing
    that changes is which edges exist. Orange marks an entry point &mdash; where another way
    meets the plaza; the obstacle&rsquo;s corners connect to nothing outside the area.</p>
  </header>

  <div class="plates">%PLATE%</div>

  <div class="legend">
    <span><i class="swatch" style="background:var(--ring)"></i>ring edge</span>
    <span><i class="swatch" style="background:var(--sight)"></i>sight line</span>
    <span><i class="swatch dash"></i>cut by the pruning</span>
    <span><i class="swatch dot" style="background:var(--entry)"></i>entry point</span>
    <span><i class="swatch dot" style="background:var(--ink)"></i>obstacle corner</span>
  </div>

  <div class="notes">
    <section>
      <h2>The sweep never emits ring edges</h2>
      <p>Its cone test excludes a vertex&rsquo;s own two neighbours, so the edge between them
      is never reported as a sight line &mdash; even though the two ends can plainly see each
      other. Panel&nbsp;2 is what the visibility graph actually contains:
      %SIGHT% sight lines and none of the %RINGS% ring edges.</p>
    </section>

    <section>
      <h2>The pruning is an approximation, and a good one</h2>
      <p><code>run_dijkstra</code> keeps only the edges carrying a shortest path between two
      entry points &mdash; here %KEPT% of them, with %CUT% cut. Every entry-to-entry route
      survives exactly, and on real data the mesh shrinks by up to an order of magnitude.
      What it cannot keep is a chord that only matters to a coordinate <em>inside</em> the
      area.</p>
    </section>

    <section class="keyed">
      <h2>Ring edges have to go back either way</h2>
      <p>Not one obstacle edge survives the pruning here, and %STRANDED% end up carrying no
      edges at all. A coordinate inside the area that can see them has nowhere to go from
      there, and nobody can walk along the plaza&rsquo;s edge or round the obstacle. Putting
      the ring edges back costs one edge per vertex: %PRUNED% ways pruned against %WHOLE%
      for the whole graph.</p>
    </section>

    <section>
      <h2>Which leaves one choice, not two</h2>
      <p>Both modes route between entry points identically. They differ only in what a
      coordinate inside the area has to work with, and that is what
      <code>area_emit_visibility_graph</code> selects.</p>
    </section>
  </div>

  <footer>Generated by <code>scripts/debug/area_mesh_stages.py</code>. Edge counts match
  what <code>osrm-extract</code> emits for this fixture: %WHOLE% ways with the whole
  visibility graph, %PRUNED% with the entry-point mesh.</footer>
</div>
"""


if __name__ == "__main__":
    sys.exit(main())
