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
        : weight(0), duration(0), distance(0), id(0), originalEdges(0), shortcut(0), forward(0),
          backward(0)
    {
    }
    ContractorEdgeData(EdgeWeight weight,
                       EdgeWeight duration,
                       EdgeDistance distance,
                       unsigned original_edges,
                       unsigned id,
                       bool shortcut,
                       bool forward,
                       bool backward)
        : weight(weight), duration(duration), distance(distance), id(id),
          originalEdges(std::min((1u << 29) - 1u, original_edges)), shortcut(shortcut),
          forward(forward), backward(backward)
    {
    }
    EdgeWeight weight;
    EdgeWeight duration;
    EdgeDistance distance;
    unsigned id;
    unsigned originalEdges : 29;
    bool shortcut : 1;
    bool forward : 1;
    bool backward : 1;
};

using ContractorGraph = util::DynamicGraph<ContractorEdgeData>;
using ContractorEdge = ContractorGraph::InputEdge;

} // namespace contractor
} // namespace osrm

#endif // OSRM_CONTRACTOR_CONTRACTOR_GRAPH_HPP_
