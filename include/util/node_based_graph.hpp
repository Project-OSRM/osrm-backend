#ifndef NODE_BASED_GRAPH_HPP
#define NODE_BASED_GRAPH_HPP

#include "extractor/class_data.hpp"
#include "extractor/node_based_edge.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/record_spool.hpp"
#include "util/dynamic_graph.hpp"
#include <tbb/parallel_for.h>

namespace osrm::util
{

struct NodeBasedEdgeData
{
    NodeBasedEdgeData()
        : weight(INVALID_EDGE_WEIGHT), duration(INVALID_EDGE_DURATION),
          distance(INVALID_EDGE_DISTANCE), geometry_id({0, false}), reversed(false),
          annotation_data(-1)
    {
    }

    NodeBasedEdgeData(EdgeWeight weight,
                      EdgeDuration duration,
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
    EdgeDuration duration;
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
                               const extractor::RecordSpool<extractor::NodeBasedEdge> &input_edges)
{
    const auto visit = [&](auto emit)
    {
        input_edges.forEachBatch(
            [&](std::span<const extractor::NodeBasedEdge> batch)
            {
                for (const auto &edge : batch)
                {
                    BOOST_ASSERT(edge.flags.forward);
                    BOOST_ASSERT(edge.source != edge.target);
                    BOOST_ASSERT(edge.weight > EdgeWeight{0});
                    BOOST_ASSERT(edge.duration > EdgeDuration{0});
                    BOOST_ASSERT(edge.distance >= EdgeDistance{0});
                    NodeBasedEdgeData data(edge.weight,
                                           edge.duration,
                                           edge.distance,
                                           {0, false},
                                           false,
                                           edge.flags,
                                           edge.annotation_data);
                    emit(edge.source, edge.target, data);
                    if (!edge.flags.is_split)
                    {
                        data.reversed = !edge.flags.backward;
                        emit(edge.target, edge.source, data);
                    }
                }
            });
    };
    auto graph = NodeBasedDynamicGraph::fromDirectedEdges(number_of_nodes, visit);
    tbb::parallel_for(
        NodeID{0}, number_of_nodes, [&](NodeID node) { graph.sortEdgesByTarget(node); });
    return graph;
}
} // namespace osrm::util

#endif // NODE_BASED_GRAPH_HPP
