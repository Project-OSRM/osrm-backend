#ifndef OSRM_PARTITIONER_BISECTION_TO_PARTITION_HPP
#define OSRM_PARTITIONER_BISECTION_TO_PARTITION_HPP

#include "partitioner/multi_level_partition.hpp"
#include "partitioner/recursive_bisection.hpp"

#include <vector>

namespace osrm
{
namespace partitioner
{

using Partition = std::vector<CellID>;

// Converts a representation of the bisection to cell ids over multiple level
std::tuple<std::vector<Partition>, std::vector<std::uint32_t>>
bisectionToPartition(const std::vector<BisectionID> &node_to_bisection_id,
                     const std::vector<std::size_t> &max_cell_sizes);
}
}

#endif
