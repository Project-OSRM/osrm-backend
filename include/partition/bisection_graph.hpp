#ifndef OSRM_BISECTION_GRAPH_HPP_
#define OSRM_BISECTION_GRAPH_HPP_

#include "util/coordinate.hpp"
#include "util/static_graph.hpp"
#include "util/typedefs.hpp"

#include "extractor/edge_based_edge.hpp"

#include <cstddef>

namespace osrm
{
namespace partition
{

// Required for usage in static graph
struct BisectionNode
{
    EdgeID first_edge;
    util::Coordinate cordinate;
};

// The edge is used within a partition
struct BisectionEdge
{
    NodeID target;
    std::int32_t data; // will be one, but we have the space...
};

// workaround of how static graph assumes edges to be formatted :(
using BisectionGraph = util::FlexibleStaticGraph<BisectionNode,BisectionEdge>;

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
