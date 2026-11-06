#ifndef OSRM_ENGINE_REACHABILITY_HPP
#define OSRM_ENGINE_REACHABILITY_HPP

#include "engine/algorithm.hpp"
#include "engine/phantom_node.hpp"
#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/search_engine_data.hpp"

#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace osrm::engine::routing_algorithms
{

enum class ReachabilitySearchStatus : std::uint8_t
{
    Complete,
    SearchNodeLimitReached,
    ArithmeticOverflow
};

// Costs at the first coordinate in the edge-based node's legal geometry direction.
// Reverse-search costs describe travel from that coordinate to the target.
struct ReachabilityNode
{
    NodeID node;
    EdgeWeight weight;
    EdgeDuration duration;
};

struct ReachabilitySearchResult
{
    // Minimum-weight labels with duration at or below the cutoff.
    std::vector<ReachabilityNode> nodes;
    // Over-cutoff labels retained to suppress higher-weight alternatives on the same road.
    std::vector<ReachabilityNode> competitors;
    ReachabilitySearchStatus status = ReachabilitySearchStatus::Complete;

    // Labels are usable only after the entire search completes successfully.
    bool isComplete() const { return status == ReachabilitySearchStatus::Complete; }
};

namespace mld
{
namespace detail
{

template <typename Metric> bool checkedMetric(const std::int64_t value, Metric &result)
{
    using Value = typename Metric::value_type;
    if (value < std::numeric_limits<Value>::min() || value >= std::numeric_limits<Value>::max())
        return false;
    result = Metric{static_cast<Value>(value)};
    return true;
}

template <bool DIRECTION, typename FacadeT, typename Heap, typename HeapNodeT>
ReachabilitySearchStatus relaxReachabilityEdges(const FacadeT &facade,
                                                Heap &heap,
                                                const HeapNodeT &heap_node,
                                                const std::size_t maximum_search_nodes)
{
    for (const auto edge : facade.GetBorderEdgeRange(0, heap_node.node))
    {
        const auto traversable = DIRECTION == FORWARD_DIRECTION ? facade.IsForwardEdge(edge)
                                                                : facade.IsBackwardEdge(edge);
        if (!traversable)
            continue;

        const auto target = facade.GetTarget(edge);
        if (facade.ExcludeNode(target))
            continue;

        const auto &edge_data = facade.GetEdgeData(edge);
        const auto metric_node = DIRECTION == FORWARD_DIRECTION ? heap_node.node : target;
        const auto edge_weight =
            from_alias<std::int64_t>(facade.GetNodeWeight(metric_node)) +
            from_alias<std::int64_t>(facade.GetWeightPenaltyForEdgeID(edge_data.turn_id));
        BOOST_ASSERT(edge_weight >= 0);
        const auto candidate_weight = from_alias<std::int64_t>(heap_node.weight) + edge_weight;
        const auto current = heap.GetHeapNodeIfWasInserted(target);
        if (current && (current->WasRemoved() ||
                        candidate_weight >= from_alias<std::int64_t>(current->weight)))
            continue;

        EdgeWeight weight;
        if (!checkedMetric(candidate_weight, weight))
            return ReachabilitySearchStatus::ArithmeticOverflow;

        const auto candidate_duration =
            from_alias<std::int64_t>(heap_node.data.duration) +
            from_alias<std::int64_t>(facade.GetNodeDuration(metric_node)) +
            from_alias<std::int64_t>(facade.GetDurationPenaltyForEdgeID(edge_data.turn_id));
        EdgeDuration duration;
        if (!checkedMetric(candidate_duration, duration))
            return ReachabilitySearchStatus::ArithmeticOverflow;

        if (!current && heap.Occupancy() >= maximum_search_nodes)
            return ReachabilitySearchStatus::SearchNodeLimitReached;

        if (current)
        {
            current->weight = weight;
            current->data.duration = duration;
            heap.DecreaseKey(*current);
        }
        else
        {
            heap.Insert(target, weight, {duration});
        }
    }
    return ReachabilitySearchStatus::Complete;
}

} // namespace detail

// Searches base-level MLD edges in profile-weight order, carrying the selected path's duration.
// Equal-weight alternatives retain the first label; duration is not a secondary search metric.
// The cutoff partitions output labels only: arbitrary profile weights and turn durations do not
// permit duration-based pruning, so a complete search may visit an entire connected component.
// The budget counts distinct discovered network nodes, including queued and over-cutoff labels;
// virtual phantom seeds do not count. It is not a bound on total memory or execution time.
template <bool DIRECTION = FORWARD_DIRECTION, typename FacadeT>
ReachabilitySearchResult
reachabilitySearch(SearchEngineData<Algorithm> &engine_working_data,
                   const FacadeT &facade,
                   const PhantomNodeCandidates &endpoint_candidates,
                   const EdgeDuration max_duration,
                   const std::size_t maximum_search_nodes = std::numeric_limits<std::size_t>::max())
{
    BOOST_ASSERT(max_duration >= EdgeDuration{0});

    ReachabilitySearchResult result;
    engine_working_data.InitializeOrClearReachabilityThreadLocalStorage(facade.GetNumberOfNodes());
    auto &query_heap = *engine_working_data.reachability_heap;

    struct SourceNode
    {
        NodeID node;
        EdgeWeight weight;
        ReachabilityHeapData data;
    };

    const auto insert_source = [&](const NodeID node,
                                   const EdgeWeight weight,
                                   const EdgeDuration duration) -> ReachabilitySearchStatus
    {
        BOOST_ASSERT(node < facade.GetNumberOfNodes());
        if (!facade.ExcludeNode(node))
        {
            // A source label is virtual: it starts at a partial segment and is therefore not a
            // label for the edge-based node itself.  Relax it directly into the query heap so a
            // real path that later returns to this node is not discarded as a worse duplicate.
            const SourceNode source_node{node, weight, {duration}};
            return detail::relaxReachabilityEdges<DIRECTION>(
                facade, query_heap, source_node, maximum_search_nodes);
        }
        return ReachabilitySearchStatus::Complete;
    };

    for (const auto &endpoint : endpoint_candidates)
    {
        const auto forward_is_valid = DIRECTION == FORWARD_DIRECTION
                                          ? endpoint.IsValidForwardSource()
                                          : endpoint.IsValidForwardTarget();
        if (forward_is_valid)
        {
            result.status = insert_source(
                endpoint.forward_segment_id.id,
                DIRECTION == FORWARD_DIRECTION ? endpoint.GetForwardWeightAsSource()
                                               : endpoint.GetForwardWeightAsTarget(),
                DIRECTION == FORWARD_DIRECTION ? endpoint.GetForwardDurationAsSource()
                                               : endpoint.GetForwardDurationAsTarget());
            if (!result.isComplete())
                return result;
        }
        const auto reverse_is_valid = DIRECTION == FORWARD_DIRECTION
                                          ? endpoint.IsValidReverseSource()
                                          : endpoint.IsValidReverseTarget();
        if (reverse_is_valid)
        {
            result.status = insert_source(
                endpoint.reverse_segment_id.id,
                DIRECTION == FORWARD_DIRECTION ? endpoint.GetReverseWeightAsSource()
                                               : endpoint.GetReverseWeightAsTarget(),
                DIRECTION == FORWARD_DIRECTION ? endpoint.GetReverseDurationAsSource()
                                               : endpoint.GetReverseDurationAsTarget());
            if (!result.isComplete())
                return result;
        }
    }

    while (!query_heap.Empty())
    {
        const auto heap_node = query_heap.DeleteMinGetHeapNode();

        // Weight is the optimization metric.  An over-duration, lower-weight label still needs
        // to propagate: it can suppress an in-cutoff but higher-weight competing path.  The
        // duration cutoff is therefore applied only to returned labels.
        if (heap_node.data.duration <= max_duration)
        {
            result.nodes.push_back({heap_node.node, heap_node.weight, heap_node.data.duration});
        }
        else
        {
            result.competitors.push_back(
                {heap_node.node, heap_node.weight, heap_node.data.duration});
        }

        result.status = detail::relaxReachabilityEdges<DIRECTION>(
            facade, query_heap, heap_node, maximum_search_nodes);
        if (!result.isComplete())
            return result;
    }

    return result;
}

} // namespace mld
} // namespace osrm::engine::routing_algorithms

#endif // OSRM_ENGINE_REACHABILITY_HPP
