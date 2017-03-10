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
        if (input_edge.data.weight == INVALID_EDGE_WEIGHT)
            continue;

#ifndef NDEBUG
        const unsigned int constexpr DAY_IN_DECI_SECONDS = 24 * 60 * 60 * 10;
        if (static_cast<unsigned int>(std::max(input_edge.data.weight, 1)) > DAY_IN_DECI_SECONDS)
        {
            util::Log(logWARNING) << "Edge weight large -> "
                                  << static_cast<unsigned int>(std::max(input_edge.data.weight, 1))
                                  << " : " << static_cast<unsigned int>(input_edge.source) << " -> "
                                  << static_cast<unsigned int>(input_edge.target);
        }
#endif
        edges.emplace_back(input_edge.source,
                           input_edge.target,
                           std::max(input_edge.data.weight, 1),
                           input_edge.data.duration,
                           1,
                           input_edge.data.turn_id,
                           false,
                           input_edge.data.forward ? true : false,
                           input_edge.data.backward ? true : false);

        edges.emplace_back(input_edge.target,
                           input_edge.source,
                           std::max(input_edge.data.weight, 1),
                           input_edge.data.duration,
                           1,
                           input_edge.data.turn_id,
                           false,
                           input_edge.data.backward ? true : false,
                           input_edge.data.forward ? true : false);
    }
    // FIXME not sure if we need this
    edges.shrink_to_fit();
    return edges;
}

} // namespace contractor
} // namespace osrm

#endif // OSRM_CONTRACTOR_GRAPH_CONTRACTION_ADAPTORS_HPP_
