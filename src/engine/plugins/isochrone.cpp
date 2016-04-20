//
// Created by robin on 4/13/16.
//


#include "engine/plugins/isochrone.hpp"
#include "engine/api/isochrone_api.hpp"
#include "util/graph_loader.hpp"
#include "engine/phantom_node.hpp"
#include "util/coordinate.hpp"
#include "util/simple_logger.hpp"

#include <unordered_map>
#include <unordered_set>
#include <util/binary_heap.hpp>

namespace osrm
{
namespace engine
{
namespace plugins
{

IsochronePlugin::IsochronePlugin(datafacade::BaseDataFacade &facade, const std::string base)
    : BasePlugin{facade}, base{base}
{
    number_of_nodes = loadGraph(base, coordinate_list, graph_edge_list);

    tbb::parallel_sort(graph_edge_list.begin(), graph_edge_list.end());
    graph = std::make_shared<engine::plugins::SimpleGraph>(number_of_nodes, graph_edge_list);
    graph_edge_list.clear();
    graph_edge_list.shrink_to_fit();
}

Status IsochronePlugin::HandleRequest(const api::IsochroneParameters &params,
                                      util::json::Object &json_result)
{
    BOOST_ASSERT(params.IsValid());

    if (!CheckAllCoordinates(params.coordinates))
        return Error("InvalidOptions", "Coordinates are invalid", json_result);

    if (params.coordinates.size() != 1)
    {
        return Error("InvalidOptions", "Only one input coordinate is supported", json_result);
    }

    auto phantomnodes = GetPhantomNodes(params, params.number_of_results);
    auto phantomnodes2 = GetPhantomNodes(params);

//    if (phantomnodes.front().)
//    {
//        return Error("NoSegment", "Could not find a matching segments for coordinate", json_result);
//    }
//    BOOST_ASSERT(phantomnodes[0] > 0);



    util::SimpleLogger().Write() << util::toFloating(params.coordinates.front().lat);
    util::SimpleLogger().Write() << util::toFloating(params.coordinates.front().lon);


    util::SimpleLogger().Write() << "Phantom-Node-Size: "  << phantomnodes.size();
    util::SimpleLogger().Write() << "Phantom-Node-Size2: "  << phantomnodes.front().size();

//    api::IsochroneAPI isochroneAPI(facade, params);
//    isochroneAPI.MakeResponse(phantom_nodes, json_result);

//    std::sort(phantomnodes.front().begin(), phantomnodes.front().end(),
//              [&](const osrm::engine::PhantomNodeWithDistance &a,
//                  const osrm::engine::PhantomNodeWithDistance &b)
//              {
//                  return a.distance > b.distance;
//              });
    auto phantom = phantomnodes.front();

    util::SimpleLogger().Write() << util::toFloating(phantom.front().phantom_node.location.lat);
    util::SimpleLogger().Write() << util::toFloating(phantom.front().phantom_node.location.lon);

    util::SimpleLogger().Write() << util::toFloating(phantomnodes2.front().first.location.lat);
    util::SimpleLogger().Write() << util::toFloating(phantomnodes2.front().first.location.lon);
    NodeID source = 0;

//    if (facade.Edgeie(phantom.phantom_node.forward_segment_id.id))
//    {
//        std::vector<NodeID> forward_id_vector;
//        facade.GetUncompressedGeometry(phantom.phantom_node.forward_packed_geometry_id,
//                                        forward_id_vector);
//        source = forward_id_vector[phantom.phantom_node.fwd_segment_position];
//    }
//    else
//

        source = phantom.front().phantom_node.forward_packed_geometry_id;
//    }

//    util::SimpleLogger().Write() << "Node-Coord" << sourceLat << "     " << sourceLon;

    // Init complete

    struct HeapData
    {
        NodeID parent;
        /* explicit */ HeapData(NodeID p) : parent(p) {}
    };

    using QueryHeap = osrm::util::BinaryHeap<NodeID, NodeID, int, HeapData,
            osrm::util::UnorderedMapStorage<NodeID, int>>;

    QueryHeap heap(number_of_nodes);

    util::SimpleLogger().Write() << "asdasdasd";
    heap.Insert(source, -phantom.front().phantom_node.GetForwardWeightPlusOffset(), source);

    // value is in metres
    const int MAX = 1000;

    std::unordered_set<NodeID> edgepoints;
    std::unordered_set<NodeID> insidepoints;
    std::vector<NodeID> border;

    {
        // Standard Dijkstra search, terminating when path length > MAX
        while (!heap.Empty())
        {
            const NodeID source = heap.DeleteMin();
            const std::int32_t distance = heap.GetKey(source);

            for (const auto current_edge : graph->GetAdjacentEdgeRange(source))
            {
                const auto target = graph->GetTarget(current_edge);
                if (target != SPECIAL_NODEID)
                {
                    const auto data = graph->GetEdgeData(current_edge);
                    if (data.real)
                    {
                        int to_distance = distance + data.weight;
//                        util::SimpleLogger().Write() << "I am Node " << target;
//                        util::SimpleLogger().Write() << "Distance to Source: " << to_distance;
                        if (to_distance > MAX)
                        {
                            edgepoints.insert(target);
                            //                                distance_map[target] =
                            //                                to_distance;
                            //                                predecessor_map[target] = source;
                        }
                        else if (!heap.WasInserted(target))
                        {
                            heap.Insert(target, to_distance, source);
                            insidepoints.insert(source);
                            distance_map[target] = to_distance;
                            predecessor_map[target] = source;
                        }
                        else if (to_distance < heap.GetKey(target))
                        {
                            heap.GetData(target).parent = source;
                            heap.DecreaseKey(target, to_distance);
                            distance_map[target] = to_distance;
                            predecessor_map[target] = source;
                        }
                    }
                }
            }
        }
    }
    util::SimpleLogger().Write();
    util::SimpleLogger().Write() << "Inside-Points";
    util::SimpleLogger().Write() << insidepoints.size();
//    for (auto p : insidepoints)
//    {
//        auto lon = coordinate_list[p].lon;
//        auto lat = coordinate_list[p].lat;
//        auto pre = predecessor_map[p];
//        auto d = distance_map[p];
//        util::SimpleLogger().Write() << "Lat: " << lat;
//        util::SimpleLogger().Write() << "Lon: " << lon;
//        util::SimpleLogger().Write() << "Source: " << p << " Predecessor: " << pre
//        << "  Distance " << d;
//        //            util::SimpleLogger().Write();
//    }

    util::SimpleLogger().Write() << "Edgepoints";
    util::SimpleLogger().Write() << edgepoints.size();
    //        for (auto p : edgepoints)
    //        {
    //            double lon = coordinate_list[p].lon / osrm::COORDINATE_PRECISION;
    //            double lat = coordinate_list[p].lat / osrm::COORDINATE_PRECISION;
    //            auto pre = predecessor_map[p];
    //            auto d = distance_map[p];
    ////            util::SimpleLogger().Write() << "Lat: " << lat;
    ////            util::SimpleLogger().Write() << "Lon: " << lon;
    //            util::SimpleLogger().Write() << "Predecessor: " << pre << "Distance " << d;
    //            util::SimpleLogger().Write();
    //        }

    util::json::Array data;
    for (auto p : insidepoints)
    {
        util::json::Object object;

        util::json::Object source;
        source.values["lat"] = static_cast<double>(util::toFloating(coordinate_list[p].lat));
        source.values["lon"] = static_cast<double>(util::toFloating(coordinate_list[p].lon));
        object.values["p1"] = std::move(source);

        util::json::Object predecessor;
        auto pre = predecessor_map[p];
        predecessor.values["lat"] = static_cast<double>(util::toFloating(coordinate_list[pre].lat));
        predecessor.values["lon"] = static_cast<double>(util::toFloating(coordinate_list[pre].lon));
        object.values["p2"] = std::move(predecessor);

        util::json::Object distance;
        object.values["distance_from_start"] = distance_map[p];

        data.values.push_back(object);
    }

    json_result.values["Range-Analysis"] = std::move(data);

    return Status::Ok;
}




std::size_t IsochronePlugin::loadGraph(const std::string &path,
                      std::vector<extractor::QueryNode> &coordinate_list,
                      std::vector<SimpleEdge> &graph_edge_list)
{

    std::ifstream input_stream(path, std::ifstream::in | std::ifstream::binary);
    if (!input_stream.is_open())
    {
        throw util::exception("Cannot open osrm file");
    }

    // load graph data
    std::vector<extractor::NodeBasedEdge> edge_list;
    std::vector<NodeID> traffic_light_node_list;
    std::vector<NodeID> barrier_node_list;

    auto number_of_nodes = util::loadNodesFromFile(input_stream, barrier_node_list,
                                                   traffic_light_node_list, coordinate_list);

    util::loadEdgesFromFile(input_stream, edge_list);

    traffic_light_node_list.clear();
    traffic_light_node_list.shrink_to_fit();

    // Building an node-based graph
    for (const auto &input_edge : edge_list)
    {
        if (input_edge.source == input_edge.target)
        {
            continue;
        }

        // forward edge
        graph_edge_list.emplace_back(input_edge.source, input_edge.target, input_edge.weight,
                                     input_edge.forward);
        // backward edge
        graph_edge_list.emplace_back(input_edge.target, input_edge.source, input_edge.weight,
                                     input_edge.backward);
    }

    return number_of_nodes;
}
}
}
}
