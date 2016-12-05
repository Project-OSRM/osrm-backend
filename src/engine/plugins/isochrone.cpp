#include "engine/plugins/isochrone.hpp"
#include "engine/edge_unpacker.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "util/timing_util.hpp"
#include "util/coordinate_calculation.hpp"

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
    // Struct to hold info on all the EdgeBasedNodes that are visible in our tile
    // When we create these, we insure that (source, target) and packed_geometry_id
    // are all pointed in the same direction.
    struct EdgeBasedNodeInfo
    {
        bool is_geometry_forward; // Is the geometry forward or reverse?
        unsigned packed_geometry_id;
    };
    // Lookup table for edge-based-nodes
    std::unordered_map<NodeID, EdgeBasedNodeInfo> edge_based_node_info;

    struct SegmentData
    {
        NodeID target_node;
        EdgeID edge_based_node_id;
    };

    std::unordered_map<NodeID, std::vector<SegmentData>> node_based_graph;
    // Reserve enough space for unique edge-based-nodes on every edge.
    // Only a tile with all unique edges will use this much, but
    // it saves us a bunch of re-allocations during iteration.
    node_based_graph.reserve(edges.size() * 2);

    TIMER_START(CONSTRUCT_LOOKUP);
    // Build an adjacency list for all the road segments visible in
    // the tile
    for (const auto &edge : edges)
    {
        if (edge.forward_segment_id.enabled)
        {
            // operator[] will construct an empty vector at [edge.u] if there is no value.
            node_based_graph[edge.u].push_back({edge.v, edge.forward_segment_id.id});
            if (edge_based_node_info.count(edge.forward_segment_id.id) == 0)
            {
                edge_based_node_info[edge.forward_segment_id.id] = {true, edge.packed_geometry_id};
            }
            else
            {
                BOOST_ASSERT(edge_based_node_info[edge.forward_segment_id.id].is_geometry_forward ==
                             true);
                BOOST_ASSERT(edge_based_node_info[edge.forward_segment_id.id].packed_geometry_id ==
                             edge.packed_geometry_id);
            }
        }
        if (edge.reverse_segment_id.enabled)
        {
            node_based_graph[edge.v].push_back({edge.u, edge.reverse_segment_id.id});
            if (edge_based_node_info.count(edge.reverse_segment_id.id) == 0)
            {
                edge_based_node_info[edge.reverse_segment_id.id] = {false, edge.packed_geometry_id};
            }
            else
            {
                BOOST_ASSERT(edge_based_node_info[edge.reverse_segment_id.id].is_geometry_forward ==
                             false);
                BOOST_ASSERT(edge_based_node_info[edge.reverse_segment_id.id].packed_geometry_id ==
                             edge.packed_geometry_id);
            }
        }
    }
    TIMER_STOP(CONSTRUCT_LOOKUP);
    util::SimpleLogger().Write(logINFO) << "Create lookup table " << TIMER_MSEC(CONSTRUCT_LOOKUP);

    // Given a turn:
    //     u---v
    //         |
    //         w
    //  uv is the "approach"
    //  vw is the "exit"
    std::vector<contractor::QueryEdge::EdgeData> unpacked_shortcut;
    std::vector<EdgeWeight> approach_weight_vector;

    // Make sure we traverse the startnodes in a consistent order
    // to ensure identical PBF encoding on all platforms.
    std::vector<NodeID> sorted_startnodes;
    sorted_startnodes.reserve(node_based_graph.size());
    for (const auto &startnode : node_based_graph)
        sorted_startnodes.push_back(startnode.first);
    std::sort(sorted_startnodes.begin(), sorted_startnodes.end());

    struct SimpleEdgeData
    {
        NodeID target;
        std::uint64_t weight;
        SimpleEdgeData(NodeID target_, std::uint64_t weight_) : target(target_), weight(weight_) {}
    };

    // Our edge-based-graph
    std::unordered_map<NodeID, std::vector<SimpleEdgeData>> edge_based_graph;

    TIMER_START(CONSTRUCT_GRAPH);
    // Look at every node in the directed graph we created
    for (const auto &startnode : sorted_startnodes)
    {
        const auto &nodedata = node_based_graph[startnode];
        // For all the outgoing edges from the node
        for (const auto &approachedge : nodedata)
        {
            // If the target of this edge doesn't exist in our directed
            // graph, it's probably outside the tile, so we can skip it
            if (node_based_graph.count(approachedge.target_node) == 0)
            {
                continue;
            }

            // For each of the outgoing edges from our target coordinate
            for (const auto &exit_edge : node_based_graph[approachedge.target_node])
            {
                // If the next edge has the same edge_based_node_id, then it's
                // not a turn, so skip it
                if (approachedge.edge_based_node_id == exit_edge.edge_based_node_id)
                    continue;

                // Skip u-turns
                if (startnode == exit_edge.target_node)
                    continue;

                // Find the connection between our source road and the target node
                // Since we only want to find direct edges, we cannot check shortcut edges here.
                // Otherwise we might find a forward edge even though a shorter backward edge
                // exists (due to oneways).
                //
                // a > - > - > - b
                // |             |
                // |------ c ----|
                //
                // would offer a backward edge at `b` to `a` (due to the oneway from a to b)
                // but could also offer a shortcut (b-c-a) from `b` to `a` which is longer.
                EdgeID smaller_edge_id =
                    facade->FindSmallestEdge(approachedge.edge_based_node_id,
                                             exit_edge.edge_based_node_id,
                                             [](const contractor::QueryEdge::EdgeData &data) {
                                                 return data.forward && !data.shortcut;
                                             });

                // Depending on how the graph is constructed, we might have to look for
                // a backwards edge instead.  They're equivalent, just one is available for
                // a forward routing search, and one is used for the backwards dijkstra
                // steps.  Their weight should be the same, we can use either one.
                // If we didn't find a forward edge, try for a backward one
                if (SPECIAL_EDGEID == smaller_edge_id)
                {
                    smaller_edge_id =
                        facade->FindSmallestEdge(exit_edge.edge_based_node_id,
                                                 approachedge.edge_based_node_id,
                                                 [](const contractor::QueryEdge::EdgeData &data) {
                                                     return data.backward && !data.shortcut;
                                                 });
                }

                // If no edge was found, it means that there's no connection between these
                // nodes, due to oneways or turn restrictions. Given the edge-based-nodes
                // that we're examining here, we *should* only find directly-connected
                // edges, not shortcuts
                if (smaller_edge_id != SPECIAL_EDGEID)
                {
                    const auto &data = facade->GetEdgeData(smaller_edge_id);
                    BOOST_ASSERT_MSG(!data.shortcut, "Connecting edge must not be a shortcut");

                    edge_based_graph[approachedge.edge_based_node_id].emplace_back(
                        exit_edge.edge_based_node_id, data.weight);
                }
            }
        }
    }
    TIMER_STOP(CONSTRUCT_GRAPH);
    util::SimpleLogger().Write(logINFO) << "Building graph" << TIMER_MSEC(CONSTRUCT_GRAPH);

    // Begin the isochrone search

    // Structure to hold the parent nodes
    std::unordered_map<NodeID, NodeID> parent;
    std::unordered_map<NodeID, std::uint64_t> cheapest_path_so_far;

    auto comparator = [&cheapest_path_so_far](const NodeID &lhs, const NodeID &rhs) {
        return cheapest_path_so_far.at(lhs) < cheapest_path_so_far.at(rhs);
    };

    /***************************************************************/
    // This bit is the actual traversal
    TIMER_START(DO_SEARCH);

    std::priority_queue<NodeID,
                        std::vector<NodeID>,
                        std::function<bool(const NodeID &, const NodeID &)>>
        priority_queue(comparator);

    for (const auto &phantom_node : startpoints)
    {
        if (phantom_node.phantom_node.forward_segment_id.enabled)
        {
            cheapest_path_so_far[phantom_node.phantom_node.forward_segment_id.id] = 0;
            priority_queue.push(phantom_node.phantom_node.forward_segment_id.id);
        }
        if (phantom_node.phantom_node.reverse_segment_id.enabled)
        {
            cheapest_path_so_far[phantom_node.phantom_node.reverse_segment_id.id] = 0;
            priority_queue.push(phantom_node.phantom_node.reverse_segment_id.id);
        }
    }

    while (priority_queue.size() > 0)
    {
        const auto thisnode = priority_queue.top();
        priority_queue.pop();
        const auto weight_so_far = cheapest_path_so_far[thisnode];

        for (const auto &edge : edge_based_graph[thisnode])
        {
            // If the target hasn't been visited yet, or if we find a shorter path
            if (cheapest_path_so_far.count(edge.target) == 0 ||
                weight_so_far + edge.weight < cheapest_path_so_far[edge.target])
            {
                if (weight_so_far + edge.weight < range * 10)
                {
                    cheapest_path_so_far[edge.target] = weight_so_far + edge.weight;
                    parent[edge.target] = thisnode;
                    priority_queue.push(edge.target);
                }
            }

            // TODO: if we hit the edge of our graph, we may need to extend it by
            // fetching a tile
        }
    }
    TIMER_STOP(DO_SEARCH);
    util::SimpleLogger().Write(logINFO) << "Dijkstra search" << TIMER_MSEC(DO_SEARCH);
    /********************************************************************/

    // Serialize it out
    std::stringstream buf;

    buf << "{\"type\":\"FeatureCollection\",\"features\":[";

    // TODO: now, unpacking.  This is just a quick demo
    bool first = true;
    for (const auto &p : parent)
    {

        if (edge_based_node_info.count(p.first) > 0)
        {
            const auto &info = edge_based_node_info[p.first];
            const auto &geomnodes =
                info.is_geometry_forward
                    ? facade->GetUncompressedForwardGeometry(info.packed_geometry_id)
                    : facade->GetUncompressedReverseGeometry(info.packed_geometry_id);

            if (!first)
                buf << ",";

            first = false;

            buf << "{\"type\":\"Feature\",\"properties\":{\"edge_based_node_id\":" << p.first
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
