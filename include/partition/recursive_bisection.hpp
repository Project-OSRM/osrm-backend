#ifndef OSRM_PARTITION_RECURSIVE_BISECTION_HPP_
#define OSRM_PARTITION_RECURSIVE_BISECTION_HPP_

#include "partition/bisection_graph.hpp"
#include "partition/graph_view.hpp"
#include "partition/recursive_bisection_state.hpp"

#include <cstddef>
#include <vector>

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
                       std::size_t num_optimizing_cuts,
                       BisectionGraph &bisection_graph);

    const std::vector<RecursiveBisectionState::BisectionID> &BisectionIDs() const;

  private:
    BisectionGraph &bisection_graph;
    RecursiveBisectionState internal_state;

    // on larger graphs, SCCs give perfect cuts (think Amerika vs Europe)
    // This function performs an initial pre-partitioning using these sccs.
    std::vector<GraphView> FakeFirstPartitionWithSCC(const std::size_t small_component_size);
};

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_RECURSIVE_BISECTION_HPP_
