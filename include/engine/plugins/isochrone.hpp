//
// Created by robin on 4/6/16.
//

#ifndef ISOCHRONE_HPP
#define ISOCHRONE_HPP

#include "engine/plugins/plugin_base.hpp"
#include "engine/search_engine.hpp"
#include "engine/datafacade/internal_datafacade.hpp"

#include "util/binary_heap.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/dynamic_graph.hpp"
#include "util/fingerprint.hpp"
#include "util/graph_loader.hpp"
#include "util/make_unique.hpp"
#include "util/routed_options.hpp"
#include "util/static_graph.hpp"
#include "util/string_util.hpp"
#include "util/simple_logger.hpp"
#include "util/typedefs.hpp"

#include "osrm/json_container.hpp"
#include "osrm/coordinate.hpp"

#include <boost/filesystem.hpp>

#include <cstdlib>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <map>
#include <string>
#include <cmath>

namespace osrm
{
namespace engine
{
namespace plugins
{
struct SimpleEdgeData
{
    SimpleEdgeData() : weight(INVALID_EDGE_WEIGHT), real(false) {}
    SimpleEdgeData(unsigned weight_, bool real_) : weight(weight_), real(real_) {}
    unsigned weight;
    bool real;
};

using SimpleGraph = util::StaticGraph<SimpleEdgeData>;
using SimpleEdge = SimpleGraph::InputEdge;
using Datafacade =
    osrm::engine::datafacade::InternalDataFacade<osrm::contractor::QueryEdge::EdgeData>;

template <class DataFacadeT> class Isochrone final : public BasePlugin
{

  private:
    std::string temp_string;
    std::string descriptor_string;
    std::unique_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;
    std::shared_ptr<engine::plugins::SimpleGraph> graph;
//    std::unique_ptr<Datafacade> datafacade;
    DataFacadeT *facade;
    std::string path;
    std::vector<osrm::extractor::QueryNode> coordinate_list;
    std::map<NodeID, int> distance_map;
    std::map<NodeID, NodeID> predecessor_map;
    std::vector<engine::plugins::SimpleEdge> graph_edge_list;
    std::size_t number_of_nodes;


    void deleteFileIfExists(const std::string &file_name)
    {
        if (boost::filesystem::exists(file_name))
        {
            boost::filesystem::remove(file_name);
        }
    }

    std::size_t loadGraph(const std::string &path,
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

  public:
    explicit Isochrone(DataFacadeT *facade, const std::string path) : descriptor_string("isochrone"), facade(facade), path(path)
    {
        number_of_nodes = loadGraph(path, coordinate_list, graph_edge_list);

        tbb::parallel_sort(graph_edge_list.begin(), graph_edge_list.end());
        graph = std::make_shared<engine::plugins::SimpleGraph>(number_of_nodes, graph_edge_list);
        graph_edge_list.clear();
        graph_edge_list.shrink_to_fit();
//        {
//            std::unordered_map<std::string, boost::filesystem::path> server_paths;
//            server_paths["base"] = base;
//            osrm::util::populate_base_path(server_paths);
//            datafacade = std::unique_ptr<Datafacade>(new Datafacade(server_paths));
//        }
    }

    virtual ~Isochrone() {}

    const std::string GetDescriptor() const override final { return descriptor_string; }

    Status HandleRequest(const RouteParameters &routeParameters,
                         util::json::Object &json_result) override final
    {
        RouteParameters a = routeParameters;
        util::json::Object o = json_result;

        auto phantomnodes = facade->NearestPhantomNodes(
            {static_cast<int>(routeParameters.coordinates.front().lat),
             static_cast<int>(routeParameters.coordinates.front().lon)},
            1);

        std::sort(phantomnodes.begin(), phantomnodes.end(),
                  [&](const osrm::engine::PhantomNodeWithDistance &a,
                      const osrm::engine::PhantomNodeWithDistance &b)
                  {
                      return a.distance > b.distance;
                  });
        auto phantom = phantomnodes[0];
        NodeID source = 0;

        double sourceLat = phantom.phantom_node.location.lat / osrm::COORDINATE_PRECISION;
        double sourceLon = phantom.phantom_node.location.lon / osrm::COORDINATE_PRECISION;

        if (facade->EdgeIsCompressed(phantom.phantom_node.forward_node_id))
        {
            std::vector<NodeID> forward_id_vector;
            facade->GetUncompressedGeometry(phantom.phantom_node.forward_packed_geometry_id,
                                                forward_id_vector);
            source = forward_id_vector[phantom.phantom_node.fwd_segment_position];
        }
        else
        {
            source = phantom.phantom_node.forward_packed_geometry_id;
        }

        util::SimpleLogger().Write() << "Node-Coord" << sourceLat << "     " << sourceLon;

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
        heap.Insert(source, -phantom.phantom_node.GetForwardWeightPlusOffset(), source);

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
                            util::SimpleLogger().Write() << "I am Node " << target;
                            util::SimpleLogger().Write() << "Distance to Source: " << to_distance;
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
        for (auto p : insidepoints)
        {
            double lon = coordinate_list[p].lon / osrm::COORDINATE_PRECISION;
            double lat = coordinate_list[p].lat / osrm::COORDINATE_PRECISION;
            auto pre = predecessor_map[p];
            auto d = distance_map[p];
            util::SimpleLogger().Write() << "Lat: " << lat;
            util::SimpleLogger().Write() << "Lon: " << lon;
            util::SimpleLogger().Write() << "Source: " << p << " Predecessor: " << pre
                                         << "  Distance " << d;
            //            util::SimpleLogger().Write();
        }

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
            source.values["lat"] = coordinate_list[p].lat / osrm::COORDINATE_PRECISION;
            source.values["lon"] = coordinate_list[p].lon / osrm::COORDINATE_PRECISION;
            object.values["p1"] = std::move(source);

            util::json::Object predecessor;
            auto pre = predecessor_map[p];
            predecessor.values["lat"] = coordinate_list[pre].lat / osrm::COORDINATE_PRECISION;
            predecessor.values["lon"] = coordinate_list[pre].lon / osrm::COORDINATE_PRECISION;
            object.values["p2"] = std::move(predecessor);

            util::json::Object distance;
            object.values["distance_from_start"] = distance_map[p];

            data.values.push_back(object);
        }

        json_result.values["Range-Analysis"] = std::move(data);
        return Status::Ok;
    }
};
}
}
}

#endif // ISOCHRONE_HPP
