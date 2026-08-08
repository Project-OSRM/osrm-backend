#include "extractor/area/visibility_graph.hpp"

#include "extractor/area/typedefs.hpp"
#include "extractor/area/util.hpp"

#include <boost/test/unit_test.hpp>

#include <osmium/osm/location.hpp>
#include <osmium/osm/node_ref.hpp>

#include <array>
#include <set>
#include <string>
#include <vector>

BOOST_AUTO_TEST_SUITE(area_visibility_graph_test)

using namespace osrm;
using namespace osrm::extractor::area;

namespace
{

// The test polygons are laid out on a small lon/lat grid.  One grid step is roughly
// 25 m in x and 50 m in y after projection, which is exactly the aspect ratio the
// cucumber node maps use, so the fixtures below can be read like those maps.
constexpr double LON0 = 1.0;
constexpr double LAT0 = 1.0;
constexpr double DLON = 0.000449225603374;
constexpr double DLAT = 0.000452183355507;

osmium::NodeRef node(osmium::object_id_type id, double col, double row)
{ return osmium::NodeRef{id, osmium::Location{LON0 + col * DLON, LAT0 - row * DLAT}}; }

/** Add a ring, as a sequence of (id, column, row) triples. */
template <typename Ring> void add_ring(Ring &ring, const std::vector<std::array<double, 3>> &pts)
{
    for (const auto &p : pts)
        ring.push_back(node(static_cast<osmium::object_id_type>(p[0]), p[1], p[2]));
}

/** All vertices of the polygon, which is what we usually want to observe from. */
NodeRefSet all_vertices(const OsmiumPolygon &poly)
{
    NodeRefSet work;
    for_each_ring(poly,
                  [&](const auto &ring)
                  {
                      for (const auto &n : ring)
                          work.emplace(n);
                  });
    return work;
}

/** Renders the result as a sorted, readable "1-2 1-3 ..." so failures are legible. */
std::string render(const std::set<OsmiumSegment> &segments)
{
    std::string out;
    for (const auto &s : segments)
    {
        if (!out.empty())
            out += " ";
        out += std::to_string(s.first.ref()) + "-" + std::to_string(s.second.ref());
    }
    return out;
}

bool has_segment(const std::set<OsmiumSegment> &segments,
                 osmium::object_id_type a,
                 osmium::object_id_type b)
{ return segments.contains(OsmiumSegment{osmium::NodeRef{a}, osmium::NodeRef{b}}); }

} // namespace

// A convex ring has no obstacles, so both diagonals are lines of sight.
//
// The ring edges themselves are *not* reported.  A vertex is never visible from its own
// ring neighbour when that neighbour is a genuine corner: in_open_cone() is asked whether
// the observer lies in the obstacle's interior cone at the vertex, and one arm of that
// cone points straight at the observer, which the reflex branch resolves to "inside".
// This is harmless for meshing, because AreaMesher::run_dijkstra() feeds the ring edges
// into the graph separately, but it is behaviour worth pinning down.
BOOST_AUTO_TEST_CASE(visibility_graph_convex_ring_yields_the_diagonals)
{
    OsmiumPolygon poly;
    // counter-clockwise, as libosmium delivers outer rings
    add_ring(poly.outer(), {{1, 0, 8}, {2, 8, 8}, {3, 8, 0}, {4, 0, 0}});

    NodeRefSet work = all_vertices(poly);
    VisibilityGraph vg;
    const auto segments = vg.run(poly, work);

    BOOST_CHECK_EQUAL(render(segments), "1-3 2-4");
}

