#include "engine/routing_algorithms/bounded_duration_one_to_all.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

namespace osrm::engine::routing_algorithms
{
namespace
{

struct LevelZeroFacade
{
    unsigned number_of_nodes;
    std::vector<isochrone::DurationGraphArc> forward_arcs;
    std::vector<std::pair<EdgeID, EdgeID>> forward_ranges;
    std::vector<isochrone::DurationGraphArc> reverse_arcs;
    std::vector<std::pair<EdgeID, EdgeID>> reverse_ranges;
    std::vector<bool> excluded;
    std::vector<GeometryID> geometry_indices;
    std::vector<std::vector<SegmentWeight>> forward_geometry_weights;
    std::vector<std::vector<SegmentWeight>> reverse_geometry_weights;

    unsigned GetNumberOfNodes() const { return number_of_nodes; }
    bool HasIsochroneGraph() const { return true; }

    std::span<const isochrone::DurationGraphArc>
    getRange(const std::vector<isochrone::DurationGraphArc> &arcs,
             const std::vector<std::pair<EdgeID, EdgeID>> &ranges,
             const NodeID node) const
    {
        const auto [begin, end] = ranges[node];
        if (begin == end)
            return {};
        return {arcs.data() + begin, static_cast<std::size_t>(end - begin)};
    }

    auto GetIsochroneForwardEdgeRange(const NodeID node) const
    { return getRange(forward_arcs, forward_ranges, node); }

    auto GetIsochroneReverseEdgeRange(const NodeID node) const
    { return getRange(reverse_arcs, reverse_ranges, node); }

    bool ExcludeNode(const NodeID node) const { return excluded[node]; }

    GeometryID GetGeometryIndex(const NodeID node) const
    {
        if (geometry_indices.empty())
            return {static_cast<PackedGeometryID>(node), true};
        return geometry_indices[node];
    }

    std::span<const SegmentWeight>
    GetUncompressedForwardWeights(const PackedGeometryID geometry_id) const
    {
        if (geometry_id >= forward_geometry_weights.size())
            return {};
        return forward_geometry_weights[geometry_id];
    }

