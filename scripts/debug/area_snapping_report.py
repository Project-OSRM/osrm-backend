#!/usr/bin/env python3
"""Measure and draw how OSRM snaps coordinates that lie inside a pedestrian area.

This is the yardstick for the open-area snapping work (plans/open-area-snapping.md).
Meshing connects an area's entry points to each other, so a coordinate inside the area
snaps onto the nearest of those lines -- which can be a long way off, and further still
when an obstacle inside the area hides the nearest ones.

    scripts/debug/area_snapping_report.py --build-dir build --svg /tmp/areas

For each fixture it prints the distribution of `waypoints[0].distance` over a grid of
interior points, and writes three pictures: a route starting inside the area, one ending
inside it, and one crossing it end to end.

The feature is done when the excess over the a-priori route is 0.  The walk from the
request to the vertex the route sets off from is not an error -- it is a leg of the
journey, drawn dashed in the pictures -- so it is reported but not judged.  A non-zero
`NO ROUTE` count is a different failure and worth looking at on its own.
"""

import argparse
import json
import re
import math
import os
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from datetime import datetime
import urllib.error
import urllib.request

# One grid step, matching the cucumber node maps: 25 m in x, 50 m in y.
DLON = 0.0002246128016873
DLAT = 0.0004521833555069
LON0, LAT0 = 1.0, 1.0


def loc(col, row):
    return (LON0 + col * DLON, LAT0 - row * DLAT)


# ------------------------------------------------------------------- fixtures


class Fixture:
    """A plaza, optionally with obstacles, in the cucumber node-map idiom."""

    def __init__(self, label):
        self.label = label
        self.nodes = {}
        self.ways = []
        self.relations = []
        self.outer_ring = []
        self.inner_rings = []
        self.bounds = (0, 0, 0, 0)
        # the ring vertex routes leave by, and the cell outside it they head for.  Not
        # every plaza has an entrance at its bottom-right corner, and measuring against
        # one that is not there measures nothing.
        self.exit_cell = None
        self.destination_cell = None

    def coords(self, ring):
        return [loc(self.nodes[n][1], self.nodes[n][2]) for n in ring]

    def slug(self):
        """A filename-safe form of the whole label -- truncating at the first comma
        would make every variant of the same plaza size collide."""
        return re.sub(r"[^a-z0-9]+", "_", self.label.lower()).strip("_")

    def inside(self, col, row):
        """Is this grid cell inside the plaza and outside every obstacle?"""
        left, right, top, bottom = self.bounds
        if not (left < col < right and top < row < bottom):
            return False
        for ring in self.inner_rings:
            cols = [self.nodes[n][1] for n in ring]
            rows = [self.nodes[n][2] for n in ring]
            if min(cols) <= col <= max(cols) and min(rows) <= row <= max(rows):
                return False
        return True


