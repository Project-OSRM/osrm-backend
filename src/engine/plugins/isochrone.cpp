#include "engine/api/isochrone_api.hpp"
#include "engine/phantom_node.hpp"
#include "engine/plugins/isochrone.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/graph_loader.hpp"
#include "util/concave_hull.hpp"
#include "util/monotone_chain.hpp"
#include "util/simple_logger.hpp"
#include "util/timing_util.hpp"

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

    // Loads Graph into memory
    if (!base.empty())
    {
        number_of_nodes = loadGraph(base, coordinate_list, graph_edge_list);
    }

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

    if (params.distance != 0 && params.duration != 0)
    {
        return Error("InvalidOptions", "Only distance or duration should be set", json_result);
    }

    if (params.concavehull == true && params.convexhull == false) {
        return Error("InvalidOptions", "If concavehull is set, convexhull must be set too", json_result);
    }

    auto phantomnodes = GetPhantomNodes(params, 1);

    if (phantomnodes.front().size() <= 0)
    {
        return Error("PhantomNode", "PhantomNode couldnt be found for coordinate", json_result);
    }

    auto phantom = phantomnodes.front();
    std::vector<NodeID> forward_id_vector;
    facade.GetUncompressedGeometry(phantom.front().phantom_node.reverse_packed_geometry_id,
                                   forward_id_vector);
    auto source = forward_id_vector[0];

    isochroneVector.clear();

    if (params.duration != 0)
    {
        TIMER_START(DIJKSTRA);
        dijkstraByDuration(isochroneVector, source, params.duration);
        TIMER_STOP(DIJKSTRA);
        util::SimpleLogger().Write() << "DijkstraByDuration took: " << TIMER_MSEC(DIJKSTRA) << "ms";
        TIMER_START(SORTING);
        std::sort(isochroneVector.begin(), isochroneVector.end(),
                  [&](const IsochroneNode n1, const IsochroneNode n2)
                  {
                      return n1.duration < n2.duration;
                  });
        TIMER_STOP(SORTING);
        util::SimpleLogger().Write() << "SORTING took: " << TIMER_MSEC(SORTING) << "ms";
    }
    if (params.distance != 0)
    {
        TIMER_START(DIJKSTRA);
        dijkstraByDistance(isochroneVector, source, params.distance);
        TIMER_STOP(DIJKSTRA);
        util::SimpleLogger().Write() << "DijkstraByDistance took: " << TIMER_MSEC(DIJKSTRA) << "ms";
        TIMER_START(SORTING);
        std::sort(isochroneVector.begin(), isochroneVector.end(),
                  [&](const IsochroneNode n1, const IsochroneNode n2)
                  {
                      return n1.distance < n2.distance;
                  });
        TIMER_STOP(SORTING);
        util::SimpleLogger().Write() << "SORTING took: " << TIMER_MSEC(SORTING) << "ms";
    }

    util::SimpleLogger().Write() << "Nodes Found: " << isochroneVector.size();

    convexhull.clear();
    concavehull.clear();

    // Optional param for calculating Convex Hull
    if (params.convexhull)
    {

        TIMER_START(CONVEXHULL);
        convexhull = util::monotoneChain(isochroneVector);
        TIMER_STOP(CONVEXHULL);
        util::SimpleLogger().Write() << "CONVEXHULL took: " << TIMER_MSEC(CONVEXHULL) << "ms";
    }
    if(params.concavehull && params.convexhull) {
        TIMER_START(CONCAVEHULL);
        concavehull = util::concavehull(convexhull, params.threshold, isochroneVector);
        TIMER_STOP(CONCAVEHULL);
        util::SimpleLogger().Write() << "CONCAVEHULL took: " << TIMER_MSEC(CONCAVEHULL) << "ms";
    }

    TIMER_START(RESPONSE);
    api::IsochroneAPI isochroneAPI(facade, params);
    isochroneAPI.MakeResponse(isochroneVector, convexhull, concavehull, json_result);
    TIMER_STOP(RESPONSE);
    util::SimpleLogger().Write() << "RESPONSE took: " << TIMER_MSEC(RESPONSE) << "ms";
    isochroneVector.clear();
    isochroneVector.shrink_to_fit();
    convexhull.clear();
    convexhull.shrink_to_fit();
    concavehull.clear();
    concavehull.shrink_to_fit();
    return Status::Ok;
}

