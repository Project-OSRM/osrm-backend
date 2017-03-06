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

template <bool DIRECTION>
void routingStep(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::MLD> &facade,
                 const partition::MultiLevelPartitionView &partition,
                 const partition::CellStorageView &cells,
                 SearchEngineData::MultiLayerDijkstraHeap &forward_heap,
                 SearchEngineData::MultiLayerDijkstraHeap &reverse_heap,
                 const std::pair<LevelID, CellID> &parent_cell,
                 NodeID &middle_node,
                 EdgeWeight &path_upper_bound,
                 EdgeWeight &forward_upper_bound,
                 EdgeWeight &reverse_upper_bound)
{
    const auto node = forward_heap.DeleteMin();
    const auto weight = forward_heap.GetKey(node);

    auto update_upper_bounds = [&](NodeID to, EdgeWeight forward_weight, EdgeWeight edge_weight) {
        if (reverse_heap.WasInserted(to) && reverse_heap.WasRemoved(to))
        {
            auto reverse_weight = reverse_heap.GetKey(to);
            auto path_weight = forward_weight + edge_weight + reverse_weight;
            BOOST_ASSERT(path_weight >= 0);
            if (path_weight < path_upper_bound)
            {
                middle_node = to;
                path_upper_bound = path_weight;
                forward_upper_bound = forward_weight + edge_weight;
                reverse_upper_bound = reverse_weight + edge_weight;
            }
        }
    };

    const auto &node_data = forward_heap.GetData(node);
    const auto level = node_data.level;
    const auto check_overlay_edges =
        (level >= 1) &&                        // only if at least the first level and
        (node_data.parent == node ||           //   is the first point of the path
         node_data.edge_id != SPECIAL_EDGEID); //   or an overlay entreé point

    // Edge case: single node path
    update_upper_bounds(node, weight, 0);

    if (check_overlay_edges)
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
                        forward_heap.Insert(to, to_weight, {node, level});
                        update_upper_bounds(to, weight, shortcut_weight);
                    }
                    else if (to_weight < forward_heap.GetKey(to))
                    {
                        forward_heap.GetData(to) = {node, level};
                        forward_heap.DecreaseKey(to, to_weight);
                        update_upper_bounds(to, weight, shortcut_weight);
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
                        forward_heap.Insert(to, to_weight, {node, level});
                        update_upper_bounds(to, weight, shortcut_weight);
                    }
                    else if (to_weight < forward_heap.GetKey(to))
                    {
                        forward_heap.GetData(to) = {node, level};
                        forward_heap.DecreaseKey(to, to_weight);
                        update_upper_bounds(to, weight, shortcut_weight);
                    }
                }
                ++source;
            }
        }
    }

    // Boundary edges
    for (const auto edge : facade.GetAdjacentEdgeRange(node))
    {
        const auto &edge_data = facade.GetEdgeData(edge);
        if (DIRECTION == FORWARD_DIRECTION ? edge_data.forward : edge_data.backward)
        {
            const NodeID to = facade.GetTarget(edge);
            const auto to_level =
                std::min(parent_cell.first, partition.GetHighestDifferentLevel(node, to));

            if ( // Routing is unrestricted or restricted to the highest level cell
                (parent_cell.second == INVALID_CELL_ID ||
                 parent_cell.second == partition.GetCell(parent_cell.first + 1, to)) &&
                // "Never-go-down" at border edges
                to_level >= level)
            {
                BOOST_ASSERT_MSG(edge_data.weight > 0, "edge_weight invalid");
                const EdgeWeight to_weight = weight + edge_data.weight;

                if (!forward_heap.WasInserted(to))
                {
                    forward_heap.Insert(to, to_weight, {node, to_level, edge});
                    update_upper_bounds(to, weight, edge_data.weight);
                }
                else if (to_weight < forward_heap.GetKey(to))
                {
                    forward_heap.GetData(to) = {node, to_level, edge};
                    forward_heap.DecreaseKey(to, to_weight);
                    update_upper_bounds(to, weight, edge_data.weight);
                }
            }
        }
    }
}

