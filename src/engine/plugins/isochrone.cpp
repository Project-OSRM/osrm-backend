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
#include <utility>
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
    //        return Error("NoSegment", "Could not find a matching segments for coordinate",
    //        json_result);
    //    }
    //    BOOST_ASSERT(phantomnodes[0] > 0);

    util::SimpleLogger().Write() << util::toFloating(params.coordinates.front().lat);
    util::SimpleLogger().Write() << util::toFloating(params.coordinates.front().lon);

    util::SimpleLogger().Write() << "Phantom-Node-Size: " << phantomnodes.size();
    util::SimpleLogger().Write() << "Phantom-Node-Size2: " << phantomnodes.front().size();

    //    api::IsochroneAPI isochroneAPI(facade, params);
    //    isochroneAPI.MakeResponse(phantom_nodes, json_result);

    std::sort(phantomnodes.front().begin(), phantomnodes.front().end(),
              [&](const osrm::engine::PhantomNodeWithDistance &a,
                  const osrm::engine::PhantomNodeWithDistance &b)
              {
                  return a.distance > b.distance;
              });
    auto phantom = phantomnodes.front();

    util::SimpleLogger().Write() << util::toFloating(phantom.front().phantom_node.location.lat);
    util::SimpleLogger().Write() << util::toFloating(phantom.front().phantom_node.location.lon);

    util::SimpleLogger().Write() << util::toFloating(phantomnodes2.front().first.location.lat);
    util::SimpleLogger().Write() << util::toFloating(phantomnodes2.front().first.location.lon);
    auto source = 0;

    //    if (facade.Edgeie(phantom.phantom_node.forward_segment_id.id))
    //    {
    //        std::vector<NodeID> forward_id_vector;
    //        facade.GetUncompressedGeometry(phantom.phantom_node.forward_packed_geometry_id,
    //                                        forward_id_vector);
    //        source = forward_id_vector[phantom.phantom_node.fwd_segment_position];
    //    }
    //    else
    //

    std::vector<NodeID> forward_id_vector;
    facade.GetUncompressedGeometry(phantom.front().phantom_node.forward_packed_geometry_id,
                                   forward_id_vector);
    source = forward_id_vector[phantom.front().phantom_node.fwd_segment_position];
    util::SimpleLogger().Write() << coordinate_list[source].lat << "    "
                                 << coordinate_list[source].lon;

    util::SimpleLogger().Write() << "segment id " << phantom.front().phantom_node.forward_segment_id;
    util::SimpleLogger().Write() << "packed id " << source;
    // Init complete

    struct HeapData
    {
        NodeID parent;
        /* explicit */ HeapData(NodeID p) : parent(p) {}
    };

    using QueryHeap = osrm::util::BinaryHeap<NodeID, NodeID, int, HeapData,
                                             osrm::util::UnorderedMapStorage<NodeID, int>>;

    QueryHeap heap(number_of_nodes);
    heap.Insert(source, 0, source);
    // value is in metres
    const int MAX = 2000;

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
                        if (to_distance > MAX)
                        {
                            edgepoints.insert(target);
                        }
                        else if (!heap.WasInserted(target))
                        {
                            heap.Insert(target, to_distance, source);
                            insidepoints.insert(source);
                            isochroneSet.insert(IsochroneNode(target, source, to_distance));
                        }
                        else if (to_distance < heap.GetKey(target))
                        {
                            heap.GetData(target).parent = source;
                            heap.DecreaseKey(target, to_distance);
                            update(isochroneSet, IsochroneNode(target, source, to_distance));
                        }
                    }
                }
            }
        }
    }

    util::SimpleLogger().Write() << isochroneSet.size();

    util::json::Array data;
    for (auto node : isochroneSet)
    {
        util::json::Object object;

        util::json::Object source;
        source.values["lat"] =
            static_cast<double>(util::toFloating(coordinate_list[node.node].lat));
        source.values["lon"] =
            static_cast<double>(util::toFloating(coordinate_list[node.node].lon));
        object.values["p1"] = std::move(source);

        util::json::Object predecessor;
        predecessor.values["lat"] =
            static_cast<double>(util::toFloating(coordinate_list[node.predecessor].lat));
        predecessor.values["lon"] =
            static_cast<double>(util::toFloating(coordinate_list[node.predecessor].lon));
        object.values["p2"] = std::move(predecessor);

        util::json::Object distance;
        object.values["distance_from_start"] = node.distance;

        //        util::SimpleLogger().Write() << "Node: " << node.node
        //                                     << " Predecessor: " << node.predecessor
        //                                     << " Distance:  " << node.distance;
        data.values.push_back(object);
    }
    json_result.values["isochrone"] = std::move(data);

    //    util::json::Array data2;
    //    for (auto b : edgepoints)
    //    {
    //        util::json::Object point;
    //        point.values["lat"] = static_cast<double>(util::toFloating(coordinate_list[b].lat));
    //        point.values["lon"] = static_cast<double>(util::toFloating(coordinate_list[b].lon));
    //        data2.values.push_back(point);
    //    }

    //    json_result.values["border"] = std::move(data2);
    //            findBorder(insidepoints, json_result);
    return Status::Ok;
}