    std::span<const SegmentWeight>
    GetUncompressedReverseWeights(const PackedGeometryID geometry_id) const
    {
        if (geometry_id >= reverse_geometry_weights.size())
            return {};
        return reverse_geometry_weights[geometry_id];
    }
};

using Adjacency = std::initializer_list<std::tuple<NodeID, NodeID, int>>;

LevelZeroFacade makeFacade(const unsigned number_of_nodes,
                           const Adjacency forward,
                           const Adjacency reverse,
                           std::vector<bool> excluded)
{
    const auto makeGraph = [number_of_nodes](const Adjacency adjacency,
                                             std::vector<isochrone::DurationGraphArc> &arcs,
                                             std::vector<std::pair<EdgeID, EdgeID>> &ranges)
    {
        ranges.resize(number_of_nodes);
        for (NodeID source = 0; source < number_of_nodes; ++source)
        {
            const auto begin = EdgeID{static_cast<unsigned>(arcs.size())};
            for (const auto &[from, to, duration] : adjacency)
            {
                if (from == source)
                    arcs.push_back({to, EdgeWeight{std::max(1, duration)}, EdgeDuration{duration}});
            }
            ranges[source] = {begin, EdgeID{static_cast<unsigned>(arcs.size())}};
        }
    };

    LevelZeroFacade facade{number_of_nodes, {}, {}, {}, {}, std::move(excluded), {}, {}, {}};
    makeGraph(forward, facade.forward_arcs, facade.forward_ranges);
    makeGraph(reverse, facade.reverse_arcs, facade.reverse_ranges);
    return facade;
}

LevelZeroFacade makeFacade(const unsigned number_of_nodes,
                           const std::vector<std::tuple<NodeID, NodeID, int>> &forward,
                           const std::vector<std::tuple<NodeID, NodeID, int>> &reverse,
                           std::vector<bool> excluded)
{
    const auto makeGraph = [number_of_nodes](const auto &adjacency,
                                             std::vector<isochrone::DurationGraphArc> &arcs,
                                             std::vector<std::pair<EdgeID, EdgeID>> &ranges)
    {
        ranges.resize(number_of_nodes);
        for (NodeID source = 0; source < number_of_nodes; ++source)
        {
            const auto begin = EdgeID{static_cast<unsigned>(arcs.size())};
            for (const auto &[from, to, duration] : adjacency)
            {
                if (from == source)
                    arcs.push_back({to, EdgeWeight{std::max(1, duration)}, EdgeDuration{duration}});
            }
            ranges[source] = {begin, EdgeID{static_cast<unsigned>(arcs.size())}};
        }
    };

    LevelZeroFacade facade{number_of_nodes, {}, {}, {}, {}, std::move(excluded), {}, {}, {}};
    makeGraph(forward, facade.forward_arcs, facade.forward_ranges);
    makeGraph(reverse, facade.reverse_arcs, facade.reverse_ranges);
    return facade;
}

using WeightedAdjacency = std::initializer_list<std::tuple<NodeID, NodeID, int, int>>;

LevelZeroFacade makeWeightedFacade(const unsigned number_of_nodes,
                                   const WeightedAdjacency forward,
                                   const WeightedAdjacency reverse,
                                   std::vector<bool> excluded)
{
    const auto make_graph = [number_of_nodes](const WeightedAdjacency adjacency,
                                              std::vector<isochrone::DurationGraphArc> &arcs,
                                              std::vector<std::pair<EdgeID, EdgeID>> &ranges)
    {
        ranges.resize(number_of_nodes);
        for (NodeID source = 0; source < number_of_nodes; ++source)
        {
            const auto begin = EdgeID{static_cast<unsigned>(arcs.size())};
            for (const auto &[from, to, weight, duration] : adjacency)
            {
                if (from == source)
                    arcs.push_back({to, EdgeWeight{weight}, EdgeDuration{duration}});
            }
            ranges[source] = {begin, EdgeID{static_cast<unsigned>(arcs.size())}};
        }
    };

    LevelZeroFacade facade{number_of_nodes, {}, {}, {}, {}, std::move(excluded), {}, {}, {}};
    make_graph(forward, facade.forward_arcs, facade.forward_ranges);
    make_graph(reverse, facade.reverse_arcs, facade.reverse_ranges);
    return facade;
}

using WeightedEdges = std::vector<std::tuple<NodeID, NodeID, int, int>>;

LevelZeroFacade makeWeightedFacade(const unsigned number_of_nodes,
                                   const WeightedEdges &forward,
                                   const WeightedEdges &reverse,
                                   std::vector<bool> excluded)
{
    const auto makeGraph = [number_of_nodes](const WeightedEdges &adjacency,
                                             std::vector<isochrone::DurationGraphArc> &arcs,
                                             std::vector<std::pair<EdgeID, EdgeID>> &ranges)
    {
        ranges.resize(number_of_nodes);
        for (NodeID source = 0; source < number_of_nodes; ++source)
        {
            const auto begin = EdgeID{static_cast<unsigned>(arcs.size())};
            for (const auto &[from, to, weight, duration] : adjacency)
            {
                if (from == source)
                    arcs.push_back({to, EdgeWeight{weight}, EdgeDuration{duration}});
            }
            ranges[source] = {begin, EdgeID{static_cast<unsigned>(arcs.size())}};
        }
    };

    LevelZeroFacade facade{number_of_nodes, {}, {}, {}, {}, std::move(excluded), {}, {}, {}};
    makeGraph(forward, facade.forward_arcs, facade.forward_ranges);
    makeGraph(reverse, facade.reverse_arcs, facade.reverse_ranges);
    return facade;
}

using WeightedLabel = std::pair<int, int>;

std::vector<WeightedLabel> bellmanFordWeighted(const unsigned number_of_nodes,
                                               const WeightedEdges &edges,
                                               const std::vector<bool> &excluded,
                                               const NodeID source)
{
    const auto infinity = std::numeric_limits<int>::max();
    std::vector<WeightedLabel> labels(number_of_nodes, {infinity, infinity});
    labels[source] = {0, 0};

    for (unsigned pass = 0; pass < number_of_nodes - 1; ++pass)
    {
        auto changed = false;
        for (const auto &[from, to, weight, duration] : edges)
        {
            if (excluded[to] || labels[from].first == infinity)
                continue;
            const WeightedLabel candidate{labels[from].first + weight,
                                          labels[from].second + duration};
            if (candidate < labels[to])
            {
                labels[to] = candidate;
                changed = true;
            }
        }
        if (!changed)
            break;
    }
    return labels;
}

void checkAgainstWeightedOracle(const isochrone::SearchResult &result,
                                const std::vector<WeightedLabel> &expected,
                                const std::vector<bool> &excluded,
                                const NodeID synthetic_seed,
                                const int cutoff)
{
    const auto infinity = std::numeric_limits<int>::max();
    std::vector<WeightedLabel> actual(expected.size(), {infinity, infinity});
    for (const auto &node : result.nodes)
    {
        BOOST_REQUIRE(node.provenance == isochrone::NodeProvenance::Network);
        BOOST_REQUIRE(node.node != synthetic_seed);
        BOOST_REQUIRE(actual[node.node].first == infinity);
        actual[node.node] = {from_alias<int>(node.weight), from_alias<int>(node.duration)};
    }

    for (NodeID node = 0; node < expected.size(); ++node)
    {
        const auto &expected_label = expected[node];
        if (node == synthetic_seed || excluded[node] || expected_label.second > cutoff)
            BOOST_CHECK_EQUAL(actual[node].first, infinity);
        else
        {
            BOOST_CHECK_EQUAL(actual[node].first, expected_label.first);
            BOOST_CHECK_EQUAL(actual[node].second, expected_label.second);
        }
    }
}

std::vector<int> bellmanFord(const unsigned number_of_nodes,
                             const std::vector<std::tuple<NodeID, NodeID, int>> &edges,
                             const std::vector<bool> &excluded,
                             const NodeID source,
                             const int cutoff)
{
    const auto infinity = std::numeric_limits<int>::max();
    std::vector<int> durations(number_of_nodes, infinity);
    durations[source] = 0;

    for (unsigned pass = 0; pass < number_of_nodes - 1; ++pass)
    {
        auto changed = false;
        for (const auto &[from, to, duration] : edges)
        {
            if (excluded[to] || durations[from] == infinity || durations[from] + duration > cutoff)
                continue;
            if (durations[from] + duration < durations[to])
            {
                durations[to] = durations[from] + duration;
                changed = true;
            }
        }
        if (!changed)
            break;
    }
    return durations;
}

void checkAgainstBellmanFord(const isochrone::SearchResult &result,
                             const std::vector<int> &expected,
                             const std::vector<bool> &excluded,
                             const NodeID virtual_seed)
{
    const auto infinity = std::numeric_limits<int>::max();
    std::vector<int> actual(expected.size(), infinity);
    for (const auto &node : result.nodes)
    {
        BOOST_REQUIRE(node.provenance == isochrone::NodeProvenance::Network);
        BOOST_REQUIRE(node.node != virtual_seed);
        BOOST_REQUIRE(actual[node.node] == infinity);
        actual[node.node] = from_alias<int>(node.duration);
    }

    for (NodeID node = 0; node < expected.size(); ++node)
    {
        if (node == virtual_seed || excluded[node])
            BOOST_CHECK_EQUAL(actual[node], infinity);
        else
            BOOST_CHECK_EQUAL(actual[node], expected[node]);
    }
}

PhantomNode makePhantom(const NodeID forward_segment,
                        const NodeID reverse_segment,
                        const EdgeDuration forward_duration,
                        const EdgeDuration reverse_duration,
                        const EdgeDuration forward_duration_offset,
                        const EdgeDuration reverse_duration_offset,
                        const bool forward_source,
                        const bool forward_target,
                        const bool reverse_source,
                        const bool reverse_target)
{
    struct Segment
    {
        SegmentID forward_segment_id;
        SegmentID reverse_segment_id;
        unsigned short fwd_segment_position;
    } segment{{forward_segment, true}, {reverse_segment, true}, 0};

    const util::Coordinate coordinate{util::FixedLongitude{0}, util::FixedLatitude{0}};
    return PhantomNode{segment,
                       ComponentID{1, false},
                       EdgeWeight{0},
                       EdgeWeight{0},
                       EdgeWeight{0},
                       EdgeWeight{0},
                       EdgeDistance{0},
                       EdgeDistance{0},
                       EdgeDistance{0},
                       EdgeDistance{0},
                       forward_duration,
                       reverse_duration,
                       forward_duration_offset,
                       reverse_duration_offset,
                       forward_source,
                       forward_target,
                       reverse_source,
                       reverse_target,
                       coordinate,
                       coordinate,
                       0};
}

void checkNode(const isochrone::SearchResult::Node &node,
               const NodeID expected_node,
               const int expected_duration)
{
    BOOST_CHECK_EQUAL(node.node, expected_node);
    BOOST_CHECK_EQUAL(from_alias<int>(node.duration), expected_duration);
}

} // namespace

BOOST_AUTO_TEST_SUITE(bounded_duration_one_to_all)

BOOST_AUTO_TEST_CASE(forward_search_uses_the_independent_directed_isochrone_graph)
{
    auto facade = makeFacade(6,
                             {{0, 1, 0}, {0, 2, 0}, {0, 5, 0}, {1, 3, 40}, {2, 3, 15}},
                             {},
                             {false, false, false, false, false, true});
    const auto source = makePhantom(0, 5, {0}, {0}, {0}, {0}, true, false, false, false);

    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{30}, 100);

