#ifndef OSRM_PARTITION_GRAPHVIEW_HPP_
#define OSRM_PARTITION_GRAPHVIEW_HPP_

#include "partition/bisection_graph.hpp"
#include "partition/recursive_bisection_state.hpp"

namespace osrm
{
namespace partition
{

class GraphView
{
  public:
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
