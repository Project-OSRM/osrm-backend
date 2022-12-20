#ifndef OSRM_PARTITIONER_TARJAN_GRAPH_WRAPPER_HPP_
#define OSRM_PARTITIONER_TARJAN_GRAPH_WRAPPER_HPP_

#include "partitioner/bisection_graph.hpp"
#include "util/integer_range.hpp"
#include "util/typedefs.hpp"

namespace osrm::partitioner
{

class TarjanGraphWrapper
{
  public:
    TarjanGraphWrapper(const BisectionGraph &bisection_graph);

    std::size_t GetNumberOfNodes() const;
    util::range<EdgeID> GetAdjacentEdgeRange(const NodeID nid) const;
    NodeID GetTarget(const EdgeID eid) const;

  protected:
    const BisectionGraph &bisection_graph;
};

} // namespace osrm::partitioner

#endif // OSRM_PARTITIONER_TARJAN_GRAPH_WRAPPER_HPP_