    // The isochrone sidecar is charged from the forward source edge-based
    // node. Both first-hop nodes therefore inherit node 0's zero duration;
    // the second path uses node 2's duration plus its turn penalty.
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 3);
    checkNode(result.nodes[0], 1, 0);
    checkNode(result.nodes[1], 2, 0);
    checkNode(result.nodes[2], 3, 15);
}

BOOST_AUTO_TEST_CASE(search_minimizes_weight_and_measures_the_winning_path_duration)
{
    // The path through node 1 is physically faster (2 duration units) but costs 11 weight units.
    // The path through node 2 costs only 4 weight units and therefore wins even though it takes
    // 12 duration units.
    auto facade = makeWeightedFacade(4,
                                     {{0, 1, 1, 1}, {1, 3, 10, 1}, {0, 2, 2, 6}, {2, 3, 2, 6}},
                                     {},
                                     {false, false, false, false});
    const auto source = makePhantom(0, 3, {0}, {0}, {0}, {0}, true, false, false, false);

    const auto below_winning_duration =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{10}, 100);
    BOOST_CHECK(std::none_of(below_winning_duration.nodes.begin(),
                             below_winning_duration.nodes.end(),
                             [](const auto &node) { return node.node == 3; }));

    const auto at_winning_duration =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{12}, 100);
    const auto target = std::find_if(at_winning_duration.nodes.begin(),
                                     at_winning_duration.nodes.end(),
                                     [](const auto &node) { return node.node == 3; });
    BOOST_REQUIRE(target != at_winning_duration.nodes.end());
    checkNode(*target, 3, 12);
}

BOOST_AUTO_TEST_CASE(equal_weight_paths_use_duration_as_the_secondary_label)
{
    auto facade = makeWeightedFacade(4,
                                     {{0, 1, 2, 10}, {1, 3, 2, 10}, {0, 2, 2, 1}, {2, 3, 2, 1}},
                                     {},
                                     {false, false, false, false});
    const auto source = makePhantom(0, 3, {0}, {0}, {0}, {0}, true, false, false, false);

    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{5}, 100);
    const auto target = std::find_if(
        result.nodes.begin(), result.nodes.end(), [](const auto &node) { return node.node == 3; });
    BOOST_REQUIRE(target != result.nodes.end());
    checkNode(*target, 3, 2);
}

BOOST_AUTO_TEST_CASE(duration_cutoff_does_not_terminate_the_weight_ordered_heap)
{
    auto facade = makeWeightedFacade(3,
                                     {{0, 1, 1, 100}, {0, 2, 100, 1}},
                                     {{0, 1, 1, 100}, {0, 2, 100, 1}},
                                     {false, false, false});
    const auto endpoint = makePhantom(0, 2, {0}, {0}, {0}, {0}, true, true, false, false);

    const auto outbound =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {endpoint}, EdgeDuration{10}, 100);
    BOOST_REQUIRE_EQUAL(outbound.nodes.size(), 1);
    checkNode(outbound.nodes.front(), 2, 1);

    const auto inbound =
        boundedDurationOneToAllSearch<REVERSE_DIRECTION>(facade, {endpoint}, EdgeDuration{10}, 100);
    BOOST_REQUIRE_EQUAL(inbound.nodes.size(), 1);
    checkNode(inbound.nodes.front(), 2, 1);
    BOOST_REQUIRE_EQUAL(inbound.inbound_frontiers.size(), 1);
    checkNode(inbound.inbound_frontiers.front().entry, 1, 100);
}

