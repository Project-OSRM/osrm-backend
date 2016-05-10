//
// Created by robin on 4/13/16.
//

#include "engine/plugins/isochrone.hpp"
#include "engine/api/isochrone_api.hpp"
#include "util/graph_loader.hpp"
#include "engine/phantom_node.hpp"
#include "util/simple_logger.hpp"
#include <util/graham_scan.hpp>

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <algorithm>

namespace osrm
{
namespace engine
{
namespace plugins
{

IsochronePlugin::IsochronePlugin(datafacade::BaseDataFacade &facade, const std::string base)
    : BasePlugin{facade}, base{base}
{
    // Prepares uncontracted Graph
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

    if (phantomnodes.front().size() <= 0)
    {
        return Error("PhantomNode", "PhantomNode couldnt be found for coordinate", json_result);
    }

    // Find closest phantomnode
    std::sort(phantomnodes.front().begin(), phantomnodes.front().end(),
              [&](const osrm::engine::PhantomNodeWithDistance &a,
                  const osrm::engine::PhantomNodeWithDistance &b)
              {
                  return a.distance > b.distance;
              });
    auto phantom = phantomnodes.front();

    util::SimpleLogger().Write() << util::toFloating(phantom.front().phantom_node.location.lat);
    util::SimpleLogger().Write() << util::toFloating(phantom.front().phantom_node.location.lon);

    std::vector<NodeID> forward_id_vector;
    facade.GetUncompressedGeometry(phantom.front().phantom_node.forward_packed_geometry_id,
                                   forward_id_vector);
    auto source = forward_id_vector[phantom.front().phantom_node.fwd_segment_position];

    QueryHeap heap(number_of_nodes);
    heap.Insert(source, 0, source);

    // value is in metres
//    const int MAX = 2000;
    const int MAX = 7500;

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
                            continue;
                        }
                        else if (!heap.WasInserted(target))
                        {
                            heap.Insert(target, to_distance, source);
                            isochroneSet.insert(IsochroneNode(coordinate_list[target], coordinate_list[source], to_distance));
                        }
                        else if (to_distance < heap.GetKey(target))
                        {
                            heap.GetData(target).parent = source;
                            heap.DecreaseKey(target, to_distance);
                            update(isochroneSet, IsochroneNode(coordinate_list[target], coordinate_list[source], to_distance));
                        }
                    }
                }
            }
        }
    }

    util::SimpleLogger().Write() << isochroneSet.size();
    std::vector<IsochroneNode> isoByDistance(isochroneSet.begin(), isochroneSet.end());
    std::sort(isoByDistance.begin(), isoByDistance.end(),[] (IsochroneNode n1, IsochroneNode n2){
        return n1.distance < n2.distance;
    });
    util::json::Array data;
    for (auto isochrone : isoByDistance)
    {
        util::json::Object object;

        util::json::Object source;
        source.values["lat"] =
            static_cast<double>(util::toFloating(isochrone.node.lat));
        source.values["lon"] =
            static_cast<double>(util::toFloating(isochrone.node.lon));
        object.values["p1"] = std::move(source);

        util::json::Object predecessor;
        predecessor.values["lat"] =
            static_cast<double>(util::toFloating(isochrone.predecessor.lat));
        predecessor.values["lon"] =
            static_cast<double>(util::toFloating(isochrone.predecessor.lon));
        object.values["p2"] = std::move(predecessor);

        util::json::Object distance;
        object.values["distance_from_start"] = isochrone.distance;

        data.values.push_back(object);
    }
    json_result.values["isochrone"] = std::move(data);

    util::convexHull(isochroneSet, json_result);
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