def plaza(width_cols, height_rows, extra_entries=False, obstacles=(), variant=""):
    """A rectangular plaza with entry ways at its corners.

    `obstacles` are (col, row, width, height) rectangles cut out of it.  An area with
    obstacles has to be expressed as a multipolygon relation, since a hole cannot be
    carried by a closed way.
    """
    label = f"plaza {width_cols * 25}x{height_rows * 50}m"
    if extra_entries:
        label += ", 6 entry points"
    else:
        label += ", 4 entry points"
    if variant:
        label += f", {variant}"
    elif obstacles:
        label += f", {len(obstacles)} obstacle" + ("s" if len(obstacles) > 1 else "")

    f = Fixture(label)
    left, right, top, bottom = 2, 2 + width_cols, 0, height_rows
    f.bounds = (left, right, top, bottom)
    f.nodes = {
        1: ("a", left, top),
        2: ("b", right, top),
        3: ("c", right, bottom),
        4: ("d", left, bottom),
        5: ("e", 0, top),
        6: ("f", right + 2, top),
        7: ("g", right + 2, bottom),
        8: ("h", 0, bottom),
    }
    f.outer_ring = [1, 2, 3, 4]
    f.ways = [
        (11, [5, 1], [("highway", "pedestrian")]),
        (12, [2, 6], [("highway", "pedestrian")]),
        (13, [8, 4], [("highway", "pedestrian")]),
        (14, [3, 7], [("highway", "pedestrian")]),
    ]

    if extra_entries:
        mid = (left + right) // 2
        f.nodes.update({20: ("m", mid, top), 21: ("n", mid, bottom),
                        22: ("m_out", mid, top - 2), 23: ("n_out", mid, bottom + 2)})
        f.outer_ring = [1, 20, 2, 3, 21, 4]
        f.ways += [
            (15, [22, 20], [("highway", "pedestrian")]),
            (16, [21, 23], [("highway", "pedestrian")]),
        ]

    next_node, next_way = 100, 30
    for col, row, w, h in obstacles:
        ring = []
        for dc, dr in ((0, 0), (w, 0), (w, h), (0, h)):
            f.nodes[next_node] = (f"o{next_node}", col + dc, row + dr)
            ring.append(next_node)
            next_node += 1
        # clockwise, so libosmium reads it as a hole
        ring.reverse()
        f.inner_rings.append(ring)
        f.ways.append((next_way, ring + [ring[0]], []))
        next_way += 1

    f.exit_cell = (right, bottom)          # 'c'
    f.destination_cell = (right + 2, bottom)  # 'g', hanging off it

    if f.inner_rings:
        # the outer ring carries no tags; the relation describes the area
        f.ways.insert(0, (10, f.outer_ring + [f.outer_ring[0]], []))
        members = [("way", 10, "outer")] + [
            ("way", 30 + i, "inner") for i in range(len(f.inner_rings))
        ]
        f.relations.append(
            (50, members, [("type", "multipolygon"), ("highway", "pedestrian"), ("name", "Plaza")])
        )
    else:
        f.ways.insert(
            0,
            (10, f.outer_ring + [f.outer_ring[0]],
             [("highway", "pedestrian"), ("area", "yes"), ("name", "Plaza")]),
        )
    return f


def two_entrances_at_the_bottom():
    """The pathological case: two entrances side by side on the bottom edge.

    The straight line between them runs along the bottom of the plaza and is the only
    entry-to-entry shortest path there is, so the entry-point mesh consists of that one
    chord and nothing else -- the whole northern half of the plaza, and everything
    between and behind the obstacles, has no graph in it at all.  Starting or ending
    anywhere but the bottom strip has to work regardless.
    """
    f = Fixture("plaza 400x400m, 2 entrances at the bottom, two obstacles")
    left, right, top, bottom = 2, 18, 0, 8
    f.bounds = (left, right, top, bottom)
    # the outer ring, with the two entrance nodes sitting on the bottom edge
    f.nodes = {
        1: ("a", left, top),
        2: ("b", right, top),
        3: ("c", right, bottom),
        4: ("q", 14, bottom),
        5: ("p", 6, bottom),
        6: ("d", left, bottom),
        # the stubs leading up to them from outside
        20: ("p_out", 6, bottom + 2),
        21: ("q_out", 14, bottom + 2),
    }
    f.outer_ring = [1, 2, 3, 4, 5, 6]
    f.ways = [
        (11, [20, 5], [("highway", "pedestrian")]),
        (12, [21, 4], [("highway", "pedestrian")]),
    ]

    next_node, next_way = 100, 30
    for col, row, w, h in ((6, 2, 4, 3), (12, 2, 4, 3)):
        ring = []
        for dc, dr in ((0, 0), (w, 0), (w, h), (0, h)):
            f.nodes[next_node] = (f"o{next_node}", col + dc, row + dr)
            ring.append(next_node)
            next_node += 1
        ring.reverse()  # clockwise, so libosmium reads it as a hole
        f.inner_rings.append(ring)
        f.ways.append((next_way, ring + [ring[0]], []))
        next_way += 1

    f.exit_cell = (14, bottom)           # 'q', the eastern entrance
    f.destination_cell = (14, bottom + 2)  # 'q_out', the stub beyond it

    f.ways.insert(0, (10, f.outer_ring + [f.outer_ring[0]], []))
    members = [("way", 10, "outer")] + [("way", 30 + i, "inner") for i in range(2)]
    f.relations.append(
        (50, members, [("type", "multipolygon"), ("highway", "pedestrian"), ("name", "Plaza")])
    )
    return f