// A vertex that is merely a collinear point along a straight edge is not a corner, so
// the exclusion above does not apply to it and it does see its ring neighbours.
BOOST_AUTO_TEST_CASE(visibility_graph_collinear_ring_point_sees_its_neighbours)
{
    OsmiumPolygon poly;
    // 5 sits halfway down the right hand edge between 2 and 3, so 2, 5 and 3 are collinear
    add_ring(poly.outer(), {{1, 0, 8}, {2, 8, 8}, {5, 8, 4}, {3, 8, 0}, {4, 0, 0}});

    NodeRefSet work = all_vertices(poly);
    VisibilityGraph vg;
    const auto segments = vg.run(poly, work);

    BOOST_CHECK(has_segment(segments, 2, 5));
    BOOST_CHECK(has_segment(segments, 5, 3));
    // while the two genuine corners it sits between still exclude each other
    BOOST_CHECK(!has_segment(segments, 2, 3));
}

// A hole in the middle blocks both diagonals but nothing along the perimeter.
BOOST_AUTO_TEST_CASE(visibility_graph_hole_blocks_the_diagonals)
{
    OsmiumPolygon poly;
    add_ring(poly.outer(), {{1, 0, 8}, {2, 8, 8}, {3, 8, 0}, {4, 0, 0}});
    {
        OsmiumPolygon::ring_type inner;
        // clockwise, as libosmium delivers inner rings
        add_ring(inner, {{5, 3, 5}, {6, 3, 3}, {7, 5, 3}, {8, 5, 5}});
        poly.inners().push_back(inner);
    }

    NodeRefSet work = all_vertices(poly);
    VisibilityGraph vg;
    const auto segments = vg.run(poly, work);

    // the hole sits across both diagonals of the outer ring
    BOOST_CHECK(!has_segment(segments, 1, 3));
    BOOST_CHECK(!has_segment(segments, 2, 4));
    // the diagonals of the hole itself run through its interior
    BOOST_CHECK(!has_segment(segments, 5, 7));
    BOOST_CHECK(!has_segment(segments, 6, 8));
    // a corner of the hole sees the corner of the outer ring it faces
    BOOST_CHECK(has_segment(segments, 4, 6));
}

// Only vertices in the work set are reported, even though the whole polygon is used
// to work out what blocks the view.
BOOST_AUTO_TEST_CASE(visibility_graph_reports_only_the_work_set)
{
    OsmiumPolygon poly;
    add_ring(poly.outer(), {{1, 0, 8}, {2, 8, 8}, {3, 8, 0}, {4, 0, 0}});

    NodeRefSet work;
    work.emplace(node(1, 0, 8));
    work.emplace(node(3, 8, 0));

    VisibilityGraph vg;
    const auto segments = vg.run(poly, work);

    BOOST_CHECK_EQUAL(render(segments), "1-3");
}

// An observer that is not a vertex of the polygon is skipped rather than crashing.
BOOST_AUTO_TEST_CASE(visibility_graph_skips_an_observer_outside_the_polygon)
{
    OsmiumPolygon poly;
    add_ring(poly.outer(), {{1, 0, 8}, {2, 8, 8}, {3, 8, 0}, {4, 0, 0}});

    NodeRefSet work = all_vertices(poly);
    work.emplace(node(99, 4, 4)); // never part of any ring

    VisibilityGraph vg;
    const auto segments = vg.run(poly, work);

    for (const auto &s : segments)
    {
        BOOST_CHECK(s.first.ref() != 99);
        BOOST_CHECK(s.second.ref() != 99);
    }
    BOOST_CHECK_EQUAL(render(segments), "1-3 2-4");
}

