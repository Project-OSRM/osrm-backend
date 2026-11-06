#include "engine/routing_algorithms/reachability.hpp"

#include "customizer/edge_based_graph.hpp"

#include "util/integer_range.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <utility>

namespace osrm::engine::routing_algorithms::mld
{
namespace
{
class ReachabilityFacade final
{
  public:
    explicit ReachabilityFacade(const bool include_source_self_loop = false,
                                const bool reverse_search_graph = false,
                                const bool overflow_duration = false)
        : m_include_source_self_loop(include_source_self_loop),
          m_reverse_search_graph(reverse_search_graph), m_overflow_duration(overflow_duration)
    {
    }

    unsigned GetNumberOfNodes() const { return 7; }

    auto GetBorderEdgeRange(LevelID level, NodeID node) const
    {
        BOOST_ASSERT(level == 0);
        constexpr std::array<std::pair<EdgeID, EdgeID>, 7> edge_ranges = {
            {{0, 4}, {4, 6}, {6, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}}};
        const auto [first, last] = edge_ranges[node];
        return util::irange<EdgeID>(first, last);
    }

    bool IsForwardEdge(EdgeID edge) const
    { return !m_reverse_search_graph && (edge != 3 || m_include_source_self_loop); }
    bool IsBackwardEdge(EdgeID edge) const
    { return m_reverse_search_graph && (edge != 3 || m_include_source_self_loop); }
    bool ExcludeNode(NodeID node) const { return node == 4; }

    NodeID GetTarget(EdgeID edge) const
    {
        constexpr std::array<NodeID, 7> targets = {{1, 2, 5, 0, 3, 4, 3}};
        return targets[edge];
    }

    const customizer::EdgeBasedGraphEdgeData &GetEdgeData(EdgeID edge) const
    {
        static const std::array<customizer::EdgeBasedGraphEdgeData, 7> edge_data = {
            {{0}, {1}, {2}, {3}, {4}, {5}, {6}}};
        return edge_data[edge];
    }

    EdgeWeight GetNodeWeight(NodeID node) const
    {
        constexpr std::array<EdgeWeight, 7> weights = {{{201}, {1}, {1}, {1}, {1}, {100}, {1}}};
        return weights[node];
    }

    EdgeDuration GetNodeDuration(NodeID node) const
    {
        if (m_overflow_duration)
            return EdgeDuration{std::numeric_limits<EdgeDuration::value_type>::max() - 100};
        constexpr std::array<EdgeDuration, 7> durations = {{{13}, {1}, {1}, {1}, {1}, {1}, {1}}};
        return durations[node];
    }

    TurnPenalty GetWeightPenaltyForEdgeID(NodeID turn_id) const
    {
        if (m_reverse_search_graph)
            return TurnPenalty{0};
        constexpr std::array<TurnPenalty, 7> penalties = {
            {{0}, {-199}, {-100}, {0}, {0}, {0}, {0}}};
        return penalties[turn_id];
    }

    TurnPenalty GetDurationPenaltyForEdgeID(NodeID turn_id) const
    {
        constexpr std::array<TurnPenalty, 7> penalties = {{{1}, {98}, {0}, {0}, {0}, {0}, {0}}};
        return penalties[turn_id];
    }

  private:
    bool m_include_source_self_loop;
    bool m_reverse_search_graph;
    bool m_overflow_duration;
};

class ArithmeticReachabilityFacade final
{
  public:
    std::array<EdgeWeight, 4> NodeWeights = {{{1}, {1}, {1}, {1}}};
    std::array<EdgeDuration, 4> NodeDurations = {{{1}, {1}, {1}, {1}}};
    std::array<TurnPenalty, 4> WeightPenalties = {{{0}, {0}, {0}, {0}}};
    std::array<TurnPenalty, 4> DurationPenalties = {{{0}, {0}, {0}, {0}}};

    unsigned GetNumberOfNodes() const { return 4; }

    auto GetBorderEdgeRange(LevelID level, NodeID node) const
    {
        BOOST_ASSERT(level == 0);
        constexpr std::array<std::pair<EdgeID, EdgeID>, 4> ranges = {
            {{0, 2}, {2, 3}, {3, 4}, {4, 4}}};
        return util::irange<EdgeID>(ranges[node].first, ranges[node].second);
    }

