#ifndef OSRM_GUIDANCE_FILES_HPP
#define OSRM_GUIDANCE_FILES_HPP

#include "guidance/serialization.hpp"
#include "guidance/turn_data_container.hpp"

#include "util/packed_vector.hpp"
#include "util/range_table.hpp"
#include "util/serialization.hpp"

#include <boost/assert.hpp>

#include <cstdint>
#include <filesystem>
#include <type_traits>

namespace osrm::guidance::files
{

// reads .osrm.edges
template <typename TurnDataT>
inline void readTurnData(const std::filesystem::path &path,
                         TurnDataT &turn_data,
                         std::uint32_t &connectivity_checksum)
{
    static_assert(std::is_same<guidance::TurnDataContainer, TurnDataT>::value ||
                      std::is_same<guidance::TurnDataView, TurnDataT>::value ||
                      std::is_same<guidance::TurnDataExternalContainer, TurnDataT>::value,
                  "");
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    reader.ReadInto("/common/connectivity_checksum", connectivity_checksum);
    serialization::read(reader, "/common/turn_data", turn_data);
}

// writes .osrm.edges
template <typename TurnDataT>
inline void writeTurnData(const std::filesystem::path &path,
                          const TurnDataT &turn_data,
                          const std::uint32_t connectivity_checksum)
{
    static_assert(std::is_same<guidance::TurnDataContainer, TurnDataT>::value ||
                      std::is_same<guidance::TurnDataView, TurnDataT>::value ||
                      std::is_same<guidance::TurnDataExternalContainer, TurnDataT>::value,
                  "");
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    writer.WriteElementCount64("/common/connectivity_checksum", 1);
    writer.WriteFrom("/common/connectivity_checksum", connectivity_checksum);
    serialization::write(writer, "/common/turn_data", turn_data);
}
} // namespace osrm::guidance::files

#endif