BOOST_AUTO_TEST_CASE(over_cutoff_labels_propagate_to_suppress_more_expensive_paths)
{
    // Node 1 is reachable quickly at weight 100, but its globally preferred route goes through
    // over-cutoff node 2 at weight 2. The slow label must still propagate to node 1 so the fast,
    // expensive route is not rendered as if OSRM would select it.
    auto facade = makeWeightedFacade(3,
                                     {{0, 1, 100, 1}, {0, 2, 1, 20}, {2, 1, 1, 0}},
                                     {{0, 1, 100, 1}, {0, 2, 1, 20}, {2, 1, 1, 0}},
                                     {false, false, false});
    const auto endpoint = makePhantom(0, 2, {0}, {0}, {0}, {0}, true, true, false, false);

    for (const auto direction : {FORWARD_DIRECTION, REVERSE_DIRECTION})
    {
        const auto result = direction == FORWARD_DIRECTION
                                ? boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
                                      facade, {endpoint}, EdgeDuration{10}, 100)
                                : boundedDurationOneToAllSearch<REVERSE_DIRECTION>(
                                      facade, {endpoint}, EdgeDuration{10}, 100);
        BOOST_CHECK(std::none_of(result.nodes.begin(),
                                 result.nodes.end(),
                                 [](const auto &node) { return node.node == 1; }));
    }
}

BOOST_AUTO_TEST_CASE(over_cutoff_phantom_seed_can_suppress_a_faster_candidate)
{
    auto facade = makeWeightedFacade(
        3, {{0, 2, 1, 1}, {1, 2, 1, 1}}, {{0, 2, 1, 1}, {1, 2, 1, 1}}, {false, false, false});
    auto slow_preferred = makePhantom(0, 2, {0}, {0}, {0}, {0}, true, true, false, false);
    slow_preferred.approach_duration = {20};
    auto fast_expensive = makePhantom(1, 2, {0}, {0}, {0}, {0}, true, true, false, false);
    fast_expensive.approach_weight = {100};

    for (const auto direction : {FORWARD_DIRECTION, REVERSE_DIRECTION})
    {
        const auto result =
            direction == FORWARD_DIRECTION
                ? boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
                      facade, {slow_preferred, fast_expensive}, EdgeDuration{10}, 100)
                : boundedDurationOneToAllSearch<REVERSE_DIRECTION>(
                      facade, {slow_preferred, fast_expensive}, EdgeDuration{10}, 100);
        BOOST_CHECK(std::none_of(result.nodes.begin(),
                                 result.nodes.end(),
                                 [](const auto &node) { return node.node == 2; }));
    }
}

BOOST_AUTO_TEST_CASE(forward_termination_uses_the_lowest_full_geometry_bound)
{
    // The first direction of geometry 1 settles with a full bound of 100.  The reverse direction
    // settles next with a lower bound of 10, so the unrelated weight-50 branch must not be
    // expanded.  Its expansion would exceed the record budget.
    auto facade = makeWeightedFacade(5,
                                     {{0, 1, 1, 1}, {0, 2, 2, 1}, {0, 3, 50, 100}, {3, 4, 1, 0}},
                                     {},
                                     {false, false, false, false, false});
    facade.geometry_indices = {GeometryID{0, true},
                               GeometryID{1, true},
                               GeometryID{1, false},
                               GeometryID{2, true},
                               GeometryID{3, true}};
    facade.forward_geometry_weights = {
        {SegmentWeight{0}}, {SegmentWeight{99}}, {SegmentWeight{0}}, {SegmentWeight{0}}};
    facade.reverse_geometry_weights = {
        {SegmentWeight{0}}, {SegmentWeight{8}}, {SegmentWeight{0}}, {SegmentWeight{0}}};
    const auto source = makePhantom(0, 0, {0}, {0}, {0}, {0}, true, false, false, false);

    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{10}, 6);

    BOOST_CHECK(result.isComplete());
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 2);
    checkNode(result.nodes[0], 1, 1);
    checkNode(result.nodes[1], 2, 1);
}

BOOST_AUTO_TEST_CASE(candidate_weight_limit_tracks_full_and_partial_geometries)
{
    const auto make_targets = []
    {
        detail::DurationCandidateTargets targets;
        targets.geometries.emplace(1,
                                   detail::DurationCandidateTargets::Geometry{
                                       true, std::nullopt, std::nullopt, 7, std::nullopt});
        targets.geometries.emplace(
            2,
            detail::DurationCandidateTargets::Geometry{false, 30, std::nullopt, 9, std::nullopt});
        return targets;
    };

    auto forward_targets = make_targets();
    detail::initializeCandidateWeightLimits<FORWARD_DIRECTION>(forward_targets);
    BOOST_CHECK(!detail::candidateWeightLimit(forward_targets).ready);

    auto &forward_geometry = forward_targets.geometries.at(1);
    forward_geometry.full_upper_weight = 45;
    detail::updateCandidateWeightLimit<FORWARD_DIRECTION>(forward_targets, 1, forward_geometry);
    BOOST_CHECK_EQUAL(detail::candidateWeightLimit(forward_targets).weight, 45);

    // A later direction of the same physical geometry can have a lower full traversal bound.
    forward_geometry.full_upper_weight = 10;
    detail::updateCandidateWeightLimit<FORWARD_DIRECTION>(forward_targets, 1, forward_geometry);
    BOOST_CHECK_EQUAL(detail::candidateWeightLimit(forward_targets).weight, 30);

    auto reverse_targets = make_targets();
    detail::initializeCandidateWeightLimits<REVERSE_DIRECTION>(reverse_targets);
    auto &reverse_geometry = reverse_targets.geometries.at(1);
    reverse_geometry.full_upper_weight = 45;
    detail::updateCandidateWeightLimit<REVERSE_DIRECTION>(reverse_targets, 1, reverse_geometry);
    BOOST_CHECK_EQUAL(detail::candidateWeightLimit(reverse_targets).weight, 52);

    reverse_geometry.full_upper_weight = 10;
    detail::updateCandidateWeightLimit<REVERSE_DIRECTION>(reverse_targets, 1, reverse_geometry);
    BOOST_CHECK_EQUAL(detail::candidateWeightLimit(reverse_targets).weight, 39);
}

