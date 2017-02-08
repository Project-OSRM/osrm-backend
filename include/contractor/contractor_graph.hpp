#ifndef OSRM_CONTRACTOR_CONTRACTOR_GRAPH_HPP_
#define OSRM_CONTRACTOR_CONTRACTOR_GRAPH_HPP_

#include "util/dynamic_graph.hpp"
#include <algorithm>

namespace osrm
{
namespace contractor
{

struct ContractorEdgeData
{
    ContractorEdgeData()
        : weight(0), duration(0), id(0), originalEdges(0), shortcut(0), forward(0), backward(0),
          is_original_via_node_ID(false)
    {
    }
    ContractorEdgeData(EdgeWeight weight,
                       EdgeWeight duration,
                       unsigned original_edges,
                       unsigned id,
                       bool shortcut,
                       bool forward,
                       bool backward)
        : weight(weight), duration(duration), id(id),
          originalEdges(std::min((1u << 28) - 1u, original_edges)), shortcut(shortcut),
          forward(forward), backward(backward), is_original_via_node_ID(false)
    {
    }
    EdgeWeight weight;
    EdgeWeight duration;
    unsigned id;
    unsigned originalEdges : 28;
    bool shortcut : 1;
    bool forward : 1;
    bool backward : 1;
    bool is_original_via_node_ID : 1;
};

using ContractorGraph = util::DynamicGraph<ContractorEdgeData>;
using ContractorEdge = ContractorGraph::InputEdge;

} // namespace contractor
} // namespace osrm

#endif // OSRM_CONTRACTOR_CONTRACTOR_GRAPH_HPP_
