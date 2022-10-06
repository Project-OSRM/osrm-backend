#include "osrm/partitioner.hpp"
#include "partitioner/partitioner.hpp"
#include "osrm/partitioner_config.hpp"

namespace osrm
{

// Pimpl-like facade

void partition(const PartitionerConfig &config) { partitioner::Partitioner().Run(config); }

} // namespace osrm