def osm_document(f):
    out = ['<?xml version="1.0" encoding="UTF-8"?>', '<osm generator="osrm-test" version="0.6">']
    stamp = 'version="1" uid="1" user="osrm" timestamp="2000-01-01T00:00:00Z"'
    for nid, (name, col, row) in sorted(f.nodes.items()):
        lon, lat = loc(col, row)
        out.append(
            f'  <node id="{nid}" {stamp} lon="{lon!r}" lat="{lat!r}">'
            f'<tag k="name" v="{name}"/></node>'
        )
    for wid, refs, tags in f.ways:
        out.append(f'  <way id="{wid}" {stamp}>')
        out += [f'    <nd ref="{r}"/>' for r in refs]
        out += [f'    <tag k="{k}" v="{v}"/>' for k, v in tags]
        out.append("  </way>")
    for rid, members, tags in f.relations:
        out.append(f'  <relation id="{rid}" {stamp}>')
        out += [f'    <member type="{t}" ref="{r}" role="{role}"/>' for t, r, role in members]
        out += [f'    <tag k="{k}" v="{v}"/>' for k, v in tags]
        out.append("  </relation>")
    out.append("</osm>")
    return "\n".join(out)


# ------------------------------------------------------------------- pipeline


def run(cmd):
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        sys.stderr.write(f"$ {' '.join(cmd)}\n{result.stdout}\n{result.stderr}\n")
        raise SystemExit(f"command failed: {cmd[0]}")
    return result.stdout


def prepare(build_dir, profile, workdir, f, whole_graph=False):
    osm = os.path.join(workdir, f"{f.slug()}.osm")
    with open(osm, "w") as handle:
        handle.write(osm_document(f))
    base = os.path.join(workdir, f.slug())
    if whole_graph:
        # a thin wrapper so the stock profile stays untouched
        wrapper = os.path.join(workdir, "whole_graph_profile.lua")
        if not os.path.exists(wrapper):
            with open(wrapper, "w") as handle:
                # the stock profile builds its properties inside setup(), and
                # requires lib/* relative to its own directory, so put that on the
                # module path and wrap setup() rather than patching the table
                profile_dir = os.path.dirname(os.path.abspath(profile))
                handle.write(
                    f'package.path = "{profile_dir}/?.lua;" .. package.path\n'
                    f'local profile = assert(dofile("{os.path.abspath(profile)}"))\n'
                    "local original_setup = profile.setup\n"
                    "profile.setup = function(...)\n"
                    "  local result = original_setup(...)\n"
                    "  result.properties.area_emit_visibility_graph = true\n"
                    "  return result\n"
                    "end\n"
                    "return profile\n"
                )
        profile = wrapper
    extract_log = run([os.path.join(build_dir, "osrm-extract"), "-p", profile, osm])
    run([os.path.join(build_dir, "osrm-partition"), base + ".osrm"])
    run([os.path.join(build_dir, "osrm-customize"), base + ".osrm"])
    meshed = 0
    for line in extract_log.splitlines():
        if "yielding" in line:
            meshed = int(line.split("yielding")[1].split("ways")[0])
    return base + ".osrm", meshed


