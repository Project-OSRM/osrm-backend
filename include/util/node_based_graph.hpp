#ifndef NODE_BASED_GRAPH_HPP
#define NODE_BASED_GRAPH_HPP

#include "extractor/class_data.hpp"
#include "extractor/node_based_edge.hpp"
#include "extractor/node_data_container.hpp"
#include "util/dynamic_graph.hpp"
#include "util/graph_utils.hpp"

#include <tbb/parallel_sort.h>

#include <iostream>
#include <memory>
#include <utility>

namespace osrm
{
namespace util
{

struct NodeBasedEdgeData
{
    NodeBasedEdgeData()
        : weight(INVALID_EDGE_WEIGHT), duration(INVALID_EDGE_WEIGHT),
          distance(INVALID_EDGE_DISTANCE), geometry_id({0, false}), reversed(false),
          annotation_data(-1)
    {
    }

    NodeBasedEdgeData(EdgeWeight weight,
                      EdgeWeight duration,
                      EdgeDistance distance,
                      GeometryID geometry_id,
                      bool reversed,
                      extractor::NodeBasedEdgeClassification flags,
                      AnnotationID annotation_data)
        : weight(weight), duration(duration), distance(distance), geometry_id(geometry_id),
          reversed(reversed), flags(flags), annotation_data(annotation_data)
    {
    }

    EdgeWeight weight;
    EdgeWeight duration;
    EdgeDistance distance;
    GeometryID geometry_id;
    bool reversed : 1;
    extractor::NodeBasedEdgeClassification flags;
    AnnotationID annotation_data;
};

// Check if two edge data elements can be compressed into a single edge (i.e. match in terms of
// their meta-data).
inline bool CanBeCompressed(const NodeBasedEdgeData &lhs,
                            const NodeBasedEdgeData &rhs,
                            const extractor::EdgeBasedNodeDataContainer &node_data_container)
{
    if (!(lhs.flags == rhs.flags))
        return false;

    auto const &lhs_annotation = node_data_container.GetAnnotation(lhs.annotation_data);
    auto const &rhs_annotation = node_data_container.GetAnnotation(rhs.annotation_data);

    if (lhs_annotation.is_left_hand_driving != rhs_annotation.is_left_hand_driving)
        return false;

    if (lhs_annotation.travel_mode != rhs_annotation.travel_mode)
        return false;

    return lhs_annotation.classes == rhs_annotation.classes;
}

using NodeBasedDynamicGraph = DynamicGraph<NodeBasedEdgeData>;

/// Factory method to create NodeBasedDynamicGraph from NodeBasedEdges
/// Since DynamicGraph expects directed edges, we need to insert
/// two edges for undirected edges.
inline NodeBasedDynamicGraph
NodeBasedDynamicGraphFromEdges(NodeID number_of_nodes,
                               const std::vector<extractor::NodeBasedEdge> &input_edge_list)
{
    auto edges_list = directedEdgesFromCompressed<NodeBasedDynamicGraph::InputEdge>(
        input_edge_list,
        [](NodeBasedDynamicGraph::InputEdge &output_edge,
           const extractor::NodeBasedEdge &input_edge) {
            output_edge.data.weight = input_edge.weight;
            output_edge.data.duration = input_edge.duration;
            output_edge.data.distance = input_edge.distance;
            output_edge.data.flags = input_edge.flags;
            output_edge.data.annotation_data = input_edge.annotation_data;

            BOOST_ASSERT(output_edge.data.weight > 0);
            BOOST_ASSERT(output_edge.data.duration > 0);
            BOOST_ASSERT(output_edge.data.distance >= 0);
        });

    tbb::parallel_sort(edges_list.begin(), edges_list.end());

    return NodeBasedDynamicGraph(number_of_nodes, edges_list);
}
}
}

#endif // NODE_BASED_GRAPH_HPP
