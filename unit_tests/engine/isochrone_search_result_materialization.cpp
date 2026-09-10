#include "engine/isochrone/search_result_materialization.hpp"

#include "mocks/mock_datafacade.hpp"

#include <boost/assert.hpp>
#include <boost/test/unit_test.hpp>

#include <array>
#include <cstdint>

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
    std::array<std::uint64_t, 3> reverse_weight_storage = {packWeights(40, 30), 0, 0};
    // These intentionally differ from the weights. Isochrones measure duration.
    std::array<std::uint64_t, 3> forward_duration_storage = {packDurations(100, 300), 0, 0};
    std::array<std::uint64_t, 3> reverse_duration_storage = {packDurations(700, 500), 0, 0};
    osrm::extractor::SegmentDataView::SegmentWeightVector forward_weights;
    osrm::extractor::SegmentDataView::SegmentWeightVector reverse_weights;
    osrm::extractor::SegmentDataView::SegmentDurationVector forward_durations;
    osrm::extractor::SegmentDataView::SegmentDurationVector reverse_durations;
};

osrm::engine::PhantomNode makePhantom(const NodeID forward_segment,
                                      const NodeID reverse_segment,
                                      const EdgeWeight forward_weight,
                                      const EdgeWeight reverse_weight,
                                      const EdgeDuration forward_duration,
                                      const EdgeDuration reverse_duration,
                                      const unsigned short segment_position,
                                      const osrm::util::Coordinate location,
                                      const osrm::util::Coordinate input_location)
{
    struct Segment
    {
        SegmentID forward_segment_id;
        SegmentID reverse_segment_id;
        unsigned short fwd_segment_position;
    } segment{{forward_segment, true}, {reverse_segment, true}, segment_position};

    return osrm::engine::PhantomNode{segment,
                                     ComponentID{1, false},
                                     forward_weight,
                                     reverse_weight,
                                     EdgeWeight{0},
                                     EdgeWeight{0},
                                     EdgeDistance{0},
                                     EdgeDistance{0},
                                     EdgeDistance{0},
                                     EdgeDistance{0},
                                     forward_duration,
                                     reverse_duration,
                                     EdgeDuration{0},
                                     EdgeDuration{0},
                                     true,
                                     true,
                                     true,
                                     true,
                                     location,
                                     input_location,
                                     0};
}

double longitude(const osrm::engine::isochrone::WeightedPolylinePoint &point)
{ return static_cast<double>(osrm::util::toFloating(point.coordinate.lon)); }

osrm::engine::isochrone::MaterializationResult
materialize(const GeometryFacade &facade,
            const osrm::engine::isochrone::SearchResult &search_result,
            const bool inbound,
            const EdgeDuration cutoff = EdgeDuration{1'000},
            const osrm::engine::isochrone::MaterializationLimits &limits = {})
{
    return osrm::engine::isochrone::materializeSearchResult(
        facade, search_result, cutoff, inbound, limits);
}

} // namespace

BOOST_AUTO_TEST_SUITE(isochrone_search_result_materialization)

BOOST_AUTO_TEST_CASE(converts_outbound_labels_from_the_first_coordinate)
{
    const GeometryFacade facade;
    osrm::engine::isochrone::SearchResult search_result;
    search_result.nodes.push_back(
        {0, EdgeDuration{100}, osrm::engine::isochrone::NodeProvenance::Network});

    const auto result = materialize(facade, search_result, false);

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_CHECK_CLOSE(result.polylines[0][0].duration, 10., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0][2].duration, 50., 1e-9);
}

BOOST_AUTO_TEST_CASE(normalizes_inbound_labels_to_the_last_coordinate)
{
    const GeometryFacade facade;
    osrm::engine::isochrone::SearchResult search_result;
    search_result.nodes.push_back(
        {0, EdgeDuration{500}, osrm::engine::isochrone::NodeProvenance::Network});

    const auto result = materialize(facade, search_result, true);

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_CHECK_CLOSE(result.polylines[0][0].duration, 50., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0][1].duration, 40., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0][2].duration, 10., 1e-9);
}

BOOST_AUTO_TEST_CASE(rejects_an_invalid_duration_before_inbound_normalization)
{
    const GeometryFacade facade;
    osrm::engine::isochrone::SearchResult search_result;
    search_result.nodes.push_back(
        {0, INVALID_EDGE_DURATION, osrm::engine::isochrone::NodeProvenance::Network});

    const auto result = materialize(facade, search_result, true);

    BOOST_CHECK(result.error == osrm::engine::isochrone::MaterializationError::InvalidLabel);
    BOOST_CHECK(result.polylines.empty());
}

