#ifndef MATCHING_DEBUG_INFO_HPP
#define MATCHING_DEBUG_INFO_HPP

#include "util/json_logger.hpp"
#include "util/json_util.hpp"
#include "engine/map_matching/hidden_markov_model.hpp"

#include "osrm/coordinate.hpp"

// Provides the debug interface for introspection tools
struct MatchingDebugInfo
{
    MatchingDebugInfo(const osrm::json::Logger *logger) : logger(logger)
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

        osrm::json::Array states;
        for (auto &elem : candidates_list)
        {
            osrm::json::Array timestamps;
            for (auto &elem_s : elem)
            {
                osrm::json::Object state;
                state.values["transitions"] = osrm::json::Array();
                state.values["coordinate"] =
                    osrm::json::make_array(elem_s.phantom_node.location.lat / COORDINATE_PRECISION,
                                           elem_s.phantom_node.location.lon / COORDINATE_PRECISION);
                state.values["viterbi"] =
                    osrm::json::clamp_float(osrm::matching::IMPOSSIBLE_LOG_PROB);
                state.values["pruned"] = 0u;
                timestamps.values.push_back(state);
            }
            states.values.push_back(timestamps);
        }
        osrm::json::get(*object, "states") = states;
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

        osrm::json::Object transistion;
        transistion.values["to"] = osrm::json::make_array(current_t, current_state);
        transistion.values["properties"] = osrm::json::make_array(
            osrm::json::clamp_float(prev_viterbi), osrm::json::clamp_float(emission_pr),
            osrm::json::clamp_float(transition_pr), network_distance, haversine_distance);

        osrm::json::get(*object, "states", prev_t, prev_state, "transitions")
            .get<mapbox::util::recursive_wrapper<osrm::json::Array>>()
            .get()
            .values.push_back(transistion);
    }

    void set_viterbi(const std::vector<std::vector<double>> &viterbi,
                     const std::vector<std::vector<bool>> &pruned,
                     const std::vector<std::vector<bool>> &suspicious)
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
                osrm::json::get(*object, "states", t, s_prime, "viterbi") =
                    osrm::json::clamp_float(viterbi[t][s_prime]);
                osrm::json::get(*object, "states", t, s_prime, "pruned") =
                    static_cast<unsigned>(pruned[t][s_prime]);
                osrm::json::get(*object, "states", t, s_prime, "suspicious") =
                    static_cast<unsigned>(suspicious[t][s_prime]);
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

        osrm::json::get(*object, "states", t, s, "chosen") = true;
    }

    void add_breakage(const std::vector<bool> &breakage)
    {
        // json logger not enabled
        if (!logger)
        {
            return;
        }

        osrm::json::get(*object, "breakage") = osrm::json::make_array(breakage);
    }

    const osrm::json::Logger *logger;
    osrm::json::Value *object;
};

#endif // MATCHING_DEBUG_INFO_HPP
