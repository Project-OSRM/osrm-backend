#ifndef OSRM_PARTITIONER_BISECTION_GRAPH_HPP_
#define OSRM_PARTITIONER_BISECTION_GRAPH_HPP_

#include "util/coordinate.hpp"
#include "util/typedefs.hpp"

#include "partitioner/partition_graph.hpp"

#include "extractor/edge_based_edge.hpp"

#include <cstddef>

#include <algorithm>
#include <iterator>
#include <utility>

namespace osrm
{
namespace partitioner
{

// Node in the bisection graph. We require the original node id (since we remap the nodes all the
// time and can track the correct ID this way). In addtition, the node provides the coordinate its
// located at for use in the inertial flow sorting by slope.
struct BisectionNode
{
    BisectionNode(util::Coordinate coordinate_ = {util::FloatLongitude{0}, util::FloatLatitude{0}},
                  const NodeID original_id_ = SPECIAL_NODEID)
        : coordinate(std::move(coordinate_)), original_id(original_id_)
    {
    }

    // the coordinate the node is located at
    util::Coordinate coordinate;

    // the node id to access the bisection result
    NodeID original_id;
};

// For max-flow/min-cut computations, we operate on a undirected graph. This has some benefits:
// - we don't disconnect the graph more than we have to
// - small components will actually be disconnected (no border nodes)
// - parts of the graph that are clonnected in one way (not reachable/not exitable) will remain
// close to their connected nodes
// As a result, we only require a target as our only data member in the edge.
struct BisectionEdge
{
    BisectionEdge(const NodeID target_ = SPECIAL_NODEID) : target(target_) {}
    // StaticGraph Edge requirement (see static graph traits): .target, .data
    NodeID target;
};

// Aliases for the graph used during the bisection, based on the Remappable graph
using BisectionGraphNode = NodeEntryWrapper<BisectionNode>;
using BisectionInputEdge = GraphConstructionWrapper<BisectionEdge>;
using BisectionGraph = RemappableGraph<BisectionGraphNode, BisectionEdge>;

// Factory method to construct the bisection graph form a set of coordinates and Input Edges (need
// to contain source and target). Edges needs to be labeled from zero
inline BisectionGraph makeBisectionGraph(const std::vector<util::Coordinate> &coordinates,
                                         const std::vector<BisectionInputEdge> &edges)
{
    std::vector<BisectionGraph::NodeT> result_nodes;
    result_nodes.reserve(coordinates.size());
    std::vector<BisectionGraph::EdgeT> result_edges;
    result_edges.reserve(edges.size());

    // find the end of edges that belong to node_id
    const auto advance_edge_itr = [&edges, &result_edges](const std::size_t node_id,
                                                          auto edge_itr) {
        while (edge_itr != edges.end() && edge_itr->source == node_id)
        {
            result_edges.push_back(edge_itr->Reduce());
            ++edge_itr;
        }
        return edge_itr;
    };

    // create a bisection node, requires the ID of the node as well as the lower bound to its edges
    const auto make_bisection_node = [&edges, &coordinates](
        const std::size_t node_id, const auto begin_itr, const auto end_itr) {
        std::size_t range_begin = std::distance(edges.begin(), begin_itr);
        std::size_t range_end = std::distance(edges.begin(), end_itr);
        return BisectionGraph::NodeT(range_begin, range_end, coordinates[node_id], node_id);
    };

    auto edge_itr = edges.begin();
    for (std::size_t node_id = 0; node_id < coordinates.size(); ++node_id)
    {
        const auto begin_itr = edge_itr;
        edge_itr = advance_edge_itr(node_id, edge_itr);
        result_nodes.emplace_back(make_bisection_node(node_id, begin_itr, edge_itr));
    }

    return BisectionGraph(std::move(result_nodes), std::move(result_edges));
}

// Reduce any edge to a fitting input edge for the bisection graph
template <typename InputEdge>
std::vector<BisectionInputEdge> adaptToBisectionEdge(std::vector<InputEdge> edges)
{
    std::vector<BisectionInputEdge> result;
    result.reserve(edges.size());

    std::transform(begin(edges), end(edges), std::back_inserter(result), [](const auto &edge) {
        return BisectionInputEdge{edge.source, edge.target};
    });

    return result;
}

} // namespace partitioner
} // namespace osrm

#endif // OSRM_PARTITIONER_BISECTION_GRAPH_HPP_
