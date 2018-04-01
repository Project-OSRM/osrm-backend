#ifndef OSRM_CUSTOMIZER_FILES_HPP
#define OSRM_CUSTOMIZER_FILES_HPP

#include "customizer/serialization.hpp"

#include "storage/tar.hpp"

#include "util/integer_range.hpp"

namespace osrm
{
namespace customizer
{
namespace files
{

// reads .osrm.cell_metrics file
template <typename CellMetricT>
inline void readCellMetrics(const boost::filesystem::path &path, std::vector<CellMetricT> &metrics)
{
    static_assert(std::is_same<CellMetricView, CellMetricT>::value ||
                      std::is_same<CellMetric, CellMetricT>::value,
                  "");

    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    auto num_metrics = reader.ReadElementCount64("/mld/metrics");
    metrics.resize(num_metrics);

    auto id = 0;
    for (auto &metric : metrics)
    {
        serialization::read(reader, "/mld/metrics/" + std::to_string(id++), metric);
    }
}

// writes .osrm.cell_metrics file
template <typename CellMetricT>
inline void writeCellMetrics(const boost::filesystem::path &path,
                             const std::vector<CellMetricT> &metrics)
{
    static_assert(std::is_same<CellMetricView, CellMetricT>::value ||
                      std::is_same<CellMetric, CellMetricT>::value,
                  "");

    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    writer.WriteElementCount64("/mld/metrics", metrics.size());

    auto id = 0;
    for (const auto &metric : metrics)
    {
        serialization::write(writer, "/mld/metrics/" + std::to_string(id++), metric);
    }
}
}
}
}

#endif
