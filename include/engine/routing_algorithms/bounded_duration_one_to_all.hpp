#ifndef OSRM_ENGINE_ROUTING_ALGORITHMS_BOUNDED_DURATION_ONE_TO_ALL_HPP
#define OSRM_ENGINE_ROUTING_ALGORITHMS_BOUNDED_DURATION_ONE_TO_ALL_HPP

#include "engine/isochrone/search_result.hpp"
#include "engine/phantom_node.hpp"
#include "engine/routing_algorithms/routing_base.hpp"

#include "util/query_heap.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace osrm::engine::routing_algorithms
{
namespace detail
{

struct BoundedDurationOneToAllHeapData
{
    EdgeDuration duration{0};
    bool crosses_duration_cutoff = false;
};

using BoundedDurationOneToAllHeap = util::QueryHeap<NodeID,
                                                    NodeID,
                                                    EdgeWeight,
                                                    BoundedDurationOneToAllHeapData,
                                                    util::UnorderedMapStorage<NodeID, int>>;

inline bool isBetterLabel(const EdgeWeight candidate_weight,
                          const EdgeDuration candidate_duration,
                          const EdgeWeight current_weight,
                          const EdgeDuration current_duration)
{
    return std::tie(candidate_weight, candidate_duration) <
           std::tie(current_weight, current_duration);
}

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

template <typename Metric> bool isValidMetric(const Metric metric)
{
    using Value = typename Metric::value_type;
    return from_alias<Value>(metric) < std::numeric_limits<Value>::max();
}

template <typename Metric> bool isValidNonnegativeMetric(const Metric metric)
{ return isValidMetric(metric) && metric >= Metric{0}; }

struct SearchState
{
    SearchState(const std::size_t max_search_records, const std::size_t initial_records)
        : max_search_records(max_search_records), used_records(initial_records)
    { BOOST_ASSERT(initial_records <= max_search_records); }

    bool isComplete() const { return result.isComplete(); }

    bool reserveSearchNode() { return reserveRecord(); }

    void markArithmeticOverflow()
    {
        if (isComplete())
            result.status = isochrone::SearchStatus::ArithmeticOverflow;
    }

    void emitSettledNode(const NodeID node,
                         const EdgeWeight weight,
                         const BoundedDurationOneToAllHeapData data)
    { result.nodes.push_back({node, data.duration, isochrone::NodeProvenance::Network, weight}); }

    void emitInboundFrontier(const NodeID node,
                             const EdgeWeight weight,
                             const BoundedDurationOneToAllHeapData data)
    {
        result.inbound_frontiers.push_back(
            {{node, data.duration, isochrone::NodeProvenance::Network, weight}});
    }

    void emitCompetitor(const NodeID node,
                        const EdgeWeight weight,
                        const BoundedDurationOneToAllHeapData data)
    {
        result.competitors.push_back(
            {node, data.duration, isochrone::NodeProvenance::Network, weight});
    }

    bool recordPhantomPartial(isochrone::PhantomPartialTraversal partial)
    {
        if (!reserveRecord())
            return false;
        result.phantom_partials.push_back(std::move(partial));
        return true;
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
    // Every retained weighted label and independently materialized fragment reserves a record.
    // Phantom seeds are transient; their clipped partials are retained separately.
    std::size_t used_records;
};

template <typename Heap>
bool insertOrUpdateBoundedDurationOneToAll(Heap &heap,
                                           const NodeID node,
                                           const EdgeWeight weight,
                                           const EdgeDuration duration,
                                           const bool crosses_duration_cutoff,
                                           SearchState &state)
{
    const BoundedDurationOneToAllHeapData data{duration, crosses_duration_cutoff};
    const auto heap_node = heap.GetHeapNodeIfWasInserted(node);
    if (!heap_node)
    {
        if (!state.reserveSearchNode())
            return false;
        heap.Insert(node, weight, data);
    }
    else if (!heap_node->WasRemoved() &&
             isBetterLabel(weight, duration, heap_node->weight, heap_node->data.duration))
    {
        heap_node->weight = weight;
        heap_node->data = data;
        heap.DecreaseKey(*heap_node);
    }
    return true;
}

inline void relaxNetworkEdge(const EdgeWeight from_weight,
                             const EdgeDuration from_duration,
                             const isochrone::DurationGraphArc &arc,
                             const EdgeDuration duration_cutoff,
                             BoundedDurationOneToAllHeap &heap,
                             SearchState &state)
{
    BOOST_ASSERT(arc.weight > EdgeWeight{0});
    BOOST_ASSERT(arc.duration >= EdgeDuration{0});

    EdgeWeight to_weight;
    EdgeDuration to_duration;
    if (!checkedAdd(from_weight, arc.weight, to_weight))
        return;
    if (!checkedAdd(from_duration, arc.duration, to_duration))
        to_duration = MAXIMAL_EDGE_DURATION;

    insertOrUpdateBoundedDurationOneToAll(heap,
                                          arc.node,
                                          to_weight,
                                          to_duration,
                                          from_duration <= duration_cutoff &&
                                              to_duration > duration_cutoff,
                                          state);
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
        relaxNetworkEdge(
            heap_node.weight, heap_node.data.duration, arc, duration_cutoff, heap, state);
        if (!state.isComplete())
            return;
    }
}

template <bool DIRECTION>
bool makeSeedMetrics(const PhantomNode &phantom,
                     const bool forward,
                     EdgeWeight &seed_weight,
                     EdgeDuration &seed_duration)
{
    if constexpr (DIRECTION == FORWARD_DIRECTION)
    {
        seed_weight =
            forward ? phantom.GetForwardWeightAsSource() : phantom.GetReverseWeightAsSource();
        seed_duration =
            forward ? phantom.GetForwardDurationAsSource() : phantom.GetReverseDurationAsSource();
    }
    else
    {
        seed_weight =
            forward ? phantom.GetForwardWeightAsTarget() : phantom.GetReverseWeightAsTarget();
        seed_duration =
            forward ? phantom.GetForwardDurationAsTarget() : phantom.GetReverseDurationAsTarget();
    }

    return isValidMetric(seed_weight) && isValidMetric(seed_duration);
}

using DurationCandidateHeap =
    util::QueryHeap<NodeID, NodeID, EdgeDuration, bool, util::UnorderedMapStorage<NodeID, int>>;

struct DurationCandidateTargets
{
    using WeightLimitEntries = std::set<std::pair<std::int64_t, PackedGeometryID>>;

    struct Geometry
    {
        bool has_duration_label = false;
        std::optional<std::int64_t> partial_upper_weight;
        std::optional<std::int64_t> full_upper_weight;
        std::optional<std::int64_t> maximum_geometry_weight;
        std::optional<std::int64_t> current_weight_limit;
    };

    std::unordered_map<PackedGeometryID, Geometry> geometries;
    // Once the duration prepass has completed, candidate geometries are immutable.  Keep one
    // ordered entry for every resolved candidate instead of recomputing the global maximum for
    // each settled Dijkstra node.
    WeightLimitEntries weight_limits;
    std::size_t unresolved_duration_geometries = 0;
    std::size_t retained_records = 0;
    isochrone::SearchStatus status = isochrone::SearchStatus::Complete;
};

inline bool reserveDurationCandidateRecord(DurationCandidateTargets &targets,
                                           const std::size_t max_search_records)
{
    if (targets.retained_records >= max_search_records)
    {
        targets.status = isochrone::SearchStatus::SearchRecordLimitReached;
        return false;
    }
    ++targets.retained_records;
    return true;
}

template <typename WeightRange>
std::optional<std::int64_t> totalGeometryWeight(const WeightRange &weights)
{
    std::int64_t total = 0;
    for (const auto weight : weights)
    {
        if (weight == INVALID_SEGMENT_WEIGHT)
            return std::nullopt;
        const auto value = static_cast<std::int64_t>(from_alias<std::uint32_t>(weight));
        if (value > std::numeric_limits<std::int64_t>::max() - total)
            return std::nullopt;
        total += value;
    }
    return total;
}

template <typename FacadeT>
std::optional<std::int64_t> directedGeometryWeight(const FacadeT &facade, const NodeID node)
{
    const auto geometry = facade.GetGeometryIndex(node);
    if (geometry.id == SPECIAL_GEOMETRYID)
        return std::nullopt;
    if (geometry.forward)
        return totalGeometryWeight(facade.GetUncompressedForwardWeights(geometry.id));
    return totalGeometryWeight(facade.GetUncompressedReverseWeights(geometry.id));
}

template <typename FacadeT>
std::optional<std::int64_t> maximumGeometryWeight(const FacadeT &facade,
                                                  const PackedGeometryID geometry_id)
{
    if (geometry_id == SPECIAL_GEOMETRYID)
        return std::nullopt;
    const auto forward = totalGeometryWeight(facade.GetUncompressedForwardWeights(geometry_id));
    const auto reverse = totalGeometryWeight(facade.GetUncompressedReverseWeights(geometry_id));
    if (forward && reverse)
        return std::max(*forward, *reverse);
    if (forward)
        return forward;
    return reverse;
}

template <typename FacadeT>
DurationCandidateTargets::Geometry *getOrAddCandidateGeometry(DurationCandidateTargets &targets,
                                                              const FacadeT &facade,
                                                              const PackedGeometryID geometry_id,
                                                              const std::size_t max_search_records)
{
    const auto found = targets.geometries.find(geometry_id);
    if (found != targets.geometries.end())
        return &found->second;

    const auto maximum_weight = maximumGeometryWeight(facade, geometry_id);
    if (!reserveDurationCandidateRecord(targets, max_search_records))
        return nullptr;
    const auto [inserted, was_inserted] = targets.geometries.emplace(
        geometry_id,
        DurationCandidateTargets::Geometry{
            false, std::nullopt, std::nullopt, maximum_weight, std::nullopt});
    BOOST_ASSERT(was_inserted);
    return &inserted->second;
}

template <typename FacadeT>
bool addDurationCandidateTarget(DurationCandidateTargets &targets,
                                const FacadeT &facade,
                                const NodeID node,
                                const std::size_t max_search_records)
{
    const auto geometry_index = facade.GetGeometryIndex(node);
    auto *geometry =
        getOrAddCandidateGeometry(targets, facade, geometry_index.id, max_search_records);
    if (geometry == nullptr)
        return false;
    // A duration label represents a complete directed geometry. Reverse-search pruning needs a
    // finite upper bound for that geometry; unlike a snapped target prefix, no invalid suffix can
    // be ignored here.
    if (!geometry->maximum_geometry_weight)
    {
        targets.status = isochrone::SearchStatus::ArithmeticOverflow;
        return false;
    }
    geometry->has_duration_label = true;
    return true;
}

template <bool DIRECTION, typename FacadeT>
bool addPhantomCandidateTarget(DurationCandidateTargets &targets,
                               const FacadeT &facade,
                               const NodeID node,
                               const EdgeWeight seed_weight,
                               const std::size_t max_search_records)
{
    auto upper_weight = static_cast<std::int64_t>(from_alias<EdgeWeight::value_type>(seed_weight));
    if constexpr (DIRECTION == FORWARD_DIRECTION)
    {
        // Source validity covers the complete remaining directed geometry. Its cost converts the
        // negative source seed into an upper bound at the far end of that geometry.
        const auto directed_weight = directedGeometryWeight(facade, node);
        if (!directed_weight)
        {
            targets.status = isochrone::SearchStatus::ArithmeticOverflow;
            return false;
        }
        if (upper_weight > std::numeric_limits<std::int64_t>::max() - *directed_weight)
            upper_weight = std::numeric_limits<std::int64_t>::max();
        else
            upper_weight += *directed_weight;
    }
    if (upper_weight < 0)
    {
        targets.status = isochrone::SearchStatus::ArithmeticOverflow;
        return false;
    }

    const auto geometry_index = facade.GetGeometryIndex(node);
    auto *geometry =
        getOrAddCandidateGeometry(targets, facade, geometry_index.id, max_search_records);
    if (geometry == nullptr)
        return false;
    geometry->partial_upper_weight =
        std::max(geometry->partial_upper_weight.value_or(upper_weight), upper_weight);
    return true;
}

template <typename FacadeT>
bool insertOrUpdateDurationCandidate(const FacadeT &facade,
                                     DurationCandidateHeap &heap,
                                     DurationCandidateTargets &targets,
                                     const NodeID node,
                                     const EdgeDuration duration,
                                     const EdgeDuration duration_cutoff,
                                     const std::size_t max_search_records)
{
    if (duration > duration_cutoff)
        return true;
    if (!addDurationCandidateTarget(targets, facade, node, max_search_records))
        return false;

    const auto heap_node = heap.GetHeapNodeIfWasInserted(node);
    if (!heap_node)
    {
        if (!reserveDurationCandidateRecord(targets, max_search_records))
            return false;
        heap.Insert(node, duration, false);
    }
    else if (!heap_node->WasRemoved() && duration < heap_node->weight)
    {
        heap_node->weight = duration;
        heap.DecreaseKey(*heap_node);
    }
    return true;
}

template <bool DIRECTION, typename FacadeT>
DurationCandidateTargets
findDurationCandidateTargets(const FacadeT &facade,
                             const PhantomNodeCandidates &endpoint_candidates,
                             const EdgeDuration duration_cutoff,
                             const std::size_t max_search_records)
{
    DurationCandidateTargets targets;
    DurationCandidateHeap heap(facade.GetNumberOfNodes());

    const auto relax = [&](const EdgeDuration from_duration, const isochrone::DurationGraphArc &arc)
    {
        if (facade.ExcludeNode(arc.node))
            return true;

        EdgeDuration to_duration;
        if (!checkedAdd(from_duration, arc.duration, to_duration))
        {
            if constexpr (DIRECTION == REVERSE_DIRECTION)
            {
                if (from_duration <= duration_cutoff)
                {
                    targets.status = isochrone::SearchStatus::ArithmeticOverflow;
                    return false;
                }
            }
            return true;
        }

        if constexpr (DIRECTION == REVERSE_DIRECTION)
        {
            if (from_duration <= duration_cutoff &&
                !addDurationCandidateTarget(targets, facade, arc.node, max_search_records))
            {
                return false;
            }
        }
        return insertOrUpdateDurationCandidate(
            facade, heap, targets, arc.node, to_duration, duration_cutoff, max_search_records);
    };

    const auto seed = [&](const PhantomNode &phantom, const NodeID node, const bool forward)
    {
        if (facade.ExcludeNode(node))
            return true;
        if (!isValidNonnegativeMetric(phantom.approach_weight) ||
            !isValidNonnegativeMetric(phantom.approach_duration))
        {
            targets.status = isochrone::SearchStatus::ArithmeticOverflow;
            return false;
        }
        EdgeWeight seed_weight;
        EdgeDuration seed_duration;
        if (!makeSeedMetrics<DIRECTION>(phantom, forward, seed_weight, seed_duration))
        {
            targets.status = isochrone::SearchStatus::ArithmeticOverflow;
            return false;
        }
        if (phantom.approach_duration <= duration_cutoff &&
            !addPhantomCandidateTarget<DIRECTION>(
                targets, facade, node, seed_weight, max_search_records))
        {
            return false;
        }

        const auto arcs = [&]()
        {
            if constexpr (DIRECTION == FORWARD_DIRECTION)
                return facade.GetIsochroneForwardEdgeRange(node);
            else
                return facade.GetIsochroneReverseEdgeRange(node);
        }();
        for (const auto &arc : arcs)
        {
            if (!relax(seed_duration, arc))
                return false;
        }
        return true;
    };

    for (const auto &phantom : endpoint_candidates)
    {
        if constexpr (DIRECTION == FORWARD_DIRECTION)
        {
            if (phantom.IsValidForwardSource() &&
                !seed(phantom, phantom.forward_segment_id.id, true))
                return targets;
            if (phantom.IsValidReverseSource() &&
                !seed(phantom, phantom.reverse_segment_id.id, false))
                return targets;
        }
        else
        {
            if (phantom.IsValidForwardTarget() &&
                !seed(phantom, phantom.forward_segment_id.id, true))
                return targets;
            if (phantom.IsValidReverseTarget() &&
                !seed(phantom, phantom.reverse_segment_id.id, false))
                return targets;
        }
    }

    while (targets.status == isochrone::SearchStatus::Complete && !heap.Empty())
    {
        const auto heap_node = heap.DeleteMinGetHeapNode();
        const auto arcs = [&]()
        {
            if constexpr (DIRECTION == FORWARD_DIRECTION)
                return facade.GetIsochroneForwardEdgeRange(heap_node.node);
            else
                return facade.GetIsochroneReverseEdgeRange(heap_node.node);
        }();
        for (const auto &arc : arcs)
        {
            if (!relax(heap_node.weight, arc))
                return targets;
        }
    }

    return targets;
}

template <bool DIRECTION, typename FacadeT>
void seedEndpoint(const FacadeT &facade,
                  const PhantomNode &phantom,
                  const EdgeDuration duration_cutoff,
                  const bool search_network,
                  BoundedDurationOneToAllHeap &heap,
                  SearchState &state)
{
    const auto seed = [&](const NodeID node, const bool forward)
    {
        if (facade.ExcludeNode(node))
            return false;

        if (!isValidNonnegativeMetric(phantom.approach_weight) ||
            !isValidNonnegativeMetric(phantom.approach_duration))
        {
            state.markArithmeticOverflow();
            return true;
        }

        const auto direction = forward ? isochrone::PhantomTraversalDirection::Forward
                                       : isochrone::PhantomTraversalDirection::Reverse;
        const auto kind = DIRECTION == FORWARD_DIRECTION
                              ? isochrone::PhantomTraversalKind::OutboundInitial
                              : isochrone::PhantomTraversalKind::InboundTerminal;
        EdgeWeight seed_weight;
        EdgeDuration seed_duration;
        if (!makeSeedMetrics<DIRECTION>(phantom, forward, seed_weight, seed_duration))
        {
            state.markArithmeticOverflow();
            return true;
        }

        const auto reaches_network = phantom.approach_duration <= duration_cutoff;
        const isochrone::PhantomPartialTraversal partial{
            node, phantom, direction, kind, seed_duration, reaches_network, seed_weight};
        if (!state.recordPhantomPartial(partial))
            return true;
        if (!search_network)
            return true;

        const auto arcs = [&]()
        {
            if constexpr (DIRECTION == FORWARD_DIRECTION)
                return facade.GetIsochroneForwardEdgeRange(node);
            else
                return facade.GetIsochroneReverseEdgeRange(node);
        }();
        for (const auto &arc : arcs)
        {
            if (facade.ExcludeNode(arc.node))
                continue;
            relaxNetworkEdge(seed_weight, seed_duration, arc, duration_cutoff, heap, state);
            if (!state.isComplete())
                return true;
        }
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

template <bool DIRECTION>
std::int64_t adjustedCandidateWeightLimit(const DurationCandidateTargets::Geometry &geometry,
                                          std::int64_t upper_weight)
{
    if constexpr (DIRECTION == REVERSE_DIRECTION)
    {
        // A target phantom may be valid only over the prefix ending at the snapped point. If a
        // later traffic closure makes both complete directions invalid, there is no finite whole-
        // geometry bound to add. Disabling early termination is conservative and keeps the valid
        // prefix instead of turning the query into an internal error.
        if (!geometry.maximum_geometry_weight)
            return std::numeric_limits<std::int64_t>::max();
        if (upper_weight >
            std::numeric_limits<std::int64_t>::max() - *geometry.maximum_geometry_weight)
        {
            return std::numeric_limits<std::int64_t>::max();
        }
        upper_weight += *geometry.maximum_geometry_weight;
    }
    return upper_weight;
}

template <bool DIRECTION> void initializeCandidateWeightLimits(DurationCandidateTargets &targets)
{
    BOOST_ASSERT(targets.weight_limits.empty());
    BOOST_ASSERT(targets.unresolved_duration_geometries == 0);

    for (auto &[geometry_id, geometry] : targets.geometries)
    {
        if (geometry.has_duration_label)
        {
            ++targets.unresolved_duration_geometries;
            continue;
        }

        if (!geometry.partial_upper_weight)
        {
            targets.status = isochrone::SearchStatus::ArithmeticOverflow;
            return;
        }

        const auto weight =
            adjustedCandidateWeightLimit<DIRECTION>(geometry, *geometry.partial_upper_weight);
        geometry.current_weight_limit = weight;
        const auto inserted = targets.weight_limits.emplace(weight, geometry_id).second;
        BOOST_ASSERT(inserted);
        static_cast<void>(inserted);
    }
}

template <bool DIRECTION>
void updateCandidateWeightLimit(DurationCandidateTargets &targets,
                                const PackedGeometryID geometry_id,
                                DurationCandidateTargets::Geometry &geometry)
{
    BOOST_ASSERT(geometry.has_duration_label);

    const auto weight =
        adjustedCandidateWeightLimit<DIRECTION>(geometry, *geometry.full_upper_weight);
    if (geometry.current_weight_limit)
    {
        const auto existing =
            targets.weight_limits.find({*geometry.current_weight_limit, geometry_id});
        BOOST_ASSERT(existing != targets.weight_limits.end());
        targets.weight_limits.erase(existing);
    }
    else
    {
        BOOST_ASSERT(targets.unresolved_duration_geometries > 0);
        --targets.unresolved_duration_geometries;
    }

    geometry.current_weight_limit = weight;
    const auto inserted = targets.weight_limits.emplace(weight, geometry_id).second;
    BOOST_ASSERT(inserted);
    static_cast<void>(inserted);
}

template <bool DIRECTION, typename FacadeT>
void retainCandidateLabel(const FacadeT &facade,
                          const EdgeDuration duration_cutoff,
                          const typename BoundedDurationOneToAllHeap::HeapNode &heap_node,
                          DurationCandidateTargets &targets,
                          SearchState &state)
{
    const auto geometry_index = facade.GetGeometryIndex(heap_node.node);
    const auto candidate = targets.geometries.find(geometry_index.id);
    if (candidate == targets.geometries.end())
        return;

    if (heap_node.data.duration == MAXIMAL_EDGE_DURATION)
    {
        state.markArithmeticOverflow();
        return;
    }

    const auto directed_weight = directedGeometryWeight(facade, heap_node.node);
    if (!directed_weight)
    {
        state.markArithmeticOverflow();
        return;
    }

    auto upper_weight =
        static_cast<std::int64_t>(from_alias<EdgeWeight::value_type>(heap_node.weight));
    if constexpr (DIRECTION == FORWARD_DIRECTION)
    {
        if (upper_weight > std::numeric_limits<std::int64_t>::max() - *directed_weight)
            upper_weight = std::numeric_limits<std::int64_t>::max();
        else
            upper_weight += *directed_weight;
    }
    if (!candidate->second.full_upper_weight || upper_weight < *candidate->second.full_upper_weight)
    {
        candidate->second.full_upper_weight = upper_weight;
        if (candidate->second.has_duration_label)
        {
            updateCandidateWeightLimit<DIRECTION>(targets, geometry_index.id, candidate->second);
        }
    }

    if (heap_node.data.duration <= duration_cutoff)
    {
        state.emitSettledNode(heap_node.node, heap_node.weight, heap_node.data);
    }
    else if constexpr (DIRECTION == REVERSE_DIRECTION)
    {
        if (heap_node.data.crosses_duration_cutoff)
            state.emitInboundFrontier(heap_node.node, heap_node.weight, heap_node.data);
        else
            state.emitCompetitor(heap_node.node, heap_node.weight, heap_node.data);
    }
    else
    {
        state.emitCompetitor(heap_node.node, heap_node.weight, heap_node.data);
    }
}

struct WeightSearchLimit
{
    bool ready = true;
    std::int64_t weight = std::numeric_limits<std::int64_t>::min();
};

inline WeightSearchLimit candidateWeightLimit(const DurationCandidateTargets &targets)
{
    if (targets.unresolved_duration_geometries != 0)
        return {false, 0};

    BOOST_ASSERT(targets.weight_limits.size() == targets.geometries.size());
    if (targets.weight_limits.empty())
        return {};
    return {true, targets.weight_limits.rbegin()->first};
}

template <bool DIRECTION, typename FacadeT>
void runBoundedSearch(const FacadeT &facade,
                      const EdgeDuration duration_cutoff,
                      BoundedDurationOneToAllHeap &heap,
                      SearchState &state,
                      DurationCandidateTargets &targets)
{
    while (state.isComplete() && !heap.Empty())
    {
        const auto limit = candidateWeightLimit(targets);
        if (limit.ready && static_cast<std::int64_t>(
                               from_alias<EdgeWeight::value_type>(heap.MinKey())) > limit.weight)
        {
            break;
        }

        const auto heap_node = heap.DeleteMinGetHeapNode();
        retainCandidateLabel<DIRECTION>(facade, duration_cutoff, heap_node, targets, state);
        if (!state.isComplete())
            return;
        relaxAdjacentEdges<DIRECTION>(facade, heap_node, duration_cutoff, heap, state);
    }

    if (state.isComplete() && !candidateWeightLimit(targets).ready)
        state.markArithmeticOverflow();
}

} // namespace detail

// Runs a weight-ordered one-to-all search and retains elapsed-duration labels for contouring.
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

    auto targets = detail::findDurationCandidateTargets<DIRECTION>(
        facade, endpoint_candidates, duration_cutoff, max_search_records);
    if (targets.status != isochrone::SearchStatus::Complete)
    {
        isochrone::SearchResult result;
        result.status = targets.status;
        return result;
    }

    detail::initializeCandidateWeightLimits<DIRECTION>(targets);
    if (targets.status != isochrone::SearchStatus::Complete)
    {
        isochrone::SearchResult result;
        result.status = targets.status;
        return result;
    }

    detail::SearchState state(max_search_records, targets.geometries.size());
    detail::BoundedDurationOneToAllHeap heap(facade.GetNumberOfNodes());
    for (const auto &phantom : endpoint_candidates)
    {
        detail::seedEndpoint<DIRECTION>(
            facade, phantom, duration_cutoff, !targets.geometries.empty(), heap, state);
        if (!state.isComplete())
            return std::move(state.result);
    }

    detail::runBoundedSearch<DIRECTION>(facade, duration_cutoff, heap, state, targets);
    return std::move(state.result);
}

} // namespace osrm::engine::routing_algorithms

#endif // OSRM_ENGINE_ROUTING_ALGORITHMS_BOUNDED_DURATION_ONE_TO_ALL_HPP
