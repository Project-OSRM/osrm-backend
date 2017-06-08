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
                heap.Insert(to, to_weight, node);
            }
            else if (!heap.WasRemoved(to) && to_weight < heap.GetKey(to))
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
void insertForwardSourceNode(const Facade &facade,
                             Heap &heap,
                             const PhantomNode &node,
                             const EdgeWeight initial_weight)
{ // Insert source node in forward direction into a forward heap
    // For the weights vector of the phantom node and fwd_segment_position = 4:
    //   0---1---2---3---(4-s==)5===6===end
    //      first_segment ^              ^ last_segment
    // source.fwd_segment_ratio = distance(4→s) / distance(4→5)
    const auto node_id = node.forward_segment_id.id;
    const auto geometry_index = facade.GetGeometryIndex(node_id);
    const auto weights = facade.GetUncompressedForwardWeights(geometry_index.id);
    const auto first_segment = weights.begin() + node.fwd_segment_position;
    const auto last_segment = weights.end();
    if (areSegmentsValid(first_segment, last_segment))
    {
        auto weight = sumSegmentValues(first_segment, last_segment, node.fwd_segment_ratio, 1.);
        heap.InsertVisited(node_id, initial_weight + weight, node_id);
        detail::relaxSourceEdges(facade, heap, node_id, initial_weight + weight);
    }
}

template <typename Facade, typename Heap>
void insertReverseSourceNode(const Facade &facade,
                             Heap &heap,
                             const PhantomNode &node,
                             const EdgeWeight initial_weight)
{ // Insert source node in reverse direction into a forward heap
    // For the weights vector of the phantom node and fwd_segment_position = 4:
    // end===6===5===4===3(=s--2)---1---0
    //  ^ last_segment         ^ first_segment
    // source.fwd_segment_ratio = distance(s→3) / distance(2→3)
    const auto node_id = node.reverse_segment_id.id;
    const auto geometry_index = facade.GetGeometryIndex(node_id);
    const auto weights = facade.GetUncompressedReverseWeights(geometry_index.id);
    const auto first_segment = weights.end() - node.fwd_segment_position - 1;
    const auto last_segment = weights.end();
    if (areSegmentsValid(first_segment, last_segment))
    {
        auto weight = sumSegmentValues(first_segment, last_segment, 1 - node.fwd_segment_ratio, 1.);
        heap.InsertVisited(node_id, initial_weight + weight, node_id);
        detail::relaxSourceEdges(facade, heap, node_id, initial_weight + weight);
    }
}

template <typename Facade, typename Heap>
void insertForwardTargetNode(const Facade &facade,
                             Heap &heap,
                             const PhantomNode &node,
                             const EdgeWeight initial_weight)
{ // Insert target node in forward direction into a backward heap
    // For the weights vector of the phantom node and fwd_segment_position = 4:
    //   0===1===2===3===(4=t--)5---6---end
    //   ^ first_segment        ^ last_segment
    // target.fwd_segment_ratio = distance(4→t) / distance(4→5)
    const auto node_id = node.forward_segment_id.id;
    const auto geometry_index = facade.GetGeometryIndex(node_id);
    const auto weights = facade.GetUncompressedForwardWeights(geometry_index.id);
    const auto first_segment = weights.begin();
    const auto last_segment = weights.begin() + node.fwd_segment_position + 1;
    if (areSegmentsValid(first_segment, last_segment))
    {
        auto weight = sumSegmentValues(first_segment, last_segment, 0., node.fwd_segment_ratio);
        heap.Insert(node_id, initial_weight + weight, node_id);
    }
}

