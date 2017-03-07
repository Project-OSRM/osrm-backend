#ifndef OSRM_ENGINE_ROUTING_BASE_MLD_HPP
#define OSRM_ENGINE_ROUTING_BASE_MLD_HPP

#include "engine/algorithm.hpp"
#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/search_engine_data.hpp"

#include "util/typedefs.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{
namespace mld
{

namespace
{
// Unrestricted search (Args is const PhantomNodes &):
//   * use partition.GetQueryLevel to find the node query level based on source and target phantoms
//   * allow to traverse all cells
LevelID getNodeQureyLevel(const partition::MultiLevelPartitionView &partition,
                          NodeID node,
                          const PhantomNodes &phantom_nodes)
{
    auto level = [&partition, node](const SegmentID &source, const SegmentID &target) {
        if (source.enabled && target.enabled)
            return partition.GetQueryLevel(source.id, target.id, node);
        return INVALID_LEVEL_ID;
    };
    return std::min(std::min(level(phantom_nodes.source_phantom.forward_segment_id,
                                   phantom_nodes.target_phantom.forward_segment_id),
                             level(phantom_nodes.source_phantom.forward_segment_id,
                                   phantom_nodes.target_phantom.reverse_segment_id)),
                    std::min(level(phantom_nodes.source_phantom.reverse_segment_id,
                                   phantom_nodes.target_phantom.forward_segment_id),
                             level(phantom_nodes.source_phantom.reverse_segment_id,
                                   phantom_nodes.target_phantom.reverse_segment_id)));
}

bool checkParentCellRestriction(CellID, const PhantomNodes &) { return true; }

// Restricted search (Args is LevelID, CellID):
//   * use the fixed level for queries
//   * check if the node cell is the same as the specified parent onr
LevelID getNodeQureyLevel(const partition::MultiLevelPartitionView &, NodeID, LevelID level, CellID)
{
    return level;
}

bool checkParentCellRestriction(CellID cell, LevelID, CellID parent) { return cell == parent; }
}

template <bool DIRECTION, typename... Args>
void routingStep(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::MLD> &facade,
                 SearchEngineData::MultiLayerDijkstraHeap &forward_heap,
                 SearchEngineData::MultiLayerDijkstraHeap &reverse_heap,
                 NodeID &middle_node,
                 EdgeWeight &path_upper_bound,
                 Args... args)
{
    const auto &partition = facade.GetMultiLevelPartition();
    const auto &cells = facade.GetCellStorage();

    const auto node = forward_heap.DeleteMin();
    const auto weight = forward_heap.GetKey(node);

    // Upper bound for the path source -> target with
    // weight(source -> node) = weight weight(to -> target) ≤ reverse_weight
    // is weight + reverse_weight
    // More tighter upper bound requires additional condition reverse_heap.WasRemoved(to)
    // with weight(to -> target) = reverse_weight and all weights ≥ 0
    if (reverse_heap.WasInserted(node))
    {
        auto reverse_weight = reverse_heap.GetKey(node);
        auto path_weight = weight + reverse_weight;
        if (path_weight >= 0 && path_weight < path_upper_bound)
        {
            middle_node = node;
            path_upper_bound = path_weight;
        }
    }

    const auto level = getNodeQureyLevel(partition, node, args...);

    if (level >= 1 && !forward_heap.GetData(node).from_clique_arc)
    {
        if (DIRECTION == FORWARD_DIRECTION)
        {
            // Shortcuts in forward direction
            const auto &cell = cells.GetCell(level, partition.GetCell(level, node));
            auto destination = cell.GetDestinationNodes().begin();
            for (auto shortcut_weight : cell.GetOutWeight(node))
            {
                BOOST_ASSERT(destination != cell.GetDestinationNodes().end());
                const NodeID to = *destination;
                if (shortcut_weight != INVALID_EDGE_WEIGHT && node != to)
                {
                    const EdgeWeight to_weight = weight + shortcut_weight;
                    if (!forward_heap.WasInserted(to))
                    {
                        forward_heap.Insert(to, to_weight, {node, true});
                    }
                    else if (to_weight < forward_heap.GetKey(to))
                    {
                        forward_heap.GetData(to) = {node, true};
                        forward_heap.DecreaseKey(to, to_weight);
                    }
                }
                ++destination;
            }
        }
        else
        {
            // Shortcuts in backward direction
            const auto &cell = cells.GetCell(level, partition.GetCell(level, node));
            auto source = cell.GetSourceNodes().begin();
            for (auto shortcut_weight : cell.GetInWeight(node))
            {
                BOOST_ASSERT(source != cell.GetSourceNodes().end());
                const NodeID to = *source;
                if (shortcut_weight != INVALID_EDGE_WEIGHT && node != to)
                {
                    const EdgeWeight to_weight = weight + shortcut_weight;
                    if (!forward_heap.WasInserted(to))
                    {
                        forward_heap.Insert(to, to_weight, {node, true});
                    }
                    else if (to_weight < forward_heap.GetKey(to))
                    {
                        forward_heap.GetData(to) = {node, true};
                        forward_heap.DecreaseKey(to, to_weight);
                    }
                }
                ++source;
            }
        }
    }

    // Boundary edges
    for (const auto edge : facade.GetBorderEdgeRange(level, node))
    {
        const auto &edge_data = facade.GetEdgeData(edge);
        if (DIRECTION == FORWARD_DIRECTION ? edge_data.forward : edge_data.backward)
        {
            const NodeID to = facade.GetTarget(edge);

            if (checkParentCellRestriction(partition.GetCell(level + 1, to), args...))
            {
                BOOST_ASSERT_MSG(edge_data.weight > 0, "edge_weight invalid");
                const EdgeWeight to_weight = weight + edge_data.weight;

                if (!forward_heap.WasInserted(to))
                {
                    forward_heap.Insert(to, to_weight, {node, false});
                }
                else if (to_weight < forward_heap.GetKey(to))
                {
                    forward_heap.GetData(to) = {node, false};
                    forward_heap.DecreaseKey(to, to_weight);
                }
            }
        }
    }
}

template <typename... Args>
std::tuple<EdgeWeight, NodeID, NodeID, std::vector<EdgeID>>
search(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::MLD> &facade,
       SearchEngineData::MultiLayerDijkstraHeap &forward_heap,
       SearchEngineData::MultiLayerDijkstraHeap &reverse_heap,
       Args... args)
{

    const auto &partition = facade.GetMultiLevelPartition();

    BOOST_ASSERT(!forward_heap.Empty() && forward_heap.MinKey() < INVALID_EDGE_WEIGHT);
    BOOST_ASSERT(!reverse_heap.Empty() && reverse_heap.MinKey() < INVALID_EDGE_WEIGHT);

    // run two-Target Dijkstra routing step.
    NodeID middle = SPECIAL_NODEID;
    EdgeWeight weight = INVALID_EDGE_WEIGHT;
    EdgeWeight forward_heap_min = forward_heap.MinKey();
    EdgeWeight reverse_heap_min = reverse_heap.MinKey();
    while (forward_heap.Size() + reverse_heap.Size() > 0 &&
           forward_heap_min + reverse_heap_min < weight)
    {
        if (!forward_heap.Empty())
        {
            routingStep<FORWARD_DIRECTION>(
                facade, forward_heap, reverse_heap, middle, weight, args...);
            if (!forward_heap.Empty())
                forward_heap_min = forward_heap.MinKey();
        }
        if (!reverse_heap.Empty())
        {
            routingStep<REVERSE_DIRECTION>(
                facade, reverse_heap, forward_heap, middle, weight, args...);
            if (!reverse_heap.Empty())
                reverse_heap_min = reverse_heap.MinKey();
        }
    };

    // No path found for both target nodes?
    if (weight == INVALID_EDGE_WEIGHT || SPECIAL_NODEID == middle)
    {
        return std::make_tuple(
            INVALID_EDGE_WEIGHT, SPECIAL_NODEID, SPECIAL_NODEID, std::vector<EdgeID>());
    }

    // Get packed path as edges {from node ID, to node ID, edge ID}
    std::vector<std::tuple<NodeID, NodeID, bool>> packed_path;
    NodeID current_node = middle, parent_node = forward_heap.GetData(middle).parent;
    while (parent_node != current_node)
    {
        const auto &data = forward_heap.GetData(current_node);
        packed_path.push_back(std::make_tuple(parent_node, current_node, data.from_clique_arc));
        current_node = parent_node;
        parent_node = forward_heap.GetData(parent_node).parent;
    }
    std::reverse(std::begin(packed_path), std::end(packed_path));
    const NodeID source_node = current_node;

    current_node = middle, parent_node = reverse_heap.GetData(middle).parent;
    while (parent_node != current_node)
    {
        const auto &data = reverse_heap.GetData(current_node);
        packed_path.push_back(std::make_tuple(current_node, parent_node, data.from_clique_arc));
        current_node = parent_node;
        parent_node = reverse_heap.GetData(parent_node).parent;
    }
    const NodeID target_node = current_node;

    // Unpack path
    std::vector<EdgeID> unpacked_path;
    unpacked_path.reserve(packed_path.size());
    for (auto const &packed_edge : packed_path)
    {
        NodeID source, target;
        bool overlay_edge;
        std::tie(source, target, overlay_edge) = packed_edge;
        if (!overlay_edge)
        { // a base graph edge
            unpacked_path.push_back(facade.FindEdge(source, target));
        }
        else
        { // an overlay graph edge
            LevelID level = getNodeQureyLevel(partition, source, args...);
            CellID parent_cell_id = partition.GetCell(level, source);
            BOOST_ASSERT(parent_cell_id == partition.GetCell(level, target));

            LevelID sublevel = level - 1;

            // Here heaps can be reused, let's go deeper!
            forward_heap.Clear();
            reverse_heap.Clear();
            forward_heap.Insert(source, 0, {source});
            reverse_heap.Insert(target, 0, {target});

            // TODO: when structured bindings will be allowed change to
            // auto [subpath_weight, subpath_source, subpath_target, subpath] = ...
            EdgeWeight subpath_weight;
            NodeID subpath_source, subpath_target;
            std::vector<EdgeID> subpath;
            std::tie(subpath_weight, subpath_source, subpath_target, subpath) =
                search(facade, forward_heap, reverse_heap, sublevel, parent_cell_id);
            BOOST_ASSERT(!subpath.empty());
            BOOST_ASSERT(subpath_source == source);
            BOOST_ASSERT(subpath_target == target);
            unpacked_path.insert(unpacked_path.end(), subpath.begin(), subpath.end());
        }
    }

    return std::make_tuple(weight, source_node, target_node, std::move(unpacked_path));
}

} // namespace mld
} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif // OSRM_ENGINE_ROUTING_BASE_MLD_HPP
