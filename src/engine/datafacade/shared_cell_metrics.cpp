#include "engine/datafacade/shared_cell_metrics.hpp"

#include "customizer/shared_cell_metrics.hpp"
#include "extractor/files.hpp"
#include "partitioner/files.hpp"
#include "storage/storage.hpp"
#include "storage/view_factory.hpp"
#include "util/mmap_file.hpp"

namespace osrm::engine::datafacade
{
std::unique_ptr<customizer::SharedCellMetrics>
makeSharedCellMetrics(const storage::StorageConfig &config, std::string &metric_name)
{
    extractor::ProfileProperties properties;
    extractor::files::readProfileProperties(config.GetPath(".osrm.properties"), properties);
    metric_name = properties.GetWeightName();
    const auto path = config.GetPath(".osrm.cell_metrics");
    storage::tar::FileReader reader(path, storage::tar::FileReader::VerifyFingerprint);
    if (reader.HasEntry("/mld/metrics/" + metric_name + "/shared/weights"))
        return nullptr;
    partitioner::CellStorage cells;
    partitioner::files::readCells(config.GetPath(".osrm.cells"), cells);
    auto layout = std::make_unique<storage::TarDataLayout>();
    storage::populateLayoutFromFile(path, *layout);
    boost::iostreams::mapped_file_source mapped;
    const auto bytes = util::mmapFile<char>(path, mapped);
    std::vector<storage::SharedDataIndex::AllocatedRegion> regions;
    regions.push_back({const_cast<char *>(bytes.data()), std::move(layout)});
    const storage::SharedDataIndex index{std::move(regions)};
    const auto metrics = storage::make_cell_metric_view(index, "/mld/metrics/" + metric_name);
    return std::make_unique<customizer::SharedCellMetrics>(
        customizer::shareCellMetrics(cells.GetCellData(), metrics));
}

void appendSharedCellMetrics(std::vector<storage::SharedDataIndex::AllocatedRegion> &regions,
                             customizer::SharedCellMetrics &metrics,
                             const std::string &metric_name)
{
    const auto add = [&](const std::string &name, auto &values)
    {
        using Value = typename std::decay_t<decltype(values)>::value_type;
        auto layout = std::make_unique<storage::TarDataLayout>();
        layout->SetBlock(name, storage::make_block<Value>(values.size()));
        regions.push_back({values.data(), std::move(layout)});
    };
    const auto prefix = "/mld/metrics/" + metric_name;
    add(prefix + "/shared/weights", metrics.values.weights);
    add(prefix + "/shared/durations", metrics.values.durations);
    add(prefix + "/shared/distances", metrics.values.distances);
    for (std::size_t i = 0; i < metrics.offsets.size(); ++i)
        add(prefix + "/exclude/" + std::to_string(i) + "/cell_value_offsets", metrics.offsets[i]);
}
} // namespace osrm::engine::datafacade
