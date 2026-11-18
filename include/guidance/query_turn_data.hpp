#ifndef OSRM_GUIDANCE_QUERY_TURN_DATA_HPP
#define OSRM_GUIDANCE_QUERY_TURN_DATA_HPP

#include "extractor/files.hpp"
#include "guidance/compact_turn_data.hpp"

namespace osrm::guidance
{
struct QueryTurnData
{
    TurnDataContainer Values;
    std::vector<TurnPenalty> Weights;
    std::vector<TurnPenalty> Durations;
    std::vector<EdgeID> Mapping;

    void write(storage::tar::FileWriter &writer, const std::string &prefix) const
    {
        serialization::write(writer, prefix + "/turn_data", Values);
        storage::serialization::write(writer, prefix + "/turn_penalty/weight", Weights);
        storage::serialization::write(writer, prefix + "/turn_penalty/duration", Durations);
    }
};

inline QueryTurnData makeQueryTurnData(const std::filesystem::path &base)
{
    auto original = readOriginalTurnData(base.string() + ".osrm.edges");
    std::vector<TurnPenalty> weights, durations;
    extractor::files::readTurnWeightPenalty(base.string() + ".osrm.turn_weight_penalties", weights);
    extractor::files::readTurnDurationPenalty(base.string() + ".osrm.turn_duration_penalties",
                                              durations);
    if (weights.size() != original.Mapping.size() || durations.size() != weights.size())
        throw util::exception("Turn costs and original identities do not agree");
    QueryTurnData result;
    result.Mapping.resize(weights.size());
    std::unordered_map<std::uint64_t, EdgeID, TurnKeyHash> dictionary;
    dictionary.reserve(original.Values.GetNumberOfTurns());
    for (std::size_t i = 0; i < weights.size(); ++i)
    {
        const auto id = original.Mapping[i];
        const auto key = (std::uint64_t(id) << 32) |
                         (std::uint64_t(std::bit_cast<std::uint16_t>(weights[i])) << 16) |
                         std::bit_cast<std::uint16_t>(durations[i]);
        const auto [it, inserted] = dictionary.try_emplace(key, result.Weights.size());
        if (inserted)
        {
            result.Values.push_back({original.Values.GetTurnInstruction(id),
                                     original.Values.GetLaneDataID(id),
                                     original.Values.GetEntryClassID(id),
                                     original.Values.GetPreTurnBearing(id),
                                     original.Values.GetPostTurnBearing(id)});
            result.Weights.push_back(weights[i]);
            result.Durations.push_back(durations[i]);
        }
        result.Mapping[i] = it->second;
    }
    return result;
}
} // namespace osrm::guidance
#endif
