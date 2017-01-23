#ifndef OSRM_PARTITION_GRAPHVIEW_HPP_
#define OSRM_PARTITION_GRAPHVIEW_HPP_

#include "partition/bisection_graph.hpp"
#include "partition/recursive_bisection_state.hpp"

#include <boost/iterator/filter_iterator.hpp>

namespace osrm
{
namespace partition
{

struct HasSamePartitionID
{
    HasSamePartitionID(const RecursiveBisectionState::BisectionID bisection_id,
                       const BisectionGraph &bisection_graph,
                       const RecursiveBisectionState &recursive_bisection_state);

    bool operator()(const EdgeID eid) const;

  private:
    const RecursiveBisectionState::BisectionID bisection_id;
    const BisectionGraph &bisection_graph;
    const RecursiveBisectionState &recursive_bisection_state;
};

class GraphView
{
  public:
    using EdgeIterator = boost::filter_iterator<HasSamePartitionID, BisectionGraph::EdgeIterator>;

    GraphView(const BisectionGraph &graph,
              const RecursiveBisectionState &bisection_state,
              const RecursiveBisectionState::IDIterator begin,
              const RecursiveBisectionState::IDIterator end);

    std::size_t NumberOfNodes() const;

    RecursiveBisectionState::IDIterator Begin() const;
    RecursiveBisectionState::IDIterator End() const;

  private:
    const BisectionGraph &graph;
    const RecursiveBisectionState &bisection_state;

    const RecursiveBisectionState::IDIterator begin;
    const RecursiveBisectionState::IDIterator end;
};

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_GRAPHVIEW_HPP_