    bool IsForwardEdge(EdgeID) const { return true; }
    bool IsBackwardEdge(EdgeID) const { return false; }
    bool ExcludeNode(NodeID) const { return false; }

    NodeID GetTarget(EdgeID edge) const
    {
        constexpr std::array<NodeID, 4> targets = {{1, 3, 2, 3}};
        return targets[edge];
    }

    const customizer::EdgeBasedGraphEdgeData &GetEdgeData(EdgeID edge) const
    {
        static const std::array<customizer::EdgeBasedGraphEdgeData, 4> edges = {
            {{0}, {1}, {2}, {3}}};
        return edges[edge];
    }

    EdgeWeight GetNodeWeight(NodeID node) const { return NodeWeights[node]; }
    EdgeDuration GetNodeDuration(NodeID node) const { return NodeDurations[node]; }
    TurnPenalty GetWeightPenaltyForEdgeID(NodeID turn_id) const { return WeightPenalties[turn_id]; }
    TurnPenalty GetDurationPenaltyForEdgeID(NodeID turn_id) const
    { return DurationPenalties[turn_id]; }
};

class SparseReachabilityFacade final
{
  public:
    unsigned NumberOfNodes = 10'000'001;
    bool HasEdges = true;
    bool Reverse = false;
    NodeID ExcludedNode = SPECIAL_NODEID;

    unsigned GetNumberOfNodes() const { return NumberOfNodes; }

    auto GetBorderEdgeRange(LevelID level, NodeID node) const
    {
        BOOST_ASSERT(level == 0);
        if (HasEdges && node < 2)
            return util::irange<EdgeID>(node, node + 1);
        return util::irange<EdgeID>(0, 0);
    }

    bool IsForwardEdge(EdgeID) const { return !Reverse; }
    bool IsBackwardEdge(EdgeID) const { return Reverse; }
    bool ExcludeNode(NodeID node) const { return node == ExcludedNode; }
    NodeID GetTarget(EdgeID edge) const { return edge + 1; }

    const customizer::EdgeBasedGraphEdgeData &GetEdgeData(EdgeID edge) const
    {
        static const std::array<customizer::EdgeBasedGraphEdgeData, 2> edges = {{{0}, {1}}};
        return edges[edge];
    }

