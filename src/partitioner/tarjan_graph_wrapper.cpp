#include "partitioner/tarjan_graph_wrapper.hpp"

namespace osrm::partitioner
{

TarjanGraphWrapper::TarjanGraphWrapper(const BisectionGraph &bisection_graph_)
    : bisection_graph(bisection_graph_)
{
}

std::size_t TarjanGraphWrapper::GetNumberOfNodes() const { return bisection_graph.NumberOfNodes(); }

util::range<EdgeID> TarjanGraphWrapper::GetAdjacentEdgeRange(const NodeID nid) const
{
    return util::irange<EdgeID>(bisection_graph.BeginEdgeID(nid), bisection_graph.EndEdgeID(nid));
}

NodeID TarjanGraphWrapper::GetTarget(const EdgeID eid) const
{
    return bisection_graph.Edge(eid).target;
}

} // namespace osrm::partitioner
