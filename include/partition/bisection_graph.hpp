#ifndef OSRM_BISECTION_GRAPH_HPP_
#define OSRM_BISECTION_GRAPH_HPP_

#include "util/coordinate.hpp"
#include "util/typedefs.hpp"

#include "partition/partition_graph.hpp"

#include "extractor/edge_based_edge.hpp"

#include <cstddef>

#include <algorithm>
#include <iterator>
#include <utility>

namespace osrm
{
namespace partition
{

// Graph node and its corresponding coordinate.
// The coordinate will be used in the partitioning step.
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

// Graph edge and data for Max-Flow Min-Cut augmentation.
struct BisectionEdge
{
    BisectionEdge(const NodeID target_ = SPECIAL_NODEID) : target(target_) {}
    // StaticGraph Edge requirement (see static graph traits): .target, .data
    NodeID target;
};

// The graph layout we use as a basis for partitioning.
using RemappableGraphNode = NodeEntryWrapper<BisectionNode>;
using BisectionInputEdge = GraphConstructionWrapper<BisectionEdge>;
using BisectionGraph = RemappableGraph<RemappableGraphNode, BisectionEdge>;

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
    const auto make_bisection_node = [&edges, &coordinates](const std::size_t node_id,
                                                            const auto edge_itr) {
        std::size_t range_begin = std::distance(edges.begin(), edge_itr);
        return BisectionGraph::NodeT(range_begin, range_begin, coordinates[node_id], node_id);
    };

    auto edge_itr = edges.begin();
    for (std::size_t node_id = 0; node_id < coordinates.size(); ++node_id)
    {
        result_nodes.emplace_back(make_bisection_node(node_id, edge_itr));
        edge_itr = advance_edge_itr(node_id, edge_itr);
        result_nodes.back().edges_end = std::distance(edges.begin(), edge_itr);
    }

    return BisectionGraph(std::move(result_nodes), std::move(result_edges));
}

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

} // namespace partition
} // namespace osrm

#endif // OSRM_BISECTION_GRAPH_HPP_