    EdgeWeight GetNodeWeight(NodeID) const { return EdgeWeight{1}; }
    EdgeDuration GetNodeDuration(NodeID) const { return EdgeDuration{10}; }
    TurnPenalty GetWeightPenaltyForEdgeID(NodeID) const { return TurnPenalty{0}; }
    TurnPenalty GetDurationPenaltyForEdgeID(NodeID) const { return TurnPenalty{0}; }
};

PhantomNode makeEndpoint(const bool source, const bool forward = true, const bool reverse = false)
{
    struct Seed
    {
        SegmentID forward_segment_id;
        SegmentID reverse_segment_id;
        unsigned short fwd_segment_position = 0;
    };

    return PhantomNode{Seed{{0, forward}, {0, reverse}},
                       {1, false},
                       {0},
                       {0},
                       {0},
                       {0},
                       {0},
                       {0},
                       {0},
                       {0},
                       {0},
                       {0},
                       {0},
                       {0},
                       source,
                       !source,
                       source,
                       !source,
                       {},
                       {},
                       0};
}

PhantomNode makeSource()
{
    auto source = makeEndpoint(true);
    source.forward_weight = EdgeWeight{5};
    source.forward_weight_offset = EdgeWeight{95};
    source.forward_duration = EdgeDuration{4};
    source.forward_duration_offset = EdgeDuration{6};
    return source;
}

PhantomNode makeZeroOffsetSource() { return makeEndpoint(true); }

PhantomNode makeTarget() { return makeEndpoint(false); }

PhantomNode reverseEndpoint(const PhantomNode &endpoint)
{
    auto reverse = makeEndpoint(endpoint.IsValidForwardSource(), false, true);
    reverse.reverse_segment_id = endpoint.forward_segment_id;
    reverse.reverse_weight = endpoint.forward_weight;
    reverse.reverse_weight_offset = endpoint.forward_weight_offset;
    reverse.reverse_duration = endpoint.forward_duration;
    reverse.reverse_duration_offset = endpoint.forward_duration_offset;
    reverse.approach_weight = endpoint.approach_weight;
    reverse.approach_duration = endpoint.approach_duration;
    return reverse;
}
} // namespace

BOOST_AUTO_TEST_SUITE(reachability)

BOOST_AUTO_TEST_CASE(over_duration_lower_weight_labels_suppress_in_cutoff_competitors)
{
    SearchEngineData<Algorithm> heaps;
    const ReachabilityFacade facade;

    const auto result = reachabilitySearch(heaps, facade, {makeSource()}, EdgeDuration{5});
    const auto &reachable_nodes = result.nodes;

    BOOST_REQUIRE(result.isComplete());
    BOOST_REQUIRE_EQUAL(reachable_nodes.size(), 2);

    BOOST_CHECK_EQUAL(reachable_nodes[0].node, 5);
    BOOST_CHECK_EQUAL(reachable_nodes[0].weight, EdgeWeight{1});
    BOOST_CHECK_EQUAL(reachable_nodes[0].duration, EdgeDuration{3});

    BOOST_CHECK_EQUAL(reachable_nodes[1].node, 1);
    BOOST_CHECK_EQUAL(reachable_nodes[1].weight, EdgeWeight{101});
    BOOST_CHECK_EQUAL(reachable_nodes[1].duration, EdgeDuration{4});

    // The direct route through node 1 reaches node 3 in five deciseconds with weight 102.  The
    // route through node 2 has lower weight (-97) but duration 102 and must suppress it.
    BOOST_CHECK(std::none_of(reachable_nodes.begin(),
                             reachable_nodes.end(),
                             [](const auto &result) { return result.node == 3; }));
}

BOOST_AUTO_TEST_CASE(charges_the_source_node_duration_and_turn_penalty)
{
    SearchEngineData<Algorithm> heaps;
    const ReachabilityFacade facade;

    const auto result = reachabilitySearch(heaps, facade, {makeSource()}, EdgeDuration{4});
    const auto &reachable_nodes = result.nodes;

    BOOST_REQUIRE(result.isComplete());
    const auto node_one = std::find_if(reachable_nodes.begin(),
                                       reachable_nodes.end(),
                                       [](const auto &result) { return result.node == 1; });
    BOOST_REQUIRE(node_one != reachable_nodes.end());
    // The source seed is -10.  Edge 0 charges node 0's duration (13) and its turn penalty (1).
    // Using the target node's duration would instead yield -8 and fail to model the segment.
    BOOST_CHECK_EQUAL(node_one->duration, EdgeDuration{4});
}

BOOST_AUTO_TEST_CASE(uses_every_source_candidate)
{
    SearchEngineData<Algorithm> heaps;
    const ReachabilityFacade facade;
    auto additional_source = makeSource();
    additional_source.forward_segment_id = {1, true};
    additional_source.forward_weight = {0};
    additional_source.forward_weight_offset = {200};
    additional_source.forward_duration = {0};
    additional_source.forward_duration_offset = {0};

    const auto result =
        reachabilitySearch(heaps, facade, {makeSource(), additional_source}, EdgeDuration{1});
    const auto &reachable_nodes = result.nodes;

    BOOST_REQUIRE(result.isComplete());
    const auto node_three = std::find_if(reachable_nodes.begin(),
                                         reachable_nodes.end(),
                                         [](const auto &result) { return result.node == 3; });
    BOOST_REQUIRE(node_three != reachable_nodes.end());
    BOOST_CHECK_EQUAL(node_three->weight, EdgeWeight{-199});
    BOOST_CHECK_EQUAL(node_three->duration, EdgeDuration{1});
}

BOOST_AUTO_TEST_CASE(allows_a_real_path_to_reenter_its_source_node)
{
    SearchEngineData<Algorithm> heaps;
    const ReachabilityFacade facade{true};

    const auto result = reachabilitySearch(heaps, facade, {makeSource()}, EdgeDuration{3});
    const auto &reachable_nodes = result.nodes;

    BOOST_REQUIRE(result.isComplete());
    const auto source_node = std::find_if(reachable_nodes.begin(),
                                          reachable_nodes.end(),
                                          [](const auto &result) { return result.node == 0; });
    BOOST_REQUIRE(source_node != reachable_nodes.end());
    BOOST_CHECK_EQUAL(source_node->weight, EdgeWeight{101});
    BOOST_CHECK_EQUAL(source_node->duration, EdgeDuration{3});
}

BOOST_AUTO_TEST_CASE(inbound_search_uses_target_seeds_and_reverse_edges)
{
    SearchEngineData<Algorithm> heaps;
    const ReachabilityFacade facade{false, true};

    const auto result =
        reachabilitySearch<REVERSE_DIRECTION>(heaps, facade, {makeTarget()}, EdgeDuration{2});

    BOOST_REQUIRE(result.isComplete());
    const auto node_one = std::find_if(
        result.nodes.begin(), result.nodes.end(), [](const auto &node) { return node.node == 1; });
    BOOST_REQUIRE(node_one != result.nodes.end());
    BOOST_CHECK_EQUAL(node_one->weight, EdgeWeight{1});
    BOOST_CHECK_EQUAL(node_one->duration, EdgeDuration{2});
}

BOOST_AUTO_TEST_CASE(reports_duration_arithmetic_overflow)
{
    SearchEngineData<Algorithm> heaps;
    const ReachabilityFacade facade{false, false, true};

    const auto result = reachabilitySearch(heaps, facade, {makeSource()}, EdgeDuration{100});

    BOOST_CHECK(result.status == ReachabilitySearchStatus::ArithmeticOverflow);
}

BOOST_AUTO_TEST_CASE(ignores_dominated_weight_overflow)
{
    SearchEngineData<Algorithm> heaps;
    ArithmeticReachabilityFacade facade;
    facade.NodeWeights = {{{1}, {1'200'000'000}, {1'200'000'000}, {1}}};

    const auto result =
        reachabilitySearch(heaps, facade, {makeZeroOffsetSource()}, EdgeDuration{3});

    BOOST_REQUIRE(result.isComplete());
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 3);
    const auto target = std::find_if(
        result.nodes.begin(), result.nodes.end(), [](const auto &node) { return node.node == 3; });
    BOOST_REQUIRE(target != result.nodes.end());
    BOOST_CHECK_EQUAL(target->weight, EdgeWeight{1});
    BOOST_CHECK_EQUAL(target->duration, EdgeDuration{1});
}

