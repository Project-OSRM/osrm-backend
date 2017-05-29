#ifndef OSRM_ENGINE_ROUTING_INIT_HPP
#define OSRM_ENGINE_ROUTING_INIT_HPP

#include "engine/routing_algorithms/routing_base_ch.hpp"
#include "engine/routing_algorithms/routing_base_mld.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

namespace detail
{
template <typename Facade, typename Heap>
void relaxSourceEdges(const Facade &facade, Heap &heap, const NodeID node, const EdgeWeight weight)
{
    for (auto edge : facade.GetAdjacentEdgeRange(node))
    {
        const auto &edge_data = facade.GetEdgeData(edge);
        if (edge_data.forward)
        {
            const auto to = facade.GetTarget(edge);
            const auto to_weight = weight + getEdgeInternalWeight(facade, node, edge);
            if (!heap.WasInserted(to))
            {
                heap.Insert(facade.GetTarget(edge), to_weight, node);
            }
            else if (to_weight < heap.GetKey(to))
            {
                heap.GetData(to).parent = node;
                heap.DecreaseKey(to, to_weight);
            }
        }
    }
}
}

template <typename Iterator> bool areSegmentsValid(Iterator first, const Iterator last)
{
    return std::find(first, last, INVALID_SEGMENT_WEIGHT) == last;
}

template <typename Iterator>
typename Iterator::value_type
sumSegmentValues(Iterator first, const Iterator last, float first_ratio, float last_ratio)
{
    if (first == last)
        return typename Iterator::value_type{0};

    // compute (s→next) + Σ(next→prev) + (prev→t)
    // first--------s---------next-----...------prev-------t-------last
    //   first_ratio                              last_ratio
    auto prev = std::prev(last);
    auto result = std::accumulate(first, prev, *prev * last_ratio - *first * first_ratio);
    BOOST_ASSERT(result >= 0.);
    return result;
}

template <typename Facade, typename Heap>
auto insertNodesInHeaps(const Facade &facade,
                        Heap &forward_heap,
                        Heap &reverse_heap,
                        const PhantomNodes &nodes)
{
    // TODO: generalize
    const auto &source = nodes.source_phantom;
    const auto &target = nodes.target_phantom;

    //   0---1-s==2===3===4=t---5---6---end
    const bool cross_forward = source.forward_segment_id.enabled &&
                               target.forward_segment_id.enabled &&
                               (source.forward_segment_id.id == target.forward_segment_id.id) &&
                               !(std::tie(source.fwd_segment_position, source.fwd_segment_ratio) >
                                 std::tie(target.fwd_segment_position, target.fwd_segment_ratio));

    // end---6-t==5===4===3=s---2---1---0
    const bool cross_reverse = source.reverse_segment_id.enabled &&
                               target.reverse_segment_id.enabled &&
                               (source.reverse_segment_id.id == target.reverse_segment_id.id) &&
                               (std::tie(source.fwd_segment_position, source.fwd_segment_ratio) >
                                std::tie(target.fwd_segment_position, target.fwd_segment_ratio));

    if (source.forward_segment_id.enabled && !cross_forward)
    { //   0---1---2---3---(4-s==)5===6===end
        const auto node_id = source.forward_segment_id.id;
        const auto geometry_index = facade.GetGeometryIndex(node_id);
        const auto weights = facade.GetUncompressedForwardWeights(geometry_index.id);
        if (areSegmentsValid(weights.begin() + source.fwd_segment_position, weights.end()))
        {
            forward_heap.InsertVisited(node_id, 0, node_id);
            const auto weight = sumSegmentValues(weights.begin() + source.fwd_segment_position,
                                                 weights.end(),
                                                 source.fwd_segment_ratio,
                                                 1.);
            detail::relaxSourceEdges(facade, forward_heap, node_id, weight);
        }
    }

    if (source.reverse_segment_id.enabled && !cross_reverse)
    { // end===6===5===4===3(=s--2)---1---0
        const auto node_id = source.reverse_segment_id.id;
        const auto geometry_index = facade.GetGeometryIndex(node_id);
        const auto weights = facade.GetUncompressedReverseWeights(geometry_index.id);
        if (areSegmentsValid(weights.end() - source.fwd_segment_position - 1, weights.end()))
        {
            forward_heap.InsertVisited(node_id, 0, node_id);
            const auto weight = sumSegmentValues(weights.end() - source.fwd_segment_position - 1,
                                                 weights.end(),
                                                 1. - source.fwd_segment_ratio,
                                                 1.);
            detail::relaxSourceEdges(facade, forward_heap, node_id, weight);
        }
    }

    if (target.forward_segment_id.enabled && !cross_forward)
    { //   0===1===2===3===(4=t--)5---6---end
        const auto geometry_index = facade.GetGeometryIndex(target.forward_segment_id.id);
        const auto weights = facade.GetUncompressedForwardWeights(geometry_index.id);
        if (areSegmentsValid(weights.begin(), weights.begin() + target.fwd_segment_position + 1))
        {
            const auto weight = sumSegmentValues(weights.begin(),
                                                 weights.begin() + target.fwd_segment_position + 1,
                                                 0.,
                                                 target.fwd_segment_ratio);
            reverse_heap.Insert(target.forward_segment_id.id, weight, target.forward_segment_id.id);
        }
    }

    if (target.reverse_segment_id.enabled && !cross_reverse)
    { // end---6---5---4---3(-t==2)===1===0
        const auto geometry_index = facade.GetGeometryIndex(target.reverse_segment_id.id);
        const auto weights = facade.GetUncompressedReverseWeights(geometry_index.id);
        if (areSegmentsValid(weights.begin(), weights.begin() + target.fwd_segment_position + 1))
        {
            const auto weight = sumSegmentValues(weights.begin(),
                                                 weights.end() - target.fwd_segment_position,
                                                 0.,
                                                 1 - target.fwd_segment_ratio);
            reverse_heap.Insert(target.reverse_segment_id.id, weight, target.reverse_segment_id.id);
        }
    }

    std::pair<NodeID, EdgeWeight> cross_path{SPECIAL_NODEID, INVALID_EDGE_WEIGHT};
    if (cross_forward)
    { //   0---1-s==2===3===4=t---5---6---end
        const auto geometry_index = facade.GetGeometryIndex(source.forward_segment_id.id);
        const auto weights = facade.GetUncompressedForwardWeights(geometry_index.id);
        const auto first = weights.begin() + source.fwd_segment_position;
        const auto last = weights.begin() + target.fwd_segment_position + 1;

        if (areSegmentsValid(first, last))
        {
            const auto weight =
                sumSegmentValues(first, last, source.fwd_segment_ratio, target.fwd_segment_ratio);
            cross_path = std::make_pair(source.forward_segment_id.id, weight);
        }
    }
    else if (cross_reverse)
    { // end---6-t==5===4===3=s---2---1---0
        const auto geometry_index = facade.GetGeometryIndex(source.reverse_segment_id.id);
        const auto weights = facade.GetUncompressedReverseWeights(geometry_index.id);
        const auto first = weights.end() - source.fwd_segment_position - 1;
        const auto last = weights.end() - target.fwd_segment_position;
        if (areSegmentsValid(first, last))
        {
            const auto weight = sumSegmentValues(
                first, last, 1. - source.fwd_segment_ratio, 1. - target.fwd_segment_ratio);
            cross_path = std::make_pair(source.reverse_segment_id.id, weight);
        }
    }

    return cross_path;
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif /* OSRM_ENGINE_ROUTING_INIT_HPP */
