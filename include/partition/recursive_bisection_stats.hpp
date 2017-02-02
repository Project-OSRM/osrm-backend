#ifndef OSRM_PARTITION_RECURSIVE_BISECTION_STATS_HPP_
#define OSRM_PARTITION_RECURSIVE_BISECTION_STATS_HPP_

#include "partition/bisection_graph.hpp"
#include "partition/recursive_bisection_state.hpp"

#include <vector>

namespace osrm
{
namespace partition
{
// generates some statistics on a recursive bisection to describe its quality/parameters
void printBisectionStats(std::vector<RecursiveBisectionState::BisectionID> const &bisection_ids,
                         const BisectionGraph &graph);
} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_RECURSIVE_BISECTION_STATS_HPP_
