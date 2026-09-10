#include "engine/isochrone/weighted_polyline_grid.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace
{

using osrm::engine::isochrone::WeightedPolyline;
using osrm::engine::isochrone::WeightedPolylinePoint;
using osrm::util::Coordinate;

constexpr double EARTH_MEAN_RADIUS_METRES = 6'371'008.8;
constexpr double PI = 3.14159265358979323846;
constexpr double METRES_PER_DEGREE_LATITUDE = EARTH_MEAN_RADIUS_METRES * PI / 180.;

WeightedPolylinePoint point(const double lon, const double lat, const double duration)
{ return {Coordinate{osrm::util::FloatLongitude{lon}, osrm::util::FloatLatitude{lat}}, duration}; }

WeightedPolylinePoint pointAtMetres(const double reference_lon,
                                    const double reference_lat,
                                    const double east_metres,
                                    const double north_metres,
                                    const double duration)
{
    const auto metres_per_degree_lon =
        METRES_PER_DEGREE_LATITUDE * std::cos(reference_lat * PI / 180.);
    return point(reference_lon + east_metres / metres_per_degree_lon,
                 reference_lat + north_metres / METRES_PER_DEGREE_LATITUDE,
                 duration);
}

double distanceMetres(const Coordinate from, const Coordinate to)
{
    const auto from_lon = static_cast<double>(osrm::util::toFloating(from.lon)) * PI / 180.;
    const auto from_lat = static_cast<double>(osrm::util::toFloating(from.lat)) * PI / 180.;
    const auto to_lon = static_cast<double>(osrm::util::toFloating(to.lon)) * PI / 180.;
    const auto to_lat = static_cast<double>(osrm::util::toFloating(to.lat)) * PI / 180.;
    const auto latitude_delta = to_lat - from_lat;
    const auto longitude_delta = to_lon - from_lon;
    const auto haversine =
        std::pow(std::sin(latitude_delta / 2.), 2) +
        std::cos(from_lat) * std::cos(to_lat) * std::pow(std::sin(longitude_delta / 2.), 2);
    return 2. * EARTH_MEAN_RADIUS_METRES * std::asin(std::sqrt(haversine));
}

} // namespace

BOOST_AUTO_TEST_SUITE(isochrone_weighted_polyline_grid)

BOOST_AUTO_TEST_CASE(rasterizes_interpolated_segment_durations_in_fixed_metre_cells)
{
    const std::vector<WeightedPolyline> polylines = {
        {pointAtMetres(0., 0., -150., 0., 0.), pointAtMetres(0., 0., 150., 0., 60.)}};

    const auto result = osrm::engine::isochrone::rasterizeWeightedPolylines(polylines);

    BOOST_REQUIRE(result.grid);
    BOOST_REQUIRE_EQUAL(result.grid->width, 4);
    BOOST_REQUIRE_EQUAL(result.grid->height, 1);
    BOOST_CHECK_SMALL(result.grid->cell_size - 100., 1e-12);
    BOOST_CHECK_SMALL(result.grid->values[0], 1e-9);
    BOOST_CHECK_CLOSE(result.grid->values[1], 10., 0.1);
    BOOST_CHECK_CLOSE(result.grid->values[2], 30., 0.1);
    BOOST_CHECK_CLOSE(result.grid->values[3], 50., 0.1);
}

BOOST_AUTO_TEST_CASE(intentionally_merges_unrelated_geometry_that_shares_a_raster_cell)
{
    const auto source = pointAtMetres(0., 0., 0., 0., 0.);
    const auto nearby = pointAtMetres(0., 0., 20., 20., 30.);
    const std::vector<WeightedPolyline> polylines = {
        {{source.coordinate, source.duration, 100., PackedGeometryID{1}}},
        {{nearby.coordinate, nearby.duration, 1., PackedGeometryID{2}}}};

    const auto result =
        osrm::engine::isochrone::rasterizeWeightedPolylines(polylines, {}, source.coordinate);

    BOOST_REQUIRE(result.grid);
    BOOST_REQUIRE_EQUAL(result.grid->width, 1);
    BOOST_REQUIRE_EQUAL(result.grid->height, 1);
    // Distinct GeometryIDs in one 100 m cell are deliberately a coverage approximation. They
    // are not target alternatives, so the earlier duration owns the cell despite its weight.
    BOOST_CHECK_SMALL(result.grid->values.front(), 1e-9);
}