void IsochronePlugin::findBorder(std::unordered_set<NodeID> &insidepoints,
                                 util::json::Object &response)
{
    NodeID startnode = SPECIAL_NODEID;
    std::vector<NodeID> border;
    // Find the north-west most edge node
    for (const auto node_id : insidepoints)
    {
        if (startnode == SPECIAL_NODEID)
        {
            startnode = node_id;
        }
        else
        {
            if (coordinate_list[node_id].lon < coordinate_list[startnode].lon)
            {
                startnode = node_id;
            }
            else if (coordinate_list[node_id].lon == coordinate_list[startnode].lon &&
                     coordinate_list[node_id].lat > coordinate_list[startnode].lat)
            {
                startnode = node_id;
            }
        }
    }
    NodeID node_u = startnode;
    border.push_back(node_u);

    // Find the outgoing edge with the angle closest to 180 (because we're at the west-most node,
    // there should be no edges with angles < 0 or > 180)
    NodeID node_v = SPECIAL_NODEID;
    for (const auto current_edge : graph->GetAdjacentEdgeRange(node_u))
    {
        const auto target = graph->GetTarget(current_edge);
        if (target != SPECIAL_NODEID && insidepoints.find(target) != insidepoints.end())
        {
            if (node_v == SPECIAL_NODEID)
            {
                node_v = target;
            }
            else
            {
                if (osrm::util::coordinate_calculation::bearing(coordinate_list[node_u],
                                                                coordinate_list[target]) >
                    osrm::util::coordinate_calculation::bearing(coordinate_list[node_u],
                                                                coordinate_list[node_v]))
                {
                    node_v = target;
                }
            }
            BOOST_ASSERT(0 <= osrm::util::coordinate_calculation::bearing(coordinate_list[node_u],
                                                                          coordinate_list[node_v]));
            BOOST_ASSERT(180 >= osrm::util::coordinate_calculation::bearing(
                                    coordinate_list[node_u], coordinate_list[node_v]));
        }
    }

    border.push_back(node_v);

    // Now, we're going to always turn right (relative to the last edge)
    // only onto nodes that are onthe inside point set
    NodeID firsttarget = node_v;
    while (true)
    {
        NodeID node_w = SPECIAL_NODEID;
        double best_angle = 361.0;
        for (const auto current_edge : graph->GetAdjacentEdgeRange(node_v))
        {
            const auto target = graph->GetTarget(current_edge);
            if (target == SPECIAL_NODEID)
                continue;
            if (insidepoints.find(target) == insidepoints.end())
                continue;

            auto angle = osrm::util::coordinate_calculation::computeAngle(
                coordinate_list[node_u], coordinate_list[node_v], coordinate_list[target]);
            if (node_w == SPECIAL_NODEID || angle > best_angle)
            {
                node_w = target;
                best_angle = angle;
            }
        }
        if (firsttarget == node_w && startnode == node_v)
        {
            // Here, we've looped all the way around the outside and we've traversed
            // the first segment again.  Break!
            break;
        }
        border.push_back(node_w);

        node_u = node_v;
        node_v = node_w;

        util::json::Array borderjson;
        for (auto b : border)
        {
            util::json::Object point;
            point.values["lat"] = static_cast<double>(util::toFloating(coordinate_list[b].lat));
            point.values["lon"] = static_cast<double>(util::toFloating(coordinate_list[b].lon));
            borderjson.values.push_back(point);
        }
        response.values["border"] = std::move(borderjson);
    }
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

void IsochronePlugin::update(IsochroneSet &s, IsochroneNode node)
{
    std::pair<IsochroneSet::iterator, bool> p = s.insert(node);
    bool alreadyThere = !p.second;
    if (alreadyThere)
    {
        IsochroneSet::iterator hint = p.first;
        hint++;
        s.erase(p.first);
        s.insert(hint, node);
    }
}
}
}
}