BOOST_AUTO_TEST_CASE(ignores_dominated_duration_overflow)
{
    SearchEngineData<Algorithm> heaps;
    ArithmeticReachabilityFacade facade;
    facade.NodeDurations = {{{1}, {1'200'000'000}, {1'200'000'000}, {1}}};

    const auto result =
        reachabilitySearch(heaps, facade, {makeZeroOffsetSource()}, EdgeDuration{3});

    BOOST_REQUIRE(result.isComplete());
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 2);
    BOOST_REQUIRE_EQUAL(result.competitors.size(), 1);
    const auto target = std::find_if(
        result.nodes.begin(), result.nodes.end(), [](const auto &node) { return node.node == 3; });
    BOOST_REQUIRE(target != result.nodes.end());
    BOOST_CHECK_EQUAL(target->weight, EdgeWeight{1});
    BOOST_CHECK_EQUAL(target->duration, EdgeDuration{1});
}

BOOST_AUTO_TEST_CASE(negative_turn_penalties_keep_final_metric_sums_representable)
{
    SearchEngineData<Algorithm> heaps;
    ArithmeticReachabilityFacade facade;
    constexpr auto maximum_metric = std::numeric_limits<EdgeWeight::value_type>::max();
    facade.NodeWeights[0] = EdgeWeight{maximum_metric - 50};
    facade.NodeWeights[1] = EdgeWeight{100};
    facade.NodeDurations[0] = EdgeDuration{maximum_metric - 50};
    facade.NodeDurations[1] = EdgeDuration{100};
    facade.WeightPenalties[2] = TurnPenalty{-75};
    facade.DurationPenalties[2] = TurnPenalty{-75};

    const auto result = reachabilitySearch(
        heaps, facade, {makeZeroOffsetSource()}, EdgeDuration{maximum_metric - 1});

    BOOST_REQUIRE(result.isComplete());
    const auto target = std::find_if(
        result.nodes.begin(), result.nodes.end(), [](const auto &node) { return node.node == 2; });
    BOOST_REQUIRE(target != result.nodes.end());
    BOOST_CHECK_EQUAL(target->weight, EdgeWeight{maximum_metric - 25});
    BOOST_CHECK_EQUAL(target->duration, EdgeDuration{maximum_metric - 25});
}

