#ifndef OSRM_ENGINE_DATAFACADE_SHARED_CELL_METRICS_HPP
#define OSRM_ENGINE_DATAFACADE_SHARED_CELL_METRICS_HPP

#include "storage/shared_data_index.hpp"
#include "storage/storage_config.hpp"

#include <memory>
#include <string>
#include <vector>

namespace osrm::customizer
{
struct SharedCellMetrics;
}

namespace osrm::engine::datafacade
{
std::unique_ptr<customizer::SharedCellMetrics>
makeSharedCellMetrics(const storage::StorageConfig &config, std::string &metric_name);

void appendSharedCellMetrics(std::vector<storage::SharedDataIndex::AllocatedRegion> &regions,
                             customizer::SharedCellMetrics &metrics,
                             const std::string &metric_name);
} // namespace osrm::engine::datafacade

#endif
