#include "engine/isochrone/cell_contours.hpp"

#include <boost/test/unit_test.hpp>

#include <bit>
#include <limits>
#include <string>
#include <vector>

namespace
{
constexpr double OUTSIDE = std::numeric_limits<double>::infinity();

double signedArea(const osrm::engine::isochrone::GridRing &ring)
{
    double twice_area = 0.;
    for (std::size_t index = 0; index + 1 < ring.size(); ++index)
    {
        const auto &from = ring[index];
        const auto &to = ring[index + 1];
        twice_area += static_cast<double>(from.x) * to.y - static_cast<double>(to.x) * from.y;
    }
    return twice_area / 2.;
}

bool isSimpleRing(const osrm::engine::isochrone::GridRing &ring)
{
    if (ring.size() < 4 || ring.front() != ring.back())
        return false;

    for (std::size_t left = 0; left + 1 < ring.size(); ++left)
    {
        for (std::size_t right = left + 1; right + 1 < ring.size(); ++right)
        {
            if (ring[left] == ring[right])
                return false;
        }
    }
    return true;
}

std::size_t sharedVertexCount(const osrm::engine::isochrone::GridRing &left,
                              const osrm::engine::isochrone::GridRing &right)
{
    std::size_t count = 0;
    for (auto left_vertex = left.begin(); left_vertex + 1 != left.end(); ++left_vertex)
    {
        for (auto right_vertex = right.begin(); right_vertex + 1 != right.end(); ++right_vertex)
        {
            if (*left_vertex == *right_vertex)
                ++count;
        }
    }
    return count;
}

std::string describeRing(const osrm::engine::isochrone::GridRing &ring)
{
    std::string description;
    for (const auto &point : ring)
    {
        if (!description.empty())
            description += " ";
        description += "(" + std::to_string(point.x) + "," + std::to_string(point.y) + ")";
    }
    return description;
}

void checkTopology(const std::vector<osrm::engine::isochrone::GridPolygon> &polygons)
{
    for (const auto &polygon : polygons)
    {
        BOOST_CHECK_MESSAGE(isSimpleRing(polygon.outer), describeRing(polygon.outer));
        BOOST_CHECK_GT(signedArea(polygon.outer), 0.);
        for (const auto &hole : polygon.holes)
        {
            BOOST_CHECK_MESSAGE(isSimpleRing(hole), describeRing(hole));
            BOOST_CHECK_LT(signedArea(hole), 0.);
        }
    }
}

double signedPolygonArea(const std::vector<osrm::engine::isochrone::GridPolygon> &polygons)
{
    double area = 0.;
    for (const auto &polygon : polygons)
    {
        area += signedArea(polygon.outer);
        for (const auto &hole : polygon.holes)
            area += signedArea(hole);
    }
    return area;
}
} // namespace

BOOST_AUTO_TEST_SUITE(isochrone_cell_contours)

BOOST_AUTO_TEST_CASE(contours_a_single_cell)
{
    const std::vector<double> values = {1.};
    const auto polygons = osrm::engine::isochrone::buildCellContours(values, 1, 1, 1.);

    BOOST_REQUIRE_EQUAL(polygons.size(), 1);
    BOOST_CHECK_EQUAL(polygons.front().outer.size(), 5);
    BOOST_CHECK(polygons.front().holes.empty());
    BOOST_CHECK(polygons.front().outer.front() == polygons.front().outer.back());
}

BOOST_AUTO_TEST_CASE(simplifies_collinear_cell_edges)
{
    const std::vector<double> values = {1., 1., 1.};
    const auto polygons = osrm::engine::isochrone::buildCellContours(values, 3, 1, 1.);

    BOOST_REQUIRE_EQUAL(polygons.size(), 1);
    BOOST_CHECK_EQUAL(polygons.front().outer.size(), 5);
}

BOOST_AUTO_TEST_CASE(preserves_holes)
{
    const std::vector<double> values = {1., 1., 1., 1., OUTSIDE, 1., 1., 1., 1.};
    const auto polygons = osrm::engine::isochrone::buildCellContours(values, 3, 3, 1.);

    BOOST_REQUIRE_EQUAL(polygons.size(), 1);
    BOOST_REQUIRE_EQUAL(polygons.front().holes.size(), 1);
    BOOST_CHECK_EQUAL(polygons.front().holes.front().size(), 5);
}

BOOST_AUTO_TEST_CASE(keeps_diagonally_touching_components_separate)
{
    const std::vector<double> values = {1., OUTSIDE, OUTSIDE, 1.};
    const auto polygons = osrm::engine::isochrone::buildCellContours(values, 2, 2, 1.);

    BOOST_REQUIRE_EQUAL(polygons.size(), 2);
    BOOST_CHECK(polygons[0].holes.empty());
    BOOST_CHECK(polygons[1].holes.empty());
    // The closed boundaries share the grid corner, while the occupied cells
    // are still two distinct four-connected components.
    BOOST_CHECK_EQUAL(sharedVertexCount(polygons[0].outer, polygons[1].outer), 1);
    checkTopology(polygons);
}

BOOST_AUTO_TEST_CASE(contours_a_concave_two_by_two_component)
{
    const std::vector<double> values = {0., OUTSIDE, 0., 0.};
    const auto polygons = osrm::engine::isochrone::buildCellContours(values, 2, 2, 0.);

    BOOST_REQUIRE_EQUAL(polygons.size(), 1);
    BOOST_CHECK(polygons.front().holes.empty());
    checkTopology(polygons);
}

