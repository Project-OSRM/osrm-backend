#ifndef OSRM_PARTITION_RECURSIVE_BISECTION_HPP_
#define OSRM_PARTITION_RECURSIVE_BISECTION_HPP_

#include "partition/bisection_graph.hpp"
#include "partition/recursive_bisection_state.hpp"

#include <cstddef>

namespace osrm
{
namespace partition
{

class RecursiveBisection
{
  public:
    RecursiveBisection(std::size_t maximum_cell_size,
                       double balance,
                       double boundary_factor,
                       const BisectionGraph &bisection_graph);

  private:
    const BisectionGraph &bisection_graph;
    RecursiveBisectionState internal_state;
};

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_RECURSIVE_BISECTION_HPP_