BOOST_AUTO_TEST_CASE(materializes_the_reachable_suffix_of_an_inbound_frontier)
{
    const GeometryFacade facade;
    osrm::engine::isochrone::SearchResult search_result;
    search_result.inbound_frontiers.push_back(
        {{0, EdgeDuration{520}, osrm::engine::isochrone::NodeProvenance::Network}});

    const auto result = materialize(facade, search_result, true, EdgeDuration{200});

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_REQUIRE_EQUAL(result.polylines[0].size(), 2);
    // The inbound label is anchored at x=2 with duration 12 s. Its preceding
    // 30 s segment starts at 42 s, so a 20 s cutoff is at x=1+22/30. OSRM
    // coordinates are fixed to six decimal places.
    BOOST_CHECK_SMALL(longitude(result.polylines[0][0]) - 1.7333333333333334, 1e-6);
    BOOST_CHECK_CLOSE(result.polylines[0][0].duration, 20., 1e-9);
    BOOST_CHECK_CLOSE(longitude(result.polylines[0][1]), 2., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0][1].duration, 12., 1e-9);
}

BOOST_AUTO_TEST_CASE(clips_forward_phantom_geometry_at_the_exact_snap)
{
    const GeometryFacade facade;
    const auto location =
        osrm::util::Coordinate{osrm::util::FloatLongitude{0.4}, osrm::util::FloatLatitude{0.}};
    auto phantom = makePhantom(0,
                               1,
                               EdgeWeight{4},
                               EdgeWeight{24},
                               EdgeDuration{40},
                               EdgeDuration{350},
                               0,
                               location,
                               location);
    osrm::engine::isochrone::SearchResult search_result;
    search_result.phantom_partials.push_back(
        {0,
         phantom,
         osrm::engine::isochrone::PhantomTraversalDirection::Forward,
         osrm::engine::isochrone::PhantomTraversalKind::OutboundInitial,
         EdgeDuration{-40},
         true});

    const auto result = materialize(facade, search_result, false);

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_CHECK_CLOSE(longitude(result.polylines[0].front()), 0.4, 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0].front().duration, 0., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0][1].duration, 6., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0].back().duration, 36., 1e-9);
}

BOOST_AUTO_TEST_CASE(clips_reverse_phantom_geometry_in_legal_direction)
{
    const GeometryFacade facade;
    const auto location =
        osrm::util::Coordinate{osrm::util::FloatLongitude{0.5}, osrm::util::FloatLatitude{0.}};
    auto phantom = makePhantom(0,
                               1,
                               EdgeWeight{5},
                               EdgeWeight{20},
                               EdgeDuration{40},
                               EdgeDuration{350},
                               0,
                               location,
                               location);
    osrm::engine::isochrone::SearchResult search_result;
    search_result.phantom_partials.push_back(
        {1,
         phantom,
         osrm::engine::isochrone::PhantomTraversalDirection::Reverse,
         osrm::engine::isochrone::PhantomTraversalKind::OutboundInitial,
         EdgeDuration{-350},
         true});

    const auto result = materialize(facade, search_result, false);

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_CHECK_CLOSE(longitude(result.polylines[0].front()), 0.5, 1e-9);
    BOOST_CHECK_CLOSE(longitude(result.polylines[0].back()), 0., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0].back().duration, 35., 1e-9);
}

BOOST_AUTO_TEST_CASE(materializes_an_inbound_phantom_prefix)
{
    const GeometryFacade facade;
    const auto location =
        osrm::util::Coordinate{osrm::util::FloatLongitude{0.4}, osrm::util::FloatLatitude{0.}};
    auto phantom = makePhantom(0,
                               1,
                               EdgeWeight{4},
                               EdgeWeight{24},
                               EdgeDuration{40},
                               EdgeDuration{350},
                               0,
                               location,
                               location);
    osrm::engine::isochrone::SearchResult search_result;
    search_result.phantom_partials.push_back(
        {0,
         phantom,
         osrm::engine::isochrone::PhantomTraversalDirection::Forward,
         osrm::engine::isochrone::PhantomTraversalKind::InboundTerminal,
         EdgeDuration{40},
         true});

    const auto result = materialize(facade, search_result, true);

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_CHECK_CLOSE(longitude(result.polylines[0].front()), 0., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0].front().duration, 4., 1e-9);
    BOOST_CHECK_CLOSE(longitude(result.polylines[0].back()), 0.4, 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0].back().duration, 0., 1e-9);
}