BOOST_AUTO_TEST_CASE(reports_unrepresentable_weight_labels)
{
    SearchEngineData<Algorithm> heaps;
    ArithmeticReachabilityFacade facade;
    facade.NodeWeights[0] = EdgeWeight{std::numeric_limits<EdgeWeight::value_type>::max() - 50};
    facade.NodeWeights[1] = EdgeWeight{100};

    const auto result =
        reachabilitySearch(heaps, facade, {makeZeroOffsetSource()}, EdgeDuration{3});

    BOOST_CHECK(result.status == ReachabilitySearchStatus::ArithmeticOverflow);
}

BOOST_AUTO_TEST_CASE(reports_unrepresentable_improving_duration_labels)
{
    SearchEngineData<Algorithm> heaps;
    ArithmeticReachabilityFacade facade;
    facade.WeightPenalties[1] = TurnPenalty{10};

    // Keep the direct target label above the candidate's weight so an unrepresentable
    // duration remains an error when that candidate would otherwise improve the label.
    facade.NodeDurations = {{{1}, {1'200'000'000}, {1'200'000'000}, {1}}};

    const auto result =
        reachabilitySearch(heaps, facade, {makeZeroOffsetSource()}, EdgeDuration{3});

    BOOST_CHECK(result.status == ReachabilitySearchStatus::ArithmeticOverflow);
}

BOOST_AUTO_TEST_CASE(bounds_discoveries_on_a_large_loaded_map_and_resets_between_queries)
{
    SearchEngineData<Algorithm> heaps;
    const SparseReachabilityFacade facade;
    const auto source = makeZeroOffsetSource();

    const auto limited =
        reachabilitySearch<FORWARD_DIRECTION>(heaps, facade, {source}, EdgeDuration{20}, 1);
    BOOST_CHECK(limited.status == ReachabilitySearchStatus::SearchNodeLimitReached);
    BOOST_REQUIRE(heaps.reachability_heap != nullptr);
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 1);

    const auto result =
        reachabilitySearch<FORWARD_DIRECTION>(heaps, facade, {source}, EdgeDuration{20}, 2);
    BOOST_REQUIRE(result.isComplete());
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 2);
    BOOST_CHECK(result.competitors.empty());
    BOOST_CHECK_EQUAL(result.nodes[0].node, 1);
    BOOST_CHECK_EQUAL(result.nodes[0].weight, EdgeWeight{1});
    BOOST_CHECK_EQUAL(result.nodes[0].duration, EdgeDuration{10});
    BOOST_CHECK_EQUAL(result.nodes[1].node, 2);
    BOOST_CHECK_EQUAL(result.nodes[1].weight, EdgeWeight{2});
    BOOST_CHECK_EQUAL(result.nodes[1].duration, EdgeDuration{20});
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 2);

    const auto zero =
        reachabilitySearch<FORWARD_DIRECTION>(heaps, facade, {source}, EdgeDuration{20}, 0);
    BOOST_CHECK(zero.status == ReachabilitySearchStatus::SearchNodeLimitReached);
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 0);
}

BOOST_AUTO_TEST_CASE(inbound_counts_settled_nodes_towards_the_search_budget)
{
    SearchEngineData<Algorithm> heaps;
    SparseReachabilityFacade facade;
    facade.Reverse = true;

    const auto limited =
        reachabilitySearch<REVERSE_DIRECTION>(heaps, facade, {makeTarget()}, EdgeDuration{20}, 1);
    BOOST_CHECK(limited.status == ReachabilitySearchStatus::SearchNodeLimitReached);
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 1);

    const auto complete =
        reachabilitySearch<REVERSE_DIRECTION>(heaps, facade, {makeTarget()}, EdgeDuration{20}, 2);
    BOOST_REQUIRE(complete.isComplete());
    BOOST_CHECK_EQUAL(complete.nodes.size(), 2);
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 2);
}

