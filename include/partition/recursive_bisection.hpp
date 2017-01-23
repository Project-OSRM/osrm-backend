#ifndef OSRM_PARTITION_RECURSIVE_BISECTION_HPP_
#define OSRM_PARTITION_RECURSIVE_BISECTION_HPP_

#include "partition/bisection_graph.hpp"
#include "partition/recursive_bisection_state.hpp"

namespace osrm
{
namespace partition
{

class RecursiveBisection
{
  public:
    RecursiveBisection(const BisectionGraph &bisection_graph);

  private:
    const BisectionGraph &bisection_graph;
};

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_RECURSIVE_BISECTION_HPP_