// Regression: the sweep used to keep its status ordered by distance along the sweep ray
// and test only the nearest edge.  A ray passing exactly through a vertex leaves a stale
// distance behind, which put a non-blocking edge in front and reported a blocked vertex
// as visible -- a line of sight straight through two holes.
//
// The fixture is the "Foot - Route across a complex multipolygon area" cucumber map:
//
//     g-a---------------b-h
//       | z-y           |
//       | | |           |
//     l-f | |      v--u |
//       | w-x      |  | |
//       |          |  | c-i
//       |          |  | |
//       |          s--t |
//     k-e---------------d-j
//
BOOST_AUTO_TEST_CASE(visibility_graph_does_not_see_through_two_holes)
{
    enum : osmium::object_id_type
    {
        a = 2,
        b = 3,
        z = 5,
        y = 6,
        f = 8,
        v = 9,
        u = 10,
        w = 11,
        x = 12,
        c = 13,
        s = 15,
        t = 16,
        e = 18,
        d = 19
    };

    OsmiumPolygon poly;
    add_ring(poly.outer(), {{e, 2, 8}, {d, 18, 8}, {c, 18, 5}, {b, 18, 0}, {a, 2, 0}, {f, 2, 3}});
    {
        OsmiumPolygon::ring_type inner;
        add_ring(inner, {{w, 4, 4}, {z, 4, 1}, {y, 6, 1}, {x, 6, 4}});
        poly.inners().push_back(inner);
    }
    {
        OsmiumPolygon::ring_type inner;
        add_ring(inner, {{s, 13, 7}, {v, 13, 3}, {u, 16, 3}, {t, 16, 7}});
        poly.inners().push_back(inner);
    }

    NodeRefSet work = all_vertices(poly);
    VisibilityGraph vg;
    const auto segments = vg.run(poly, work);

    // z is the top left corner of the left hole; c and d sit on the right edge of the
    // outer ring.  Both lines cross the right wall of the left hole *and* the whole of
    // the right hole.
    BOOST_CHECK(!has_segment(segments, z, c));
    BOOST_CHECK(!has_segment(segments, z, d));
    // the same holds for everything else hidden behind the left hole
    BOOST_CHECK(!has_segment(segments, z, u));
    BOOST_CHECK(!has_segment(segments, z, v));
    BOOST_CHECK(!has_segment(segments, z, s));

    // the lines of sight that make the scenario's expected routes possible
    BOOST_CHECK(has_segment(segments, a, u)); // g -> i goes a, u, c
    BOOST_CHECK(has_segment(segments, u, c));
    BOOST_CHECK(has_segment(segments, a, y)); // g -> j goes a, y, s, d
    BOOST_CHECK(has_segment(segments, y, s));
    BOOST_CHECK(has_segment(segments, s, d));

    BOOST_CHECK_EQUAL(segments.size(), 42u);
}

// Regression: an edge enters the sweep status at its first endpoint and stays there
// until the sweep reaches the second, so the status routinely holds edges incident to
// the very vertex being tested.  intersect() is supposed to ignore that endpoint, but
// solves for the segment parameters and lands on 0.99999999999995703437 instead of
// exactly 1, which reads as a proper crossing and destroyed the line of sight in both
// directions.
BOOST_AUTO_TEST_CASE(visibility_graph_edge_touching_the_target_does_not_hide_it)
{
    OsmiumPolygon poly;
    add_ring(poly.outer(),
             {{1, 0.00, 12.00}, {2, 8.00, 14.00}, {3, 16.00, 13.00}, {4, 14.00, 2.00}});
    {
        OsmiumPolygon::ring_type inner;
        // one corner of the hole is the target, and one of its edges runs back towards
        // the observer, so that edge is in the sweep status when the target is tested
        add_ring(inner, {{5, 5.00, 9.00}, {6, 5.00, 5.00}, {7, 9.00, 5.00}, {8, 9.00, 9.00}});
        poly.inners().push_back(inner);
    }

    NodeRefSet work = all_vertices(poly);
    VisibilityGraph vg;
    const auto segments = vg.run(poly, work);

    // vertex 4 of the outer ring has an unobstructed view of the near corners of the
    // hole; an edge merely touching the target must not hide it
    BOOST_CHECK(has_segment(segments, 4, 7));
    BOOST_CHECK(has_segment(segments, 4, 8));
    // and the far corners are still correctly hidden by the hole itself
    BOOST_CHECK(!has_segment(segments, 4, 5));
}

BOOST_AUTO_TEST_SUITE_END()
