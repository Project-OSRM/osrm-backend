#include "engine/isochrone/weighted_polyline_materialization.hpp"

#include "mocks/mock_datafacade.hpp"

#include <boost/assert.hpp>
#include <boost/test/unit_test.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace
{

class GeometryFacade final : public osrm::test::MockBaseDataFacade
{
  public:
    GeometryFacade()
        : forward_weights(osrm::util::vector_view<std::uint64_t>(forward_weight_storage.data(),
                                                                 forward_weight_storage.size()),
                          2),
          reverse_weights(osrm::util::vector_view<std::uint64_t>(reverse_weight_storage.data(),
                                                                 reverse_weight_storage.size()),
                          2),
          forward_durations(osrm::util::vector_view<std::uint64_t>(forward_duration_storage.data(),
                                                                   forward_duration_storage.size()),
                            2),
          reverse_durations(osrm::util::vector_view<std::uint64_t>(reverse_duration_storage.data(),
                                                                   reverse_duration_storage.size()),
                            2)
    {
    }

    osrm::util::Coordinate GetCoordinateOfNode(const NodeID node) const override
    {
        BOOST_ASSERT(node < coordinates.size());
        return coordinates[node];
    }

    GeometryID GetGeometryIndex(const NodeID node) const override
    {
        BOOST_ASSERT(node < 2);
        return {0, node == 0};
    }

    NodeForwardRange GetUncompressedForwardGeometry(const PackedGeometryID) const override
    { return {nodes.data(), nodes.size()}; }

    NodeReverseRange GetUncompressedReverseGeometry(const PackedGeometryID) const override
    { return NodeReverseRange{NodeForwardRange{nodes.data(), nodes.size()}}; }

    WeightForwardRange GetUncompressedForwardWeights(const PackedGeometryID) const override
    { return {forward_weights.begin(), forward_weights.end()}; }

    WeightReverseRange GetUncompressedReverseWeights(const PackedGeometryID) const override
    {
        return WeightReverseRange{
            WeightForwardRange{reverse_weights.begin(), reverse_weights.end()}};
    }

    DurationForwardRange GetUncompressedForwardDurations(const PackedGeometryID) const override
    { return {forward_durations.begin(), forward_durations.end()}; }

    DurationReverseRange GetUncompressedReverseDurations(const PackedGeometryID) const override
    {
        return DurationReverseRange{
            DurationForwardRange{reverse_durations.begin(), reverse_durations.end()}};
    }

    void setForwardDurations(const std::uint32_t first, const std::uint32_t second)
    { forward_duration_storage[0] = packDurations(first, second); }

  private:
    static std::uint64_t packWeights(const std::uint32_t first, const std::uint32_t second)
    {
        return static_cast<std::uint64_t>(first) |
               (static_cast<std::uint64_t>(second) << SEGMENT_WEIGHT_BITS);
    }

    static std::uint64_t packDurations(const std::uint32_t first, const std::uint32_t second)
    {
        return static_cast<std::uint64_t>(first) |
               (static_cast<std::uint64_t>(second) << SEGMENT_DURATION_BITS);
    }

  public:
    std::array<osrm::util::Coordinate, 3> coordinates = {
        osrm::util::Coordinate{osrm::util::FloatLongitude{0.}, osrm::util::FloatLatitude{0.}},
        osrm::util::Coordinate{osrm::util::FloatLongitude{1.}, osrm::util::FloatLatitude{0.}},
        osrm::util::Coordinate{osrm::util::FloatLongitude{2.}, osrm::util::FloatLatitude{0.}}};
    std::array<NodeID, 3> nodes = {0, 1, 2};
    std::array<std::uint64_t, 3> forward_weight_storage = {packWeights(10, 20), 0, 0};
    // The reverse facade range reverses this storage before returning it.
    std::array<std::uint64_t, 3> reverse_weight_storage = {packWeights(40, 30), 0, 0};
    // These intentionally differ from the weights. Isochrones measure duration.
    std::array<std::uint64_t, 3> forward_duration_storage = {packDurations(100, 300), 0, 0};
    // The reverse facade range reverses this storage before returning it.
    std::array<std::uint64_t, 3> reverse_duration_storage = {packDurations(700, 500), 0, 0};
    osrm::extractor::SegmentDataView::SegmentWeightVector forward_weights;
    osrm::extractor::SegmentDataView::SegmentWeightVector reverse_weights;
    osrm::extractor::SegmentDataView::SegmentDurationVector forward_durations;
    osrm::extractor::SegmentDataView::SegmentDurationVector reverse_durations;
};

double longitude(const osrm::engine::isochrone::WeightedPolylinePoint &point)
{ return static_cast<double>(osrm::util::toFloating(point.coordinate.lon)); }

osrm::engine::isochrone::MaterializationResult
materialize(const GeometryFacade &facade,
            const std::vector<osrm::engine::isochrone::DirectedLabel> &labels,
            const std::vector<osrm::engine::isochrone::GeometryClip> &clips = {},
            const std::vector<osrm::engine::isochrone::WeightedApproach> &approaches = {},
            const EdgeDuration cutoff = EdgeDuration{1'000},
            const osrm::engine::isochrone::MaterializationLimits &limits = {})
{
    return osrm::engine::isochrone::materializeWeightedPolylines(
        facade, labels, clips, approaches, {cutoff, limits});
}

} // namespace

BOOST_AUTO_TEST_SUITE(isochrone_weighted_polyline_materialization)

BOOST_AUTO_TEST_CASE(uses_forward_geometry_and_matching_forward_durations)
{
    const GeometryFacade facade;
    const std::vector<osrm::engine::isochrone::DirectedLabel> labels = {
        {0, EdgeDuration{100}, osrm::engine::isochrone::DurationAnchor::FirstCoordinate}};

    const auto result = materialize(facade, labels, {}, {}, EdgeDuration{2'000});

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_REQUIRE_EQUAL(result.polylines.front().size(), 3);
    BOOST_CHECK_CLOSE(result.polylines.front()[0].duration, 10., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines.front()[1].duration, 20., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines.front()[2].duration, 50., 1e-9);
    BOOST_CHECK_CLOSE(longitude(result.polylines.front()[0]), 0., 1e-9);
    BOOST_CHECK_CLOSE(longitude(result.polylines.front()[2]), 2., 1e-9);
}

BOOST_AUTO_TEST_CASE(uses_reverse_geometry_and_matching_reverse_durations)
{
    const GeometryFacade facade;
    const std::vector<osrm::engine::isochrone::DirectedLabel> labels = {
        {1, EdgeDuration{100}, osrm::engine::isochrone::DurationAnchor::FirstCoordinate}};

    const auto result = materialize(facade, labels, {}, {}, EdgeDuration{2'000});

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_REQUIRE_EQUAL(result.polylines.front().size(), 3);
    BOOST_CHECK_CLOSE(longitude(result.polylines.front()[0]), 2., 1e-9);
    BOOST_CHECK_CLOSE(longitude(result.polylines.front()[2]), 0., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines.front()[0].duration, 10., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines.front()[1].duration, 60., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines.front()[2].duration, 130., 1e-9);
}

BOOST_AUTO_TEST_CASE(honors_anchor_endpoint_and_fractional_clips)
{
    const GeometryFacade facade;
    const std::vector<osrm::engine::isochrone::DirectedLabel> labels;
    const std::vector<osrm::engine::isochrone::GeometryClip> clips = {
        {{0, EdgeDuration{100}, osrm::engine::isochrone::DurationAnchor::FirstCoordinate},
         {0, 0.5},
         {1, 0.5},
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt},
        {{0, EdgeDuration{100}, osrm::engine::isochrone::DurationAnchor::LastCoordinate},
         {0, 0.5},
         {1, 0.5},
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt}};

    const auto result = materialize(facade, labels, clips);

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 2);
    BOOST_REQUIRE_EQUAL(result.polylines[0].size(), 3);
    BOOST_CHECK_CLOSE(longitude(result.polylines[0][0]), 0.5, 1e-9);
    BOOST_CHECK_CLOSE(longitude(result.polylines[0][2]), 1.5, 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0][0].duration, 10., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0][1].duration, 15., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0][2].duration, 30., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[1][0].duration, 30., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[1][1].duration, 25., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[1][2].duration, 10., 1e-9);
}

BOOST_AUTO_TEST_CASE(clips_at_the_cutoff_without_duplicate_equality_points)
{
    GeometryFacade facade;
    const std::vector<osrm::engine::isochrone::DirectedLabel> labels = {
        {0, EdgeDuration{100}, osrm::engine::isochrone::DurationAnchor::FirstCoordinate}};

    const auto result = materialize(facade, labels, {}, {}, EdgeDuration{200});

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_REQUIRE_EQUAL(result.polylines.front().size(), 2);
    BOOST_CHECK_CLOSE(longitude(result.polylines.front().back()), 1., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines.front().back().duration, 20., 1e-9);

    facade.setForwardDurations(0, 0);
    const auto zero_duration_result = materialize(facade, labels, {}, {}, EdgeDuration{100});
    BOOST_REQUIRE(zero_duration_result.error ==
                  osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(zero_duration_result.polylines.front().size(), 3);
    BOOST_CHECK_CLOSE(zero_duration_result.polylines.front().back().duration, 10., 1e-9);
}

BOOST_AUTO_TEST_CASE(materializes_and_clips_duration_approaches)
{
    const GeometryFacade facade;
    const std::vector<osrm::engine::isochrone::DirectedLabel> labels;
    const std::vector<osrm::engine::isochrone::WeightedApproach> approaches = {
        {{osrm::util::Coordinate{osrm::util::FloatLongitude{0.}, osrm::util::FloatLatitude{1.}},
          EdgeDuration{0}},
         {osrm::util::Coordinate{osrm::util::FloatLongitude{1.}, osrm::util::FloatLatitude{1.}},
          EdgeDuration{20}}}};

    const auto result = materialize(facade, labels, {}, approaches, EdgeDuration{10});

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_REQUIRE_EQUAL(result.polylines.front().size(), 2);
    BOOST_CHECK_CLOSE(longitude(result.polylines.front()[0]), 0., 1e-9);
    BOOST_CHECK_CLOSE(longitude(result.polylines.front()[1]), 0.5, 1e-9);
    BOOST_CHECK_CLOSE(result.polylines.front()[0].duration, 0., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines.front()[1].duration, 1., 1e-9);
}

BOOST_AUTO_TEST_CASE(drops_reachable_fragments_without_spatial_extent)
{
    const GeometryFacade facade;
    const std::vector<osrm::engine::isochrone::DirectedLabel> labels;
    const std::vector<osrm::engine::isochrone::GeometryClip> clips = {
        {{0, EdgeDuration{0}, osrm::engine::isochrone::DurationAnchor::FirstCoordinate},
         {1, 1.},
         {1, 1.},
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt}};

    const auto result = materialize(facade, labels, clips);

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_CHECK(result.polylines.empty());
}

BOOST_AUTO_TEST_CASE(preserves_exact_phantom_endpoints_and_partial_durations)
{
    GeometryFacade facade;
    facade.setForwardDurations(1, 300);
    const std::vector<osrm::engine::isochrone::DirectedLabel> labels;
    const std::vector<osrm::engine::isochrone::GeometryClip> clips = {
        {{0, EdgeDuration{0}, osrm::engine::isochrone::DurationAnchor::FirstCoordinate},
         {0, 0.},
         {0, 1.},
         osrm::util::Coordinate{osrm::util::FloatLongitude{0.5}, osrm::util::FloatLatitude{0.}},
         std::nullopt,
         EdgeDuration{1},
         std::nullopt,
         EdgeWeight{1},
         std::nullopt}};

    const auto result = materialize(facade, labels, clips, {}, EdgeDuration{1});

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_REQUIRE_EQUAL(result.polylines.front().size(), 2);
    BOOST_CHECK_CLOSE(longitude(result.polylines.front()[0]), 0.5, 1e-9);
    BOOST_CHECK_CLOSE(longitude(result.polylines.front()[1]), 1., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines.front()[0].duration, 0., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines.front()[1].duration, 0.1, 1e-9);
}

BOOST_AUTO_TEST_CASE(deduplicates_full_labels_but_keeps_explicit_loop_reentry_clips)
{
    const GeometryFacade facade;
    const std::vector<osrm::engine::isochrone::DirectedLabel> labels = {
        {0, EdgeDuration{150}, osrm::engine::isochrone::DurationAnchor::FirstCoordinate},
        {0, EdgeDuration{100}, osrm::engine::isochrone::DurationAnchor::FirstCoordinate}};
    const std::vector<osrm::engine::isochrone::GeometryClip> clips = {
        {{0, EdgeDuration{0}, osrm::engine::isochrone::DurationAnchor::FirstCoordinate},
         {0, 0.5},
         {1, 0.},
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt}};

    const auto result = materialize(facade, labels, clips);

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 2);
    BOOST_REQUIRE_EQUAL(result.polylines[0].size(), 3);
    BOOST_CHECK_CLOSE(result.polylines[0].front().duration, 10., 1e-9);
    BOOST_REQUIRE_EQUAL(result.polylines[1].size(), 2);
    BOOST_CHECK_CLOSE(longitude(result.polylines[1].front()), 0.5, 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[1].front().duration, 0., 1e-9);
}

BOOST_AUTO_TEST_CASE(rejects_invalid_segment_durations_without_partial_output)
{
    GeometryFacade facade;
    facade.setForwardDurations(osrm::from_alias<std::uint32_t>(INVALID_SEGMENT_DURATION), 300);
    const std::vector<osrm::engine::isochrone::DirectedLabel> labels = {
        {0, EdgeDuration{0}, osrm::engine::isochrone::DurationAnchor::FirstCoordinate}};

    const auto result = materialize(facade, labels);

    BOOST_CHECK(result.error ==
                osrm::engine::isochrone::MaterializationError::InvalidSegmentDuration);
    BOOST_CHECK(result.polylines.empty());
}

BOOST_AUTO_TEST_CASE(enforces_resource_limits_without_partial_output)
{
    const GeometryFacade facade;
    const std::vector<osrm::engine::isochrone::DirectedLabel> labels = {
        {0, EdgeDuration{0}, osrm::engine::isochrone::DurationAnchor::FirstCoordinate}};
    auto limits = osrm::engine::isochrone::MaterializationLimits{};
    limits.maximum_materialized_geometry_points = 2;

    const auto result = materialize(facade, labels, {}, {}, EdgeDuration{1'000}, limits);

    BOOST_CHECK(result.error == osrm::engine::isochrone::MaterializationError::BudgetExceeded);
    BOOST_CHECK(result.polylines.empty());
}

BOOST_AUTO_TEST_CASE(rejects_geometries_the_grid_cannot_represent)
{
    GeometryFacade facade;
    const std::vector<osrm::engine::isochrone::DirectedLabel> labels = {
        {0, EdgeDuration{0}, osrm::engine::isochrone::DurationAnchor::FirstCoordinate}};

    facade.coordinates[0] = {osrm::util::FloatLongitude{179.}, osrm::util::FloatLatitude{0.}};
    facade.coordinates[1] = {osrm::util::FloatLongitude{-179.}, osrm::util::FloatLatitude{0.}};
    const auto dateline_result = materialize(facade, labels);
    BOOST_CHECK(dateline_result.error ==
                osrm::engine::isochrone::MaterializationError::RequiresLongitudeWrap);
    BOOST_CHECK(dateline_result.polylines.empty());

    facade.coordinates[0] = {osrm::util::FloatLongitude{0.}, osrm::util::FloatLatitude{90.}};
    facade.coordinates[1] = {osrm::util::FloatLongitude{1.}, osrm::util::FloatLatitude{89.}};
    const auto pole_result = materialize(facade, labels);
    BOOST_CHECK(pole_result.error == osrm::engine::isochrone::MaterializationError::TouchesPole);
    BOOST_CHECK(pole_result.polylines.empty());
}

BOOST_AUTO_TEST_SUITE_END()
