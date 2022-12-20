#ifndef OSRM_PARTITIONER_PARTITIONER_HPP_
#define OSRM_PARTITIONER_PARTITIONER_HPP_

#include "partitioner/partitioner_config.hpp"

namespace osrm::partitioner
{

// tool access to the recursive partitioner
class Partitioner
{
  public:
    int Run(const PartitionerConfig &config);
};

} // namespace osrm::partitioner

#endif // OSRM_PARTITIONER_PARTITIONER_HPP_
