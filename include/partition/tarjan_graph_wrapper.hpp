#ifndef OSRM_PARTITION_TARJAN_GRAPH_WRAPPER_HPP_
#define OSRM_PARTITION_TARJAN_GRAPH_WRAPPER_HPP_

#include "partition/bisection_graph.hpp"
#include "util/integer_range.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace partition
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

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_TARJAN_GRAPH_WRAPPER_HPP_
