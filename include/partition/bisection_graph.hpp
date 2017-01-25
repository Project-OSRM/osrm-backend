#ifndef OSRM_BISECTION_GRAPH_HPP_
#define OSRM_BISECTION_GRAPH_HPP_

#include "util/coordinate.hpp"
#include "util/static_graph.hpp"
#include "util/typedefs.hpp"

#include "extractor/edge_based_edge.hpp"

#include <cstddef>

#include <algorithm>
#include <iterator>
#include <tuple>
#include <utility>

namespace osrm
{
namespace partition
{

// Graph node and its corresponding coordinate.
// The coordinate will be used in the partitioning step.
struct BisectionNode
{
    // StaticGraph Node requirement (see static graph traits): .first_edge
    std::size_t first_edge;

    util::Coordinate coordinate;
};

// Graph edge and data for Max-Flow Min-Cut augmentation.
struct BisectionEdge
{
    // StaticGraph Edge requirement (see static graph traits): .target, .data
    NodeID target;

    // TODO: add data for augmentation here. In case we want to keep it completely external, the
    // static graph can be modified to no longer require a .data member by SFINAE-ing out features
    // based on the available compile time traits.
    std::int32_t data;
};

// The graph layout we use as a basis for partitioning.
using BisectionGraph = util::FlexibleStaticGraph<BisectionNode, BisectionEdge>;

template <typename RandomIt> void sortBySourceThenTarget(RandomIt first, RandomIt last)
{
    std::sort(first, last, [](const auto &lhs, const auto &rhs) {
        return std::tie(lhs.source, lhs.target) < std::tie(rhs.source, rhs.target);
    });
}

template <typename InputEdge>
std::vector<BisectionNode> computeNodes(const std::vector<util::Coordinate> &coordinates,
                                        const std::vector<InputEdge> &edges)
{
    std::vector<BisectionNode> result;
    result.reserve(coordinates.size() + 1 /*sentinel*/);

    // find the end of edges that belong to node_id
    const auto advance_edge_itr = [&edges](const std::size_t node_id, auto edge_itr) {
        while (edge_itr != edges.end() && edge_itr->source == node_id)
            ++edge_itr;
        return edge_itr;
    };

    // create a bisection node, requires the ID of the node as well as the lower bound to its edges
    const auto make_bisection_node = [&edges, &coordinates](const std::size_t node_id,
                                                            const auto edge_itr) -> BisectionNode {
        return {static_cast<std::size_t>(std::distance(edges.begin(), edge_itr)),
                coordinates[node_id]};
    };

    auto edge_itr = edges.begin();
    for (std::size_t node_id = 0; node_id < coordinates.size(); ++node_id)
    {
        result.emplace_back(make_bisection_node(node_id,edge_itr));
        edge_itr = advance_edge_itr(node_id,edge_itr);
    }

    auto null_island = util::Coordinate(util::FloatLongitude{0.0}, util::FloatLatitude{0.0});
    auto sentinel = BisectionNode{edges.size(), std::move(null_island)};

    result.emplace_back(std::move(sentinel));

    return result;
}

template <typename InputEdge>
std::vector<BisectionEdge> adaptToBisectionEdge(std::vector<InputEdge> edges)
{
    std::vector<BisectionEdge> result(edges.size());

    std::transform(begin(edges), end(edges), begin(result), [](const auto &edge) {
        return BisectionEdge{edge.target, 1};
    });

    return result;
}

} // namespace partition
} // namespace osrm

#endif // OSRM_BISECTION_GRAPH_HPP_