BOOST_AUTO_TEST_CASE(selects_the_lower_weight_for_opposite_directions_of_one_geometry)
{
    const auto west = pointAtMetres(0., 0., 0., 0., 0.);
    const auto east = pointAtMetres(0., 0., 20., 0., 0.);
    constexpr PackedGeometryID shared_geometry = 7;
    const std::vector<WeightedPolyline> polylines = {
        {{east.coordinate, 20., 1., shared_geometry}, {west.coordinate, 20., 1., shared_geometry}},
        {{west.coordinate, 1., 100., shared_geometry},
         {east.coordinate, 1., 100., shared_geometry}}};

    const auto result =
        osrm::engine::isochrone::rasterizeWeightedPolylines(polylines, {}, west.coordinate);

    BOOST_REQUIRE(result.grid);
    BOOST_REQUIRE_EQUAL(result.grid->width, 1);
    BOOST_REQUIRE_EQUAL(result.grid->height, 1);
    BOOST_CHECK_CLOSE(result.grid->values.front(), 20., 1e-9);
}

BOOST_AUTO_TEST_CASE(breaks_shared_geometry_weight_ties_by_duration)
{
    const auto west = pointAtMetres(0., 0., 0., 0., 0.);
    const auto east = pointAtMetres(0., 0., 20., 0., 0.);
    constexpr PackedGeometryID shared_geometry = 9;
    const std::vector<WeightedPolyline> polylines = {
        {{east.coordinate, 20., 10., shared_geometry},
         {west.coordinate, 20., 10., shared_geometry}},
        {{west.coordinate, 10., 10., shared_geometry},
         {east.coordinate, 10., 10., shared_geometry}}};

    const auto result =
        osrm::engine::isochrone::rasterizeWeightedPolylines(polylines, {}, west.coordinate);

    BOOST_REQUIRE(result.grid);
    BOOST_REQUIRE_EQUAL(result.grid->width, 1);
    BOOST_REQUIRE_EQUAL(result.grid->height, 1);
    // The slower direction is deliberately first, so preserving input order is not sufficient.
    BOOST_CHECK_CLOSE(result.grid->values.front(), 10., 1e-9);
}

BOOST_AUTO_TEST_CASE(allows_the_shared_geometry_winner_to_change_by_road_location)
{
    const auto west = pointAtMetres(0., 0., 10., 0., 0.);
    const auto middle = pointAtMetres(0., 0., 110., 0., 0.);
    const auto east = pointAtMetres(0., 0., 210., 0., 0.);
    constexpr PackedGeometryID shared_geometry = 11;
    const std::vector<WeightedPolyline> polylines = {
        // This is the lower-weight direction on the eastern two cells.
        {{east.coordinate, 30., 50., shared_geometry},
         {middle.coordinate, 40., 51., shared_geometry},
         {west.coordinate, 50., 52., shared_geometry}},
        // This direction is lower weight near the western endpoint only.
        {{west.coordinate, 1., 0., shared_geometry},
         {middle.coordinate, 2., 100., shared_geometry},
         {east.coordinate, 3., 200., shared_geometry}}};

    const auto result =
        osrm::engine::isochrone::rasterizeWeightedPolylines(polylines, {}, west.coordinate);

    BOOST_REQUIRE(result.grid);
    BOOST_REQUIRE_EQUAL(result.grid->width, 3);
    BOOST_REQUIRE_EQUAL(result.grid->height, 1);
    // A 10 s contour may retain the western cell, where the quick direction wins, but not the
    // eastern cells, where the slower direction is lexicographically lower weight.
    BOOST_CHECK_LE(result.grid->values[0], 10.);
    BOOST_CHECK_GT(result.grid->values[1], 10.);
    BOOST_CHECK_GT(result.grid->values[2], 10.);
}

