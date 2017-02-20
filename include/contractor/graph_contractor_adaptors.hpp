#ifndef OSRM_CONTRACTOR_GRAPH_CONTRACTION_ADAPTORS_HPP_
#define OSRM_CONTRACTOR_GRAPH_CONTRACTION_ADAPTORS_HPP_

#include "contractor/contractor_graph.hpp"
#include "util/log.hpp"

#include <vector>

namespace osrm
{
namespace contractor
{

// Make sure to move in the input edge list!
template <typename InputEdgeContainer>
std::vector<ContractorEdge> adaptToContractorInput(InputEdgeContainer input_edge_list)
{
    std::vector<ContractorEdge> edges;
    edges.reserve(input_edge_list.size() * 2);

    for (const auto &input_edge : input_edge_list)
    {
#ifndef NDEBUG
        if (input_edge.duration > 24 * 60 * 60 * 10)
        {
            util::Log(logWARNING) << "Edge duration large -> " << input_edge.duration << ", weight "
                                  << input_edge.weight << " : " << input_edge.source << " -> "
                                  << input_edge.target;
        }
#endif
        edges.emplace_back(input_edge.source,
                           input_edge.target,
                           std::max(input_edge.weight, EdgeWeight{1}),
                           EdgeDuration{input_edge.duration},
                           1,
                           input_edge.edge_id,
                           false,
                           input_edge.forward ? true : false,
                           input_edge.backward ? true : false);

        edges.emplace_back(input_edge.target,
                           input_edge.source,
                           std::max(input_edge.weight, EdgeWeight{1}),
                           EdgeDuration{input_edge.duration},
                           1,
                           input_edge.edge_id,
                           false,
                           input_edge.backward ? true : false,
                           input_edge.forward ? true : false);
    }
    // FIXME not sure if we need this
    edges.shrink_to_fit();
    return edges;
}

} // namespace contractor
} // namespace osrm

#endif // OSRM_CONTRACTOR_GRAPH_CONTRACTION_ADAPTORS_HPP_
