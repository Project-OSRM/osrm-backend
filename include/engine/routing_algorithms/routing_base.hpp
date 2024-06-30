#ifndef OSRM_ENGINE_ROUTING_BASE_HPP
#define OSRM_ENGINE_ROUTING_BASE_HPP

#include "guidance/turn_bearing.hpp"
#include "guidance/turn_instruction.hpp"

#include "engine/algorithm.hpp"
#include "engine/datafacade.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "engine/search_engine_data.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <stack>
#include <utility>
#include <vector>

namespace osrm::engine::routing_algorithms
{

namespace details
{
template <typename Heap>
void insertSourceInForwardHeap(Heap &forward_heap, const PhantomNode &source)
{
    if (source.IsValidForwardSource())
    {
        forward_heap.Insert(source.forward_segment_id.id,
                            EdgeWeight{0} - source.GetForwardWeightPlusOffset(),
                            source.forward_segment_id.id);
    }

    if (source.IsValidReverseSource())
    {
        forward_heap.Insert(source.reverse_segment_id.id,
                            EdgeWeight{0} - source.GetReverseWeightPlusOffset(),
                            source.reverse_segment_id.id);
    }
}

template <typename Heap>
void insertTargetInReverseHeap(Heap &reverse_heap, const PhantomNode &target)
{
    if (target.IsValidForwardTarget())
    {
        reverse_heap.Insert(target.forward_segment_id.id,
                            target.GetForwardWeightPlusOffset(),
                            target.forward_segment_id.id);
    }

    if (target.IsValidReverseTarget())
    {
        reverse_heap.Insert(target.reverse_segment_id.id,
                            target.GetReverseWeightPlusOffset(),
                            target.reverse_segment_id.id);
    }
}
} // namespace details
static constexpr bool FORWARD_DIRECTION = true;
static constexpr bool REVERSE_DIRECTION = false;

// Identify nodes in the forward(reverse) search direction that will require step forcing
// e.g. if source and destination nodes are on the same segment.
std::vector<NodeID> getForwardForceNodes(const PhantomEndpointCandidates &candidates);
std::vector<NodeID> getForwardForceNodes(const PhantomCandidatesToTarget &candidates);
std::vector<NodeID> getBackwardForceNodes(const PhantomEndpointCandidates &candidates);
std::vector<NodeID> getBackwardForceNodes(const PhantomCandidatesToTarget &candidates);

// Find the specific phantom node endpoints for a given path from a list of candidates.
PhantomEndpoints endpointsFromCandidates(const PhantomEndpointCandidates &candidates,
                                         const std::vector<NodeID> &path);

template <typename HeapNodeT>
inline bool shouldForceStep(const std::vector<NodeID> &force_nodes,
                            const HeapNodeT &forward_heap_node,
                            const HeapNodeT &reverse_heap_node)
{
    // routing steps are forced when the node is a source of both forward and reverse search heaps.
    return forward_heap_node.data.parent == forward_heap_node.node &&
           reverse_heap_node.data.parent == reverse_heap_node.node &&
           std::find(force_nodes.begin(), force_nodes.end(), forward_heap_node.node) !=
               force_nodes.end();
}

template <typename Heap>
void insertNodesInHeaps(Heap &forward_heap, Heap &reverse_heap, const PhantomEndpoints &endpoints)
{
    details::insertSourceInForwardHeap(forward_heap, endpoints.source_phantom);
    details::insertTargetInReverseHeap(reverse_heap, endpoints.target_phantom);
}

template <typename Heap>
void insertNodesInHeaps(Heap &forward_heap,
                        Heap &reverse_heap,
                        const PhantomEndpointCandidates &endpoint_candidates)
{
    for (const auto &source : endpoint_candidates.source_phantoms)
    {
        details::insertSourceInForwardHeap(forward_heap, source);
    }

    for (const auto &target : endpoint_candidates.target_phantoms)
    {
        details::insertTargetInReverseHeap(reverse_heap, target);
    }
}

template <typename ManyToManyQueryHeap>
void insertSourceInHeap(ManyToManyQueryHeap &heap, const PhantomNodeCandidates &source_candidates)
{
    for (const auto &phantom_node : source_candidates)
    {
        if (phantom_node.IsValidForwardSource())
        {
            heap.Insert(phantom_node.forward_segment_id.id,
                        EdgeWeight{0} - phantom_node.GetForwardWeightPlusOffset(),
                        {phantom_node.forward_segment_id.id,
                         EdgeDuration{0} - phantom_node.GetForwardDuration(),
                         EdgeDistance{0} - phantom_node.GetForwardDistance()});
        }
        if (phantom_node.IsValidReverseSource())
        {
            heap.Insert(phantom_node.reverse_segment_id.id,
                        EdgeWeight{0} - phantom_node.GetReverseWeightPlusOffset(),
                        {phantom_node.reverse_segment_id.id,
                         EdgeDuration{0} - phantom_node.GetReverseDuration(),
                         EdgeDistance{0} - phantom_node.GetReverseDistance()});
        }
    }
}

template <typename ManyToManyQueryHeap>
void insertTargetInHeap(ManyToManyQueryHeap &heap, const PhantomNodeCandidates &target_candidates)
{
    for (const auto &phantom_node : target_candidates)
    {
        if (phantom_node.IsValidForwardTarget())
        {
            heap.Insert(phantom_node.forward_segment_id.id,
                        phantom_node.GetForwardWeightPlusOffset(),
                        {phantom_node.forward_segment_id.id,
                         phantom_node.GetForwardDuration(),
                         phantom_node.GetForwardDistance()});
        }
        if (phantom_node.IsValidReverseTarget())
        {
            heap.Insert(phantom_node.reverse_segment_id.id,
                        phantom_node.GetReverseWeightPlusOffset(),
                        {phantom_node.reverse_segment_id.id,
                         phantom_node.GetReverseDuration(),
                         phantom_node.GetReverseDistance()});
        }
    }
}

template <typename FacadeT>
void annotatePath(const FacadeT &facade,
                  const PhantomEndpoints &endpoints,
                  const std::vector<NodeID> &unpacked_nodes,
                  const std::vector<EdgeID> &unpacked_edges,
                  std::vector<PathData> &unpacked_path)
{
    BOOST_ASSERT(!unpacked_nodes.empty());
    BOOST_ASSERT(unpacked_nodes.size() == unpacked_edges.size() + 1);

    const auto source_node_id = unpacked_nodes.front();
    const auto target_node_id = unpacked_nodes.back();
    const bool start_traversed_in_reverse =
        endpoints.source_phantom.forward_segment_id.id != source_node_id;
    const bool target_traversed_in_reverse =
        endpoints.target_phantom.forward_segment_id.id != target_node_id;

    BOOST_ASSERT(endpoints.source_phantom.forward_segment_id.id == source_node_id ||
                 endpoints.source_phantom.reverse_segment_id.id == source_node_id);
    BOOST_ASSERT(endpoints.target_phantom.forward_segment_id.id == target_node_id ||
                 endpoints.target_phantom.reverse_segment_id.id == target_node_id);

    // datastructures to hold extracted data from geometry
    std::vector<NodeID> id_vector;
    std::vector<SegmentWeight> weight_vector;
    std::vector<SegmentDuration> duration_vector;
    std::vector<DatasourceID> datasource_vector;

    const auto get_segment_geometry = [&](const auto geometry_index)
    {
        const auto copy = [](auto &vector, const auto range)
        {
            vector.resize(range.size());
            std::copy(range.begin(), range.end(), vector.begin());
        };

        if (geometry_index.forward)
        {
            copy(id_vector, facade.GetUncompressedForwardGeometry(geometry_index.id));
            copy(weight_vector, facade.GetUncompressedForwardWeights(geometry_index.id));
            copy(duration_vector, facade.GetUncompressedForwardDurations(geometry_index.id));
            copy(datasource_vector, facade.GetUncompressedForwardDatasources(geometry_index.id));
        }
        else
        {
            copy(id_vector, facade.GetUncompressedReverseGeometry(geometry_index.id));
            copy(weight_vector, facade.GetUncompressedReverseWeights(geometry_index.id));
            copy(duration_vector, facade.GetUncompressedReverseDurations(geometry_index.id));
            copy(datasource_vector, facade.GetUncompressedReverseDatasources(geometry_index.id));
        }
    };

    auto node_from = unpacked_nodes.begin(), node_last = std::prev(unpacked_nodes.end());
    for (auto edge = unpacked_edges.begin(); node_from != node_last; ++node_from, ++edge)
    {
        const auto &edge_data = facade.GetEdgeData(*edge);
        const auto turn_id = edge_data.turn_id; // edge-based graph edge index
        const auto node_id = *node_from;        // edge-based graph node index

        const auto geometry_index = facade.GetGeometryIndex(node_id);
        get_segment_geometry(geometry_index);

        BOOST_ASSERT(!id_vector.empty());
        BOOST_ASSERT(!datasource_vector.empty());
        BOOST_ASSERT(weight_vector.size() + 1 == id_vector.size());
        BOOST_ASSERT(duration_vector.size() + 1 == id_vector.size());

        const bool is_first_segment = unpacked_path.empty();

        std::size_t start_index = 0;
        if (is_first_segment)
        {
            unsigned short segment_position = endpoints.source_phantom.fwd_segment_position;
            if (start_traversed_in_reverse)
            {
                segment_position =
                    weight_vector.size() - endpoints.source_phantom.fwd_segment_position - 1;
            }
            BOOST_ASSERT(segment_position >= 0);
            start_index = static_cast<std::size_t>(segment_position);
        }
        const std::size_t end_index = weight_vector.size();

        BOOST_ASSERT(start_index < end_index);
        for (std::size_t segment_idx = start_index; segment_idx < end_index; ++segment_idx)
        {
            unpacked_path.push_back(PathData{node_id,
                                             id_vector[segment_idx + 1],
                                             alias_cast<EdgeWeight>(weight_vector[segment_idx]),
                                             {0},
                                             alias_cast<EdgeDuration>(duration_vector[segment_idx]),
                                             {0},
                                             datasource_vector[segment_idx],
                                             std::nullopt});
        }
        BOOST_ASSERT(!unpacked_path.empty());

        const auto turn_duration = facade.GetDurationPenaltyForEdgeID(turn_id);
        const auto turn_weight = facade.GetWeightPenaltyForEdgeID(turn_id);

        unpacked_path.back().duration_until_turn += alias_cast<EdgeDuration>(turn_duration);
        unpacked_path.back().duration_of_turn = alias_cast<EdgeDuration>(turn_duration);
        unpacked_path.back().weight_until_turn += alias_cast<EdgeWeight>(turn_weight);
        unpacked_path.back().weight_of_turn = alias_cast<EdgeWeight>(turn_weight);
        unpacked_path.back().turn_edge = turn_id;
    }

    std::size_t start_index = 0, end_index = 0;
    const auto source_geometry_id = facade.GetGeometryIndex(source_node_id).id;
    const auto target_geometry = facade.GetGeometryIndex(target_node_id);
    const auto is_local_path = source_geometry_id == target_geometry.id && unpacked_path.empty();

    get_segment_geometry(target_geometry);

    if (target_traversed_in_reverse)
    {
        if (is_local_path)
        {
            start_index = weight_vector.size() - endpoints.source_phantom.fwd_segment_position - 1;
        }
        end_index = weight_vector.size() - endpoints.target_phantom.fwd_segment_position - 1;
    }
    else
    {
        if (is_local_path)
        {
            start_index = endpoints.source_phantom.fwd_segment_position;
        }
        end_index = endpoints.target_phantom.fwd_segment_position;
    }

    // Given the following compressed geometry:
    // U---v---w---x---y---Z
    //    s           t
    // s: fwd_segment 0
    // t: fwd_segment 3
    // -> (U, v), (v, w), (w, x)
    // note that (x, t) is _not_ included but needs to be added later.
    for (std::size_t segment_idx = start_index; segment_idx != end_index;
         (start_index < end_index ? ++segment_idx : --segment_idx))
    {
        BOOST_ASSERT(segment_idx < static_cast<std::size_t>(id_vector.size() - 1));
        unpacked_path.push_back(
            PathData{target_node_id,
                     id_vector[start_index < end_index ? segment_idx + 1 : segment_idx - 1],
                     alias_cast<EdgeWeight>(weight_vector[segment_idx]),
                     {0},
                     alias_cast<EdgeDuration>(duration_vector[segment_idx]),
                     {0},
                     datasource_vector[segment_idx],
                     std::nullopt});
    }

    if (!unpacked_path.empty())
    {
        const auto source_weight = start_traversed_in_reverse
                                       ? endpoints.source_phantom.reverse_weight
                                       : endpoints.source_phantom.forward_weight;
        const auto source_duration = start_traversed_in_reverse
                                         ? endpoints.source_phantom.reverse_duration
                                         : endpoints.source_phantom.forward_duration;
        // The above code will create segments for (v, w), (w,x), (x, y) and (y, Z).
        // However the first segment duration needs to be adjusted to the fact that the source
        // phantom is in the middle of the segment. We do this by subtracting v--s from the
        // duration.

        // Since it's possible duration_until_turn can be less than source_weight here if
        // a negative enough turn penalty is used to modify this edge weight during
        // osrm-contract, we clamp to 0 here so as not to return a negative duration
        // for this segment.

        // TODO this creates a scenario where it's possible the duration from a phantom
        // node to the first turn would be the same as from end to end of a segment,
        // which is obviously incorrect and not ideal...
        unpacked_path.front().weight_until_turn =
            std::max(unpacked_path.front().weight_until_turn - source_weight, {0});
        unpacked_path.front().duration_until_turn =
            std::max(unpacked_path.front().duration_until_turn - source_duration, {0});
    }
}

template <typename Algorithm>
double getPathDistance(const DataFacade<Algorithm> &facade,
                       const std::vector<PathData> &unpacked_path,
                       const PhantomNode &source_phantom,
                       const PhantomNode &target_phantom)
{
    double distance = 0.0;
    auto prev_coordinate = source_phantom.location;

    for (const auto &p : unpacked_path)
    {
        const auto current_coordinate = facade.GetCoordinateOfNode(p.turn_via_node);

        distance +=
            util::coordinate_calculation::greatCircleDistance(prev_coordinate, current_coordinate);

        prev_coordinate = current_coordinate;
    }

    distance +=
        util::coordinate_calculation::greatCircleDistance(prev_coordinate, target_phantom.location);

    return distance;
}

template <typename AlgorithmT>
InternalRouteResult extractRoute(const DataFacade<AlgorithmT> &facade,
                                 const EdgeWeight weight,
                                 const PhantomEndpointCandidates &endpoint_candidates,
                                 const std::vector<NodeID> &unpacked_nodes,
                                 const std::vector<EdgeID> &unpacked_edges)
{
    InternalRouteResult raw_route_data;

    // No path found for both target nodes?
    if (INVALID_EDGE_WEIGHT == weight)
    {
        return raw_route_data;
    }

    auto phantom_endpoints = endpointsFromCandidates(endpoint_candidates, unpacked_nodes);
    raw_route_data.leg_endpoints = {phantom_endpoints};

    raw_route_data.shortest_path_weight = weight;
    raw_route_data.unpacked_path_segments.resize(1);
    raw_route_data.source_traversed_in_reverse.push_back(
        (unpacked_nodes.front() != phantom_endpoints.source_phantom.forward_segment_id.id));
    raw_route_data.target_traversed_in_reverse.push_back(
        (unpacked_nodes.back() != phantom_endpoints.target_phantom.forward_segment_id.id));

    annotatePath(facade,
                 phantom_endpoints,
                 unpacked_nodes,
                 unpacked_edges,
                 raw_route_data.unpacked_path_segments.front());

    return raw_route_data;
}

template <typename FacadeT> EdgeDistance computeEdgeDistance(const FacadeT &facade, NodeID node_id)
{
    const auto geometry_index = facade.GetGeometryIndex(node_id);

    EdgeDistance total_distance = {0};

    auto geometry_range = facade.GetUncompressedForwardGeometry(geometry_index.id);
    for (auto current = geometry_range.begin(); current < geometry_range.end() - 1; ++current)
    {
        total_distance += util::coordinate_calculation::greatCircleDistance(
            facade.GetCoordinateOfNode(*current), facade.GetCoordinateOfNode(*std::next(current)));
    }

    return total_distance;
}

} // namespace osrm::engine::routing_algorithms

#endif // OSRM_ENGINE_ROUTING_BASE_HPP
