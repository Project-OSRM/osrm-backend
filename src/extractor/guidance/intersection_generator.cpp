#include "extractor/guidance/intersection_generator.hpp"

#include "extractor/geojson_debug_policies.hpp"

#include "util/geojson_debug_logger.hpp"

#include "util/assert.hpp"
#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/log.hpp"

#include <algorithm>
#include <cmath>
#include <functional> // mem_fn
#include <limits>
#include <numeric>
#include <utility>

#include <boost/range/algorithm/count_if.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{
namespace
{
const constexpr bool USE_LOW_PRECISION_MODE = true;
// the inverse of use low precision mode
const constexpr bool USE_HIGH_PRECISION_MODE = !USE_LOW_PRECISION_MODE;
}

IntersectionGenerator::IntersectionGenerator(const util::NodeBasedDynamicGraph &node_based_graph,
                                             const EdgeBasedNodeDataContainer &node_data_container,
                                             const RestrictionMap &restriction_map,
                                             const std::unordered_set<NodeID> &barrier_nodes,
                                             const std::vector<util::Coordinate> &coordinates,
                                             const CompressedEdgeContainer &)
    : node_based_graph(node_based_graph), node_data_container(node_data_container),
      restriction_map(restriction_map), barrier_nodes(barrier_nodes), coordinates(coordinates)
{
}

IntersectionGenerationParameters
IntersectionGenerator::SkipDegreeTwoNodes(const NodeID starting_node, const EdgeID via_edge) const
{
    NodeID query_node = starting_node;
    EdgeID query_edge = via_edge;

    const auto get_next_edge = [this](const NodeID from, const EdgeID via) {
        const NodeID new_node = node_based_graph.GetTarget(via);
        BOOST_ASSERT(node_based_graph.GetOutDegree(new_node) == 2);
        const EdgeID begin_edges_new_node = node_based_graph.BeginEdges(new_node);
        return (node_based_graph.GetTarget(begin_edges_new_node) == from) ? begin_edges_new_node + 1
                                                                          : begin_edges_new_node;
    };

    std::unordered_set<NodeID> visited_nodes;
    // skip trivial nodes without generating the intersection in between, stop at the very first
    // intersection of degree > 2
    while (0 == visited_nodes.count(query_node) &&
           2 == node_based_graph.GetOutDegree(node_based_graph.GetTarget(query_edge)))
    {
        visited_nodes.insert(query_node);
        const auto next_node = node_based_graph.GetTarget(query_edge);
        const auto next_edge = get_next_edge(query_node, query_edge);

        query_node = next_node;
        query_edge = next_edge;

        // check if there is a relevant change in the graph
        if (!CanBeCompressed(node_based_graph.GetEdgeData(query_edge),
                             node_based_graph.GetEdgeData(next_edge),
                             node_data_container) ||
            (node_based_graph.GetTarget(next_edge) == starting_node))
            break;
    }

    return {query_node, query_edge};
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
