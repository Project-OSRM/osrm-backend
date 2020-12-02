#ifndef OSRM_CONTRACTOR_FILES_HPP
#define OSRM_CONTRACTOR_FILES_HPP

#include "contractor/serialization.hpp"

#include <unordered_map>

namespace osrm
{
namespace contractor
{
namespace files
{
// reads .osrm.hsgr file
template <typename ContractedMetricT>
inline void readGraph(const boost::filesystem::path &path,
                      std::unordered_map<std::string, ContractedMetricT> &metrics,
                      std::uint32_t &connectivity_checksum)
{
    static_assert(std::is_same<ContractedMetric, ContractedMetricT>::value ||
                      std::is_same<ContractedMetricView, ContractedMetricT>::value,
                  "metric must be of type ContractedMetric<>");

    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    reader.ReadInto("/ch/connectivity_checksum", connectivity_checksum);

    for (auto &pair : metrics)
    {
        serialization::read(reader, "/ch/metrics/" + pair.first, pair.second);
    }
}

// writes .osrm.hsgr file
template <typename ContractedMetricT>
inline void writeGraph(const boost::filesystem::path &path,
                       const std::unordered_map<std::string, ContractedMetricT> &metrics,
                       const std::uint32_t connectivity_checksum)
{
    static_assert(std::is_same<ContractedMetric, ContractedMetricT>::value ||
                      std::is_same<ContractedMetricView, ContractedMetricT>::value,
                  "metric must be of type ContractedMetric<>");
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    writer.WriteElementCount64("/ch/connectivity_checksum", 1);
    writer.WriteFrom("/ch/connectivity_checksum", connectivity_checksum);

    for (const auto &pair : metrics)
    {
        serialization::write(writer, "/ch/metrics/" + pair.first, pair.second);
    }
}
} // namespace files
} // namespace contractor
} // namespace osrm

#endif