BOOST_AUTO_TEST_CASE(uses_the_same_metre_grid_at_different_latitudes)
{
    const auto rasterize = [](const double latitude)
    {
        const std::vector<WeightedPolyline> polylines = {
            {pointAtMetres(10., latitude, -150., 0., 0.),
             pointAtMetres(10., latitude, 150., 0., 60.)}};
        return osrm::engine::isochrone::rasterizeWeightedPolylines(polylines);
    };

    const auto equatorial = rasterize(0.);
    const auto high_latitude = rasterize(60.);
    BOOST_REQUIRE(equatorial.grid);
    BOOST_REQUIRE(high_latitude.grid);
    BOOST_CHECK_EQUAL(high_latitude.grid->width, equatorial.grid->width);
    BOOST_CHECK_EQUAL(high_latitude.grid->height, equatorial.grid->height);
    for (std::size_t index = 0; index < equatorial.grid->values.size(); ++index)
        BOOST_CHECK_CLOSE(high_latitude.grid->values[index], equatorial.grid->values[index], 0.2);

    const auto check_cell_edges = [](const double latitude)
    {
        const std::vector<WeightedPolyline> polylines = {
            {pointAtMetres(10., latitude, 0., 0., 0.)}};
        const auto result = osrm::engine::isochrone::rasterizeWeightedPolylines(polylines);
        BOOST_REQUIRE(result.grid);
        const auto contours = result.grid->buildContours(0.);
        BOOST_REQUIRE_EQUAL(contours.size(), 1);
        const auto &ring = contours.front().outer;
        BOOST_REQUIRE_EQUAL(ring.size(), 5);
        for (std::size_t index = 1; index < ring.size(); ++index)
            BOOST_CHECK_CLOSE(distanceMetres(ring[index - 1], ring[index]), 100., 0.3);
    };
    check_cell_edges(0.);
    check_cell_edges(60.);
}

BOOST_AUTO_TEST_CASE(round_trips_an_arbitrary_point_within_the_metric_cell_error_bound)
{
    // The two higher-duration points establish a local reference at (0, 0).
    // The selected point is deliberately near the north-east corner of its
    // 100 m cell, exercising both the forward and inverse local projection.
    const auto source = pointAtMetres(0., 0., 49., 49., 7.);
    const std::vector<WeightedPolyline> polylines = {{pointAtMetres(0., 0., -150., -150., 8.)},
                                                     {source},
                                                     {pointAtMetres(0., 0., 150., 150., 8.)}};

    const auto result = osrm::engine::isochrone::rasterizeWeightedPolylines(polylines);
    BOOST_REQUIRE(result.grid);
    const auto contours = result.grid->buildContours(7.);
    BOOST_REQUIRE_EQUAL(contours.size(), 1);
    const auto &ring = contours.front().outer;
    BOOST_REQUIRE_EQUAL(ring.size(), 5);

    const auto source_lon = static_cast<double>(osrm::util::toFloating(source.coordinate.lon));
    const auto source_lat = static_cast<double>(osrm::util::toFloating(source.coordinate.lat));
    auto minimum_lon = std::numeric_limits<double>::infinity();
    auto maximum_lon = -std::numeric_limits<double>::infinity();
    auto minimum_lat = std::numeric_limits<double>::infinity();
    auto maximum_lat = -std::numeric_limits<double>::infinity();
    for (const auto vertex : ring)
    {
        const auto lon = static_cast<double>(osrm::util::toFloating(vertex.lon));
        const auto lat = static_cast<double>(osrm::util::toFloating(vertex.lat));
        minimum_lon = std::min(minimum_lon, lon);
        maximum_lon = std::max(maximum_lon, lon);
        minimum_lat = std::min(minimum_lat, lat);
        maximum_lat = std::max(maximum_lat, lat);
        BOOST_CHECK_LE(distanceMetres(source.coordinate, vertex), std::sqrt(2.) * 100. + 0.3);
    }
    BOOST_CHECK_GE(source_lon, minimum_lon);
    BOOST_CHECK_LE(source_lon, maximum_lon);
    BOOST_CHECK_GE(source_lat, minimum_lat);
    BOOST_CHECK_LE(source_lat, maximum_lat);
}

