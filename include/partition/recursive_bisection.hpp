#ifndef OSRM_PARTITION_RECURSIVE_BISECTION_HPP_
#define OSRM_PARTITION_RECURSIVE_BISECTION_HPP_

#include "partition/bisection_graph.hpp"
#include "partition/graph_view.hpp"
#include "partition/recursive_bisection_state.hpp"
#include "util/typedefs.hpp"

#include <cstddef>
#include <vector>

namespace osrm
{
namespace partition
{

class RecursiveBisection
{
  public:
    RecursiveBisection(BisectionGraph &bisection_graph,
                       const std::size_t maximum_cell_size,
                       const double balance,
                       const double boundary_factor,
                       const std::size_t num_optimizing_cuts,
                       const std::size_t small_component_size);

    const std::vector<BisectionID> &BisectionIDs() const;

    std::uint32_t SCCDepth() const;

  private:
    BisectionGraph &bisection_graph;
    RecursiveBisectionState internal_state;
};

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_RECURSIVE_BISECTION_HPP_
