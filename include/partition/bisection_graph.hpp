#ifndef OSRM_BISECTION_GRAPH_HPP_
#define OSRM_BISECTION_GRAPH_HPP_

#include "util/coordinate.hpp"
#include "util/static_graph.hpp"
#include "util/typedefs.hpp"

#include "extractor/edge_based_edge.hpp"

#include <cstddef>
#include <tuple>

namespace osrm
{
namespace partition
{

// Required for usage in static graph
struct BisectionNode
{
    std::size_t first_edge;
    util::Coordinate cordinate;
};

// The edge is used within a partition
struct BisectionEdge
{
    NodeID target;
    std::int32_t data; // will be one, but we have the space...
};

// workaround of how static graph assumes edges to be formatted :(
using BisectionGraph = util::FlexibleStaticGraph<BisectionNode, BisectionEdge>;

template <typename InputEdge> std::vector<InputEdge> groupBySource(std::vector<InputEdge> edges)
{
    std::sort(edges.begin(), edges.end(), [](const auto &lhs, const auto &rhs) {
        return std::tie(lhs.source, lhs.target) < std::tie(rhs.source, rhs.target);
    });

    return edges;
}

template <typename InputEdge>
std::vector<BisectionNode> computeNodes(const std::vector<util::Coordinate> coordinates,
                                        const std::vector<InputEdge> &edges)
{
    std::vector<BisectionNode> result;
    result.reserve(coordinates.size() + 1);

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

    std::transform(coordinates.begin(),
                   coordinates.end(),
                   std::back_inserter(result),
                   coordinate_to_bisection_node);

    // sentinel element
    result.push_back(
        {edges.size(), util::Coordinate(util::FloatLongitude{0.0}, util::FloatLatitude{0.0})});

    return result;
}

template <typename InputEdge>
std::vector<BisectionEdge> adaptToBisectionEdge(std::vector<InputEdge> edges)
{
    std::vector<BisectionEdge> result;

    result.reserve(edges.size());
    std::transform(edges.begin(),
                   edges.end(),
                   std::back_inserter(result),
                   [](const auto &edge) -> BisectionEdge {
                       return {edge.target, 1};
                   });

    return result;
}

} // namespace partition
} // namespace osrm

#endif // OSRM_BISECTION_GRAPH_HPP_