BOOST_AUTO_TEST_CASE(applies_the_requested_cutoff)
{
    const std::vector<double> values = {1., 2., 3.};
    const auto polygons = osrm::engine::isochrone::buildCellContours(values, 3, 1, 2.);

    BOOST_REQUIRE_EQUAL(polygons.size(), 1);
    BOOST_CHECK_EQUAL(polygons.front().outer.size(), 5);
}

BOOST_AUTO_TEST_CASE(produces_simple_oriented_rings_for_every_four_by_four_cell_configuration)
{
    constexpr std::size_t width = 4;
    constexpr std::size_t height = 4;
    for (std::size_t mask = 0; mask < (std::size_t{1} << (width * height)); ++mask)
    {
        BOOST_TEST_CONTEXT("mask=" << mask)
        {
            std::vector<double> values(width * height, OUTSIDE);
            for (std::size_t index = 0; index < values.size(); ++index)
            {
                if ((mask & (std::size_t{1} << index)) != 0)
                    values[index] = 0.;
            }

            const auto polygons =
                osrm::engine::isochrone::buildCellContours(values, width, height, 0.);
            if (mask == 0)
                BOOST_CHECK(polygons.empty());
            else
                BOOST_REQUIRE_MESSAGE(!polygons.empty(), "mask=" << mask);
            checkTopology(polygons);
            BOOST_CHECK_EQUAL(signedPolygonArea(polygons),
                              static_cast<double>(std::popcount(mask)));
        }
    }
}

BOOST_AUTO_TEST_CASE(uses_the_reachable_saddle_policy_for_diagonal_cells)
{
    const std::vector<double> values = {0., OUTSIDE, OUTSIDE, 0.};
    const auto polygons = osrm::engine::isochrone::buildCellContours(values, 2, 2, 0.);

    BOOST_REQUIRE_EQUAL(polygons.size(), 2);
    BOOST_CHECK(polygons[0].holes.empty());
    BOOST_CHECK(polygons[1].holes.empty());
    // Closed raster-cell boundaries touch at the shared grid vertex, but the
    // two four-connected components remain distinct polygons.
    BOOST_CHECK_EQUAL(sharedVertexCount(polygons[0].outer, polygons[1].outer), 1);
    checkTopology(polygons);
}

BOOST_AUTO_TEST_CASE(preserves_a_hole_with_a_diagonal_exterior_contact)
{
    // The central unreachable cell and south-east exterior cell meet only at
    // a corner. Exact closed-cell coverage retains the hole rather than
    // filling an unreachable area. Its ring is simple, as is the outer ring,
    // but the two rings are tangent at the shared grid vertex.
    const std::vector<double> values = {0., 0., 0., 0., OUTSIDE, 0., 0., 0., OUTSIDE};
    const auto polygons = osrm::engine::isochrone::buildCellContours(values, 3, 3, 0.);

    BOOST_REQUIRE_EQUAL(polygons.size(), 1);
    BOOST_REQUIRE_EQUAL(polygons.front().holes.size(), 1);
    BOOST_CHECK_EQUAL(sharedVertexCount(polygons.front().outer, polygons.front().holes.front()), 1);
    checkTopology(polygons);
}

BOOST_AUTO_TEST_CASE(assigns_many_disjoint_holes_by_connected_component)
{
    // Every 3x3 donut is a distinct occupied component. Hole assignment is
    // linear in the number of rings: it uses the component recorded by the
    // boundary walk rather than testing every hole against every outer ring.
    constexpr std::size_t donut_count = 64;
    constexpr std::size_t width = donut_count * 4 - 1;
    constexpr std::size_t height = 3;
    std::vector<double> values(width * height, OUTSIDE);
    for (std::size_t donut = 0; donut < donut_count; ++donut)
    {
        const auto first_x = donut * 4;
        for (std::size_t y = 0; y < height; ++y)
        {
            for (std::size_t x = first_x; x < first_x + 3; ++x)
            {
                if (x != first_x + 1 || y != 1)
                    values[y * width + x] = 0.;
            }
        }
    }

    const auto polygons = osrm::engine::isochrone::buildCellContours(values, width, height, 0.);

    BOOST_REQUIRE_EQUAL(polygons.size(), donut_count);
    for (const auto &polygon : polygons)
        BOOST_CHECK_EQUAL(polygon.holes.size(), 1);
    checkTopology(polygons);
}

BOOST_AUTO_TEST_CASE(returns_rings_and_components_in_a_stable_canonical_order)
{
    const std::vector<double> values = {OUTSIDE,
                                        OUTSIDE,
                                        OUTSIDE,
                                        OUTSIDE,
                                        0.,
                                        OUTSIDE,
                                        0.,
                                        0.,
                                        0.,
                                        OUTSIDE,
                                        0.,
                                        OUTSIDE,
                                        0.,
                                        OUTSIDE,
                                        0.,
                                        OUTSIDE,
                                        OUTSIDE,
                                        OUTSIDE};

    const auto first = osrm::engine::isochrone::buildCellContours(values, 6, 3, 0.);
    const auto second = osrm::engine::isochrone::buildCellContours(values, 6, 3, 0.);

    BOOST_REQUIRE_EQUAL(first.size(), 2);
    BOOST_REQUIRE_EQUAL(second.size(), first.size());
    for (std::size_t index = 0; index < first.size(); ++index)
    {
        BOOST_CHECK(first[index].outer == second[index].outer);
        BOOST_CHECK(first[index].holes == second[index].holes);
    }
    BOOST_CHECK((first[0].outer.front() == osrm::engine::isochrone::GridPoint{0, 1}));
    BOOST_CHECK((first[1].outer.front() == osrm::engine::isochrone::GridPoint{4, 0}));
}

BOOST_AUTO_TEST_SUITE_END()
