#include "engine/plugins/isochrone.hpp"
#include "engine/edge_unpacker.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "util/timing_util.hpp"
#include "util/coordinate_calculation.hpp"
#include "engine/edge_unpacker.hpp"

#include <queue>
#include <iomanip>
#include <cmath>

namespace osrm
{
namespace engine
{
namespace plugins
{

Status IsochronePlugin::HandleRequest(const std::shared_ptr<datafacade::BaseDataFacade> facade,
                                      const api::IsochroneParameters &parameters,
                                      std::string &pbf_buffer) const
{

    const auto range = parameters.range;

    util::Coordinate startcoord{util::FloatLongitude{parameters.lon},
                                util::FloatLatitude{parameters.lat}};

    const auto MAX_SPEED_METERS_PER_SECOND = 200 / 3.6;
    const auto MAX_TRAVEL_DISTANCE_METERS = MAX_SPEED_METERS_PER_SECOND * parameters.range;

    const auto latitude_range = MAX_TRAVEL_DISTANCE_METERS /
                                (util::coordinate_calculation::detail::EARTH_RADIUS * M_PI * 2) *
                                360;

    const auto longitude_range =
        360 * MAX_TRAVEL_DISTANCE_METERS /
        ((util::coordinate_calculation::detail::EARTH_RADIUS * M_PI * 2) *
         cos(parameters.lat * util::coordinate_calculation::detail::DEGREE_TO_RAD));

    util::Coordinate southwest{
        util::FloatLongitude{static_cast<double>(parameters.lon - longitude_range)},
        util::FloatLatitude{static_cast<double>(parameters.lat - latitude_range)}};
    util::Coordinate northeast{
        util::FloatLongitude{static_cast<double>(parameters.lon + longitude_range)},
        util::FloatLatitude{static_cast<double>(parameters.lat + latitude_range)}};

    util::SimpleLogger().Write(logINFO) << "sw " << southwest;
    util::SimpleLogger().Write(logINFO) << "ne " << northeast;

    TIMER_START(GET_EDGES_TIMER);
    const auto edges = facade->GetEdgesInBox(southwest, northeast);
    TIMER_STOP(GET_EDGES_TIMER);
    util::SimpleLogger().Write(logINFO) << "Fetch RTree " << TIMER_MSEC(GET_EDGES_TIMER);

    auto startpoints = facade->NearestPhantomNodes(startcoord, 1);

    std::vector<NodeID> edge_based_nodes;
    for (const auto &edge : edges)
    {
        if (edge.forward_segment_id.enabled)
        {
            edge_based_nodes.push_back(edge.forward_segment_id.id);
        }
        if (edge.reverse_segment_id.enabled)
        {
            edge_based_nodes.push_back(edge.reverse_segment_id.id);
        }
    }
    // Struct to hold info on all the EdgeBasedNodes that are visible in our tile
    // When we create these, we insure that (source, target) and packed_geometry_id
    // are all pointed in the same direction.
    struct EdgeBasedNodeInfo
    {
        bool is_geometry_forward; // Is the geometry forward or reverse?
        unsigned packed_geometry_id;
        std::uint64_t shortest_weight;
        NodeID parent;
    };
    // Lookup table for edge-based-nodes
    std::unordered_map<NodeID, EdgeBasedNodeInfo> edge_based_node_info;

    TIMER_START(CONSTRUCT_LOOKUP);
    // Build an adjacency list for all the road segments visible in
    // the tile
    for (const auto &edge : edges)
    {
        if (edge.forward_segment_id.enabled)
        {
            if (edge_based_node_info.count(edge.forward_segment_id.id) == 0)
                edge_based_node_info[edge.forward_segment_id.id] = {
                    true,
                    edge.packed_geometry_id,
                    std::numeric_limits<std::uint64_t>::max(),
                    SPECIAL_NODEID};
        }
        if (edge.reverse_segment_id.enabled)
        {
            if (edge_based_node_info.count(edge.reverse_segment_id.id) == 0)
                edge_based_node_info[edge.reverse_segment_id.id] = {
                    false,
                    edge.packed_geometry_id,
                    std::numeric_limits<std::uint64_t>::max(),
                    SPECIAL_NODEID};
        }
    }
    TIMER_STOP(CONSTRUCT_LOOKUP);
    util::SimpleLogger().Write(logINFO) << "Create lookup table " << TIMER_MSEC(CONSTRUCT_LOOKUP);

    // Overall strategy:
    // 1. Do a depth-limited dijkstra search on the upgraph from the startnode
    // 2. Then, for any remaining unvisited nodes, perform a reverse dijkstra search until
    //    it connects with the searched space.  On connect, reverse all the parents back
    //    to the source

    // Step 1 - the forward Dijkstra search in the CH

    auto comparator = [&edge_based_node_info](const NodeID &lhs, const NodeID &rhs) -> bool {
        return edge_based_node_info[lhs].shortest_weight <
               edge_based_node_info[rhs].shortest_weight;
    };

    /***************************************************************/
    // This bit is the actual traversal
    TIMER_START(DO_SEARCH);

    std::priority_queue<NodeID,
                        std::vector<NodeID>,
                        std::function<bool(const NodeID &, const NodeID &)>>
        priority_queue(comparator);

    // Seed the priority queue
    for (const auto &phantom_node : startpoints)
    {
        if (phantom_node.phantom_node.forward_segment_id.enabled)
        {
            edge_based_node_info[phantom_node.phantom_node.forward_segment_id.id].shortest_weight =
                0;
            priority_queue.push(phantom_node.phantom_node.forward_segment_id.id);
        }
        if (phantom_node.phantom_node.reverse_segment_id.enabled)
        {
            edge_based_node_info[phantom_node.phantom_node.reverse_segment_id.id].shortest_weight =
                0;
            priority_queue.push(phantom_node.phantom_node.reverse_segment_id.id);
        }
    }

    // First, the simple forward search in the upgraph
    while (priority_queue.size() > 0)
    {
        const auto thisnode = priority_queue.top();
        priority_queue.pop();
        const auto weight_so_far = edge_based_node_info[thisnode].shortest_weight;

        for (const auto edge : facade->GetAdjacentEdgeRange(thisnode))
        {
            const auto &edgedata = facade->GetEdgeData(edge);
            if (edgedata.forward)
            {
                const auto target = facade->GetTarget(edge);
                if (edge_based_node_info.count(target) != 0)
                {
                    const auto newweight = weight_so_far + edgedata.weight;
                    if (newweight < edge_based_node_info[target].shortest_weight)
                    {
                        edge_based_node_info[target].shortest_weight = newweight;
                        edge_based_node_info[target].parent = thisnode;
                        priority_queue.push(target);
                    }
                }
            }
        }
    }
    TIMER_STOP(DO_SEARCH);
    util::SimpleLogger().Write(logINFO) << "Dijkstra forward search" << TIMER_MSEC(DO_SEARCH);
    /********************************************************************/

    // Now, for the magic reverse search
    // Add *all* the edge-based-node ids we haven't yet visited, and search the downgraph, stopping
    // when we hit a result from the upgraph

    /***************************************************************/
    // This bit is the actual traversal
    TIMER_START(DO_REVERSE_SEARCH);

    std::unordered_map<NodeID, NodeID> reverse_parents;
    std::unordered_map<NodeID, std::uint64_t> reverse_shortest;
    auto reverse_comparator = [&reverse_shortest](const NodeID &lhs, const NodeID &rhs) -> bool {
        return reverse_shortest[lhs] < reverse_shortest[rhs];
    };
    std::priority_queue<NodeID,
                        std::vector<NodeID>,
                        std::function<bool(const NodeID &, const NodeID &)>>
        reverse_priority_queue(reverse_comparator);
    for (const auto &nodeinfo : edge_based_node_info)
    {
        if (nodeinfo.second.parent == SPECIAL_NODEID)
        {
            reverse_priority_queue.push(nodeinfo.first);
            while (reverse_priority_queue.size() > 0)
            {
                const auto thisnode = priority_queue.top();
                priority_queue.pop();
                for (const auto edge : facade->GetAdjacentEdgeRange(thisnode))
                {
                    const auto &edgedata = facade->GetEdgeData(edge);
                    if (edgedata.backward)
                    {
                        const auto target = facade->GetTarget(edge);
                        // Check if we contact the forward graph
                        if (edge_based_node_info.count(target) != 0)
                        {
                            const auto targetinfo = edge_based_node_info[target];
                            if (targetinfo.parent != SPECIAL_NODEID)
                            {
                                // Contact! Unpack path
                            }
                        }
                        else
                        {
                            // Else, normal dijkstra
                        }
                    }
                }
            }
        }
    }
    util::SimpleLogger().Write(logINFO) << "Dijkstra reverse search"
                                        << TIMER_MSEC(DO_REVERSE_SEARCH);

    // Serialize it out
    std::stringstream buf;

    buf << "{\"type\":\"FeatureCollection\",\"features\":[";

    // TODO: now, unpacking.  This is just a quick demo
    bool first = true;
    for (const auto &nodeinfo : edge_based_node_info)
    {
        // Skip this node if it wasn't visited'
        if (nodeinfo.second.parent == SPECIAL_NODEID)
            continue;

        using EdgeData = typename datafacade::BaseDataFacade::EdgeData;

        std::vector<NodeID> unpacked_path;
        std::array<NodeID, 2> path{{nodeinfo.second.parent, nodeinfo.first}};
        UnpackCHPath(
            *facade,
            path.begin(),
            path.end(),
            [&unpacked_path](const std::pair<NodeID, NodeID> &edge, const EdgeData & /* data */) {
                unpacked_path.emplace_back(edge.first);
            });

        for (const auto pathnode : unpacked_path)
        {
            const auto &geomnodes =
                edge_based_node_info[pathnode].is_geometry_forward
                    ? facade->GetUncompressedForwardGeometry(nodeinfo.second.packed_geometry_id)
                    : facade->GetUncompressedReverseGeometry(nodeinfo.second.packed_geometry_id);

            if (!first)
                buf << ",";

            first = false;

            buf << "{\"type\":\"Feature\",\"properties\":{\"edge_based_node_id\":" << pathnode
                << "},\"geometry\":{\"type\":\"LineString\",\"coordinates\":[";

            bool firstcoord = true;
            for (const auto &nodeid : geomnodes)
            {
                if (!firstcoord)
                    buf << ",";
                firstcoord = false;

                const auto coordinate = facade->GetCoordinateOfNode(nodeid);
                buf << std::setprecision(12) << "[" << toFloating(coordinate.lon) << ","
                    << toFloating(coordinate.lat) << "]";
            }
            buf << "]}}";
        }
    }
    buf << "]}";

    pbf_buffer = buf.str();

    return Status::Ok;
}

} // plugins
} // engine
} // osrm
