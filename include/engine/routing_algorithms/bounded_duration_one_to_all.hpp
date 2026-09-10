#ifndef OSRM_ENGINE_ROUTING_ALGORITHMS_BOUNDED_DURATION_ONE_TO_ALL_HPP
#define OSRM_ENGINE_ROUTING_ALGORITHMS_BOUNDED_DURATION_ONE_TO_ALL_HPP

#include "engine/isochrone/search_result.hpp"
#include "engine/phantom_node.hpp"
#include "engine/routing_algorithms/routing_base.hpp"

#include "util/query_heap.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <cstdint>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace osrm::engine::routing_algorithms
{
namespace detail
{

struct BoundedDurationOneToAllHeapData
{
    bool is_virtual_seed = false;
};

using BoundedDurationOneToAllHeap = util::QueryHeap<NodeID,
                                                    NodeID,
                                                    EdgeDuration,
                                                    BoundedDurationOneToAllHeapData,
                                                    util::UnorderedMapStorage<NodeID, int>>;

template <typename Metric> bool checkedAdd(const Metric lhs, const Metric rhs, Metric &result)
{
    using Value = typename Metric::value_type;
    const auto sum = static_cast<std::int64_t>(from_alias<Value>(lhs)) +
                     static_cast<std::int64_t>(from_alias<Value>(rhs));
    if (sum < std::numeric_limits<Value>::min() || sum >= std::numeric_limits<Value>::max())
        return false;

    result = Metric{static_cast<Value>(sum)};
    return true;
}

template <typename Metric> bool checkedSubtract(const Metric lhs, const Metric rhs, Metric &result)
{
    using Value = typename Metric::value_type;
    const auto difference = static_cast<std::int64_t>(from_alias<Value>(lhs)) -
                            static_cast<std::int64_t>(from_alias<Value>(rhs));
    if (difference < std::numeric_limits<Value>::min() ||
        difference >= std::numeric_limits<Value>::max())
        return false;

    result = Metric{static_cast<Value>(difference)};
    return true;
}

template <typename Metric> bool isValidMetric(const Metric metric)
{
    using Value = typename Metric::value_type;
    return from_alias<Value>(metric) < std::numeric_limits<Value>::max();
}

struct SearchState
{
    explicit SearchState(const std::size_t max_search_records)
        : max_search_records(max_search_records)
    {
    }

    bool isComplete() const { return result.isComplete(); }

    bool reserveSearchNode() { return reserveRecord(); }

    void markArithmeticOverflow()
    {
        if (isComplete())
            result.status = isochrone::SearchStatus::ArithmeticOverflow;
    }

    void emitSettledNode(const NodeID node,
                         const EdgeDuration duration,
                         const BoundedDurationOneToAllHeapData data)
    {
        settled_entries.push_back({node, duration});
        if (!data.is_virtual_seed)
            result.nodes.push_back({node, duration, isochrone::NodeProvenance::Network});
    }

    struct SettledEntry
    {
        NodeID node;
        EdgeDuration duration;
    };

    const std::vector<SettledEntry> &settledEntries() const { return settled_entries; }

    bool recordPhantomPartial(isochrone::PhantomPartialTraversal partial)
    {
        if (!reserveRecord())
            return false;
        result.phantom_partials.push_back(std::move(partial));
        return true;
    }

    void recordReentry(const NodeID node,
                       const EdgeDuration duration,
                       const EdgeDuration duration_cutoff)
    {
        const isochrone::SearchResult::Node entry{
            node, duration, isochrone::NodeProvenance::SeedReentry};
        if (duration <= duration_cutoff)
        {
            const auto found = reentry_indices.find(node);
            if (found == reentry_indices.end())
            {
                if (!reserveRecord())
                    return;
                reentry_indices.emplace(node, reentries.size());
                reentries.push_back(entry);
            }
            else if (duration < reentries[found->second].duration)
            {
                reentries[found->second] = entry;
            }
        }
    }

    void recordInboundFrontier(const isochrone::SearchResult::Node entry)
    {
        if (!isComplete())
            return;

        const auto found = inbound_frontier_indices.find(entry.node);
        if (found == inbound_frontier_indices.end())
        {
            if (!reserveRecord())
                return;
            inbound_frontier_indices.emplace(entry.node, result.inbound_frontiers.size());
            result.inbound_frontiers.push_back({entry});
        }
        else if (entry.duration < result.inbound_frontiers[found->second].entry.duration)
        {
            result.inbound_frontiers[found->second].entry = entry;
        }
    }

    void finish()
    {
        result.nodes.reserve(result.nodes.size() + reentries.size());
        result.nodes.insert(result.nodes.end(), reentries.begin(), reentries.end());
    }

    isochrone::SearchResult result;

  private:
    bool reserveRecord()
    {
        if (!isComplete())
            return false;
        if (used_records >= max_search_records)
        {
            result.status = isochrone::SearchStatus::SearchRecordLimitReached;
            return false;
        }

        ++used_records;
        return true;
    }

    std::size_t max_search_records;
    // Every retained Dijkstra label and independently materialized result fragment reserves a
    // record. A virtual seed and its clipped phantom partial are separate retained objects.
    std::size_t used_records = 0;
    std::unordered_map<NodeID, std::size_t> reentry_indices;
    std::unordered_map<NodeID, std::size_t> inbound_frontier_indices;
    std::vector<isochrone::SearchResult::Node> reentries;
    std::vector<SettledEntry> settled_entries;
};

template <typename Heap>
bool insertOrUpdateBoundedDurationOneToAll(Heap &heap,
                                           const NodeID node,
                                           const EdgeDuration duration,
                                           const BoundedDurationOneToAllHeapData data,
                                           SearchState &state)
{
    const auto heap_node = heap.GetHeapNodeIfWasInserted(node);
    if (!heap_node)
    {
        if (!state.reserveSearchNode())
            return false;
        heap.Insert(node, duration, data);
    }
    else if (!heap_node->WasRemoved() && duration < heap_node->weight)
    {
        heap_node->weight = duration;
        heap_node->data = data;
        heap.DecreaseKey(*heap_node);
    }
    else if (!heap_node->WasRemoved() && duration == heap_node->weight &&
             heap_node->data.is_virtual_seed && !data.is_virtual_seed)
    {
        heap_node->data = data;
    }
    return true;
}

inline void relaxNetworkEdge(const typename BoundedDurationOneToAllHeap::HeapNode &heap_node,
                             const NodeID to,
                             const EdgeDuration edge_duration,
                             const EdgeDuration duration_cutoff,
                             BoundedDurationOneToAllHeap &heap,
                             SearchState &state)
{
    BOOST_ASSERT(edge_duration >= EdgeDuration{0});
    EdgeDuration to_duration;
    if (!checkedAdd(heap_node.weight, edge_duration, to_duration))
    {
        state.markArithmeticOverflow();
        return;
    }

    const auto existing = heap.GetHeapNodeIfWasInserted(to);
    const auto virtual_seed_dominates =
        existing && existing->data.is_virtual_seed && existing->weight <= to_duration;
    if (virtual_seed_dominates)
    {
        state.recordReentry(to, to_duration, duration_cutoff);
        return;
    }

    if (!state.isComplete())
        return;

    if (to_duration <= duration_cutoff)
        insertOrUpdateBoundedDurationOneToAll(heap, to, to_duration, {false}, state);
}

template <bool DIRECTION, typename FacadeT>
void relaxAdjacentEdges(const FacadeT &facade,
                        const typename BoundedDurationOneToAllHeap::HeapNode &heap_node,
                        const EdgeDuration duration_cutoff,
                        BoundedDurationOneToAllHeap &heap,
                        SearchState &state)
{
    const auto arcs = [&]()
    {
        if constexpr (DIRECTION == FORWARD_DIRECTION)
            return facade.GetIsochroneForwardEdgeRange(heap_node.node);
        else
            return facade.GetIsochroneReverseEdgeRange(heap_node.node);
    }();
    for (const auto &arc : arcs)
    {
        if (facade.ExcludeNode(arc.node))
            continue;
        relaxNetworkEdge(heap_node, arc.node, arc.duration, duration_cutoff, heap, state);
        if (!state.isComplete())
            return;
    }
}

template <bool DIRECTION>
bool makeSeedDuration(const PhantomNode &phantom, const bool forward, EdgeDuration &seed_duration)
{
    const auto duration = forward ? phantom.forward_duration : phantom.reverse_duration;
    const auto duration_offset =
        forward ? phantom.forward_duration_offset : phantom.reverse_duration_offset;

    EdgeDuration segment_duration;
    if (!isValidMetric(phantom.approach_duration) ||
        !checkedAdd(duration, duration_offset, segment_duration))
        return false;

    if constexpr (DIRECTION == FORWARD_DIRECTION)
        return checkedSubtract(phantom.approach_duration, segment_duration, seed_duration);
    else
        return checkedAdd(phantom.approach_duration, segment_duration, seed_duration);
}

template <bool DIRECTION, typename FacadeT>
void seedEndpoint(const FacadeT &facade,
                  const PhantomNode &phantom,
                  const EdgeDuration duration_cutoff,
                  BoundedDurationOneToAllHeap &heap,
                  SearchState &state)
{
    const auto seed = [&](const NodeID node, const bool forward)
    {
        if (facade.ExcludeNode(node))
            return false;

        if (!isValidMetric(phantom.approach_duration))
        {
            state.markArithmeticOverflow();
            return true;
        }

        const auto direction = forward ? isochrone::PhantomTraversalDirection::Forward
                                       : isochrone::PhantomTraversalDirection::Reverse;
        const auto kind = DIRECTION == FORWARD_DIRECTION
                              ? isochrone::PhantomTraversalKind::OutboundInitial
                              : isochrone::PhantomTraversalKind::InboundTerminal;
        if (phantom.approach_duration > duration_cutoff)
        {
            state.recordPhantomPartial({node, phantom, direction, kind, EdgeDuration{0}, false});
            return true;
        }

        EdgeDuration seed_duration;
        if (!makeSeedDuration<DIRECTION>(phantom, forward, seed_duration))
        {
            state.markArithmeticOverflow();
            return true;
        }

        const isochrone::PhantomPartialTraversal partial{
            node, phantom, direction, kind, seed_duration, true};
        if (!state.recordPhantomPartial(partial))
            return true;
        if (seed_duration <= duration_cutoff)
            insertOrUpdateBoundedDurationOneToAll(heap, node, seed_duration, {true}, state);
        return true;
    };

    if constexpr (DIRECTION == FORWARD_DIRECTION)
    {
        if (phantom.IsValidForwardSource())
            seed(phantom.forward_segment_id.id, true);
        if (phantom.IsValidReverseSource())
            seed(phantom.reverse_segment_id.id, false);
    }
    else
    {
        if (phantom.IsValidForwardTarget())
            seed(phantom.forward_segment_id.id, true);
        if (phantom.IsValidReverseTarget())
            seed(phantom.reverse_segment_id.id, false);
    }
}

template <bool DIRECTION, typename FacadeT>
void runBoundedSearch(const FacadeT &facade,
                      const EdgeDuration duration_cutoff,
                      BoundedDurationOneToAllHeap &heap,
                      SearchState &state)
{
    while (state.isComplete() && !heap.Empty() && heap.MinKey() <= duration_cutoff)
    {
        const auto heap_node = heap.DeleteMinGetHeapNode();
        state.emitSettledNode(heap_node.node, heap_node.weight, heap_node.data);
        relaxAdjacentEdges<DIRECTION>(facade, heap_node, duration_cutoff, heap, state);
    }
}

// An over-cutoff reverse relaxation is only a contour frontier if its destination was not
// subsequently settled through a cheaper route.  Dijkstra cannot establish that while processing
// an individual arc, so collect these frontiers after all in-cutoff labels have settled.
template <typename FacadeT>
void collectInboundFrontiers(const FacadeT &facade,
                             const EdgeDuration duration_cutoff,
                             const BoundedDurationOneToAllHeap &heap,
                             SearchState &state)
{
    BOOST_ASSERT(state.isComplete());

    for (const auto &settled : state.settledEntries())
    {
        for (const auto &arc : facade.GetIsochroneReverseEdgeRange(settled.node))
        {
            if (facade.ExcludeNode(arc.node))
                continue;

            EdgeDuration entry_duration;
            if (!checkedAdd(settled.duration, arc.duration, entry_duration))
            {
                state.markArithmeticOverflow();
                return;
            }
            if (entry_duration <= duration_cutoff)
                continue;

            const auto existing = heap.GetHeapNodeIfWasInserted(arc.node);
            // A settled network label already materializes the destination geometry with a
            // cheaper duration. A settled virtual target seed only materializes the prefix up
            // to the target phantom, however. A loop can make a suffix after that phantom
            // reachable even when its label at the geometry's first coordinate exceeds the
            // cutoff, so retain that crossing for the materializer.
            if (existing && existing->WasRemoved() && !existing->data.is_virtual_seed)
                continue;

            state.recordInboundFrontier(
                {arc.node, entry_duration, isochrone::NodeProvenance::Network});
            if (!state.isComplete())
                return;
        }
    }
}

} // namespace detail

// Runs bounded Dijkstra over the independent directed duration graph.
template <bool DIRECTION, typename FacadeT>
isochrone::SearchResult
boundedDurationOneToAllSearch(const FacadeT &facade,
                              const PhantomNodeCandidates &endpoint_candidates,
                              const EdgeDuration duration_cutoff,
                              const std::size_t max_search_records)
{
    BOOST_ASSERT(duration_cutoff >= EdgeDuration{0});
    BOOST_ASSERT(duration_cutoff < MAXIMAL_EDGE_DURATION);
    BOOST_ASSERT(facade.HasIsochroneGraph());

    detail::SearchState state(max_search_records);
    detail::BoundedDurationOneToAllHeap heap(facade.GetNumberOfNodes());
    for (const auto &phantom : endpoint_candidates)
    {
        detail::seedEndpoint<DIRECTION>(facade, phantom, duration_cutoff, heap, state);
        if (!state.isComplete())
        {
            state.finish();
            return std::move(state.result);
        }
    }

    detail::runBoundedSearch<DIRECTION>(facade, duration_cutoff, heap, state);
    if constexpr (DIRECTION == REVERSE_DIRECTION)
    {
        if (state.isComplete())
            detail::collectInboundFrontiers(facade, duration_cutoff, heap, state);
    }
    state.finish();
    return std::move(state.result);
}

} // namespace osrm::engine::routing_algorithms

#endif // OSRM_ENGINE_ROUTING_ALGORITHMS_BOUNDED_DURATION_ONE_TO_ALL_HPP