BOOST_AUTO_TEST_CASE(retains_an_approach_that_reaches_the_cutoff_before_the_network)
{
    const GeometryFacade facade;
    const auto input =
        osrm::util::Coordinate{osrm::util::FloatLongitude{0.}, osrm::util::FloatLatitude{1.}};
    const auto location =
        osrm::util::Coordinate{osrm::util::FloatLongitude{2.}, osrm::util::FloatLatitude{1.}};
    auto phantom = makePhantom(
        0, 1, EdgeWeight{0}, EdgeWeight{0}, EdgeDuration{0}, EdgeDuration{0}, 0, location, input);
    phantom.approach_weight = EdgeWeight{17};
    phantom.approach_duration = EdgeDuration{200};
    phantom.approach_distance = EdgeDistance{2};
    osrm::engine::isochrone::SearchResult search_result;
    search_result.phantom_partials.push_back(
        {0,
         phantom,
         osrm::engine::isochrone::PhantomTraversalDirection::Forward,
         osrm::engine::isochrone::PhantomTraversalKind::OutboundInitial,
         EdgeDuration{0},
         false});

    const auto result = materialize(facade, search_result, false, EdgeDuration{100});

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_REQUIRE_EQUAL(result.polylines[0].size(), 2);
    BOOST_CHECK_CLOSE(longitude(result.polylines[0].front()), 0., 1e-9);
    BOOST_CHECK_CLOSE(longitude(result.polylines[0].back()), 1., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0].back().duration, 10., 1e-9);
}

BOOST_AUTO_TEST_CASE(deduplicates_identical_phantom_approaches)
{
    const GeometryFacade facade;
    const auto input =
        osrm::util::Coordinate{osrm::util::FloatLongitude{0.}, osrm::util::FloatLatitude{1.}};
    const auto location =
        osrm::util::Coordinate{osrm::util::FloatLongitude{2.}, osrm::util::FloatLatitude{1.}};
    auto phantom = makePhantom(
        0, 1, EdgeWeight{0}, EdgeWeight{0}, EdgeDuration{0}, EdgeDuration{0}, 0, location, input);
    phantom.approach_duration = EdgeDuration{200};
    phantom.approach_distance = EdgeDistance{2};

    osrm::engine::isochrone::SearchResult search_result;
    const osrm::engine::isochrone::PhantomPartialTraversal partial{
        0,
        phantom,
        osrm::engine::isochrone::PhantomTraversalDirection::Forward,
        osrm::engine::isochrone::PhantomTraversalKind::OutboundInitial,
        EdgeDuration{0},
        false};
    search_result.phantom_partials = {partial, partial};

    const auto result = materialize(facade, search_result, false, EdgeDuration{100});

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_REQUIRE_EQUAL(result.polylines.front().size(), 2);
    BOOST_CHECK_CLOSE(longitude(result.polylines.front().back()), 1., 1e-9);
}

BOOST_AUTO_TEST_CASE(no_network_fragment_produces_empty_geometry)
{
    const GeometryFacade facade;
    osrm::engine::isochrone::SearchResult search_result;

    const auto result = materialize(facade, search_result, false);

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_CHECK(result.polylines.empty());
}

BOOST_AUTO_TEST_CASE(rejects_search_results_that_exceed_the_fragment_budget_before_reserving)
{
    const GeometryFacade facade;
    osrm::engine::isochrone::SearchResult search_result;
    search_result.nodes.push_back(
        {0, EdgeDuration{0}, osrm::engine::isochrone::NodeProvenance::Network});
    search_result.inbound_frontiers.push_back(
        {{0, EdgeDuration{0}, osrm::engine::isochrone::NodeProvenance::Network}});
    auto limits = osrm::engine::isochrone::MaterializationLimits{};
    limits.maximum_input_fragments = 1;

    const auto result = materialize(facade, search_result, true, EdgeDuration{1'000}, limits);

    BOOST_CHECK(result.error == osrm::engine::isochrone::MaterializationError::BudgetExceeded);
    BOOST_CHECK(result.polylines.empty());
}

BOOST_AUTO_TEST_SUITE_END()