BOOST_AUTO_TEST_CASE(keeps_contours_aligned_to_the_explicit_request_source)
{
    const auto source = pointAtMetres(0., 0., 0., 0., 0.);
    const std::vector<WeightedPolyline> nearby = {{source}};
    const std::vector<WeightedPolyline> with_farther_geometry = {
        {source}, {pointAtMetres(0., 0., 300., 300., 8.)}};

    const auto nearby_result =
        osrm::engine::isochrone::rasterizeWeightedPolylines(nearby, {}, source.coordinate);
    const auto farther_result = osrm::engine::isochrone::rasterizeWeightedPolylines(
        with_farther_geometry, {}, source.coordinate);
    BOOST_REQUIRE(nearby_result.grid);
    BOOST_REQUIRE(farther_result.grid);

    // Higher-duration geometry expands the raster bounds but must not move
    // the 0-second contour away from the request source.
    const auto nearby_contours = nearby_result.grid->buildContours(0.);
    const auto farther_contours = farther_result.grid->buildContours(0.);
    BOOST_REQUIRE_EQUAL(nearby_contours.size(), 1);
    BOOST_REQUIRE_EQUAL(farther_contours.size(), 1);
    BOOST_CHECK(nearby_contours.front().outer == farther_contours.front().outer);
}

BOOST_AUTO_TEST_CASE(supercovers_grid_corner_crossings_in_both_directions)
{
    const auto check = [](const WeightedPolyline &polyline)
    {
        const auto result = osrm::engine::isochrone::rasterizeWeightedPolylines(
            {&polyline, 1}, {}, point(0., 0., 0.).coordinate);

        BOOST_REQUIRE(result.grid);
        BOOST_REQUIRE_EQUAL(result.grid->width, 3);
        BOOST_REQUIRE_EQUAL(result.grid->height, 3);

        // The two side cells at each exact corner crossing carry the duration at the crossing.
        BOOST_CHECK_SMALL(result.grid->values[1] - 10., 0.01);
        BOOST_CHECK_SMALL(result.grid->values[3] - 10., 0.01);
        BOOST_CHECK_SMALL(result.grid->values[5] - 30., 0.01);
        BOOST_CHECK_SMALL(result.grid->values[7] - 30., 0.01);

        const auto contours = result.grid->buildContours(40.);
        BOOST_REQUIRE_EQUAL(contours.size(), 1);
    };

    check({pointAtMetres(0., 0., -50., -50., 0.), pointAtMetres(0., 0., 150., 150., 40.)});
    check({pointAtMetres(0., 0., 150., 150., 40.), pointAtMetres(0., 0., -50., -50., 0.)});
}

BOOST_AUTO_TEST_CASE(supercovers_an_outer_grid_corner_endpoint_without_stepping_outside_the_grid)
{
    const auto source = pointAtMetres(0., 0., 0., 0., 40.);
    const auto other = pointAtMetres(0., 0., 250., -150., 0.);
    const auto check = [&](const WeightedPolyline &polyline)
    {
        const auto result = osrm::engine::isochrone::rasterizeWeightedPolylines(
            {&polyline, 1}, {}, source.coordinate);

        BOOST_REQUIRE(result.grid);
        BOOST_REQUIRE_EQUAL(result.grid->width, 3);
        BOOST_REQUIRE_EQUAL(result.grid->height, 3);
        BOOST_REQUIRE_EQUAL(result.grid->buildContours(40.).size(), 1);
    };

    check({other, source});
    check({source, other});
}

