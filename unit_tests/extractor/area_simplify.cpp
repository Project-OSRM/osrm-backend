#include "extractor/area/simplify.hpp"

#include <boost/test/unit_test.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(area_simplify_test)

using namespace osrm;
using namespace osrm::extractor::area;

namespace
{

// A degree of longitude at the equator, so the fixtures can be written in metres.
constexpr double METRE = 1.0 / 111319.49079327358;

osmium::NodeRef at(osmium::object_id_type id, double x, double y)
{ return osmium::NodeRef{id, osmium::Location{x * METRE, y * METRE}}; }

OsmiumPolygon square_with(const std::vector<osmium::NodeRef> &outer)
{
    OsmiumPolygon poly;
    for (const auto &vertex : outer)
    {
        poly.outer().push_back(vertex);
    }
    return poly;
}

std::vector<osmium::object_id_type> ids(const OsmiumPolygon &poly)
{
    std::vector<osmium::object_id_type> result;
    for (const auto &vertex : poly.outer())
    {
        result.push_back(vertex.ref());
    }
    return result;
}

} // namespace

BOOST_AUTO_TEST_CASE(effective_area_of_a_right_triangle)
{
    // legs of 100 m and 200 m, so the triangle is 10000 square metres
    BOOST_CHECK_CLOSE(effective_area(at(1, 0, 0), at(2, 100, 0), at(3, 100, 200)), 10000.0, 0.5);
    // three points on a line enclose nothing
    BOOST_CHECK_SMALL(effective_area(at(1, 0, 0), at(2, 50, 0), at(3, 100, 0)), 1e-6);
}

// A vertex sitting on the straight run between its neighbours carries no shape.
BOOST_AUTO_TEST_CASE(drops_a_collinear_vertex)
{
    const auto poly = square_with({at(1, 0, 0),
                                   at(2, 50, 0), // on the line from 1 to 3
                                   at(3, 100, 0),
                                   at(4, 100, 100),
                                   at(5, 0, 100)});

    const auto simplified = simplify(poly, 1.0, {});
    BOOST_CHECK_EQUAL(simplified.outer().size(), 4u);
    const std::vector<osmium::object_id_type> expected{1, 3, 4, 5};
    BOOST_CHECK(ids(simplified) == expected);
}

// The corners of a square are the whole of its shape and none of them may go.
BOOST_AUTO_TEST_CASE(keeps_every_corner_of_a_square)
{
    const auto poly = square_with({at(1, 0, 0), at(2, 100, 0), at(3, 100, 100), at(4, 0, 100)});

    BOOST_CHECK_EQUAL(simplify(poly, 1.0, {}).outer().size(), 4u);
    // even a threshold far larger than the square itself cannot take it below three
    BOOST_CHECK_EQUAL(simplify(poly, 1e9, {}).outer().size(), 3u);
}

// A threshold of zero means the caller did not ask for this.
BOOST_AUTO_TEST_CASE(does_nothing_when_switched_off)
{
    const auto poly =
        square_with({at(1, 0, 0), at(2, 50, 0), at(3, 100, 0), at(4, 100, 100), at(5, 0, 100)});

    BOOST_CHECK_EQUAL(simplify(poly, 0.0, {}).outer().size(), 5u);
    BOOST_CHECK_EQUAL(simplify(poly, -1.0, {}).outer().size(), 5u);
}

/**
 * An entry point may not be simplified away.
 *
 * It is where a routable way meets the perimeter. Losing it does not cost detail, it
 * costs the connection: the area stops being reachable from that way.
 */
BOOST_AUTO_TEST_CASE(never_drops_a_pinned_vertex)
{
    const auto entrance = at(2, 50, 0);
    const auto poly =
        square_with({at(1, 0, 0), entrance, at(3, 100, 0), at(4, 100, 100), at(5, 0, 100)});

    const NodeRefSet keep{entrance};
    const auto simplified = simplify(poly, 1.0, keep);

    BOOST_CHECK_EQUAL(simplified.outer().size(), 5u);
    const std::vector<osmium::object_id_type> expected{1, 2, 3, 4, 5};
    BOOST_CHECK(ids(simplified) == expected);
}

// A bump larger than the threshold is shape, not noise.
BOOST_AUTO_TEST_CASE(keeps_a_vertex_that_carries_shape)
{
    // the bump encloses 0.5 * 100 * 20 = 1000 square metres against its neighbours
    const auto poly =
        square_with({at(1, 0, 0), at(2, 50, 20), at(3, 100, 0), at(4, 100, 100), at(5, 0, 100)});

    BOOST_CHECK_EQUAL(simplify(poly, 100.0, {}).outer().size(), 5u);
    BOOST_CHECK_EQUAL(simplify(poly, 2000.0, {}).outer().size(), 4u);
}

// Obstacles are simplified too, and an obstacle that would vanish is left as drawn.
BOOST_AUTO_TEST_CASE(simplifies_obstacle_rings)
{
    OsmiumPolygon poly =
        square_with({at(1, 0, 0), at(2, 1000, 0), at(3, 1000, 1000), at(4, 0, 1000)});
    poly.inners().emplace_back();
    for (const auto &vertex : {at(10, 400, 400),
                               at(11, 500, 400), // collinear with 10 and 12
                               at(12, 600, 400),
                               at(13, 600, 600),
                               at(14, 400, 600)})
    {
        poly.inners().back().push_back(vertex);
    }

    const auto simplified = simplify(poly, 1.0, {});
    BOOST_REQUIRE_EQUAL(simplified.inners().size(), 1u);
    BOOST_CHECK_EQUAL(simplified.inners()[0].size(), 4u);
    // the outer ring is untouched, having no vertex to spare
    BOOST_CHECK_EQUAL(simplified.outer().size(), 4u);
}

// Two runs over one input have to agree, or the extracted file stops being reproducible.
BOOST_AUTO_TEST_CASE(is_deterministic_over_tied_vertices)
{
    // every vertex of a regular polygon has the same effective area, so every removal is
    // a tie and the order has to come from the data rather than from the traversal
    OsmiumPolygon poly;
    for (int i = 0; i < 24; ++i)
    {
        const auto angle = 2 * M_PI * i / 24;
        poly.outer().push_back(
            at(100 + i, 500 + 100 * std::cos(angle), 500 + 100 * std::sin(angle)));
    }

    const auto once = simplify(poly, 200.0, {});
    const auto twice = simplify(poly, 200.0, {});
    BOOST_CHECK(ids(once) == ids(twice));
    BOOST_CHECK_LT(once.outer().size(), 24u);
}

BOOST_AUTO_TEST_SUITE_END()
