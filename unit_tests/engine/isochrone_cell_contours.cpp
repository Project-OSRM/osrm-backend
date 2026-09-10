#include "engine/isochrone/cell_contours.hpp"
#include "engine/isochrone/grid_polygon_denoising.hpp"
#include "engine/isochrone/grid_polygon_generalization.hpp"

#include <boost/test/unit_test.hpp>

#include <bit>
#include <cstdint>
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

bool samePolygons(const std::vector<osrm::engine::isochrone::GridPolygon> &left,
                  const std::vector<osrm::engine::isochrone::GridPolygon> &right)
{
    if (left.size() != right.size())
        return false;

    for (std::size_t polygon = 0; polygon < left.size(); ++polygon)
    {
        if (left[polygon].outer != right[polygon].outer ||
            left[polygon].holes != right[polygon].holes)
        {
            return false;
        }
    }
    return true;
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

BOOST_AUTO_TEST_CASE(denoising_removes_only_rings_below_the_largest_outer_area_ratio)
{
    const std::vector<osrm::engine::isochrone::GridPolygon> original = {
        {{{0, 0}, {10, 0}, {10, 10}, {0, 10}, {0, 0}},
         {{{1, 1}, {1, 3}, {3, 3}, {3, 1}, {1, 1}},
          {{4, 1}, {4, 6}, {5, 6}, {5, 1}, {4, 1}},
          {{6, 1}, {6, 4}, {8, 4}, {8, 1}, {6, 1}}}},
        {{{20, 0}, {22, 0}, {22, 2}, {20, 2}, {20, 0}}, {}},
        {{{30, 0}, {35, 0}, {35, 1}, {30, 1}, {30, 0}}, {}}};
    auto polygons = original;

    BOOST_REQUIRE(osrm::engine::isochrone::denoiseGridPolygons(polygons, 0.05));
    BOOST_REQUIRE_EQUAL(polygons.size(), 2);
    BOOST_CHECK(polygons[0].outer == original[0].outer);
    BOOST_REQUIRE_EQUAL(polygons[0].holes.size(), 2);
    BOOST_CHECK(polygons[0].holes[0] == original[0].holes[1]);
    BOOST_CHECK(polygons[0].holes[1] == original[0].holes[2]);
    BOOST_CHECK(polygons[1].outer == original[2].outer);
    checkTopology(polygons);
}

BOOST_AUTO_TEST_CASE(denoising_one_retains_largest_outer_area_ties)
{
    const std::vector<osrm::engine::isochrone::GridPolygon> original = {
        {{{0, 0}, {10, 0}, {10, 10}, {0, 10}, {0, 0}}, {{{1, 1}, {1, 3}, {3, 3}, {3, 1}, {1, 1}}}},
        {{{20, 0}, {30, 0}, {30, 10}, {20, 10}, {20, 0}}, {}},
        {{{40, 0}, {45, 0}, {45, 5}, {40, 5}, {40, 0}}, {}}};
    auto polygons = original;

    BOOST_REQUIRE(osrm::engine::isochrone::denoiseGridPolygons(polygons, 1.));
    BOOST_REQUIRE_EQUAL(polygons.size(), 2);
    BOOST_CHECK(polygons[0].outer == original[0].outer);
    BOOST_CHECK(polygons[0].holes.empty());
    BOOST_CHECK(polygons[1].outer == original[1].outer);
}

BOOST_AUTO_TEST_CASE(denoising_rejects_invalid_inputs_without_modification)
{
    const std::vector<osrm::engine::isochrone::GridPolygon> original = {
        {{{0, 0}, {2, 0}, {2, 2}, {0, 2}, {0, 0}}, {}}};
    for (const auto threshold : {-1.,
                                 1.1,
                                 std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::infinity()})
    {
        auto polygons = original;
        BOOST_CHECK(!osrm::engine::isochrone::denoiseGridPolygons(polygons, threshold));
        BOOST_CHECK(samePolygons(polygons, original));
    }

    auto zero = original;
    BOOST_REQUIRE(osrm::engine::isochrone::denoiseGridPolygons(zero, 0.));
    BOOST_CHECK(samePolygons(zero, original));

    std::vector<osrm::engine::isochrone::GridPolygon> empty;
    BOOST_CHECK(osrm::engine::isochrone::denoiseGridPolygons(empty, 0.5));

    std::vector<osrm::engine::isochrone::GridPolygon> malformed = {{{{0, 0}, {1, 0}, {0, 1}}, {}}};
    const auto malformed_original = malformed;
    BOOST_CHECK(!osrm::engine::isochrone::denoiseGridPolygons(malformed, 0.5));
    BOOST_CHECK(samePolygons(malformed, malformed_original));
}

BOOST_AUTO_TEST_CASE(generalizes_a_staircase_deterministically)
{
    // A shallow sawtooth along the top of the polygon is one grid unit away from a straight edge.
    // A two-unit tolerance can replace it with that edge without altering the polygon topology.
    const std::vector<osrm::engine::isochrone::GridPolygon> original = {
        {{{0, 0}, {10, 0}, {10, 10}, {9, 10}, {9, 9}, {8, 9}, {8, 10}, {7, 10},
          {7, 9}, {6, 9},  {6, 10},  {5, 10}, {5, 9}, {4, 9}, {4, 10}, {3, 10},
          {3, 9}, {2, 9},  {2, 10},  {1, 10}, {1, 9}, {0, 9}, {0, 0}},
         {}}};

    auto first = original;
    auto second = original;
    BOOST_REQUIRE(osrm::engine::isochrone::generalizeGridPolygons(first, 2.));
    BOOST_REQUIRE(osrm::engine::isochrone::generalizeGridPolygons(second, 2.));

    BOOST_CHECK_LT(first.front().outer.size(), original.front().outer.size());
    BOOST_CHECK(samePolygons(first, second));
    checkTopology(first);
}

BOOST_AUTO_TEST_CASE(generalization_preserves_holes_and_shared_tangent_vertices)
{
    // The inner unreachable cell and the exterior meet at one corner. The simplifier must retain
    // that shared vertex whether it can simplify the other edges or falls back to the raw rings.
    const std::vector<double> values = {0., 0., 0., 0., OUTSIDE, 0., 0., 0., OUTSIDE};
    auto polygons = osrm::engine::isochrone::buildCellContours(values, 3, 3, 0.);

    BOOST_REQUIRE_EQUAL(polygons.size(), 1);
    BOOST_REQUIRE_EQUAL(polygons.front().holes.size(), 1);
    static_cast<void>(osrm::engine::isochrone::generalizeGridPolygons(polygons, 100.));

    BOOST_REQUIRE_EQUAL(polygons.size(), 1);
    BOOST_REQUIRE_EQUAL(polygons.front().holes.size(), 1);
    BOOST_CHECK_EQUAL(sharedVertexCount(polygons.front().outer, polygons.front().holes.front()), 1);
    checkTopology(polygons);
}

BOOST_AUTO_TEST_CASE(generalization_preserves_diagonal_component_contacts)
{
    const std::vector<double> values = {0., OUTSIDE, OUTSIDE, 0.};
    auto polygons = osrm::engine::isochrone::buildCellContours(values, 2, 2, 0.);

    BOOST_REQUIRE_EQUAL(polygons.size(), 2);
    static_cast<void>(osrm::engine::isochrone::generalizeGridPolygons(polygons, 100.));

    BOOST_REQUIRE_EQUAL(polygons.size(), 2);
    BOOST_CHECK_EQUAL(sharedVertexCount(polygons[0].outer, polygons[1].outer), 1);
    checkTopology(polygons);
}

BOOST_AUTO_TEST_CASE(generalization_backs_off_when_a_shortcut_crosses_another_component)
{
    // The C's simplification adds a diagonal from (8, 8) to (0, 0), which crosses the second
    // component. The unsafe candidate must be rejected before retrying at a smaller tolerance.
    const std::vector<osrm::engine::isochrone::GridPolygon> original = {
        {{{0, 0}, {8, 0}, {8, 8}, {0, 8}, {0, 6}, {6, 6}, {6, 2}, {0, 2}, {0, 0}}, {}},
        {{{4, 4}, {5, 4}, {5, 5}, {4, 5}, {4, 4}}, {}}};
    auto polygons = original;

    BOOST_REQUIRE(osrm::engine::isochrone::generalizeGridPolygons(polygons, 100.));
    BOOST_CHECK(polygons.front().outer == original.front().outer);
    BOOST_CHECK_LT(polygons[1].outer.size(), original[1].outer.size());
    checkTopology(polygons);
}

BOOST_AUTO_TEST_CASE(generalization_falls_back_when_a_shortcut_changes_component_nesting)
{
    // This component is outside the original C but fully inside its simplified triangle. The
    // boundaries do not intersect, so the nesting check is needed to reject the candidate.
    const std::vector<osrm::engine::isochrone::GridPolygon> original = {
        {{{0, 0}, {8, 0}, {8, 8}, {0, 8}, {0, 6}, {6, 6}, {6, 2}, {0, 2}, {0, 0}}, {}},
        {{{4, 2}, {5, 2}, {5, 3}, {4, 3}, {4, 2}}, {}}};
    auto polygons = original;

    BOOST_CHECK(!osrm::engine::isochrone::generalizeGridPolygons(polygons, 100.));
    BOOST_CHECK(samePolygons(polygons, original));
}

BOOST_AUTO_TEST_CASE(generalization_rejects_invalid_tolerances_without_modifying_the_input)
{
    const std::vector<osrm::engine::isochrone::GridPolygon> original = {
        {{{0, 0}, {10, 0}, {10, 10}, {0, 10}, {0, 0}}, {}}};
    for (const auto tolerance : {-1.,
                                 0.,
                                 std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::infinity()})
    {
        auto polygons = original;
        BOOST_CHECK(!osrm::engine::isochrone::generalizeGridPolygons(polygons, tolerance));
        BOOST_CHECK(samePolygons(polygons, original));
    }
}

BOOST_AUTO_TEST_CASE(generalization_rejects_malformed_rings_without_modifying_the_input)
{
    const std::vector<std::vector<osrm::engine::isochrone::GridPolygon>> malformed = {
        {{{}, {}}}, {{{{0, 0}, {1, 0}, {0, 1}}, {}}}, {{{{0, 0}, {1, 0}, {0, 1}, {0, 0}}, {{}}}}};

    for (const auto &original : malformed)
    {
        auto polygons = original;
        BOOST_CHECK(!osrm::engine::isochrone::generalizeGridPolygons(polygons, 1.));
        BOOST_CHECK(samePolygons(polygons, original));
    }
}

BOOST_AUTO_TEST_CASE(generalization_falls_back_above_its_coordinate_cap)
{
    osrm::engine::isochrone::GridRing ring;
    ring.reserve(100'001);
    for (std::int64_t x = 0; x < 99'998; ++x)
        ring.push_back({x, 0});
    ring.push_back({99'997, 1});
    ring.push_back({0, 1});
    ring.push_back({0, 0});

    const std::vector<osrm::engine::isochrone::GridPolygon> original = {{std::move(ring), {}}};
    auto polygons = original;

    BOOST_REQUIRE_EQUAL(original.front().outer.size(), 100'001);
    BOOST_CHECK(!osrm::engine::isochrone::generalizeGridPolygons(polygons, 1.));
    BOOST_CHECK(samePolygons(polygons, original));
}

BOOST_AUTO_TEST_CASE(generalization_falls_back_when_the_topology_budget_is_exhausted)
{
    osrm::engine::isochrone::GridRing outer;
    outer.reserve(50'004);
    outer.push_back({0, 0});
    outer.push_back({25'000, 0});
    outer.push_back({25'000, 10});
    for (std::int64_t x = 24'999; x > 0; x -= 2)
    {
        outer.push_back({x, 10});
        outer.push_back({x, 9});
        outer.push_back({x - 1, 9});
        outer.push_back({x - 1, 10});
    }
    outer.push_back({0, 0});

    std::vector<osrm::engine::isochrone::GridPolygon> original = {{std::move(outer), {}}};
    for (std::int64_t index = 0; index < 24; ++index)
    {
        const auto x = 10'000 + 3 * index;
        original.front().holes.push_back({{x, 2}, {x, 3}, {x + 1, 3}, {x + 1, 2}, {x, 2}});
    }

    auto within_budget = original;
    within_budget.front().holes.resize(8);
    BOOST_REQUIRE(osrm::engine::isochrone::generalizeGridPolygons(within_budget, 100.));
    BOOST_CHECK_LT(within_budget.front().outer.size(), original.front().outer.size());

    auto polygons = original;
    BOOST_CHECK(!osrm::engine::isochrone::generalizeGridPolygons(polygons, 100.));
    BOOST_CHECK(samePolygons(polygons, original));
}

BOOST_AUTO_TEST_CASE(generalization_handles_a_huge_finite_tolerance)
{
    const std::vector<osrm::engine::isochrone::GridPolygon> original = {
        {{{0, 0}, {10, 0}, {10, 10}, {9, 10}, {9, 9}, {8, 9}, {8, 10}, {0, 10}, {0, 0}}, {}}};
    auto polygons = original;

    BOOST_REQUIRE(osrm::engine::isochrone::generalizeGridPolygons(
        polygons, std::numeric_limits<double>::max()));
    BOOST_CHECK_LE(polygons.front().outer.size(), original.front().outer.size());
    checkTopology(polygons);
}

BOOST_AUTO_TEST_CASE(generalization_uses_exact_predicates_at_int64_limits)
{
    const auto minimum = std::numeric_limits<std::int64_t>::min();
    const auto maximum = std::numeric_limits<std::int64_t>::max();
    const std::vector<osrm::engine::isochrone::GridPolygon> original = {{{{minimum, minimum},
                                                                          {maximum, minimum},
                                                                          {maximum, maximum},
                                                                          {minimum, maximum},
                                                                          {minimum, minimum}},
                                                                         {}}};
    auto polygons = original;

    BOOST_REQUIRE(osrm::engine::isochrone::generalizeGridPolygons(
        polygons, std::numeric_limits<double>::max()));
    BOOST_CHECK_LT(polygons.front().outer.size(), original.front().outer.size());
    checkTopology(polygons);
}

BOOST_AUTO_TEST_SUITE_END()
