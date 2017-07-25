#ifndef OSRM_CUSTOMIZER_FILES_HPP
#define OSRM_CUSTOMIZER_FILES_HPP

#include "customizer/serialization.hpp"

#include "storage/io.hpp"

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

    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    auto num_metrics = reader.ReadElementCount64();
    metrics.resize(num_metrics);

    for (auto& metric : metrics)
    {
        serialization::read(reader, metric);
    }
}

// writes .osrm.cell_metrics file
template <typename CellMetricT>
inline void writeCellMetrics(const boost::filesystem::path &path, const std::vector<CellMetricT> &metrics)
{
    static_assert(std::is_same<CellMetricView, CellMetricT>::value ||
                      std::is_same<CellMetric, CellMetricT>::value,
                  "");

    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    writer.WriteElementCount64(metrics.size());
    for (const auto& metric : metrics)
    {
        serialization::write(writer, metric);
    }
}

}
}
}

#endif