BOOST_AUTO_TEST_CASE(randomized_acyclic_graphs_match_a_bellman_ford_oracle_in_both_directions)
{
    // Acyclic graphs keep the virtual seed out of the reference result. The dedicated reentry
    // tests below exercise virtual-seed behavior separately, while this oracle checks the normal
    // settled labels with a completely independent relaxation algorithm.
    auto random_state = std::uint32_t{0x9e3779b9};
    const auto next_random = [&random_state]()
    {
        random_state = random_state * 1'664'525U + 1'013'904'223U;
        return random_state;
    };

    for (unsigned graph_index = 0; graph_index < 64; ++graph_index)
    {
        const auto number_of_nodes = 8U + next_random() % 8U;
        std::vector<std::tuple<NodeID, NodeID, int>> forward;
        std::vector<std::tuple<NodeID, NodeID, int>> reverse;
        for (NodeID from = 0; from < number_of_nodes; ++from)
        {
            for (NodeID to = from + 1; to < number_of_nodes; ++to)
            {
                if (next_random() % 4U == 0)
                    continue;
                const auto duration = 1 + static_cast<int>(next_random() % 31U);
                forward.emplace_back(from, to, duration);
                reverse.emplace_back(to, from, duration);
            }
        }

        std::vector<bool> excluded(number_of_nodes, false);
        for (NodeID node = 1; node + 1 < number_of_nodes; ++node)
            excluded[node] = next_random() % 5U == 0;

        const auto source =
            makePhantom(0, number_of_nodes - 1, {0}, {0}, {0}, {0}, true, false, false, false);
        const auto target =
            makePhantom(number_of_nodes - 1, 0, {0}, {0}, {0}, {0}, false, true, false, false);
        for (const auto cutoff : {10, 25, 50, 100})
        {
            const auto facade = makeFacade(number_of_nodes, forward, reverse, excluded);

            const auto forward_result = boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
                facade, {source}, EdgeDuration{cutoff}, 10'000);
            BOOST_REQUIRE(forward_result.isComplete());
            checkAgainstBellmanFord(forward_result,
                                    bellmanFord(number_of_nodes, forward, excluded, 0, cutoff),
                                    excluded,
                                    0);

            const auto reverse_result = boundedDurationOneToAllSearch<REVERSE_DIRECTION>(
                facade, {target}, EdgeDuration{cutoff}, 10'000);
            BOOST_REQUIRE(reverse_result.isComplete());
            checkAgainstBellmanFord(
                reverse_result,
                bellmanFord(number_of_nodes, reverse, excluded, number_of_nodes - 1, cutoff),
                excluded,
                number_of_nodes - 1);
        }
    }
}

BOOST_AUTO_TEST_CASE(randomized_weighted_graphs_match_a_lexicographic_bellman_ford_oracle)
{
    auto random_state = std::uint32_t{0x85ebca6b};
    const auto nextRandom = [&random_state]()
    {
        random_state = random_state * 1'664'525U + 1'013'904'223U;
        return random_state;
    };

    for (unsigned graph_index = 0; graph_index < 64; ++graph_index)
    {
        const auto number_of_nodes = 8U + nextRandom() % 8U;
        WeightedEdges forward;
        WeightedEdges reverse;
        for (NodeID from = 0; from < number_of_nodes; ++from)
        {
            for (NodeID to = from + 1; to < number_of_nodes; ++to)
            {
                if (nextRandom() % 4U == 0)
                    continue;
                const auto weight = 1 + static_cast<int>(nextRandom() % 31U);
                const auto duration = static_cast<int>(nextRandom() % 31U);
                forward.emplace_back(from, to, weight, duration);
                reverse.emplace_back(to, from, weight, duration);
            }
        }

        std::vector<bool> excluded(number_of_nodes, false);
        for (NodeID node = 1; node + 1 < number_of_nodes; ++node)
            excluded[node] = nextRandom() % 5U == 0;

        const auto source =
            makePhantom(0, number_of_nodes - 1, {0}, {0}, {0}, {0}, true, false, false, false);
        const auto target =
            makePhantom(number_of_nodes - 1, 0, {0}, {0}, {0}, {0}, false, true, false, false);
        const auto facade = makeWeightedFacade(number_of_nodes, forward, reverse, excluded);
        for (const auto cutoff : {10, 25, 50, 100})
        {
            const auto forward_result = boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
                facade, {source}, EdgeDuration{cutoff}, 10'000);
            BOOST_REQUIRE(forward_result.isComplete());
            checkAgainstWeightedOracle(forward_result,
                                       bellmanFordWeighted(number_of_nodes, forward, excluded, 0),
                                       excluded,
                                       0,
                                       cutoff);

            const auto reverse_result = boundedDurationOneToAllSearch<REVERSE_DIRECTION>(
                facade, {target}, EdgeDuration{cutoff}, 10'000);
            BOOST_REQUIRE(reverse_result.isComplete());
            checkAgainstWeightedOracle(
                reverse_result,
                bellmanFordWeighted(number_of_nodes, reverse, excluded, number_of_nodes - 1),
                excluded,
                number_of_nodes - 1,
                cutoff);
        }
    }
}

BOOST_AUTO_TEST_CASE(reverse_search_uses_target_validity_and_backward_edges)
{
    auto facade = makeFacade(5, {}, {{4, 3, 10}}, {false, false, false, false, false});
    const auto target = makePhantom(1, 4, {0}, {0}, {0}, {0}, false, false, false, true);

    const auto result =
        boundedDurationOneToAllSearch<REVERSE_DIRECTION>(facade, {target}, EdgeDuration{10}, 100);

    BOOST_REQUIRE_EQUAL(result.nodes.size(), 1);
    checkNode(result.nodes.front(), 3, 10);
    BOOST_REQUIRE_EQUAL(result.phantom_partials.size(), 1);

    const auto forward_result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {target}, EdgeDuration{10}, 100);
    BOOST_CHECK(forward_result.nodes.empty());
}

BOOST_AUTO_TEST_CASE(source_offsets_and_an_exact_cutoff_are_preserved)
{
    auto facade = makeFacade(2, {{0, 1, 200}}, {}, {false, false});
    auto source = makePhantom(0, 1, {100}, {0}, {7}, {0}, true, false, false, false);
    source.approach_duration = {5};

    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{98}, 100);

    BOOST_REQUIRE_EQUAL(result.nodes.size(), 1);
    checkNode(result.nodes.front(), 1, 98);
    BOOST_REQUIRE_EQUAL(result.phantom_partials.size(), 1);
    BOOST_CHECK_EQUAL(from_alias<int>(result.phantom_partials.front().seed_duration), -102);
}