void IsochronePlugin::dijkstraByDuration(IsochroneVector &isochroneSet,
                                         NodeID &source,
                                         int duration)
{

    QueryHeap heap(number_of_nodes);
    heap.Insert(source, 0, source);

    isochroneSet.emplace_back(
        IsochroneNode(coordinate_list[source], coordinate_list[source], 0, 0));

    int MAX_DURATION = duration * 60 *10;
    {
        // Standard Dijkstra search, terminating when path length > MAX
        while (!heap.Empty())
        {
            const NodeID source = heap.DeleteMin();
            const std::int32_t weight = heap.GetKey(source);

            for (const auto current_edge : graph->GetAdjacentEdgeRange(source))
            {
                const auto target = graph->GetTarget(current_edge);
                if (target != SPECIAL_NODEID)
                {
                    const auto data = graph->GetEdgeData(current_edge);
                    if (data.real)
                    {
                        int to_duration = weight + data.weight;
                        if (to_duration > MAX_DURATION)
                        {
                            continue;
                        }
                        else if (!heap.WasInserted(target))
                        {
                            heap.Insert(target, to_duration, source);
                            isochroneSet.emplace_back(IsochroneNode(
                                coordinate_list[target], coordinate_list[source], 0, to_duration));
                        }
                        else if (to_duration < heap.GetKey(target))
                        {
                            heap.GetData(target).parent = source;
                            heap.DecreaseKey(target, to_duration);
                            update(isochroneSet,
                                   IsochroneNode(coordinate_list[target], coordinate_list[source],
                                                 0, to_duration));
                        }
                    }
                }
            }
        }
    }
}
void IsochronePlugin::dijkstraByDistance(IsochroneVector &isochroneSet,
                                         NodeID &source,
                                         double distance)
{
    QueryHeap heap(number_of_nodes);
    heap.Insert(source, 0, source);

    isochroneSet.emplace_back(
        IsochroneNode(coordinate_list[source], coordinate_list[source], 0, 0));

    int MAX_DISTANCE = distance;
    {
        // Standard Dijkstra search, terminating when path length > MAX
        while (!heap.Empty())
        {
            NodeID source = heap.DeleteMin();
            std::int32_t weight = heap.GetKey(source);

            for (const auto current_edge : graph->GetAdjacentEdgeRange(source))
            {
                const auto target = graph->GetTarget(current_edge);
                if (target != SPECIAL_NODEID)
                {
                    const auto data = graph->GetEdgeData(current_edge);
                    if (data.real)
                    {
                        Coordinate s(coordinate_list[source].lon, coordinate_list[source].lat);
                        Coordinate t(coordinate_list[target].lon, coordinate_list[target].lat);
                        //FIXME this might not be accurate enough
                        int to_distance =
                            static_cast<int>(
                                util::coordinate_calculation::haversineDistance(s, t)) +
                            weight;

                        if (to_distance > MAX_DISTANCE)
                        {
                            continue;
                        }
                        else if (!heap.WasInserted(target))
                        {
                            heap.Insert(target, to_distance, source);
                            isochroneSet.emplace_back(IsochroneNode(
                                coordinate_list[target], coordinate_list[source], to_distance, 0));
                        }
                        else if (to_distance < heap.GetKey(target))
                        {
                            heap.GetData(target).parent = source;
                            heap.DecreaseKey(target, to_distance);
                            update(isochroneSet,
                                   IsochroneNode(coordinate_list[target], coordinate_list[source],
                                                 to_distance, 0));
                        }
                    }
                }
            }
        }
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

void IsochronePlugin::update(IsochroneVector &v, IsochroneNode n)
{
    for (auto node : v)
    {
        if (node.node.node_id == n.node.node_id)
        {
            node = n;
        }
    }
}
}
}
}
