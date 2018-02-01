#ifndef OSRM_PARTITIONER_PARTITIONER_HPP_
#define OSRM_PARTITIONER_PARTITIONER_HPP_

#include "partitioner/partitioner_config.hpp"

namespace osrm
{
namespace partitioner
{

// tool access to the recursive partitioner
class Partitioner
{
  public:
    int Run(const PartitionerConfig &config);
};

} // namespace partitioner
} // namespace osrm

#endif // OSRM_PARTITIONER_PARTITIONER_HPP_
