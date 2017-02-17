#include "engine/plugins/isochrone.hpp"
#include "engine/edge_unpacker.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "util/timing_util.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/log.hpp"

#include <queue>
#include <iomanip>
#include <cmath>
#include <unordered_set>

namespace osrm
{
namespace engine
{
namespace plugins
{

IsochronePlugin::IsochronePlugin() : shortest_path(heaps) {}

Status
IsochronePlugin::HandleRequest(const std::shared_ptr<const datafacade::BaseDataFacade> facade,
                               const api::IsochroneParameters &parameters,
                               std::string &pbf_buffer) const
{

    const auto range = parameters.range;

    util::Coordinate startcoord = parameters.coordinates.front();

    const auto MAX_SPEED_METERS_PER_SECOND = 200 / 3.6;
    const auto MAX_TRAVEL_DISTANCE_METERS = MAX_SPEED_METERS_PER_SECOND * parameters.range;

    const auto latitude_range = MAX_TRAVEL_DISTANCE_METERS /
                                (util::coordinate_calculation::detail::EARTH_RADIUS * M_PI * 2) *
                                360;

    const auto longitude_range = 360 * MAX_TRAVEL_DISTANCE_METERS /
                                 ((util::coordinate_calculation::detail::EARTH_RADIUS * M_PI * 2) *
                                  cos(static_cast<double>(toFloating(startcoord.lat)) *
                                      util::coordinate_calculation::detail::DEGREE_TO_RAD));

    util::Coordinate southwest{
        util::FloatLongitude{
            static_cast<double>(static_cast<double>(toFloating(startcoord.lon)) - longitude_range)},
        util::FloatLatitude{
            static_cast<double>(static_cast<double>(toFloating(startcoord.lat)) - latitude_range)}};
    util::Coordinate northeast{
        util::FloatLongitude{
            static_cast<double>(static_cast<double>(toFloating(startcoord.lon)) + longitude_range)},
        util::FloatLatitude{
            static_cast<double>(static_cast<double>(toFloating(startcoord.lat)) + latitude_range)}};

    util::Log() << "sw " << southwest;
    util::Log() << "ne " << northeast;

    TIMER_START(GET_EDGES_TIMER);
    const auto edges = facade->GetEdgesInBox(southwest, northeast);
    TIMER_STOP(GET_EDGES_TIMER);
    util::Log() << "Fetch RTree " << TIMER_MSEC(GET_EDGES_TIMER);

    auto startpoints = facade->NearestPhantomNodes(startcoord, 1);

    // Perform the initial upwards search
    heaps.InitializeOrClearFirstThreadLocalStorage(facade->GetNumberOfNodes());
    auto &forward_heap = *(heaps.forward_heap_1);
    forward_heap.Clear();

    std::unordered_set<NodeID> visited;

    for (const auto &phantom_node : startpoints)
    {
        if (phantom_node.phantom_node.forward_segment_id.enabled)
        {
            forward_heap.Insert(phantom_node.phantom_node.forward_segment_id.id,
                                0,
                                phantom_node.phantom_node.forward_segment_id.id);
            visited.insert(phantom_node.phantom_node.forward_segment_id.id);
        }
        if (phantom_node.phantom_node.reverse_segment_id.enabled)
        {
            forward_heap.Insert(phantom_node.phantom_node.reverse_segment_id.id,
                                0,
                                phantom_node.phantom_node.reverse_segment_id.id);
            visited.insert(phantom_node.phantom_node.reverse_segment_id.id);
        }
    }

    // Dijkstra forward from the source
    while (!forward_heap.Empty())
    {
        const NodeID node = forward_heap.DeleteMin();
        const EdgeWeight weight = forward_heap.GetKey(node);
        for (const auto edge : facade->GetAdjacentEdgeRange(node))
        {
            const auto &data = facade->GetEdgeData(edge);
            if (data.forward)
            {
                const NodeID to = facade->GetTarget(edge);
                const EdgeWeight edge_weight = data.weight;

                BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
                const EdgeWeight to_weight = weight + edge_weight;

                // New Node discovered -> Add to Heap + Node Info Storage
                if (!forward_heap.WasInserted(to) && weight < range)
                {
                    forward_heap.Insert(to, to_weight, node);
                    visited.insert(to);
                }
                // Found a shorter Path -> Update weight
                else if (to_weight < forward_heap.GetKey(to))
                {
                    // new parent
                    forward_heap.GetData(to).parent = node;
                    forward_heap.DecreaseKey(to, to_weight);
                }
            }
        }
    }

    std::stringstream buf;
    buf << std::setprecision(12);
    buf << "{\"type\":\"FeatureCollection\",\"features\":[";
    bool firstline = true;
    std::for_each(visited.begin(), visited.end(), [&](const NodeID n) {
        auto data = forward_heap.GetData(n);

        // Skip the self loops at the start
        if (data.parent == n)
            return;

        std::vector<NodeID> unpacked_path;

        using EdgeData = datafacade::BaseDataFacade::EdgeData;

        std::array<NodeID, 2> path{{data.parent, n}};
        UnpackCHPath(
            *facade,
            path.begin(),
            path.end(),
            [&](const std::pair<NodeID, NodeID> & /* edge */, const EdgeData &edge_data) {
                const auto geometry_index = facade->GetGeometryIndexForEdgeID(edge_data.id);
                std::vector<NodeID> id_vector;
                if (geometry_index.forward)
                {
                    id_vector = facade->GetUncompressedForwardGeometry(geometry_index.id);
                }
                else
                {
                    id_vector = facade->GetUncompressedReverseGeometry(geometry_index.id);
                }
                std::copy(id_vector.begin(), id_vector.end(), std::back_inserter(unpacked_path));
            });

        if (!firstline)
            buf << ",";
        firstline = false;
        buf << "{\"type\":\"Feature\",\"properties\":{"
            << "},\"geometry\":{\"type\":\"LineString\",\"coordinates\":[";

        bool firstcoord = true;
        std::for_each(unpacked_path.begin(), unpacked_path.end(), [&](const NodeID n) {
            auto coord = facade->GetCoordinateOfNode(n);
            buf << (firstcoord ? "" : ",") << "[" << toFloating(coord.lon) << ","
                << toFloating(coord.lat) << "]";
            firstcoord = false;
        });
        buf << "]}}";
    });

    // **********************************************************************
    // NOTE: this implementation is not complete yet
    // TODO: Step 2 - reverse search up the CH from remaining edge-based-nodes in the RTree
    // that weren't found in the initial forward search
    // Also, need to properly offset the start coordinate, and properly trim
    // the geometry for the maximum range value
    // **********************************************************************

    // Now do the reverse search for every remaining target, basically dijkstra until
    // it hits something in `visited()`
    std::unordered_set<NodeID> seen_ebn;
    for (const auto &edge : edges)
    {
        auto &reverse_heap = *(heaps.reverse_heap_1);
        reverse_heap.Clear();
        if (edge.forward_segment_id.enabled && seen_ebn.count(edge.forward_segment_id.id) == 0 &&
            visited.count(edge.forward_segment_id.id) == 0)
        {
            reverse_heap.Insert(edge.forward_segment_id.id, 0, edge.forward_segment_id.id);
            visited.insert(edge.forward_segment_id.id);
            seen_ebn.insert(edge.forward_segment_id.id);
        }
        if (edge.reverse_segment_id.enabled && seen_ebn.count(edge.reverse_segment_id.id) == 0 &&
            visited.count(edge.reverse_segment_id.id) == 0)
        {
            reverse_heap.Insert(edge.reverse_segment_id.id, 0, edge.reverse_segment_id.id);
            visited.insert(edge.reverse_segment_id.id);
            seen_ebn.insert(edge.reverse_segment_id.id);
        }
        if (reverse_heap.Empty())
            continue;

        NodeID node = 0;
        // Dijkstra forward from the source
        while (!reverse_heap.Empty())
        {
            node = reverse_heap.DeleteMin();
            const EdgeWeight weight = reverse_heap.GetKey(node);

            // They met!
            if (forward_heap.WasInserted(node))
            {
                reverse_heap.DeleteAll();
                break;
            }

            for (const auto edge : facade->GetAdjacentEdgeRange(node))
            {
                const auto &data = facade->GetEdgeData(edge);
                if (data.backward)
                {
                    const NodeID to = facade->GetTarget(edge);
                    const EdgeWeight edge_weight = data.weight;

                    BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
                    const EdgeWeight to_weight = weight + edge_weight;

                    // New Node discovered -> Add to Heap + Node Info Storage
                    if (!reverse_heap.WasInserted(to) && weight < range)
                    {
                        reverse_heap.Insert(to, to_weight, node);
                    }
                    // Found a shorter Path -> Update weight
                    else if (to_weight < reverse_heap.GetKey(to))
                    {
                        // new parent
                        reverse_heap.GetData(to).parent = node;
                        reverse_heap.DecreaseKey(to, to_weight);
                    }
                }
            }
        }

        if (forward_heap.WasInserted(node))
        {
            auto data = reverse_heap.GetData(node);
            // Trace back along the path
            while (data.parent != node)
            {
                std::vector<NodeID> unpacked_path;
                using EdgeData = datafacade::BaseDataFacade::EdgeData;
                // Note, values are reversed because we're looking at backward edges
                std::array<NodeID, 2> path{{node, data.parent}};
                UnpackCHPath(
                    *facade,
                    path.begin(),
                    path.end(),
                    [&](const std::pair<NodeID, NodeID> & /* edge */, const EdgeData &edge_data) {
                        const auto geometry_index = facade->GetGeometryIndexForEdgeID(edge_data.id);
                        std::vector<NodeID> id_vector;
                        if (geometry_index.forward)
                        {
                            id_vector = facade->GetUncompressedForwardGeometry(geometry_index.id);
                        }
                        else
                        {
                            id_vector = facade->GetUncompressedReverseGeometry(geometry_index.id);
                        }
                        std::copy(
                            id_vector.begin(), id_vector.end(), std::back_inserter(unpacked_path));
                    });

                if (!firstline)
                    buf << ",";
                firstline = false;
                buf << "{\"type\":\"Feature\",\"properties\":{"
                    << "},\"geometry\":{\"type\":\"LineString\",\"coordinates\":[";

                bool firstcoord = true;
                std::for_each(unpacked_path.begin(), unpacked_path.end(), [&](const NodeID n) {
                    auto coord = facade->GetCoordinateOfNode(n);
                    buf << (firstcoord ? "" : ",") << "[" << toFloating(coord.lon) << ","
                        << toFloating(coord.lat) << "]";
                    firstcoord = false;
                });
                buf << "]}}";
                node = data.parent;
                data = reverse_heap.GetData(node);
            }
        }
    }

    buf << "]}";

    pbf_buffer = buf.str();

    return Status::Ok;
}

} // plugins
} // engine
} // osrm