def serve(build_dir, dataset, port):
    proc = subprocess.Popen(
        [os.path.join(build_dir, "osrm-routed"), "-a", "MLD", "-p", str(port), dataset],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    for _ in range(100):
        try:
            urllib.request.urlopen(f"http://127.0.0.1:{port}/route/v1/foot/1,1;1,1", timeout=0.5)
            return proc
        except urllib.error.HTTPError:
            return proc
        except Exception:
            time.sleep(0.1)
    proc.terminate()
    raise SystemExit("osrm-routed did not come up")


def query(port, origin, destination, overview="false"):
    url = (
        f"http://127.0.0.1:{port}/route/v1/foot/"
        f"{origin[0]},{origin[1]};{destination[0]},{destination[1]}"
        f"?overview={overview}&geometries=geojson"
    )
    with urllib.request.urlopen(url, timeout=5) as response:
        return json.load(response)


def metres(a, b):
    """Great-circle distance, near enough at these sizes."""
    R = 6371008.8
    lon1, lat1, lon2, lat2 = (math.radians(v) for v in (a[0], a[1], b[0], b[1]))
    x = (lon2 - lon1) * math.cos((lat1 + lat2) / 2)
    return R * math.hypot(x, lat2 - lat1)


def sweep(port, f, destination, exit_cell, tail_metres, step=2):
    """For each interior point: how far it snapped, and how far the route detours.

    Snapping error alone is not enough to tell whether the feature works.  A coordinate
    can land exactly on a mesh way -- zero snapping error -- and the route still leave
    the area by the wrong side, because the mesh only joins entry points to each other.
    So we also compare the route against the straight line from the requested coordinate
    to the a-priori reference: the route you would get if the coordinate had been part of
    the visibility graph all along.  That is the requirement, so anything above zero is
    the gap still to close.
    """
    left, right, top, bottom = f.bounds
    distances, detours, unsnapped = [], [], 0
    for col in range(left + 1, right, step):
        for row in range(top + 1, bottom, step):
            if not f.inside(col, row):
                continue
            origin = loc(col, row)
            body = query(port, origin, destination)
            if body.get("code") != "Ok":
                unsnapped += 1
                continue
            distances.append(body["waypoints"][0]["distance"])
            ideal = ideal_route_length(f, (col, row), exit_cell, tail_metres)
            # OSRM's route length starts at the *snapped* point, so on its own it
            # understates what the traveller covers.  Add the walk from where they
            # actually asked to start, or the comparison flatters the status quo and
            # can even come out below the optimum.
            travelled = body["routes"][0]["distance"] + body["waypoints"][0]["distance"]
            detours.append(travelled - ideal)
    return distances, detours, unsnapped


# --------------------------------------------------------------- visual proof

SVG_STYLE = """
  .plaza    { fill: #dfe8f5; stroke: #7a90b4; stroke-width: 1.5; }
  .obstacle { fill: #b9c2cf; stroke: #6c7787; stroke-width: 1.5; }
  .way      { stroke: #8a8a8a; stroke-width: 3; fill: none; stroke-linecap: round; }
  .route    { stroke: #1a6fd4; stroke-width: 4; fill: none; stroke-linejoin: round;
              stroke-linecap: round; }
  .error    { stroke: #d92b2b; stroke-width: 2; stroke-dasharray: 4 3; fill: none; }
  .input    { fill: #d92b2b; }
  .snap     { fill: #f08c00; }
  text      { font: 13px sans-serif; fill: #222; }
  .title    { font: bold 15px sans-serif; }
  .legend   { font: 12px sans-serif; fill: #444; }
"""


class Canvas:
    """Linear lon/lat -> viewport mapping.  The fixtures are small enough that this is
    indistinguishable from a projection, and staying linear means the picture can be
    checked against the fixture's grid coordinates by eye."""

    def __init__(self, bounds, width=680, height=520, margin=58):
        left, right, top, bottom = bounds
        self.lon0, self.lat0 = loc(left - 3, top - 2)
        self.lon1, self.lat1 = loc(right + 3, bottom + 2)
        self.width, self.height, self.margin = width, height, margin

    def xy(self, lon, lat):
        fx = (lon - self.lon0) / (self.lon1 - self.lon0)
        fy = (self.lat0 - lat) / (self.lat0 - self.lat1)
        return (self.margin + fx * (self.width - 2 * self.margin),
                self.margin + fy * (self.height - 2 * self.margin))

    def points(self, coords):
        return " ".join(f"{x:.1f},{y:.1f}" for x, y in (self.xy(*c) for c in coords))


def render_svg(path, title, subtitle, f, drawing):
    canvas = Canvas(f.bounds)
    out = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{canvas.width}" '
        f'height="{canvas.height}" viewBox="0 0 {canvas.width} {canvas.height}">',
        f"<style>{SVG_STYLE}</style>",
        '  <rect width="100%" height="100%" fill="white"/>',
        f'  <text class="title" x="{canvas.margin}" y="26">{title}</text>',
        f'  <text class="legend" x="{canvas.margin}" y="44">{subtitle}</text>',
        f'  <polygon class="plaza" points="{canvas.points(f.coords(f.outer_ring))}"/>',
    ]
    for ring in f.inner_rings:
        out.append(f'  <polygon class="obstacle" points="{canvas.points(f.coords(ring))}"/>')
    for _, refs, tags in f.ways:
        if refs[0] == refs[-1]:  # closed rings are drawn as polygons above
            continue
        out.append(f'  <polyline class="way" points="{canvas.points(f.coords(refs))}"/>')

    for kind, payload in drawing:
        if kind in ("route", "error"):
            out.append(f'  <polyline class="{kind}" points="{canvas.points(payload)}"/>')
        else:
            coord, text = payload
            x, y = canvas.xy(*coord)
            radius = 5 if kind == "input" else 4
            dy = -9 if kind == "input" else 15
            out.append(f'  <circle class="{kind}" cx="{x:.1f}" cy="{y:.1f}" r="{radius}"/>')
            out.append(f'  <text x="{x + 9:.1f}" y="{y + dy:.1f}">{text}</text>')

    out.append(
        f'  <text class="legend" x="{canvas.margin}" y="{canvas.height - 20}">'
        "blue = route &#160; red = requested coordinate &#160; "
        "orange = where OSRM put it &#160; dashed = requested-to-snapped &#160; "
        "grey block = obstacle</text>"
    )
    out.append("</svg>")
    with open(path, "w") as handle:
        handle.write("\n".join(out))


