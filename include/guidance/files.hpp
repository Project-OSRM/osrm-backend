#ifndef OSRM_GUIDANCE_FILES_HPP
#define OSRM_GUIDANCE_FILES_HPP

#include "guidance/compact_turn_data.hpp"
#include "guidance/serialization.hpp"
#include "guidance/turn_data_container.hpp"

#include "util/packed_vector.hpp"
#include "util/range_table.hpp"
#include "util/serialization.hpp"

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
    if (reader.HasEntry("/preprocessing/turn_data/turn_instructions"))
    {
        if constexpr (std::is_same_v<TurnDataT, TurnDataView>)
            throw util::exception("Routing turn data must be loaded from the prepared graph");
        else
        {
            const auto original = readOriginalTurnData(path);
            turn_data = TurnDataT{};
            for (auto id : original.Mapping)
                turn_data.push_back({original.Values.GetTurnInstruction(id),
                                     original.Values.GetLaneDataID(id),
                                     original.Values.GetEntryClassID(id),
                                     original.Values.GetPreTurnBearing(id),
                                     original.Values.GetPostTurnBearing(id)});
        }
    }
    else
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
    const auto compact = compactTurnData(turn_data);
    serialization::write(writer, "/preprocessing/turn_data", compact.Values);
    storage::writeCompactVector<EdgeID>(
        writer, "/preprocessing/turn_ids", {compact.Mapping.data(), compact.Mapping.size()});
}
} // namespace osrm::guidance::files

#endif
