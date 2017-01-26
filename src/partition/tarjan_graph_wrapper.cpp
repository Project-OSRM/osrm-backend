#include "partition/tarjan_graph_wrapper.hpp"

namespace osrm
{
namespace partition
{

TarjanGraphWrapper::TarjanGraphWrapper(const BisectionGraph &bisection_graph_)
    : bisection_graph(bisection_graph_)
{
}

std::size_t TarjanGraphWrapper::GetNumberOfNodes() const { return bisection_graph.NumberOfNodes(); }

util::range<EdgeID> TarjanGraphWrapper::GetAdjacentEdgeRange(const NodeID nid) const
{
    const auto &node = bisection_graph.Node(nid);
    return util::irange<EdgeID>(node.edges_begin, node.edges_end);
}

NodeID TarjanGraphWrapper::GetTarget(const EdgeID eid) const
{
    return bisection_graph.Edge(eid).target;
}

} // namespace partition
} // namespace osrm