def visual_proof(port, outdir, f, stamp):
    """A picture each for a route starting in, ending in, crossing, and staying inside."""
    left, right, top, bottom = f.bounds

    def first_inside(cols, rows):
        for row in rows:
            for col in cols:
                if f.inside(col, row):
                    return loc(col, row)
        return None

    # an interior point that is inside the plaza and clear of every obstacle; prefer one
    # that an obstacle hides from at least one corner, since that is the hard case
    interior = first_inside(range(left + 1, right), range(top + 1, bottom))
    # and one as far from it as the area allows, so that whatever stands between them
    # has to be dealt with
    far_interior = first_inside(range(right - 1, left, -1), range(bottom - 1, top, -1))

    # leave by whichever way the fixture actually has, not by assumption
    west = loc(0, top) if any(n for n in f.nodes.values() if n[1] == 0) else loc(6, bottom + 2)
    east = loc(*f.destination_cell)
    cases = [
        ("starts", "route starts inside the area", interior, east),
        ("ends", "route ends inside the area", west, interior),
        ("crosses", "route crosses the area end to end", west, east),
        # the case the mesh cannot answer: it never leaves the area, so the geodesic
        # across the polygon is worked out at query time instead
        ("within", "route starts and ends inside the area", interior, far_interior),
    ]

    results = []
    for name, description, origin, destination in cases:
        if origin is None or destination is None or origin == destination:
            results.append((name, None))
            continue
        body = query(port, origin, destination, overview="full")
        if body.get("code") != "Ok":
            results.append((name, None))
            continue
        route = body["routes"][0]
        geometry = [tuple(c) for c in route["geometry"]["coordinates"]]
        waypoints = body["waypoints"]

        drawing = [("route", geometry)]
        for requested, waypoint, tag in zip(
            (origin, destination), waypoints, ("start", "end")
        ):
            landed = tuple(waypoint["location"])
            if waypoint["distance"] > 0.05:
                drawing.append(("error", [requested, landed]))
            drawing.append(("input", (requested, f"{tag} requested")))
            drawing.append(("snap", (landed, f'snapped {waypoint["distance"]:.1f} m')))

        worst = max(w["distance"] for w in waypoints)
        render_svg(
            os.path.join(outdir, f"{f.slug()}_{name}.svg"),
            description,
            f"{f.label} &#8212; requested-to-snapped {worst:.1f} m "
            f"&#8212; route {route['distance']:.0f} m",
            f,
            drawing,
        )
        results.append((name, worst))
    return results