BOOST_AUTO_TEST_CASE(counts_initial_source_fanout_before_heap_insertion)
{
    SearchEngineData<Algorithm> heaps;
    const ReachabilityFacade facade;

    const auto result =
        reachabilitySearch<FORWARD_DIRECTION>(heaps, facade, {makeSource()}, EdgeDuration{100}, 2);

    BOOST_CHECK(result.status == ReachabilitySearchStatus::SearchNodeLimitReached);
    BOOST_CHECK(result.nodes.empty());
    BOOST_CHECK(result.competitors.empty());
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 2);
}

BOOST_AUTO_TEST_CASE(counts_over_duration_competitors)
{
    SearchEngineData<Algorithm> heaps;
    const SparseReachabilityFacade facade;
    const auto source = makeZeroOffsetSource();

    const auto limited =
        reachabilitySearch<FORWARD_DIRECTION>(heaps, facade, {source}, EdgeDuration{0}, 1);
    BOOST_CHECK(limited.status == ReachabilitySearchStatus::SearchNodeLimitReached);
    BOOST_CHECK(limited.nodes.empty());
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 1);

    const auto complete =
        reachabilitySearch<FORWARD_DIRECTION>(heaps, facade, {source}, EdgeDuration{0}, 2);
    BOOST_REQUIRE(complete.isComplete());
    BOOST_CHECK(complete.nodes.empty());
    BOOST_CHECK_EQUAL(complete.competitors.size(), 2);
}

BOOST_AUTO_TEST_CASE(does_not_count_duplicate_sources_or_label_improvements_twice)
{
    SearchEngineData<Algorithm> heaps;
    ArithmeticReachabilityFacade facade;
    facade.WeightPenalties[1] = TurnPenalty{10};
    const auto source = makeZeroOffsetSource();

    const auto result =
        reachabilitySearch<FORWARD_DIRECTION>(heaps, facade, {source, source}, EdgeDuration{3}, 3);

    BOOST_REQUIRE(result.isComplete());
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 3);
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 3);
    const auto target = std::find_if(
        result.nodes.begin(), result.nodes.end(), [](const auto &node) { return node.node == 3; });
    BOOST_REQUIRE(target != result.nodes.end());
    BOOST_CHECK_EQUAL(target->weight, EdgeWeight{3});
    BOOST_CHECK_EQUAL(target->duration, EdgeDuration{3});
}

BOOST_AUTO_TEST_CASE(counts_phantom_reentry_but_not_the_virtual_source)
{
    SearchEngineData<Algorithm> heaps;
    SparseReachabilityFacade isolated;
    isolated.HasEdges = false;

    const auto empty = reachabilitySearch<FORWARD_DIRECTION>(
        heaps, isolated, {makeZeroOffsetSource()}, EdgeDuration{10}, 0);
    BOOST_REQUIRE(empty.isComplete());
    BOOST_CHECK(empty.nodes.empty());
    BOOST_CHECK(empty.competitors.empty());
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 0);

    const ReachabilityFacade without_loop;
    const auto base = reachabilitySearch<FORWARD_DIRECTION>(
        heaps, without_loop, {makeSource()}, EdgeDuration{100}, 4);
    BOOST_REQUIRE(base.isComplete());
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 4);

    const ReachabilityFacade loop{true};
    const auto limited =
        reachabilitySearch<FORWARD_DIRECTION>(heaps, loop, {makeSource()}, EdgeDuration{100}, 4);
    BOOST_CHECK(limited.status == ReachabilitySearchStatus::SearchNodeLimitReached);
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 4);

    const auto complete =
        reachabilitySearch<FORWARD_DIRECTION>(heaps, loop, {makeSource()}, EdgeDuration{100}, 5);
    BOOST_REQUIRE(complete.isComplete());
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 5);
    BOOST_CHECK_EQUAL(std::count_if(complete.nodes.begin(),
                                    complete.nodes.end(),
                                    [](const auto &node) { return node.node == 0; }),
                      1);
}

