#ifndef MATCHING_DEBUG_INFO_HPP
#define MATCHING_DEBUG_INFO_HPP

#include "util/json_logger.hpp"
#include "util/json_util.hpp"
#include "engine/map_matching/hidden_markov_model.hpp"

#include "osrm/coordinate.hpp"

namespace osrm
{
namespace util
{

// Provides the debug interface for introspection tools
struct MatchingDebugInfo
{
    MatchingDebugInfo(const json::Logger *logger) : logger(logger)
    {
        if (logger)
        {
            object = &logger->map->at("matching");
        }
    }

    template <class CandidateLists> void initialize(const CandidateLists &candidates_list)
    {
        // json logger not enabled
        if (!logger)
        {
            return;
        }

        json::Array states;
        for (auto &elem : candidates_list)
        {
            json::Array timestamps;
            for (auto &elem_s : elem)
            {
                json::Object state;
                state.values["transitions"] = json::Array();
                state.values["coordinate"] = json::make_array(
                    static_cast<double>(toFloating(elem_s.phantom_node.location.lat)),
                    static_cast<double>(toFloating(elem_s.phantom_node.location.lon)));
                state.values["viterbi"] =
                    json::clamp_float(engine::map_matching::IMPOSSIBLE_LOG_PROB);
                state.values["pruned"] = 0u;
                timestamps.values.push_back(state);
            }
            states.values.push_back(timestamps);
        }
        json::get(*object, "states") = states;
    }

    void add_transition_info(const unsigned prev_t,
                             const unsigned current_t,
                             const unsigned prev_state,
                             const unsigned current_state,
                             const double prev_viterbi,
                             const double emission_pr,
                             const double transition_pr,
                             const double network_distance,
                             const double haversine_distance)
    {
        // json logger not enabled
        if (!logger)
        {
            return;
        }

        json::Object transistion;
        transistion.values["to"] = json::make_array(current_t, current_state);
        transistion.values["properties"] = json::make_array(
            json::clamp_float(prev_viterbi), json::clamp_float(emission_pr),
            json::clamp_float(transition_pr), network_distance, haversine_distance);

        json::get(*object, "states", prev_t, prev_state, "transitions")
            .get<mapbox::util::recursive_wrapper<json::Array>>()
            .get()
            .values.push_back(transistion);
    }

    void set_viterbi(const std::vector<std::vector<double>> &viterbi,
                     const std::vector<std::vector<bool>> &pruned)
    {
        // json logger not enabled
        if (!logger)
        {
            return;
        }

        for (auto t = 0u; t < viterbi.size(); t++)
        {
            for (auto s_prime = 0u; s_prime < viterbi[t].size(); ++s_prime)
            {
                json::get(*object, "states", t, s_prime, "viterbi") =
                    json::clamp_float(viterbi[t][s_prime]);
                json::get(*object, "states", t, s_prime, "pruned") =
                    static_cast<unsigned>(pruned[t][s_prime]);
            }
        }
    }

    void add_chosen(const unsigned t, const unsigned s)
    {
        // json logger not enabled
        if (!logger)
        {
            return;
        }

        json::get(*object, "states", t, s, "chosen") = true;
    }

    void add_breakage(const std::vector<bool> &breakage)
    {
        // json logger not enabled
        if (!logger)
        {
            return;
        }

        // convert std::vector<bool> to osrm::json::Array
        json::Array a;
        for (const bool v : breakage)
        {
            if (v)
                a.values.emplace_back(json::True());
            else
                a.values.emplace_back(json::False());
        }

        json::get(*object, "breakage") = std::move(a);
    }

    const json::Logger *logger;
    json::Value *object;
};
}
}

#endif // MATCHING_DEBUG_INFO_HPP