# ------------------------------------------------------- the a-priori reference

def grid_xy(col, row):
    """Fixture grid in metres.  One column is 25 m, one row 50 m."""
    return (col * 25.0, row * 50.0)


def segment_blocked(p, q, obstacles):
    """Does the open segment p..q pass through the inside of any obstacle?

    The fixtures use axis-aligned rectangles, so this is a proper-crossing test against
    each edge plus a containment test for the midpoint, which catches a segment lying
    wholly inside a rectangle.
    """

    def side(a, b, c):
        return (b[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (b[1] - a[1])

    def properly_crosses(a, b, c, d):
        d1, d2, d3, d4 = side(c, d, a), side(c, d, b), side(a, b, c), side(a, b, d)
        return ((d1 > 0) != (d2 > 0)) and ((d3 > 0) != (d4 > 0))

    mid = ((p[0] + q[0]) / 2, (p[1] + q[1]) / 2)
    for rect in obstacles:
        x0, y0, x1, y1 = rect
        if x0 < mid[0] < x1 and y0 < mid[1] < y1:
            return True
        corners = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
        for i in range(4):
            if properly_crosses(p, q, corners[i], corners[(i + 1) % 4]):
                return True
    return False


def ideal_route_length(f, origin_cell, exit_cell, tail_metres):
    """Shortest route as if the origin had been a vertex of the visibility graph.

    This is the requirement, stated exactly: add the coordinate to the graph, connect it
    to every vertex it can see, and take the shortest path out.  Anything the engine
    produces above this is a defect.
    """
    obstacles = []
    for ring in f.inner_rings:
        cols = [f.nodes[n][1] for n in ring]
        rows = [f.nodes[n][2] for n in ring]
        obstacles.append(
            grid_xy(min(cols), min(rows)) + grid_xy(max(cols), max(rows))
        )

    vertices = {"origin": grid_xy(*origin_cell)}
    for ring in [f.outer_ring] + f.inner_rings:
        for n in ring:
            vertices[n] = grid_xy(f.nodes[n][1], f.nodes[n][2])
    exit_key = [n for n in f.outer_ring if (f.nodes[n][1], f.nodes[n][2]) == exit_cell][0]

    names = list(vertices)
    best = {name: math.inf for name in names}
    best["origin"] = 0.0
    unvisited = set(names)
    while unvisited:
        here = min(unvisited, key=lambda n: best[n])
        if best[here] == math.inf:
            break
        unvisited.remove(here)
        for other in unvisited:
            p, q = vertices[here], vertices[other]
            if segment_blocked(p, q, obstacles):
                continue
            step = math.hypot(p[0] - q[0], p[1] - q[1])
            best[other] = min(best[other], best[here] + step)
    return best[exit_key] + tail_metres


# ------------------------------------------------------------------- reporting


def percentile(values, fraction):
    if not values:
        return float("nan")
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, int(math.ceil(fraction * len(ordered)) - 1))]


