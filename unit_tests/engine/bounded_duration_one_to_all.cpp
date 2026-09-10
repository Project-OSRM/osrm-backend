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
                    arcs.push_back({to, EdgeDuration{duration}});
            }
            ranges[source] = {begin, EdgeID{static_cast<unsigned>(arcs.size())}};
        }
    };

    LevelZeroFacade facade{number_of_nodes, {}, {}, {}, {}, std::move(excluded)};
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
                    arcs.push_back({to, EdgeDuration{duration}});
            }
            ranges[source] = {begin, EdgeID{static_cast<unsigned>(arcs.size())}};
        }
    };

    LevelZeroFacade facade{number_of_nodes, {}, {}, {}, {}, std::move(excluded)};
    makeGraph(forward, facade.forward_arcs, facade.forward_ranges);
    makeGraph(reverse, facade.reverse_arcs, facade.reverse_ranges);
    return facade;
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

BOOST_AUTO_TEST_CASE(forward_search_uses_the_independent_directed_duration_graph)
{
    auto facade = makeFacade(6,
                             {{0, 1, 0}, {0, 2, 0}, {0, 5, 0}, {1, 3, 40}, {2, 3, 15}},
                             {},
                             {false, false, false, false, false, true});
    const auto source = makePhantom(0, 5, {0}, {0}, {0}, {0}, true, false, false, false);

    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{30}, 100);

    // The duration sidecar is charged from the forward source edge-based
    // node. Both first-hop nodes therefore inherit node 0's zero duration;
    // the second path uses node 2's duration plus its turn penalty.
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 3);
    checkNode(result.nodes[0], 1, 0);
    checkNode(result.nodes[1], 2, 0);
    checkNode(result.nodes[2], 3, 15);
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
        boundedDurationOneToAllSearch<REVERSE_DIRECTION>(facade, {target}, EdgeDuration{10}, 4);

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
    BOOST_CHECK_EQUAL(from_alias<int>(partial.seed_duration), 0);
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

BOOST_AUTO_TEST_CASE(virtual_seed_reentry_obeys_the_travel_direction)
{
    auto forward_facade = makeFacade(1, {{0, 0, 130}}, {}, {false});
    const auto source = makePhantom(0, 0, {80}, {0}, {0}, {0}, true, false, false, false);

    const auto forward_result = boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
        forward_facade, {source}, EdgeDuration{60}, 100);
    BOOST_REQUIRE_EQUAL(forward_result.nodes.size(), 1);
    checkNode(forward_result.nodes.front(), 0, 50);
    BOOST_CHECK(forward_result.nodes.front().provenance == isochrone::NodeProvenance::SeedReentry);
    BOOST_CHECK_EQUAL(from_alias<int>(forward_result.phantom_partials.front().seed_duration), -80);

    auto reverse_facade = makeFacade(1, {{0, 0, 130}}, {}, {false});
    const auto target = makePhantom(0, 0, {0}, {80}, {0}, {0}, false, true, false, false);
    const auto reverse_result = boundedDurationOneToAllSearch<REVERSE_DIRECTION>(
        reverse_facade, {target}, EdgeDuration{60}, 100);
    BOOST_CHECK(reverse_result.nodes.empty());
    BOOST_CHECK(reverse_result.inbound_frontiers.empty());
}

BOOST_AUTO_TEST_CASE(equal_cost_virtual_seed_reentry_is_retained_for_geometry)
{
    auto facade = makeFacade(1, {{0, 0, 0}}, {}, {false});
    const auto source = makePhantom(0, 0, {0}, {0}, {0}, {0}, true, false, false, false);

    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{0}, 100);

    BOOST_REQUIRE_EQUAL(result.nodes.size(), 1);
    checkNode(result.nodes.front(), 0, 0);
    BOOST_CHECK(result.nodes.front().provenance == isochrone::NodeProvenance::SeedReentry);
}

BOOST_AUTO_TEST_CASE(equal_cost_reentry_preserves_an_unsettled_virtual_seed)
{
    auto facade = makeFacade(2, {{0, 1, 0}}, {}, {false, false});
    const auto first_source = makePhantom(0, 1, {0}, {0}, {0}, {0}, true, false, false, false);
    const auto second_source = makePhantom(1, 0, {0}, {0}, {0}, {0}, true, false, false, false);

    const auto result = boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
        facade, {first_source, second_source}, EdgeDuration{0}, 100);

    BOOST_REQUIRE_EQUAL(result.nodes.size(), 1);
    checkNode(result.nodes.front(), 1, 0);
    BOOST_CHECK(result.nodes.front().provenance == isochrone::NodeProvenance::SeedReentry);
}

BOOST_AUTO_TEST_CASE(inbound_frontier_crossings_are_deduplicated_by_entry_node)
{
    auto facade = makeFacade(3, {}, {{1, 0, 100}, {1, 0, 100}, {2, 1, 0}}, {false, false, false});
    const auto target = makePhantom(2, 0, {0}, {0}, {0}, {0}, false, true, false, false);

    const auto result =
        boundedDurationOneToAllSearch<REVERSE_DIRECTION>(facade, {target}, EdgeDuration{10}, 4);

    BOOST_CHECK(result.isComplete());
    BOOST_REQUIRE_EQUAL(result.inbound_frontiers.size(), 1);
    checkNode(result.inbound_frontiers.front().entry, 0, 100);
}

BOOST_AUTO_TEST_CASE(supplemental_reentry_records_share_the_search_budget)
{
    auto facade = makeFacade(2, {{0, 0, 0}, {0, 1, 0}}, {}, {false, false});
    const auto source = makePhantom(0, 1, {0}, {0}, {0}, {0}, true, false, false, false);

    // The phantom partial, virtual source, and virtual-seed reentry consume
    // all three records. Discovery of node 1 must stop the search.
    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{0}, 3);

    BOOST_CHECK(result.status == isochrone::SearchStatus::SearchRecordLimitReached);
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 1);
    checkNode(result.nodes.front(), 0, 0);
    BOOST_CHECK(result.nodes.front().provenance == isochrone::NodeProvenance::SeedReentry);
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

    // The phantom partial and virtual source consume two records. The first network discovery
    // consumes the third;
    // the next discovery must stop the query before it can grow an unbounded heap.
    const auto result =
        boundedDurationOneToAllSearch<FORWARD_DIRECTION>(facade, {source}, EdgeDuration{0}, 3);

    BOOST_CHECK(result.status == isochrone::SearchStatus::SearchRecordLimitReached);
    BOOST_CHECK(result.nodes.empty());
}

BOOST_AUTO_TEST_CASE(arithmetic_overflow_returns_an_incomplete_result)
{
    const auto maximum = std::numeric_limits<std::int32_t>::max();
    auto facade = makeFacade(2, {{0, 1, maximum}}, {}, {false, false});
    const auto source = makePhantom(0, 1, {0}, {0}, {0}, {0}, true, false, false, false);

    const auto result = boundedDurationOneToAllSearch<FORWARD_DIRECTION>(
        facade, {source}, EdgeDuration{maximum - 2}, 100);

    BOOST_CHECK(result.status == isochrone::SearchStatus::ArithmeticOverflow);
}

BOOST_AUTO_TEST_CASE(network_label_replaces_a_virtual_seed_before_settlement)
{
    auto facade = makeFacade(2, {{1, 0, 1}}, {}, {false, false});
    auto first_source = makePhantom(0, 1, {0}, {0}, {0}, {0}, true, false, false, false);
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