BOOST_AUTO_TEST_CASE(inbound_frontier_retains_the_first_duration_crossing)
{
    auto facade = makeFacade(2, {}, {{1, 0, 100}}, {false, false});
    const auto target = makePhantom(1, 0, {0}, {0}, {0}, {0}, false, true, false, false);

    const auto result =
        boundedDurationOneToAllSearch<REVERSE_DIRECTION>(facade, {target}, EdgeDuration{10}, 100);

    BOOST_CHECK(result.isComplete());
    BOOST_CHECK(result.nodes.empty());
    BOOST_REQUIRE_EQUAL(result.inbound_frontiers.size(), 1);
    checkNode(result.inbound_frontiers.front().entry, 0, 100);
    BOOST_CHECK(result.inbound_frontiers.front().entry.provenance ==
                isochrone::NodeProvenance::Network);
}

BOOST_AUTO_TEST_CASE(inbound_search_discards_a_provisional_crossing_reached_within_the_cutoff)
{
    // The first 0 -> 1 reverse relaxation is outside the cutoff, but node 1 is subsequently
    // reached through 0 -> 2 -> 1. A stale frontier would consume the last record-budget slot.
    auto facade = makeFacade(3, {}, {{0, 1, 100}, {0, 2, 1}, {2, 1, 1}}, {false, false, false});
    const auto target = makePhantom(0, 2, {0}, {0}, {0}, {0}, false, true, false, false);

    const auto result =
        boundedDurationOneToAllSearch<REVERSE_DIRECTION>(facade, {target}, EdgeDuration{10}, 6);

    BOOST_CHECK(result.isComplete());
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 2);
    checkNode(result.nodes[0], 2, 1);
    checkNode(result.nodes[1], 1, 2);
    BOOST_CHECK(result.inbound_frontiers.empty());
}

BOOST_AUTO_TEST_CASE(inbound_search_retains_a_frontier_past_a_virtual_target_after_a_loop)
{
    // The target's virtual seed covers only the legal prefix ending at the target phantom. A
    // loop that returns to the same edge-based node can make a suffix after the phantom reachable.
    // The search does not know the geometry duration, so it must retain the over-cutoff entry and
    // let the materializer decide whether such a suffix intersects the cutoff.
    auto facade = makeFacade(1, {}, {{0, 0, 100}}, {false});
    const auto target = makePhantom(0, 0, {0}, {0}, {0}, {0}, false, true, false, false);

    const auto result =
        boundedDurationOneToAllSearch<REVERSE_DIRECTION>(facade, {target}, EdgeDuration{10}, 100);

    BOOST_CHECK(result.isComplete());
    BOOST_CHECK(result.nodes.empty());
    BOOST_REQUIRE_EQUAL(result.inbound_frontiers.size(), 1);
    checkNode(result.inbound_frontiers.front().entry, 0, 100);
}

BOOST_AUTO_TEST_CASE(approach_past_cutoff_retains_a_non_network_terminal_partial)
{
    auto facade = makeFacade(1, {}, {}, {false});
    auto target = makePhantom(0, 0, {0}, {0}, {0}, {0}, false, true, false, false);
    target.approach_duration = {81};

    const auto result =
        boundedDurationOneToAllSearch<REVERSE_DIRECTION>(facade, {target}, EdgeDuration{10}, 100);

    BOOST_CHECK(result.isComplete());
    BOOST_CHECK(result.nodes.empty());
    BOOST_CHECK(result.inbound_frontiers.empty());
    BOOST_REQUIRE_EQUAL(result.phantom_partials.size(), 1);
    const auto &partial = result.phantom_partials.front();
    BOOST_CHECK(!partial.reaches_network);
    BOOST_CHECK(partial.kind == isochrone::PhantomTraversalKind::InboundTerminal);
    BOOST_CHECK_EQUAL(from_alias<int>(partial.seed_duration), 81);
}

BOOST_AUTO_TEST_CASE(returns_no_geometry_when_no_direction_is_usable)
{
    auto facade = makeFacade(1, {}, {}, {false});
    auto phantom = makePhantom(0, 0, {0}, {0}, {0}, {0}, false, false, false, false);
    phantom.input_location = {util::FloatLongitude{1.}, util::FloatLatitude{2.}};

    const auto outbound = boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
        facade, {phantom, phantom}, EdgeDuration{10}, 1);
    const auto inbound = boundedDurationOneToAllSearch<REVERSE_DIRECTION>(
        facade, {phantom, phantom}, EdgeDuration{10}, 1);

    for (const auto *result : {&outbound, &inbound})
    {
        BOOST_CHECK(result->isComplete());
        BOOST_CHECK(result->nodes.empty());
        BOOST_CHECK(result->phantom_partials.empty());
    }
}