auto search(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::MLD> &facade,
            const partition::MultiLevelPartitionView &partition,
            const partition::CellStorageView &cells,
            SearchEngineData::MultiLayerDijkstraHeap &forward_heap,
            SearchEngineData::MultiLayerDijkstraHeap &reverse_heap,
            const std::pair<LevelID, CellID> &parent_cell)
{
    // run two-Target Dijkstra routing step.
    NodeID middle = SPECIAL_NODEID;
    EdgeWeight weight = INVALID_EDGE_WEIGHT;
    EdgeWeight forward_search_radius = INVALID_EDGE_WEIGHT;
    EdgeWeight reverse_search_radius = INVALID_EDGE_WEIGHT;
    bool progress;
    do
    {
        progress = false;
        if (!forward_heap.Empty() && (forward_heap.MinKey() < forward_search_radius))
        {
            progress = true;
            routingStep<FORWARD_DIRECTION>(facade,
                                           partition,
                                           cells,
                                           forward_heap,
                                           reverse_heap,
                                           parent_cell,
                                           middle,
                                           weight,
                                           forward_search_radius,
                                           reverse_search_radius);
        }
        if (!reverse_heap.Empty() && (reverse_heap.MinKey() < reverse_search_radius))
        {
            progress = true;
            routingStep<REVERSE_DIRECTION>(facade,
                                           partition,
                                           cells,
                                           reverse_heap,
                                           forward_heap,
                                           parent_cell,
                                           middle,
                                           weight,
                                           reverse_search_radius,
                                           forward_search_radius);
        }
    } while (progress);

    // No path found for both target nodes?
    if (weight == INVALID_EDGE_WEIGHT || SPECIAL_NODEID == middle)
    {
        return std::make_tuple(
            INVALID_EDGE_WEIGHT, SPECIAL_NODEID, SPECIAL_NODEID, std::vector<EdgeID>());
    }

    // Get packed path as edges {level, from node ID, to node ID, edge ID}
    std::vector<std::tuple<LevelID, NodeID, NodeID, EdgeID>> packed_path;
    NodeID current_node = middle, parent_node = forward_heap.GetData(middle).parent;
    while (parent_node != current_node)
    {
        const auto &data = forward_heap.GetData(current_node);
        packed_path.push_back(std::make_tuple(data.level, parent_node, current_node, data.edge_id));
        current_node = parent_node;
        parent_node = forward_heap.GetData(parent_node).parent;
    }
    std::reverse(std::begin(packed_path), std::end(packed_path));
    const NodeID source_node = current_node;

    current_node = middle, parent_node = reverse_heap.GetData(middle).parent;
    while (parent_node != current_node)
    {
        const auto &data = reverse_heap.GetData(current_node);
        packed_path.push_back(std::make_tuple(data.level, current_node, parent_node, data.edge_id));
        current_node = parent_node;
        parent_node = reverse_heap.GetData(parent_node).parent;
    }
    const NodeID target_node = current_node;

    // Unpack path
    std::vector<EdgeID> unpacked_path;
    unpacked_path.reserve(packed_path.size());
    for (auto &packed_edge : packed_path)
    {
        LevelID level;
        NodeID source, target;
        EdgeID edge_id;
        std::tie(level, source, target, edge_id) = packed_edge;
        if (edge_id != SPECIAL_EDGEID)
        { // a base graph edge
            unpacked_path.push_back(edge_id);
        }
        else
        { // an overlay graph edge
            LevelID sublevel = level - 1;
            CellID parent_cell_id = partition.GetCell(level, source);
            BOOST_ASSERT(parent_cell_id == partition.GetCell(level, target));

            // Here heaps can be reused, let's go deeper!
            forward_heap.Clear();
            reverse_heap.Clear();
            forward_heap.Insert(source, 0, {source, sublevel});
            reverse_heap.Insert(target, 0, {target, sublevel});

            // TODO: when structured bindings will be allowed change to
            // auto [subpath_weight, subpath_source, subpath_target, subpath] = ...
            EdgeWeight subpath_weight;
            NodeID subpath_source, subpath_target;
            std::vector<EdgeID> subpath;
            std::tie(subpath_weight, subpath_source, subpath_target, subpath) = search(
                facade, partition, cells, forward_heap, reverse_heap, {sublevel, parent_cell_id});
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
