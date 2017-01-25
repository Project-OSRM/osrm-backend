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
    std::vector<BisectionNode> result(coordinates.size() + 1 /*sentinel*/);

    // stateful transform, counting node ids and moving the edge itr forward
    const auto coordinate_to_bisection_node =
        [ edge_itr = edges.begin(), node_id = 0u, &edges ](
            const util::Coordinate coordinate) mutable->BisectionNode
    {
        const auto edges_of_node = edge_itr;

        // count all edges with this source
        while (edge_itr != edges.end() && edge_itr->source == node_id)
            ++edge_itr;

        // go to the next node
        ++node_id;

        return {static_cast<std::size_t>(std::distance(edges.begin(), edges_of_node)), coordinate};
    };

    std::transform(
        begin(coordinates), end(coordinates), begin(result), coordinate_to_bisection_node);

    auto null_island = util::Coordinate(util::FloatLongitude{0.0}, util::FloatLatitude{0.0});
    auto sentinel = BisectionNode{edges.size(), std::move(null_island)};

    result.back() = std::move(sentinel);

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