BOOST_AUTO_TEST_CASE(returns_wgs84_contours_with_holes)
{
    const std::vector<WeightedPolyline> polylines = {{pointAtMetres(0., 0., -150., -150., 0.),
                                                      pointAtMetres(0., 0., 150., -150., 0.),
                                                      pointAtMetres(0., 0., 150., 150., 0.),
                                                      pointAtMetres(0., 0., -150., 150., 0.),
                                                      pointAtMetres(0., 0., -150., -150., 0.)}};

    const auto result = osrm::engine::isochrone::rasterizeWeightedPolylines(polylines);
    BOOST_REQUIRE(result.grid);
    const auto contours = result.grid->buildContours(0.);

    BOOST_REQUIRE_EQUAL(contours.size(), 1);
    BOOST_REQUIRE_EQUAL(contours.front().holes.size(), 1);
    BOOST_CHECK(contours.front().outer.front() == contours.front().outer.back());
    BOOST_CHECK(contours.front().holes.front().front() == contours.front().holes.front().back());
    for (const auto coordinate : contours.front().outer)
        BOOST_CHECK(coordinate.IsValid());
}

BOOST_AUTO_TEST_CASE(keeps_disconnected_components_and_their_order_stable)
{
    const std::vector<WeightedPolyline> polylines = {{pointAtMetres(0., 0., -150., -150., 0.)},
                                                     {pointAtMetres(0., 0., 150., 150., 0.)}};

    const auto result = osrm::engine::isochrone::rasterizeWeightedPolylines(polylines);
    BOOST_REQUIRE(result.grid);
    const auto first = result.grid->buildContours(0.);
    const auto second = result.grid->buildContours(0.);

    BOOST_REQUIRE_EQUAL(first.size(), 2);
    BOOST_REQUIRE_EQUAL(second.size(), first.size());
    BOOST_CHECK(first[0].outer == second[0].outer);
    BOOST_CHECK(first[1].outer == second[1].outer);
}

BOOST_AUTO_TEST_CASE(stops_contouring_when_the_exact_coordinate_budget_is_exceeded)
{
    const std::vector<WeightedPolyline> polylines = {{pointAtMetres(0., 0., 0., 0., 0.)}};
    const auto result = osrm::engine::isochrone::rasterizeWeightedPolylines(polylines);
    BOOST_REQUIRE(result.grid);

    // A single-cell contour is a closed ring with five coordinates.
    BOOST_CHECK(!result.grid->buildContours(0., 4));
    BOOST_REQUIRE(result.grid->buildContours(0., 5));
}

BOOST_AUTO_TEST_CASE(rejects_a_grid_that_exceeds_the_cell_cap)
{
    const std::vector<WeightedPolyline> polylines = {
        {pointAtMetres(0., 0., -1'000., 0., 0.), pointAtMetres(0., 0., 1'000., 0., 10.)}};
    const osrm::engine::isochrone::WeightedGridOptions options{100., 16};

    const auto result = osrm::engine::isochrone::rasterizeWeightedPolylines(polylines, options);

    BOOST_CHECK(!result.grid);
    BOOST_CHECK(result.error == osrm::engine::isochrone::RasterizationError::TooBig);
}

