#ifndef BISECTION_TO_PARTITION_HPP
#define BISECTION_TO_PARTITION_HPP

#include "partition/multi_level_partition.hpp"
#include "partition/recursive_bisection.hpp"

#include <vector>

namespace osrm
{
namespace partition
{

using Partition = std::vector<CellID>;

// Converts a representation of the bisection to cell ids over multiple level
std::tuple<std::vector<Partition>, std::vector<std::uint32_t>>
bisectionToPartition(const std::vector<BisectionID> &node_to_bisection_id,
                     const std::vector<std::size_t> &max_cell_sizes);
}
}

#endif
