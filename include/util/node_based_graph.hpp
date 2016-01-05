#ifndef NODE_BASED_GRAPH_HPP
#define NODE_BASED_GRAPH_HPP

#include "util/dynamic_graph.hpp"
#include "extractor/import_edge.hpp"
#include "util/graph_utils.hpp"

#include <tbb/parallel_sort.h>

#include <memory>

struct NodeBasedEdgeData
{
    NodeBasedEdgeData()
        : distance(INVALID_EDGE_WEIGHT), edge_id(SPECIAL_NODEID),
          name_id(std::numeric_limits<unsigned>::max()), access_restricted(false), reversed(false),
          roundabout(false), travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    NodeBasedEdgeData(int distance,
                      unsigned edge_id,
                      unsigned name_id,
                      bool access_restricted,
                      bool reversed,
                      bool roundabout,
                      bool startpoint,
                      TravelMode travel_mode)
        : distance(distance), edge_id(edge_id), name_id(name_id),
          access_restricted(access_restricted), reversed(reversed), roundabout(roundabout),
          startpoint(startpoint), travel_mode(travel_mode)
    {
    }

    int distance;
    unsigned edge_id;
    unsigned name_id;
    bool access_restricted : 1;
    bool reversed : 1;
    bool roundabout : 1;
    bool startpoint : 1;
    TravelMode travel_mode : 4;

    bool IsCompatibleTo(const NodeBasedEdgeData &other) const
    {
        return (reversed == other.reversed) && (name_id == other.name_id) &&
               (travel_mode == other.travel_mode);
    }
};

using NodeBasedDynamicGraph = DynamicGraph<NodeBasedEdgeData>;

/// Factory method to create NodeBasedDynamicGraph from NodeBasedEdges
/// Since DynamicGraph expects directed edges, we need to insert
/// two edges for undirected edges.
inline std::shared_ptr<NodeBasedDynamicGraph>
NodeBasedDynamicGraphFromEdges(std::size_t number_of_nodes,
                               const std::vector<NodeBasedEdge> &input_edge_list)
{
    auto edges_list = directedEdgesFromCompressed<NodeBasedDynamicGraph::InputEdge>(
        input_edge_list,
        [](NodeBasedDynamicGraph::InputEdge &output_edge, const NodeBasedEdge &input_edge)
        {
            output_edge.data.distance = static_cast<int>(input_edge.weight);
            BOOST_ASSERT(output_edge.data.distance > 0);

            output_edge.data.roundabout = input_edge.roundabout;
            output_edge.data.name_id = input_edge.name_id;
            output_edge.data.access_restricted = input_edge.access_restricted;
            output_edge.data.travel_mode = input_edge.travel_mode;
            output_edge.data.startpoint = input_edge.startpoint;
        });

    tbb::parallel_sort(edges_list.begin(), edges_list.end());

    auto graph = std::make_shared<NodeBasedDynamicGraph>(
        static_cast<NodeBasedDynamicGraph::NodeIterator>(number_of_nodes), edges_list);

    return graph;
}

#endif // NODE_BASED_GRAPH_HPP