template <typename Facade, typename Heap>
void insertReverseTargetNode(const Facade &facade,
                             Heap &heap,
                             const PhantomNode &node,
                             const EdgeWeight initial_weight)
{ // Insert target node in forward direction into a backward heap
    // For the weights vector of the phantom node and fwd_segment_position = 4:
    // end---6---5---4---3(-t==2)===1===0
    //      last_segment ^              ^ first_segment
    // target.fwd_segment_ratio = distance(t→3) / distance(2→3)
    const auto node_id = node.reverse_segment_id.id;
    const auto geometry_index = facade.GetGeometryIndex(node_id);
    const auto weights = facade.GetUncompressedReverseWeights(geometry_index.id);
    const auto first_segment = weights.begin();
    const auto last_segment = weights.end() - node.fwd_segment_position;
    if (areSegmentsValid(first_segment, last_segment))
    {
        auto weight = sumSegmentValues(first_segment, last_segment, 0., 1 - node.fwd_segment_ratio);
        heap.Insert(node_id, initial_weight + weight, node_id);
    }
}

inline bool nodesOverlapInForwardDirection(const PhantomNode &source, const PhantomNode &target)
{ // Check if source and target phantom nodes overlap in forward direction as
    //   0---1-s==2===3===4=t---5---6---end
    return source.forward_segment_id.enabled && target.forward_segment_id.enabled &&
           (source.forward_segment_id.id == target.forward_segment_id.id) &&
           (std::tie(source.fwd_segment_position, source.fwd_segment_ratio) <=
            std::tie(target.fwd_segment_position, target.fwd_segment_ratio));
}

inline bool nodesOverlapInReverseDirection(const PhantomNode &source, const PhantomNode &target)
{ // Check if source and target phantom nodes overlap in reverse direction as
    // end---6-t==5===4===3=s---2---1---0
    return source.reverse_segment_id.enabled && target.reverse_segment_id.enabled &&
           (source.reverse_segment_id.id == target.reverse_segment_id.id) &&
           (std::tie(source.fwd_segment_position, source.fwd_segment_ratio) >=
            std::tie(target.fwd_segment_position, target.fwd_segment_ratio));
}

template <typename Facade>
auto getForwardSingleNodePath(const Facade &facade,
                              const PhantomNode &source,
                              const PhantomNode &target,
                              const EdgeWeight weight_to_source)
{ // Compute a single node path case in forward direction
    // for source.fwd_segment_position = 1 and target.fwd_segment_position = 4
    //   0---1-s==2===3===4=t---5---6---end
    //   ^ first_segment        ^ last_segment
    const auto node_id = source.forward_segment_id.id;
    const auto geometry_index = facade.GetGeometryIndex(node_id);
    const auto weights = facade.GetUncompressedForwardWeights(geometry_index.id);
    const auto first_segment = weights.begin() + source.fwd_segment_position;
    const auto last_segment = weights.begin() + target.fwd_segment_position + 1;
    if (areSegmentsValid(first_segment, last_segment))
    {
        const auto weight = sumSegmentValues(
            first_segment, last_segment, source.fwd_segment_ratio, target.fwd_segment_ratio);
        return std::make_pair(node_id, weight_to_source + weight);
    }
    return std::pair<NodeID, EdgeWeight>{SPECIAL_NODEID, INVALID_EDGE_WEIGHT};
}

template <typename Facade>
auto getReverseSingleNodePath(const Facade &facade,
                              const PhantomNode &source,
                              const PhantomNode &target,
                              const EdgeWeight weight_to_reverse)
{ // Compute a single node path case in reverse direction
    // for source.fwd_segment_position = 4 and target.fwd_segment_position = 1
    // end---6-t==5===4===3=s---2---1---0
    //       ^ last_segment     ^ first_segment
    const auto node_id = source.reverse_segment_id.id;
    const auto geometry_index = facade.GetGeometryIndex(node_id);
    const auto weights = facade.GetUncompressedReverseWeights(geometry_index.id);
    const auto first_segment = weights.end() - source.fwd_segment_position - 1;
    const auto last_segment = weights.end() - target.fwd_segment_position;
    if (areSegmentsValid(first_segment, last_segment))
    {
        const auto weight = sumSegmentValues(first_segment,
                                             last_segment,
                                             1. - source.fwd_segment_ratio,
                                             1. - target.fwd_segment_ratio);
        return std::make_pair(node_id, weight_to_reverse + weight);
    }
    return std::pair<NodeID, EdgeWeight>{SPECIAL_NODEID, INVALID_EDGE_WEIGHT};
}

