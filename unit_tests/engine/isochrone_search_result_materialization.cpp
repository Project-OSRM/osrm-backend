#include "engine/isochrone/search_result_materialization.hpp"
#include "engine/routing_algorithms/bounded_duration_one_to_all.hpp"

#include "mocks/mock_datafacade.hpp"

#include <boost/assert.hpp>
#include <boost/test/unit_test.hpp>

#include <array>
#include <cstdint>
#include <span>

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
        BOOST_ASSERT(node < 4);
        return {node < 2 ? 0U : 1U, node % 2 == 0};
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

    unsigned GetNumberOfNodes() const { return 4; }

    bool HasIsochroneGraph() const { return true; }

    std::span<const osrm::engine::isochrone::DurationGraphArc>
    GetIsochroneForwardEdgeRange(const NodeID node) const
    {
        if (node == 2)
            return {isochrone_forward_arcs.data(), 1};
        if (node == 3)
            return {isochrone_forward_arcs.data() + 1, 1};
        return {};
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
    std::array<osrm::engine::isochrone::DurationGraphArc, 2> isochrone_forward_arcs = {
        osrm::engine::isochrone::DurationGraphArc{0, EdgeWeight{100}, EdgeDuration{10}},
        osrm::engine::isochrone::DurationGraphArc{1, EdgeWeight{1}, EdgeDuration{200}}};

    void setDirectionWeights(const std::uint32_t forward_first,
                             const std::uint32_t forward_second,
                             const std::uint32_t reverse_first,
                             const std::uint32_t reverse_second)
    {
        forward_weight_storage[0] = packWeights(forward_first, forward_second);
        reverse_weight_storage[0] = packWeights(reverse_first, reverse_second);
    }

    void setDirectionDurations(const std::uint32_t forward_first,
                               const std::uint32_t forward_second,
                               const std::uint32_t reverse_first,
                               const std::uint32_t reverse_second)
    {
        forward_duration_storage[0] = packDurations(forward_first, forward_second);
        reverse_duration_storage[0] = packDurations(reverse_first, reverse_second);
    }
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

BOOST_AUTO_TEST_CASE(rejects_a_fast_high_weight_direction_when_its_reverse_shares_the_geometry)
{
    const GeometryFacade facade;
    const auto source_coordinate =
        osrm::util::Coordinate{osrm::util::FloatLongitude{0.}, osrm::util::FloatLatitude{0.}};
    const auto source = makePhantom(2,
                                    3,
                                    EdgeWeight{0},
                                    EdgeWeight{0},
                                    EdgeDuration{0},
                                    EdgeDuration{0},
                                    0,
                                    source_coordinate,
                                    source_coordinate);

    // Nodes 0 and 1 are the forward and reverse edge-based states of GeometryID 0. The
    // forward direction reaches the shared road in 1 s at weight 100; the reverse direction
    // reaches the same road in 20 s at weight 1. A route to any shared point chooses the latter
    // (after its local target cost), so a 10 s weight-first contour must not include the former.
    auto search_result = osrm::engine::routing_algorithms::boundedDurationOneToAllSearch<
        osrm::engine::routing_algorithms::FORWARD_DIRECTION>(
        facade, {source}, EdgeDuration{100}, 100);
    // The slow direction settles first in primary-cost order. Its maximum weight over the entire
    // physical geometry is still below the fast direction's entry weight, so the bounded search
    // can prove the fast label is dominated without settling it.
    BOOST_CHECK(search_result.nodes.empty());
    BOOST_REQUIRE_EQUAL(search_result.competitors.size(), 1);
    BOOST_CHECK_EQUAL(search_result.competitors.front().node, 1);
    BOOST_CHECK_EQUAL(osrm::from_alias<int>(search_result.competitors.front().duration), 200);
    BOOST_CHECK_EQUAL(osrm::from_alias<int>(search_result.competitors.front().weight), 1);

    // Source-phantom clips describe direct access, not the conflicting normal network states
    // under test. Keep only the SearchResult-to-materialization boundary here.
    search_result.phantom_partials.clear();
    const auto materialization = materialize(facade, search_result, false, EdgeDuration{100});

    BOOST_REQUIRE(materialization.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(materialization.polylines.size(), 1);
    const auto rasterization = osrm::engine::isochrone::rasterizeWeightedPolylines(
        materialization.polylines, {}, source_coordinate);

    BOOST_REQUIRE(rasterization.grid);
    BOOST_CHECK(rasterization.grid->buildContours(10.).empty());
}

BOOST_AUTO_TEST_CASE(over_cutoff_direct_phantom_competes_without_becoming_output)
{
    const GeometryFacade facade;
    const auto source_coordinate =
        osrm::util::Coordinate{osrm::util::FloatLongitude{0.}, osrm::util::FloatLatitude{0.}};

    // A reaches the forward direction of GeometryID 0 through the graph.  It is in range, but
    // carries a high profile weight.
    auto candidate_a = makePhantom(2,
                                   3,
                                   EdgeWeight{0},
                                   EdgeWeight{0},
                                   EdgeDuration{0},
                                   EdgeDuration{0},
                                   0,
                                   source_coordinate,
                                   source_coordinate);
    candidate_a.reverse_segment_id.enabled = false;

    // B starts directly on the reverse direction of the same GeometryID.  Its elapsed duration
    // is already over the cutoff and it has no outgoing isochrone arc, but its lower profile
    // weight must still suppress A where their road fragments overlap.
    auto competitor_b = makePhantom(0,
                                    1,
                                    EdgeWeight{0},
                                    EdgeWeight{0},
                                    EdgeDuration{0},
                                    EdgeDuration{0},
                                    1,
                                    facade.coordinates[2],
                                    facade.coordinates[2]);
    competitor_b.forward_segment_id.enabled = false;
    competitor_b.approach_weight = EdgeWeight{1};
    competitor_b.approach_duration = EdgeDuration{200};

    const auto phantom_only = osrm::engine::routing_algorithms::boundedDurationOneToAllSearch<
        osrm::engine::routing_algorithms::FORWARD_DIRECTION>(
        facade, {competitor_b}, EdgeDuration{100}, 100);
    BOOST_CHECK(phantom_only.isComplete());
    BOOST_CHECK(phantom_only.nodes.empty());
    BOOST_REQUIRE_EQUAL(phantom_only.phantom_partials.size(), 1);
    BOOST_CHECK(!phantom_only.phantom_partials.front().reaches_network);

    auto search_result = osrm::engine::routing_algorithms::boundedDurationOneToAllSearch<
        osrm::engine::routing_algorithms::FORWARD_DIRECTION>(
        facade, {candidate_a, competitor_b}, EdgeDuration{100}, 100);
    BOOST_REQUIRE(search_result.isComplete());
    BOOST_REQUIRE_EQUAL(search_result.nodes.size(), 1);
    BOOST_CHECK_EQUAL(search_result.nodes.front().node, 0);
    BOOST_CHECK_EQUAL(osrm::from_alias<int>(search_result.nodes.front().weight), 100);

    // Candidate A's source-geometry clip is unrelated to the shared road under test.  Keep B's
    // direct, over-cutoff partial at node 1 and the actual network label at node 0.
    search_result.phantom_partials.erase(std::remove_if(search_result.phantom_partials.begin(),
                                                        search_result.phantom_partials.end(),
                                                        [](const auto &partial)
                                                        { return partial.node == 2; }),
                                         search_result.phantom_partials.end());
    BOOST_REQUIRE_EQUAL(search_result.phantom_partials.size(), 1);
    BOOST_CHECK_EQUAL(search_result.phantom_partials.front().node, 1);
    BOOST_CHECK(!search_result.phantom_partials.front().reaches_network);

    const auto materialization = materialize(facade, search_result, false, EdgeDuration{100});
    BOOST_REQUIRE(materialization.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(materialization.polylines.size(), 2);

    const auto rasterization = osrm::engine::isochrone::rasterizeWeightedPolylines(
        materialization.polylines, {}, source_coordinate);
    BOOST_REQUIRE(rasterization.grid);
    BOOST_CHECK(rasterization.grid->buildContours(10.).empty());
}

BOOST_AUTO_TEST_CASE(reconciles_near_limit_weight_labels_without_overflow)
{
    const GeometryFacade facade;
    const auto invalid_weight = osrm::from_alias<std::int32_t>(INVALID_EDGE_WEIGHT);
    osrm::engine::isochrone::SearchResult search_result;
    search_result.nodes.push_back({0,
                                   EdgeDuration{10},
                                   osrm::engine::isochrone::NodeProvenance::Network,
                                   EdgeWeight{invalid_weight - 40}});
    search_result.competitors.push_back({1,
                                         EdgeDuration{200},
                                         osrm::engine::isochrone::NodeProvenance::Network,
                                         EdgeWeight{invalid_weight - 120}});

    // Both labels and their complete segment prefixes remain below INVALID_EDGE_WEIGHT. The
    // reverse geometry is still lower weight at every shared physical position, even though the
    // forward label is far quicker. Reconciliation must not overflow and reverse that ordering.
    const auto materialization = materialize(facade, search_result, false, EdgeDuration{100});

    BOOST_REQUIRE(materialization.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(materialization.polylines.size(), 2);
    const auto rasterization = osrm::engine::isochrone::rasterizeWeightedPolylines(
        materialization.polylines,
        {},
        osrm::util::Coordinate{osrm::util::FloatLongitude{0.}, osrm::util::FloatLatitude{0.}});

    BOOST_REQUIRE(rasterization.grid);
    BOOST_CHECK(rasterization.grid->buildContours(10.).empty());
}

BOOST_AUTO_TEST_CASE(retains_an_over_cutoff_competitor_settled_after_the_reachable_target)
{
    GeometryFacade facade;
    facade.setDirectionWeights(100, 100, 1, 1);
    facade.setDirectionDurations(10, 10, 10, 10);
    facade.isochrone_forward_arcs = {
        osrm::engine::isochrone::DurationGraphArc{0, EdgeWeight{100}, EdgeDuration{10}},
        osrm::engine::isochrone::DurationGraphArc{1, EdgeWeight{101}, EdgeDuration{200}}};
    const auto source_coordinate =
        osrm::util::Coordinate{osrm::util::FloatLongitude{0.}, osrm::util::FloatLatitude{0.}};
    const auto source = makePhantom(2,
                                    3,
                                    EdgeWeight{0},
                                    EdgeWeight{0},
                                    EdgeDuration{0},
                                    EdgeDuration{0},
                                    0,
                                    source_coordinate,
                                    source_coordinate);

    // Node 0 reaches the western endpoint first and exhausts the duration-candidate target set.
    // Node 1 is then next in weight order, but is over the duration cutoff. Its reverse segment
    // weights are much smaller, so it wins over node 0 through the middle and eastern portions of
    // the shared GeometryID. Stopping at node 0 would lose a necessary physical-road competitor.
    const auto search_result = osrm::engine::routing_algorithms::boundedDurationOneToAllSearch<
        osrm::engine::routing_algorithms::FORWARD_DIRECTION>(
        facade, {source}, EdgeDuration{100}, 100);

    BOOST_REQUIRE_EQUAL(search_result.nodes.size(), 1);
    BOOST_CHECK_EQUAL(search_result.nodes.front().node, 0);
    BOOST_CHECK_EQUAL(osrm::from_alias<int>(search_result.nodes.front().weight), 100);
    BOOST_REQUIRE_EQUAL(search_result.competitors.size(), 1);
    BOOST_CHECK_EQUAL(search_result.competitors.front().node, 1);
    BOOST_CHECK_EQUAL(osrm::from_alias<int>(search_result.competitors.front().duration), 200);
    BOOST_CHECK_EQUAL(osrm::from_alias<int>(search_result.competitors.front().weight), 101);
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

BOOST_AUTO_TEST_CASE(retains_a_complete_inbound_frontier_for_weight_reconciliation)
{
    const GeometryFacade facade;
    osrm::engine::isochrone::SearchResult search_result;
    search_result.inbound_frontiers.push_back(
        {{0, EdgeDuration{520}, osrm::engine::isochrone::NodeProvenance::Network}});

    const auto result = materialize(facade, search_result, true, EdgeDuration{200});

    BOOST_REQUIRE(result.error == osrm::engine::isochrone::MaterializationError::None);
    BOOST_REQUIRE_EQUAL(result.polylines.size(), 1);
    BOOST_REQUIRE_EQUAL(result.polylines[0].size(), 3);
    // Materialization retains the over-cutoff prefix so alternatives on the same physical
    // geometry can be compared by weight before rasterization applies the 20 s contour.
    BOOST_CHECK_CLOSE(longitude(result.polylines[0][0]), 0., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0][0].duration, 52., 1e-9);
    BOOST_CHECK_CLOSE(longitude(result.polylines[0][1]), 1., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0][1].duration, 42., 1e-9);
    BOOST_CHECK_CLOSE(longitude(result.polylines[0][2]), 2., 1e-9);
    BOOST_CHECK_CLOSE(result.polylines[0][2].duration, 12., 1e-9);
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
    GeometryFacade facade;
    // A traffic closure after the snapped point does not invalidate the target prefix. Both the
    // weight and duration use their packed invalid sentinels in real traffic updates.
    facade.setDirectionWeights(10, from_alias<std::uint32_t>(INVALID_SEGMENT_WEIGHT), 40, 30);
    facade.setDirectionDurations(
        100, from_alias<std::uint32_t>(INVALID_SEGMENT_DURATION), 700, 500);
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
