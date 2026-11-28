#ifndef OSRM_GUIDANCE_COMPACT_TURN_DATA_HPP
#define OSRM_GUIDANCE_COMPACT_TURN_DATA_HPP

#include "guidance/serialization.hpp"
#include "storage/block_vector_encoding.hpp"
#include <bit>
#include <unordered_map>

namespace osrm::guidance
{
struct TurnKeyHash
{
    std::size_t operator()(std::uint64_t value) const
    {
        value ^= value >> 30;
        value *= 0xbf58476d1ce4e5b9ULL;
        value ^= value >> 27;
        value *= 0x94d049bb133111ebULL;
        return value ^ (value >> 31);
    }
};

struct OriginalTurnData
{
    TurnDataContainer Values;
    std::vector<EdgeID> Mapping;
    std::uint32_t Checksum = 0;
};

template <typename TurnDataT> OriginalTurnData compactTurnData(const TurnDataT &data)
{
    OriginalTurnData result;
    std::unordered_map<std::uint64_t, EdgeID, TurnKeyHash> dictionary;
    dictionary.reserve(std::min<std::size_t>(data.GetNumberOfTurns(), 1u << 20));
    result.Mapping.reserve(data.GetNumberOfTurns());
    for (std::size_t i = 0; i < data.GetNumberOfTurns(); ++i)
    {
        const auto instruction = data.GetTurnInstruction(i);
        const auto pre = data.GetPreTurnBearing(i), post = data.GetPostTurnBearing(i);
        const auto lane = data.GetLaneDataID(i), entry = data.GetEntryClassID(i);
        const auto key = std::uint64_t(std::bit_cast<std::uint8_t>(instruction)) |
                         (std::uint64_t(lane) << 8) | (std::uint64_t(entry) << 24) |
                         (std::uint64_t(std::bit_cast<std::uint8_t>(pre)) << 40) |
                         (std::uint64_t(std::bit_cast<std::uint8_t>(post)) << 48);
        const auto [it, inserted] = dictionary.try_emplace(key, result.Values.GetNumberOfTurns());
        if (inserted)
            result.Values.push_back({instruction, lane, entry, pre, post});
        result.Mapping.push_back(it->second);
    }
    return result;
}

inline OriginalTurnData readOriginalTurnData(const std::filesystem::path &path)
{
    storage::tar::FileReader reader(path, storage::tar::FileReader::VerifyFingerprint);
    OriginalTurnData result;
    if (reader.HasEntry("/preprocessing/turn_data/turn_instructions"))
    {
        serialization::read(reader, "/preprocessing/turn_data", result.Values);
        storage::readCompactVector(reader, "/preprocessing/turn_ids", result.Mapping);
        for (auto id : result.Mapping)
            if (id >= result.Values.GetNumberOfTurns())
                throw util::exception("Invalid original turn identity");
    }
    else
    {
        TurnDataContainer original;
        serialization::read(reader, "/common/turn_data", original);
        result = compactTurnData(original);
    }
    reader.ReadInto("/common/connectivity_checksum", result.Checksum);
    return result;
}
} // namespace osrm::guidance
#endif