BOOST_AUTO_TEST_CASE(returns_no_geometry_when_every_role_valid_seed_is_excluded)
{
    auto facade = makeFacade(1, {}, {}, {true});
    auto phantom = makePhantom(0, 0, {0}, {0}, {0}, {0}, true, true, false, false);
    phantom.input_location = {util::FloatLongitude{1.}, util::FloatLatitude{2.}};

    const auto outbound =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {phantom}, EdgeDuration{10}, 1);
    const auto inbound =
        boundedDurationOneToAllSearch<REVERSE_DIRECTION>(facade, {phantom}, EdgeDuration{10}, 1);

    for (const auto *result : {&outbound, &inbound})
    {
        BOOST_CHECK(result->isComplete());
        BOOST_CHECK(result->phantom_partials.empty());
    }
}

BOOST_AUTO_TEST_CASE(synthetic_seed_reentry_obeys_the_travel_direction)
{
    auto forward_facade = makeFacade(1, {{0, 0, 130}}, {}, {false});
    const auto source = makePhantom(0, 0, {80}, {0}, {0}, {0}, true, false, false, false);

    const auto forward_result = boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
        forward_facade, {source}, EdgeDuration{60}, 100);
    BOOST_REQUIRE_EQUAL(forward_result.nodes.size(), 1);
    checkNode(forward_result.nodes.front(), 0, 50);
    BOOST_CHECK(forward_result.nodes.front().provenance == isochrone::NodeProvenance::Network);
    BOOST_CHECK_EQUAL(from_alias<int>(forward_result.phantom_partials.front().seed_duration), -80);

    auto reverse_facade = makeFacade(1, {{0, 0, 130}}, {}, {false});
    const auto target = makePhantom(0, 0, {0}, {80}, {0}, {0}, false, true, false, false);
    const auto reverse_result = boundedDurationOneToAllSearch<REVERSE_DIRECTION>(
        reverse_facade, {target}, EdgeDuration{60}, 100);
    BOOST_CHECK(reverse_result.nodes.empty());
    BOOST_CHECK(reverse_result.inbound_frontiers.empty());
}

BOOST_AUTO_TEST_CASE(equal_cost_synthetic_seed_reentry_is_retained_for_geometry)
{
    auto facade = makeFacade(1, {{0, 0, 0}}, {}, {false});
    const auto source = makePhantom(0, 0, {0}, {0}, {0}, {0}, true, false, false, false);

    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{0}, 100);

    BOOST_REQUIRE_EQUAL(result.nodes.size(), 1);
    checkNode(result.nodes.front(), 0, 0);
    BOOST_CHECK(result.nodes.front().provenance == isochrone::NodeProvenance::Network);
}

BOOST_AUTO_TEST_CASE(equal_cost_reentry_from_another_synthetic_seed_is_retained)
{
    auto facade = makeFacade(2, {{0, 1, 0}}, {}, {false, false});
    const auto first_source = makePhantom(0, 1, {0}, {0}, {0}, {0}, true, false, false, false);
    const auto second_source = makePhantom(1, 0, {0}, {0}, {0}, {0}, true, false, false, false);

    const auto result = boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
        facade, {first_source, second_source}, EdgeDuration{0}, 100);

    BOOST_REQUIRE_EQUAL(result.nodes.size(), 1);
    checkNode(result.nodes.front(), 1, 0);
    BOOST_CHECK(result.nodes.front().provenance == isochrone::NodeProvenance::Network);
}

BOOST_AUTO_TEST_CASE(inbound_frontier_crossings_are_deduplicated_by_entry_node)
{
    auto facade = makeFacade(3, {}, {{1, 0, 100}, {1, 0, 100}, {2, 1, 0}}, {false, false, false});
    const auto target = makePhantom(2, 0, {0}, {0}, {0}, {0}, false, true, false, false);

    const auto result =
        boundedDurationOneToAllSearch<REVERSE_DIRECTION>(facade, {target}, EdgeDuration{10}, 6);

    BOOST_CHECK(result.isComplete());
    BOOST_REQUIRE_EQUAL(result.inbound_frontiers.size(), 1);
    checkNode(result.inbound_frontiers.front().entry, 0, 100);
}

BOOST_AUTO_TEST_CASE(synthetic_seed_expansions_share_the_search_budget)
{
    auto facade = makeFacade(2, {{0, 0, 0}, {0, 1, 0}}, {}, {false, false});
    const auto source = makePhantom(0, 1, {0}, {0}, {0}, {0}, true, false, false, false);

    // Candidate target storage consumes two records after the prepass heap is released. The
    // phantom partial and first weighted discovery consume the remaining two; discovery of node 1
    // must stop the search.
    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{0}, 4);

    BOOST_CHECK(result.status == isochrone::SearchStatus::SearchRecordLimitReached);
    BOOST_CHECK(result.nodes.empty());
}

BOOST_AUTO_TEST_CASE(reentry_records_and_settlements_share_the_node_budget)
{
    auto reentry_facade = makeFacade(1, {{0, 0, 130}}, {}, {false});
    const auto source = makePhantom(0, 0, {80}, {0}, {0}, {0}, true, false, false, false);

    const auto reentry_result = boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
        reentry_facade, {source}, EdgeDuration{60}, 1);
    BOOST_CHECK(reentry_result.status == isochrone::SearchStatus::SearchRecordLimitReached);
    BOOST_CHECK(reentry_result.nodes.empty());

    auto settlement_facade = makeFacade(2, {{0, 1, 1}}, {}, {false, false});
    const auto simple_source = makePhantom(0, 1, {0}, {0}, {0}, {0}, true, false, false, false);
    const auto settlement_result = boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
        settlement_facade, {simple_source}, EdgeDuration{10}, 1);
    BOOST_CHECK(settlement_result.status == isochrone::SearchStatus::SearchRecordLimitReached);
    BOOST_CHECK(settlement_result.nodes.empty());
}