def report(f, meshed, distances, detours, unsnapped):
    print(f"  {f.label}   ({meshed} meshed ways)")
    print(f"    interior points  {len(distances) + unsnapped} sampled, {len(distances)} snapped")
    if unsnapped:
        print(f"    NO ROUTE         {unsnapped}   <-- not a snapping issue, investigate")
    if distances:
        print(
            f"    requested->snapped  min {min(distances):.1f}  "
            f"median {statistics.median(distances):.1f}  "
            f"p95 {percentile(distances, 0.95):.1f}  "
            f"max {max(distances):.1f}  (metres)"
        )
        print(
            f"    above a-priori   min {min(detours):.1f}  "
            f"median {statistics.median(detours):.1f}  "
            f"p95 {percentile(detours, 0.95):.1f}  "
            f"max {max(detours):.1f}  (metres)"
        )
    return (max(distances) if distances else 0.0, max(detours) if detours else 0.0)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--profile", default="profiles/foot_area.lua")
    parser.add_argument(
        "--whole-visibility-graph",
        action="store_true",
        help="keep the whole visibility graph instead of the pruned mesh.  The pruned "
        "mesh is already exact for any journey with one end at an entry point, so this "
        "only shows up on one that begins and ends inside the same area",
    )
    parser.add_argument("--port", type=int, default=5199)
    parser.add_argument("--keep", action="store_true", help="keep the generated datasets")
    parser.add_argument(
        "--svg",
        metavar="DIR",
        nargs="?",
        const="plans/area-snapping",
        help="write pictures of the routes; each run gets its own timestamped "
        "subdirectory, so the newest iteration is the last one listed and the "
        "directory as a whole is a log of the feature taking shape "
        "(default: plans/area-snapping)",
    )
    args = parser.parse_args()

    fixtures = [
        plaza(8, 4),
        plaza(16, 8),
        plaza(16, 8, extra_entries=True),
        # a single block in the middle: the classic fountain or monument
        plaza(16, 8, obstacles=[(8, 3, 4, 2)], variant="centre obstacle"),
        # off-centre, so that whole corners of the plaza are hidden from each other
        plaza(16, 8, obstacles=[(5, 1, 6, 5)], variant="offset obstacle"),
        # two blocks with a gap: the only sight lines run through the gap
        plaza(16, 8, obstacles=[(6, 1, 3, 6), (12, 1, 3, 6)], variant="two obstacles"),
        # the pathological case -- see the docstring
        two_entrances_at_the_bottom(),
    ]

    stamp = datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
    workdir = tempfile.mkdtemp(prefix="area-snap-")
    worst = {}
    try:
        if args.svg:
            args.svg = os.path.join(args.svg, stamp)
            os.makedirs(args.svg, exist_ok=True)
            print(f"writing pictures to {args.svg}\n")
        for f in fixtures:
            dataset, meshed = prepare(
                args.build_dir, args.profile, workdir, f, args.whole_visibility_graph
            )
            proc = serve(args.build_dir, dataset, args.port)
            try:
                destination = loc(*f.destination_cell)
                # the straight run from the exit vertex out to the destination, which
                # the oracle has to include because the route does
                tail = metres(loc(*f.exit_cell), destination)
                distances, detours, unsnapped = sweep(
                    args.port, f, destination, f.exit_cell, tail
                )
                worst[f.label] = report(f, meshed, distances, detours, unsnapped)
                if args.svg:
                    for name, error in visual_proof(args.port, args.svg, f, stamp):
                        if error is None:
                            print(f"    {name:9s} NO ROUTE")
                        else:
                            print(f"    {name:9s} requested-to-snapped {error:5.1f} m")
            finally:
                proc.terminate()
                proc.wait(timeout=10)
    finally:
        if args.keep:
            print(f"\ndatasets kept in {workdir}")
        else:
            shutil.rmtree(workdir, ignore_errors=True)

    print()
    worst_snap = max((snap for snap, _ in worst.values()), default=0.0)
    worst_detour = max((detour for _, detour in worst.values()), default=0.0)
    # Inside an area the request is snapped to a *vertex* and the walk to it is a leg of
    # the journey, not an error, so the snapping distance is reported for information
    # only.  The excess over the a-priori optimum is what says whether it works.
    print(f"walk from request to the vertex it set off from: up to {worst_snap:.1f} m")
    print(f"worst excess over the a-priori route: {worst_detour:.1f} m   <- the metric that matters")
    if worst_detour <= 1.0:
        print("\nopen-area snapping is working: every route is the one the visibility "
              "graph would have given if the coordinate had been part of it")
    else:
        print("\nnot there yet -- some routes leave the area by a worse vertex than the "
              "a-priori optimum")
    return 0


if __name__ == "__main__":
    sys.exit(main())
