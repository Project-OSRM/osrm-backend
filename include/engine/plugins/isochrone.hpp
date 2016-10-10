#ifndef ISOCHRONE_HPP
#define ISOCHRONE_HPP

#include "engine/api/isochrone_parameters.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "osrm/json_container.hpp"
#include "util/binary_heap.hpp"
#include "util/static_graph.hpp"

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/function.hpp>
#include <tbb/parallel_sort.h>

#include <algorithm>
#include <cstdlib>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{
namespace plugins
{
struct HeapData
{
    NodeID parent;
    HeapData(NodeID p) : parent(p) {}
};
struct SimpleEdgeData
{
    SimpleEdgeData() : weight(INVALID_EDGE_WEIGHT), real(false) {}
    SimpleEdgeData(unsigned weight_, bool real_) : weight(weight_), real(real_) {}
    unsigned weight;
    bool real;
};

struct IsochroneNode
{
    IsochroneNode(){};
    IsochroneNode(osrm::extractor::QueryNode node,
                  osrm::extractor::QueryNode predecessor,
                  double distance,
                  int duration)
        : node(node), predecessor(predecessor), distance(distance), duration(duration)
    {
    }

    osrm::extractor::QueryNode node;
    osrm::extractor::QueryNode predecessor;
    double distance;
    int duration;

    bool operator==(const IsochroneNode &n) const
    {
        if (n.node.node_id == node.node_id)
            return true;
        else
            return false;
    }
};

using SimpleGraph = util::StaticGraph<SimpleEdgeData>;
using SimpleEdge = SimpleGraph::InputEdge;
using QueryHeap = osrm::util::
    BinaryHeap<NodeID, NodeID, int, HeapData, osrm::util::UnorderedMapStorage<NodeID, int>>;
typedef std::vector<IsochroneNode> IsochroneVector;

class IsochronePlugin final : public BasePlugin
{
  private:
    boost::filesystem::path base;
    std::shared_ptr<engine::plugins::SimpleGraph> graph;
    std::vector<osrm::extractor::QueryNode> coordinate_list;
    std::vector<engine::plugins::SimpleEdge> graph_edge_list;
    std::size_t number_of_nodes;

    std::size_t loadGraph(const std::string &path,
                          std::vector<extractor::QueryNode> &coordinate_list,
                          std::vector<SimpleEdge> &graph_edge_list);

    void dijkstraByDuration(IsochroneVector &set, NodeID &source, int duration);
    void dijkstraByDistance(IsochroneVector &isochroneSet, NodeID &source, double distance);
    void update(IsochroneVector &s, IsochroneNode node);

  public:
    explicit IsochronePlugin(const std::string base);

    Status HandleRequest(const std::shared_ptr<datafacade::BaseDataFacade> facade,
                         const api::IsochroneParameters &params,
                         util::json::Object &json_result);
};
}
}
}

#endif // ISOCHRONE_HPP