BOOST_AUTO_TEST_CASE(phantom_partials_share_the_search_record_budget)
{
    auto facade = makeFacade(3, {}, {}, {false, false, false});
    auto first = makePhantom(0, 0, {0}, {0}, {0}, {0}, true, false, false, false);
    auto second = makePhantom(1, 1, {0}, {0}, {0}, {0}, true, false, false, false);
    auto third = makePhantom(2, 2, {0}, {0}, {0}, {0}, true, false, false, false);
    first.approach_duration = {11};
    second.approach_duration = {11};
    third.approach_duration = {11};

    const auto result = boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
        facade, {first, second, third}, EdgeDuration{10}, 2);

    BOOST_CHECK(result.status == isochrone::SearchStatus::SearchRecordLimitReached);
    BOOST_CHECK_EQUAL(result.phantom_partials.size(), 2);
}

BOOST_AUTO_TEST_CASE(high_degree_zero_cost_expansion_respects_the_search_node_budget)
{
    auto facade =
        makeFacade(4, {{0, 1, 0}, {0, 2, 0}, {0, 3, 0}}, {}, {false, false, false, false});
    const auto source = makePhantom(0, 1, {0}, {0}, {0}, {0}, true, false, false, false);

    // Three candidate targets survive the prepass. The phantom partial and first two weighted
    // labels consume the remaining records; the next discovery must stop the query before it can
    // grow an unbounded heap.
    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{0}, 6);

    BOOST_CHECK(result.status == isochrone::SearchStatus::SearchRecordLimitReached);
    BOOST_CHECK(result.nodes.empty());
}

BOOST_AUTO_TEST_CASE(invalid_seed_metric_returns_an_incomplete_result)
{
    auto facade = makeFacade(1, {}, {}, {false});
    auto source = makePhantom(0, 0, {0}, {0}, {0}, {0}, true, false, false, false);
    source.approach_duration = INVALID_EDGE_DURATION;

    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{10}, 100);

    BOOST_CHECK(result.status == isochrone::SearchStatus::ArithmeticOverflow);
}

BOOST_AUTO_TEST_CASE(inbound_duration_prepass_overflow_is_not_silently_discarded)
{
    const auto maximum = std::numeric_limits<std::int32_t>::max();
    // The target seed is in range. The preceding reverse arc is almost INT32_MAX, so its true
    // label overflows even though inbound normalization can subtract that predecessor geometry
    // and expose a 5-decisecond suffix. The duration-only prepass must not silently lose it.
    auto facade = makeWeightedFacade(2, {}, {{0, 1, 1, maximum - 1}}, {false, false});
    const auto target = makePhantom(0, 1, {5}, {0}, {0}, {0}, false, true, false, false);

    const auto result =
        boundedDurationOneToAllSearch<REVERSE_DIRECTION>(facade, {target}, EdgeDuration{10}, 100);

    BOOST_CHECK(result.status == isochrone::SearchStatus::ArithmeticOverflow);
}

BOOST_AUTO_TEST_CASE(weighted_candidate_label_duration_overflow_is_reported)
{
    const auto maximum = std::numeric_limits<std::int32_t>::max();
    // The duration prepass reaches node 1 directly, but the lower-weight label selected by the
    // real search reaches it through node 2 and overflows.  That selected label must fail the
    // query rather than reaching materialization as INVALID_EDGE_DURATION.
    auto facade = makeWeightedFacade(
        3, {{0, 1, 100, 1}, {0, 2, 1, maximum - 5}, {2, 1, 1, 10}}, {}, {false, false, false});
    const auto source = makePhantom(0, 1, {0}, {0}, {0}, {0}, true, false, false, false);

    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{10}, 100);

    BOOST_CHECK(result.status == isochrone::SearchStatus::ArithmeticOverflow);
}

BOOST_AUTO_TEST_CASE(overflowed_duration_on_an_irrelevant_branch_does_not_fail_the_query)
{
    const auto maximum = std::numeric_limits<std::int32_t>::max();
    auto facade = makeWeightedFacade(4,
                                     {{0, 1, 1, maximum - 1}, {1, 3, 1, 10}, {0, 2, 100, 1}},
                                     {},
                                     {false, false, false, false});
    const auto source = makePhantom(0, 3, {0}, {0}, {0}, {0}, true, false, false, false);

    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{10}, 100);

    BOOST_CHECK(result.isComplete());
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 1);
    checkNode(result.nodes.front(), 2, 1);
}

BOOST_AUTO_TEST_CASE(network_label_from_one_synthetic_seed_can_reach_another_seed_node)
{
    auto facade = makeFacade(2, {{1, 0, 1}}, {}, {false, false});
    auto first_source = makePhantom(0, 1, {0}, {0}, {0}, {0}, true, false, false, false);
    first_source.approach_weight = {5};
    first_source.approach_duration = {5};
    const auto second_source = makePhantom(1, 0, {0}, {0}, {0}, {0}, true, false, false, false);

    const auto result = boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
        facade, {first_source, second_source}, EdgeDuration{10}, 100);

    BOOST_REQUIRE_EQUAL(result.nodes.size(), 1);
    checkNode(result.nodes.front(), 0, 1);
    BOOST_CHECK(result.nodes.front().provenance == isochrone::NodeProvenance::Network);
    BOOST_CHECK_EQUAL(result.phantom_partials.size(), 2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace osrm::engine::routing_algorithms
