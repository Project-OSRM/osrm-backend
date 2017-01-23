#ifndef OSRM_PARTITION_PARTITIONER_HPP_
#define OSRM_PARTITION_PARTITIONER_HPP_

#include "partition/partition_config.hpp"

namespace osrm
{
namespace partition
{

// tool access to the recursive partitioner
class Partitioner
{
  public:
    int Run(const PartitionConfig &config);
};

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_PARTITIONER_HPP_