BOOST_AUTO_TEST_CASE(retains_the_selected_duration_for_equal_weight_alternatives)
{
    SearchEngineData<Algorithm> heaps;
    ArithmeticReachabilityFacade facade;
    facade.WeightPenalties[1] = TurnPenalty{2};
    facade.DurationPenalties[1] = TurnPenalty{9};

    const auto result =
        reachabilitySearch(heaps, facade, {makeZeroOffsetSource()}, EdgeDuration{3});

    BOOST_REQUIRE(result.isComplete());
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 2);
    BOOST_REQUIRE_EQUAL(result.competitors.size(), 1);
    // A later path also has weight 3, but duration 3 instead of 10. Like the existing MLD
    // searches, reachability does not replace an equal-weight label to minimize duration.
    BOOST_CHECK_EQUAL(result.competitors.front().node, 3);
    BOOST_CHECK_EQUAL(result.competitors.front().weight, EdgeWeight{3});
    BOOST_CHECK_EQUAL(result.competitors.front().duration, EdgeDuration{10});
}

BOOST_AUTO_TEST_CASE(supports_zero_weight_transitions)
{
    SearchEngineData<Algorithm> heaps;
    ArithmeticReachabilityFacade facade;
    facade.NodeWeights[1] = EdgeWeight{0};
    facade.NodeWeights[2] = EdgeWeight{0};
    facade.WeightPenalties[1] = TurnPenalty{2};

    const auto result =
        reachabilitySearch(heaps, facade, {makeZeroOffsetSource()}, EdgeDuration{3});

    BOOST_REQUIRE(result.isComplete());
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 3);
    const auto target = std::find_if(
        result.nodes.begin(), result.nodes.end(), [](const auto &node) { return node.node == 3; });
    BOOST_REQUIRE(target != result.nodes.end());
    BOOST_CHECK_EQUAL(target->weight, EdgeWeight{1});
    BOOST_CHECK_EQUAL(target->duration, EdgeDuration{3});
}

BOOST_AUTO_TEST_CASE(uses_reverse_segment_source_offsets)
{
    SearchEngineData<Algorithm> heaps;
    const ReachabilityFacade facade;

    const auto result =
        reachabilitySearch(heaps, facade, {reverseEndpoint(makeSource())}, EdgeDuration{4});

    BOOST_REQUIRE(result.isComplete());
    const auto target = std::find_if(
        result.nodes.begin(), result.nodes.end(), [](const auto &node) { return node.node == 1; });
    BOOST_REQUIRE(target != result.nodes.end());
    BOOST_CHECK_EQUAL(target->weight, EdgeWeight{101});
    BOOST_CHECK_EQUAL(target->duration, EdgeDuration{4});
}

BOOST_AUTO_TEST_CASE(uses_reverse_segment_target_offsets)
{
    SearchEngineData<Algorithm> heaps;
    SparseReachabilityFacade facade;
    facade.Reverse = true;
    auto target = reverseEndpoint(makeTarget());
    target.reverse_weight = EdgeWeight{2};
    target.reverse_weight_offset = EdgeWeight{3};
    target.reverse_duration = EdgeDuration{4};
    target.reverse_duration_offset = EdgeDuration{6};

    const auto result =
        reachabilitySearch<REVERSE_DIRECTION>(heaps, facade, {target}, EdgeDuration{30});

    BOOST_REQUIRE(result.isComplete());
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 2);
    BOOST_CHECK_EQUAL(result.nodes.front().node, 1);
    BOOST_CHECK_EQUAL(result.nodes.front().weight, EdgeWeight{6});
    BOOST_CHECK_EQUAL(result.nodes.front().duration, EdgeDuration{20});
}

