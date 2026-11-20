#ifndef OSRM_CUSTOMIZER_FILES_HPP
#define OSRM_CUSTOMIZER_FILES_HPP

#include "customizer/serialization.hpp"
#include "customizer/shared_cell_metrics.hpp"
#include "guidance/query_turn_data.hpp"

#include "storage/tar.hpp"

#include "util/integer_range.hpp"

#include <unordered_map>

namespace osrm::customizer::files
{

// reads .osrm.cell_metrics file
template <typename CellMetricT>
inline void readCellMetrics(const std::filesystem::path &path,
                            std::unordered_map<std::string, std::vector<CellMetricT>> &metrics)
{
    static_assert(std::is_same<CellMetricView, CellMetricT>::value ||
                      std::is_same<CellMetric, CellMetricT>::value,
                  "");

    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    for (auto &pair : metrics)
    {
        const auto &metric_name = pair.first;
        auto &metric_exclude_classes = pair.second;

        auto prefix = "/mld/metrics/" + metric_name + "/exclude";
        auto num_exclude_classes = reader.ReadElementCount64(prefix);
        metric_exclude_classes.resize(num_exclude_classes);

        const auto shared_prefix = "/mld/metrics/" + metric_name + "/shared";
        auto id = 0;
        for (auto &metric : metric_exclude_classes)
        {
            const auto exclude_prefix = prefix + "/" + std::to_string(id);
            if (reader.HasEntry(shared_prefix + "/weights"))
            {
                if (std::is_same_v<CellMetric, CellMetricT> || id == 0)
                {
                    storage::serialization::read(
                        reader, shared_prefix + "/weights", metric.weights);
                    storage::serialization::read(
                        reader, shared_prefix + "/durations", metric.durations);
                    storage::serialization::read(
                        reader, shared_prefix + "/distances", metric.distances);
                }
                storage::serialization::read(
                    reader, exclude_prefix + "/cell_value_offsets", metric.cell_value_offsets);
            }
            else
                serialization::read(reader, exclude_prefix, metric);
            ++id;
        }
    }
}

// writes .osrm.cell_metrics file
template <typename CellMetricT>
inline void
writeCellMetrics(const std::filesystem::path &path,
                 const std::unordered_map<std::string, std::vector<CellMetricT>> &metrics,
                 std::span<const partitioner::CellStorage::CellData> cells)
{
    static_assert(std::is_same<CellMetricView, CellMetricT>::value ||
                      std::is_same<CellMetric, CellMetricT>::value,
                  "");

    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    for (const auto &pair : metrics)
    {
        const auto &metric_name = pair.first;
        const auto &metric_exclude_classes = pair.second;

        const auto shared = shareCellMetrics(cells, metric_exclude_classes);
        const auto prefix = "/mld/metrics/" + metric_name;
        storage::serialization::write(writer, prefix + "/shared/weights", shared.values.weights);
        storage::serialization::write(
            writer, prefix + "/shared/durations", shared.values.durations);
        storage::serialization::write(
            writer, prefix + "/shared/distances", shared.values.distances);
        writer.WriteElementCount64(prefix + "/exclude", shared.offsets.size());
        for (std::size_t i = 0; i < shared.offsets.size(); ++i)
            storage::serialization::write(writer,
                                          prefix + "/exclude/" + std::to_string(i) +
                                              "/cell_value_offsets",
                                          shared.offsets[i]);
    }
}

// reads .osrm.mldgr file
template <typename MultiLevelGraphT>
inline void readGraph(const std::filesystem::path &path,
                      MultiLevelGraphT &graph,
                      std::uint32_t &connectivity_checksum)
{
    static_assert(std::is_same<customizer::MultiLevelEdgeBasedGraphView, MultiLevelGraphT>::value ||
                      std::is_same<customizer::MultiLevelEdgeBasedGraph, MultiLevelGraphT>::value,
                  "");

    storage::tar::FileReader reader{path, storage::tar::FileReader::VerifyFingerprint};

    reader.ReadInto("/mld/connectivity_checksum", connectivity_checksum);
    serialization::read(reader, "/mld/multilevelgraph", graph);
}

// writes .osrm.mldgr file
template <typename MultiLevelGraphT>
inline void writeGraph(const std::filesystem::path &path,
                       const MultiLevelGraphT &graph,
                       const std::uint32_t connectivity_checksum,
                       const guidance::QueryTurnData *turns = nullptr)
{
    static_assert(std::is_same<customizer::MultiLevelEdgeBasedGraphView, MultiLevelGraphT>::value ||
                      std::is_same<customizer::MultiLevelEdgeBasedGraph, MultiLevelGraphT>::value,
                  "");

    storage::tar::FileWriter writer{path, storage::tar::FileWriter::GenerateFingerprint};

    writer.WriteElementCount64("/mld/connectivity_checksum", 1);
    writer.WriteFrom("/mld/connectivity_checksum", connectivity_checksum);
    serialization::write(writer, "/mld/multilevelgraph", graph);
    if (turns)
        turns->write(writer, "/common");
}
} // namespace osrm::customizer::files

#endif
