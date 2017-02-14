#ifndef NODE_BASED_GRAPH_HPP
#define NODE_BASED_GRAPH_HPP

#include "extractor/guidance/road_classification.hpp"
#include "extractor/node_based_edge.hpp"
#include "util/dynamic_graph.hpp"
#include "util/graph_utils.hpp"

#include <tbb/parallel_sort.h>

#include <memory>

namespace osrm
{
namespace util
{

struct NodeBasedEdgeData
{
    NodeBasedEdgeData()
        : weight(INVALID_EDGE_WEIGHT), duration(INVALID_EDGE_WEIGHT), edge_id(SPECIAL_NODEID),
          name_id(std::numeric_limits<unsigned>::max()), reversed(false), roundabout(false),
          circular(false), travel_mode(TRAVEL_MODE_INACCESSIBLE),
          lane_description_id(INVALID_LANE_DESCRIPTIONID)
    {
    }

    NodeBasedEdgeData(EdgeWeight weight,
                      EdgeWeight duration,
                      unsigned edge_id,
                      unsigned name_id,
                      bool reversed,
                      bool roundabout,
                      bool circular,
                      bool startpoint,
                      bool restricted,
                      extractor::TravelMode travel_mode,
                      const LaneDescriptionID lane_description_id)
        : weight(weight), duration(duration), edge_id(edge_id), name_id(name_id),
          reversed(reversed), roundabout(roundabout), circular(circular), startpoint(startpoint),
          restricted(restricted), travel_mode(travel_mode), lane_description_id(lane_description_id)
    {
    }

    EdgeWeight weight;
    EdgeWeight duration;
    unsigned edge_id;
    unsigned name_id;
    bool reversed : 1;
    bool roundabout : 1;
    bool circular : 1;
    bool startpoint : 1;
    bool restricted : 1;
    extractor::TravelMode travel_mode : 4;
    LaneDescriptionID lane_description_id;
    extractor::guidance::RoadClassification road_classification;

    bool IsCompatibleTo(const NodeBasedEdgeData &other) const
    {
        return (reversed == other.reversed) && (roundabout == other.roundabout) &&
               (circular == other.circular) && (startpoint == other.startpoint) &&
               (travel_mode == other.travel_mode) &&
               (road_classification == other.road_classification) &&
               (restricted == other.restricted);
    }

    bool CanCombineWith(const NodeBasedEdgeData &other) const
    {
        return (name_id == other.name_id) && IsCompatibleTo(other);
    }
};

using NodeBasedDynamicGraph = DynamicGraph<NodeBasedEdgeData>;

/// Factory method to create NodeBasedDynamicGraph from NodeBasedEdges
/// Since DynamicGraph expects directed edges, we need to insert
/// two edges for undirected edges.
inline std::shared_ptr<NodeBasedDynamicGraph>
NodeBasedDynamicGraphFromEdges(NodeID number_of_nodes,
                               const std::vector<extractor::NodeBasedEdge> &input_edge_list)
{
    auto edges_list = directedEdgesFromCompressed<NodeBasedDynamicGraph::InputEdge>(
        input_edge_list,
        [](NodeBasedDynamicGraph::InputEdge &output_edge,
           const extractor::NodeBasedEdge &input_edge) {
            output_edge.data.weight = input_edge.weight;
            output_edge.data.duration = input_edge.duration;
            output_edge.data.roundabout = input_edge.roundabout;
            output_edge.data.circular = input_edge.circular;
            output_edge.data.name_id = input_edge.name_id;
            output_edge.data.travel_mode = input_edge.travel_mode;
            output_edge.data.startpoint = input_edge.startpoint;
            output_edge.data.restricted = input_edge.restricted;
            output_edge.data.road_classification = input_edge.road_classification;
            output_edge.data.lane_description_id = input_edge.lane_description_id;

            BOOST_ASSERT(output_edge.data.weight > 0);
            BOOST_ASSERT(output_edge.data.duration > 0);
        });

    tbb::parallel_sort(edges_list.begin(), edges_list.end());

    auto graph = std::make_shared<NodeBasedDynamicGraph>(number_of_nodes, edges_list);

    return graph;
}
}
}

#endif // NODE_BASED_GRAPH_HPP