BOOST_AUTO_TEST_CASE(rejects_rasterization_that_exceeds_the_work_cap)
{
    // A thin grid fits the allocation cap but a segment can still visit every
    // cell. The work limit must be enforced before the DDA can be repeated by
    // every materialized road segment.
    const std::vector<WeightedPolyline> polylines = {
        {pointAtMetres(0., 0., -50'000., 0., 0.), pointAtMetres(0., 0., 50'000., 0., 10.)}};
    const osrm::engine::isochrone::WeightedGridOptions options{100., 2'000, 999};

    const auto result = osrm::engine::isochrone::rasterizeWeightedPolylines(polylines, options);

    BOOST_CHECK(!result.grid);
    BOOST_CHECK(result.error == osrm::engine::isochrone::RasterizationError::TooBig);
}

BOOST_AUTO_TEST_CASE(ignores_unusable_points_without_bridging_across_them)
{
    const std::vector<WeightedPolyline> polylines = {
        {pointAtMetres(0., 0., -150., 0., 0.),
         pointAtMetres(0., 0., 0., 0., std::numeric_limits<double>::infinity()),
         pointAtMetres(0., 0., 150., 0., 100.)}};

    const auto result = osrm::engine::isochrone::rasterizeWeightedPolylines(polylines);

    BOOST_REQUIRE(result.grid);
    BOOST_REQUIRE_EQUAL(result.grid->width, 4);
    BOOST_CHECK_SMALL(result.grid->values[0], 1e-9);
    BOOST_CHECK(!std::isfinite(result.grid->values[1]));
    BOOST_CHECK(!std::isfinite(result.grid->values[2]));
    BOOST_CHECK_CLOSE(result.grid->values[3], 100., 1e-9);
}

BOOST_AUTO_TEST_CASE(rejects_polar_and_antimeridian_geometries_with_typed_errors)
{
    const auto rasterize_single_point = [](const double lon, const double lat)
    {
        const std::vector<WeightedPolyline> polylines = {{point(lon, lat, 0.)}};
        return osrm::engine::isochrone::rasterizeWeightedPolylines(polylines);
    };

    const auto valid_near_dateline = rasterize_single_point(179.99, 0.);
    BOOST_CHECK(valid_near_dateline.grid);

    const auto longitude_boundary = rasterize_single_point(180., 0.);
    BOOST_CHECK(!longitude_boundary.grid);
    BOOST_CHECK(longitude_boundary.error ==
                osrm::engine::isochrone::RasterizationError::TouchesLongitudeBoundary);

    const std::vector<WeightedPolyline> dateline_crossing = {
        {point(179.99, 0., 0.), point(-179.99, 0., 10.)}};
    const auto crossing = osrm::engine::isochrone::rasterizeWeightedPolylines(dateline_crossing);
    BOOST_CHECK(!crossing.grid);
    BOOST_CHECK(crossing.error ==
                osrm::engine::isochrone::RasterizationError::TouchesLongitudeBoundary);

    const auto valid_near_pole = rasterize_single_point(0., 89.998);
    BOOST_CHECK(valid_near_pole.grid);

    const auto pole = rasterize_single_point(0., 90.);
    BOOST_CHECK(!pole.grid);
    BOOST_CHECK(pole.error == osrm::engine::isochrone::RasterizationError::TouchesPole);
}

BOOST_AUTO_TEST_CASE(rejects_invalid_rasterization_options_before_doing_grid_arithmetic)
{
    const std::vector<WeightedPolyline> polylines = {{point(0., 0., 0.)}};
    const auto check_invalid =
        [&polylines](const osrm::engine::isochrone::WeightedGridOptions options)
    {
        const auto result = osrm::engine::isochrone::rasterizeWeightedPolylines(polylines, options);
        BOOST_CHECK(!result.grid);
        BOOST_CHECK(result.error == osrm::engine::isochrone::RasterizationError::InvalidOptions);
    };

    check_invalid({0., 1, 1});
    check_invalid({-100., 1, 1});
    check_invalid({std::numeric_limits<double>::quiet_NaN(), 1, 1});
    check_invalid({std::numeric_limits<double>::infinity(), 1, 1});
    check_invalid({100., 0, 1});
    check_invalid({100., 1, 0});
}

BOOST_AUTO_TEST_CASE(rejects_a_cell_size_that_cannot_preserve_output_coordinates)
{
    const std::vector<WeightedPolyline> polylines = {{point(1., 0., 0.)}};
    const osrm::engine::isochrone::WeightedGridOptions options{
        std::numeric_limits<double>::denorm_min(), 1, 1};

    const auto result = osrm::engine::isochrone::rasterizeWeightedPolylines(polylines, options);

    BOOST_CHECK(!result.grid);
    BOOST_CHECK(result.error == osrm::engine::isochrone::RasterizationError::InvalidOptions);
}

BOOST_AUTO_TEST_SUITE_END()