template <typename Facade, typename Heap>
auto insertNodesInHeaps(const Facade &facade,
                        Heap &forward_heap,
                        Heap &reverse_heap,
                        const PhantomNodes &nodes,
                        const EdgeWeight weight_to_forward_source,
                        const EdgeWeight weight_to_reverse_source,
                        bool use_forward_target,
                        bool use_reverse_target)
{
    const auto &source = nodes.source_phantom;
    const auto &target = nodes.target_phantom;

    // Get usage flags for {forward,reverse} directions of {source,target} nodes
    bool use_forward_source = weight_to_forward_source != INVALID_EDGE_WEIGHT;
    bool use_reverse_source = weight_to_reverse_source != INVALID_EDGE_WEIGHT;
    use_forward_source &= source.forward_segment_id.enabled;
    use_reverse_source &= source.reverse_segment_id.enabled;
    use_forward_target &= target.forward_segment_id.enabled;
    use_reverse_target &= target.reverse_segment_id.enabled;

    // Get flags for single edge-based-node paths
    const bool cross_forward =
        nodesOverlapInForwardDirection(source, target) && use_forward_source && use_forward_target;
    const bool cross_reverse =
        nodesOverlapInReverseDirection(source, target) && use_reverse_source && use_reverse_target;

    if (use_forward_source && !cross_forward)
    {
        insertForwardSourceNode(facade, forward_heap, source, weight_to_forward_source);
    }

    if (use_reverse_source && !cross_reverse)
    {
        insertReverseSourceNode(facade, forward_heap, source, weight_to_reverse_source);
    }

    if (use_forward_target && !cross_forward)
    {
        insertForwardTargetNode(facade, reverse_heap, target, 0);
    }

    if (use_reverse_target && !cross_reverse)
    {
        insertReverseTargetNode(facade, reverse_heap, target, 0);
    }

    if (cross_forward)
    {
        return getForwardSingleNodePath(facade, source, target, weight_to_forward_source);
    }
    else if (cross_reverse)
    {
        return getReverseSingleNodePath(facade, source, target, weight_to_reverse_source);
    }

    return std::pair<NodeID, EdgeWeight>{SPECIAL_NODEID, INVALID_EDGE_WEIGHT};
}

template <typename Facade, typename Heap>
auto insertNodesInHeaps(const Facade &facade,
                        Heap &forward_heap,
                        Heap &reverse_heap,
                        const PhantomNodes &nodes)
{
    return insertNodesInHeaps(facade, forward_heap, reverse_heap, nodes, 0, 0, true, true);
}

template <typename Facade, typename Heap>
auto insertNodesInHeaps(const Facade &facade,
                        Heap &forward_heap,
                        Heap &reverse_heap,
                        const NodeID source_node,
                        const NodeID target_node)
{
    PhantomNode source_phantom, target_phantom;
    const auto is_forward_source = facade.GetGeometryIndex(source_node).forward;
    const auto is_forward_target = facade.GetGeometryIndex(target_node).forward;

    if (source_node == target_node && is_forward_source == is_forward_target)
    { // Single-node path of weight 0
        return std::pair<NodeID, EdgeWeight>{source_node, 0};
    }

    forward_heap.Insert(source_node, 0, source_node);
    reverse_heap.Insert(target_node, 0, target_node);

    return std::pair<NodeID, EdgeWeight>{SPECIAL_NODEID, INVALID_EDGE_WEIGHT};
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif /* OSRM_ENGINE_ROUTING_INIT_HPP */