BOOST_AUTO_TEST_CASE(uses_both_legal_phantom_directions)
{
    SearchEngineData<Algorithm> heaps;
    const SparseReachabilityFacade facade;
    auto source = makeEndpoint(true, true, true);
    source.reverse_segment_id = {1, true};
    source.reverse_weight = EdgeWeight{0};
    source.reverse_duration = EdgeDuration{0};

    const auto result = reachabilitySearch(heaps, facade, {source}, EdgeDuration{10});

    BOOST_REQUIRE(result.isComplete());
    BOOST_REQUIRE_EQUAL(result.nodes.size(), 2);
    BOOST_CHECK(result.competitors.empty());
    for (const auto &node : result.nodes)
    {
        BOOST_CHECK_EQUAL(node.weight, EdgeWeight{1});
        BOOST_CHECK_EQUAL(node.duration, EdgeDuration{10});
    }
}

BOOST_AUTO_TEST_CASE(adds_phantom_approach_costs_in_both_search_directions)
{
    SearchEngineData<Algorithm> heaps;
    SparseReachabilityFacade facade;
    auto source = makeZeroOffsetSource();
    source.approach_weight = EdgeWeight{3};
    source.approach_duration = EdgeDuration{7};

    const auto outbound = reachabilitySearch(heaps, facade, {source}, EdgeDuration{30});
    BOOST_REQUIRE(outbound.isComplete());
    BOOST_REQUIRE_EQUAL(outbound.nodes.size(), 2);
    BOOST_CHECK_EQUAL(outbound.nodes.front().weight, EdgeWeight{4});
    BOOST_CHECK_EQUAL(outbound.nodes.front().duration, EdgeDuration{17});

    facade.Reverse = true;
    auto target = makeTarget();
    target.forward_weight = EdgeWeight{2};
    target.forward_duration = EdgeDuration{4};
    target.approach_weight = EdgeWeight{3};
    target.approach_duration = EdgeDuration{7};

    const auto inbound =
        reachabilitySearch<REVERSE_DIRECTION>(heaps, facade, {target}, EdgeDuration{40});
    BOOST_REQUIRE(inbound.isComplete());
    BOOST_REQUIRE_EQUAL(inbound.nodes.size(), 2);
    BOOST_CHECK_EQUAL(inbound.nodes.front().weight, EdgeWeight{6});
    BOOST_CHECK_EQUAL(inbound.nodes.front().duration, EdgeDuration{21});
}

BOOST_AUTO_TEST_CASE(does_not_traverse_excluded_sources_or_intermediate_nodes)
{
    SearchEngineData<Algorithm> heaps;
    SparseReachabilityFacade facade;
    facade.ExcludedNode = 0;

    const auto excluded_source =
        reachabilitySearch(heaps, facade, {makeZeroOffsetSource()}, EdgeDuration{20});
    BOOST_REQUIRE(excluded_source.isComplete());
    BOOST_CHECK(excluded_source.nodes.empty());
    BOOST_CHECK(excluded_source.competitors.empty());

    facade.ExcludedNode = 1;
    const auto excluded_intermediate =
        reachabilitySearch(heaps, facade, {makeZeroOffsetSource()}, EdgeDuration{20});
    BOOST_REQUIRE(excluded_intermediate.isComplete());
    BOOST_CHECK(excluded_intermediate.nodes.empty());
    BOOST_CHECK(excluded_intermediate.competitors.empty());
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 0);
}

BOOST_AUTO_TEST_CASE(returns_no_labels_without_legal_endpoint_candidates)
{
    SearchEngineData<Algorithm> heaps;
    const SparseReachabilityFacade facade;

    for (const auto &endpoints : {PhantomNodeCandidates{},
                                  PhantomNodeCandidates{makeTarget()},
                                  PhantomNodeCandidates{PhantomNode{}}})
    {
        const auto result = reachabilitySearch(heaps, facade, endpoints, EdgeDuration{20}, 0);
        BOOST_REQUIRE(result.isComplete());
        BOOST_CHECK(result.nodes.empty());
        BOOST_CHECK(result.competitors.empty());
    }

    const auto inbound =
        reachabilitySearch<REVERSE_DIRECTION>(heaps, facade, {makeSource()}, EdgeDuration{20}, 0);
    BOOST_REQUIRE(inbound.isComplete());
    BOOST_CHECK(inbound.nodes.empty());
    BOOST_CHECK(inbound.competitors.empty());
    BOOST_CHECK_EQUAL(heaps.reachability_heap->Occupancy(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace osrm::engine::routing_algorithms::mld
