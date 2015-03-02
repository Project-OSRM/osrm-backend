/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef MAP_MATCHING_HPP
#define MAP_MATCHING_HPP

#include "routing_base.hpp"

#include "../data_structures/coordinate_calculation.hpp"
#include "../util/simple_logger.hpp"
#include "../util/json_util.hpp"
#include "../util/json_logger.hpp"

#include <osrm/json_container.hpp>
#include <variant/variant.hpp>

#include <fstream>

#include <algorithm>
#include <iomanip>
#include <numeric>

namespace Matching
{

struct SubMatching
{
    std::vector<PhantomNode> nodes;
    std::vector<unsigned> indices;
    double length;
    double confidence;
};

using CandidateList = std::vector<std::pair<PhantomNode, double>>;
using CandidateLists = std::vector<CandidateList>;
using SubMatchingList = std::vector<SubMatching>;
constexpr static const unsigned max_number_of_candidates = 10;
constexpr static const double IMPOSSIBLE_LOG_PROB = -std::numeric_limits<double>::infinity();
constexpr static const double MINIMAL_LOG_PROB = -std::numeric_limits<double>::max();
constexpr static const unsigned INVALID_STATE = std::numeric_limits<unsigned>::max();
constexpr static const unsigned MAX_BROKEN_STATES = 6;
constexpr static const unsigned MAX_BROKEN_TIME = 180;
}

// implements a hidden markov model map matching algorithm
template <class DataFacadeT> class MapMatching final
    : public BasicRoutingInterface<DataFacadeT, MapMatching<DataFacadeT>>
{
    using super = BasicRoutingInterface<DataFacadeT, MapMatching<DataFacadeT>>;
    using QueryHeap = SearchEngineData::QueryHeap;
    SearchEngineData &engine_working_data;

    // FIXME this value should be a table based on samples/meter (or samples/min)
    constexpr static const double default_beta = 10.0;
    constexpr static const double default_sigma_z = 4.07;
    constexpr static const double log_2_pi = std::log(2 * M_PI);

    // closures to precompute log -> only simple floating point operations
    struct EmissionLogProbability
    {
        double sigma_z;
        double log_sigma_z;

        EmissionLogProbability(const double sigma_z)
        : sigma_z(sigma_z)
        , log_sigma_z(std::log(sigma_z))
        {
        }

        double operator()(const double distance) const
        {
            return -0.5 * (log_2_pi + (distance / sigma_z) * (distance / sigma_z)) - log_sigma_z;
        }
    };
    struct TransitionLogProbability
    {
        double beta;
        double log_beta;
        TransitionLogProbability(const double beta)
        : beta(beta)
        , log_beta(std::log(beta))
        {
        }

        double operator()(const double d_t) const
        {
            return -log_beta - d_t / beta;
        }
    };

    double get_network_distance(const PhantomNode &source_phantom,
                                const PhantomNode &target_phantom) const
    {
        EdgeWeight upper_bound = INVALID_EDGE_WEIGHT;
        NodeID middle_node = SPECIAL_NODEID;
        EdgeWeight edge_offset = std::min(0, -source_phantom.GetForwardWeightPlusOffset());
        edge_offset = std::min(edge_offset, -source_phantom.GetReverseWeightPlusOffset());

        engine_working_data.InitializeOrClearFirstThreadLocalStorage(
            super::facade->GetNumberOfNodes());
        engine_working_data.InitializeOrClearSecondThreadLocalStorage(
            super::facade->GetNumberOfNodes());

        QueryHeap &forward_heap = *(engine_working_data.forward_heap_1);
        QueryHeap &reverse_heap = *(engine_working_data.reverse_heap_1);

        if (source_phantom.forward_node_id != SPECIAL_NODEID)
        {
            forward_heap.Insert(source_phantom.forward_node_id,
                                -source_phantom.GetForwardWeightPlusOffset(),
                                source_phantom.forward_node_id);
        }
        if (source_phantom.reverse_node_id != SPECIAL_NODEID)
        {
            forward_heap.Insert(source_phantom.reverse_node_id,
                                -source_phantom.GetReverseWeightPlusOffset(),
                                source_phantom.reverse_node_id);
        }

        if (target_phantom.forward_node_id != SPECIAL_NODEID)
        {
            reverse_heap.Insert(target_phantom.forward_node_id,
                                target_phantom.GetForwardWeightPlusOffset(),
                                target_phantom.forward_node_id);
        }
        if (target_phantom.reverse_node_id != SPECIAL_NODEID)
        {
            reverse_heap.Insert(target_phantom.reverse_node_id,
                                target_phantom.GetReverseWeightPlusOffset(),
                                target_phantom.reverse_node_id);
        }

        // search from s and t till new_min/(1+epsilon) > length_of_shortest_path
        while (0 < (forward_heap.Size() + reverse_heap.Size()))
        {
            if (0 < forward_heap.Size())
            {
                super::RoutingStep(
                    forward_heap, reverse_heap, &middle_node, &upper_bound, edge_offset, true);
            }
            if (0 < reverse_heap.Size())
            {
                super::RoutingStep(
                    reverse_heap, forward_heap, &middle_node, &upper_bound, edge_offset, false);
            }
        }

        double distance = std::numeric_limits<double>::max();
        if (upper_bound != INVALID_EDGE_WEIGHT)
        {
            std::vector<NodeID> packed_leg;
            super::RetrievePackedPathFromHeap(forward_heap, reverse_heap, middle_node, packed_leg);
            std::vector<PathData> unpacked_path;
            PhantomNodes nodes;
            nodes.source_phantom = source_phantom;
            nodes.target_phantom = target_phantom;
            super::UnpackPath(packed_leg, nodes, unpacked_path);

            FixedPointCoordinate previous_coordinate = source_phantom.location;
            FixedPointCoordinate current_coordinate;
            distance = 0;
            for (const auto &p : unpacked_path)
            {
                current_coordinate = super::facade->GetCoordinateOfNode(p.node);
                distance += coordinate_calculation::great_circle_distance(previous_coordinate,
                                                                          current_coordinate);
                previous_coordinate = current_coordinate;
            }
            distance += coordinate_calculation::great_circle_distance(previous_coordinate,
                                                                      target_phantom.location);
        }

        return distance;
    }

    struct HiddenMarkovModel
    {
        std::vector<std::vector<double>> viterbi;
        std::vector<std::vector<std::pair<unsigned, unsigned>>> parents;
        std::vector<std::vector<float>> path_lengths;
        std::vector<std::vector<bool>> pruned;
        std::vector<bool> breakage;

        const Matching::CandidateLists &candidates_list;
        const EmissionLogProbability& emission_log_probability;

        HiddenMarkovModel(const Matching::CandidateLists &candidates_list, const EmissionLogProbability& emission_log_probability)
            : breakage(candidates_list.size()), candidates_list(candidates_list), emission_log_probability(emission_log_probability)
        {
            for (const auto &l : candidates_list)
            {
                viterbi.emplace_back(l.size());
                parents.emplace_back(l.size());
                path_lengths.emplace_back(l.size());
                pruned.emplace_back(l.size());
            }

            clear(0);
        }

        void clear(unsigned initial_timestamp)
        {
            BOOST_ASSERT(viterbi.size() == parents.size() &&
                         parents.size() == path_lengths.size() &&
                         path_lengths.size() == pruned.size() && pruned.size() == breakage.size());

            for (unsigned t = initial_timestamp; t < viterbi.size(); t++)
            {
                std::fill(viterbi[t].begin(), viterbi[t].end(), Matching::IMPOSSIBLE_LOG_PROB);
                std::fill(parents[t].begin(), parents[t].end(), std::make_pair(0u, 0u));
                std::fill(path_lengths[t].begin(), path_lengths[t].end(), 0);
                std::fill(pruned[t].begin(), pruned[t].end(), true);
            }
            std::fill(breakage.begin() + initial_timestamp, breakage.end(), true);
        }

        unsigned initialize(unsigned initial_timestamp)
        {
            BOOST_ASSERT(initial_timestamp < candidates_list.size());

            do
            {
                for (auto s = 0u; s < viterbi[initial_timestamp].size(); ++s)
                {
                    viterbi[initial_timestamp][s] =
                        emission_log_probability(candidates_list[initial_timestamp][s].second);
                    parents[initial_timestamp][s] = std::make_pair(initial_timestamp, s);
                    pruned[initial_timestamp][s] =
                        viterbi[initial_timestamp][s] < Matching::MINIMAL_LOG_PROB;

                    breakage[initial_timestamp] =
                        breakage[initial_timestamp] && pruned[initial_timestamp][s];
                }

                ++initial_timestamp;
            } while (breakage[initial_timestamp - 1]);

            if (initial_timestamp >= viterbi.size())
            {
                return Matching::INVALID_STATE;
            }

            BOOST_ASSERT(initial_timestamp > 0);
            --initial_timestamp;

            BOOST_ASSERT(breakage[initial_timestamp] == false);

            return initial_timestamp;
        }
    };

    // Provides the debug interface for introspection tools
    struct DebugInfo
    {
        DebugInfo(const osrm::json::Logger* logger)
        : logger(logger)
        {
            if (logger)
            {
                object = &logger->map->at("matching");
            }
        }

        void initialize(const HiddenMarkovModel& model,
                        unsigned initial_timestamp,
                        const Matching::CandidateLists& candidates_list)
        {
            // json logger not enabled
            if (!logger)
                return;

            osrm::json::Array states;
            for (unsigned t = 0; t < candidates_list.size(); t++)
            {
                osrm::json::Array timestamps;
                for (unsigned s = 0; s < candidates_list[t].size(); s++)
                {
                    osrm::json::Object state;
                    state.values["transitions"] = osrm::json::Array();
                    state.values["coordinate"] = osrm::json::make_array(
                        candidates_list[t][s].first.location.lat / COORDINATE_PRECISION,
                        candidates_list[t][s].first.location.lon / COORDINATE_PRECISION);
                    if (t < initial_timestamp)
                    {
                        state.values["viterbi"] = osrm::json::clamp_float(Matching::IMPOSSIBLE_LOG_PROB);
                        state.values["pruned"] = 0u;
                    }
                    else if (t == initial_timestamp)
                    {
                        state.values["viterbi"] = osrm::json::clamp_float(model.viterbi[t][s]);
                        state.values["pruned"] = static_cast<unsigned>(model.pruned[t][s]);
                    }
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
                                 const double great_circle_distance)
        {
            // json logger not enabled
            if (!logger)
                return;

            osrm::json::Object transistion;
            transistion.values["to"] = osrm::json::make_array(current_t, current_state);
            transistion.values["properties"] =
                osrm::json::make_array(osrm::json::clamp_float(prev_viterbi),
                                       osrm::json::clamp_float(emission_pr),
                                       osrm::json::clamp_float(transition_pr),
                                       network_distance,
                                       great_circle_distance);

            osrm::json::get(*object, "states", prev_t, prev_state, "transitions")
                .get<mapbox::util::recursive_wrapper<osrm::json::Array>>()
                .get().values.push_back(transistion);

        }

        void add_viterbi(const unsigned t,
                         const std::vector<double>& current_viterbi,
                         const std::vector<bool>& current_pruned)
        {
            // json logger not enabled
            if (!logger)
                return;

            for (auto s_prime = 0u; s_prime < current_viterbi.size(); ++s_prime)
            {
                osrm::json::get(*object, "states", t, s_prime, "viterbi") = osrm::json::clamp_float(current_viterbi[s_prime]);
                osrm::json::get(*object, "states", t, s_prime, "pruned") = static_cast<unsigned>(current_pruned[s_prime]);
            }
        }

        void add_chosen(const unsigned t, const unsigned s)
        {
            // json logger not enabled
            if (!logger)
                return;

            osrm::json::get(*object, "states", t, s, "chosen") = true;
        }

        void add_breakage(const std::vector<bool>& breakage)
        {
            // json logger not enabled
            if (!logger)
                return;

            osrm::json::get(*object, "breakage") = osrm::json::make_array(breakage);
        }

        const osrm::json::Logger* logger;
        osrm::json::Value* object;
    };

  public:
    MapMatching(DataFacadeT *facade, SearchEngineData &engine_working_data)
        : super(facade), engine_working_data(engine_working_data)
    {
    }

    void operator()(const Matching::CandidateLists &candidates_list,
                    const std::vector<FixedPointCoordinate> &trace_coordinates,
                    const std::vector<unsigned> &trace_timestamps,
                    const double matching_beta,
                    const double gps_precision,
                    Matching::SubMatchingList &sub_matchings) const
    {
        BOOST_ASSERT(candidates_list.size() > 0);

        // TODO replace default values with table lookup based on sampling frequency
        EmissionLogProbability emission_log_probability(gps_precision > 0 ? gps_precision : default_sigma_z);
        TransitionLogProbability transition_log_probability(matching_beta > 0 ? matching_beta : default_beta);

        HiddenMarkovModel model(candidates_list, emission_log_probability);

        unsigned initial_timestamp = model.initialize(0);
        if (initial_timestamp == Matching::INVALID_STATE)
        {
            return;
        }

        DebugInfo debug(osrm::json::Logger::get());
        debug.initialize(model, initial_timestamp, candidates_list);

        unsigned breakage_begin = std::numeric_limits<unsigned>::max();
        std::vector<unsigned> split_points;
        std::vector<unsigned> prev_unbroken_timestamps;
        prev_unbroken_timestamps.reserve(candidates_list.size());
        prev_unbroken_timestamps.push_back(initial_timestamp);
        for (auto t = initial_timestamp + 1; t < candidates_list.size(); ++t)
        {
            unsigned prev_unbroken_timestamp = prev_unbroken_timestamps.back();
            const auto &prev_viterbi = model.viterbi[prev_unbroken_timestamp];
            const auto &prev_pruned = model.pruned[prev_unbroken_timestamp];
            const auto &prev_unbroken_timestamps_list = candidates_list[prev_unbroken_timestamp];
            const auto &prev_coordinate = trace_coordinates[prev_unbroken_timestamp];

            auto &current_viterbi = model.viterbi[t];
            auto &current_pruned = model.pruned[t];
            auto &current_parents = model.parents[t];
            auto &current_lengths = model.path_lengths[t];
            const auto &current_timestamps_list = candidates_list[t];
            const auto &current_coordinate = trace_coordinates[t];

            // compute d_t for this timestamp and the next one
            for (auto s = 0u; s < prev_viterbi.size(); ++s)
            {
                if (prev_pruned[s])
                    continue;

                for (auto s_prime = 0u; s_prime < current_viterbi.size(); ++s_prime)
                {
                    // how likely is candidate s_prime at time t to be emitted?
                    const double emission_pr =
                        emission_log_probability(candidates_list[t][s_prime].second);
                    double new_value = prev_viterbi[s] + emission_pr;
                    if (current_viterbi[s_prime] > new_value)
                        continue;

                    // get distance diff between loc1/2 and locs/s_prime
                    const auto network_distance =
                        get_network_distance(prev_unbroken_timestamps_list[s].first,
                                             current_timestamps_list[s_prime].first);
                    const auto great_circle_distance =
                        coordinate_calculation::great_circle_distance(prev_coordinate,
                                                                      current_coordinate);

                    const auto d_t = std::abs(network_distance - great_circle_distance);

                    // very low probability transition -> prune
                    if (d_t > 500)
                        continue;

                    const double transition_pr = transition_log_probability(d_t);
                    new_value += transition_pr;

                    debug.add_transition_info(prev_unbroken_timestamp, t, s, s_prime,
                                              prev_viterbi[s],
                                              emission_pr,
                                              transition_pr,
                                              network_distance,
                                              great_circle_distance);

                    if (new_value > current_viterbi[s_prime])
                    {
                        current_viterbi[s_prime] = new_value;
                        current_parents[s_prime] = std::make_pair(prev_unbroken_timestamp, s);
                        current_lengths[s_prime] = network_distance;
                        current_pruned[s_prime] = false;
                        model.breakage[t] = false;
                    }
                }
            }

            debug.add_viterbi(t, current_viterbi, current_pruned);

            if (model.breakage[t])
            {
                BOOST_ASSERT(prev_unbroken_timestamps.size() > 0);

                // save start of breakage -> we need this as split point
                if (t < breakage_begin)
                {
                    breakage_begin = t;
                }

                // remove both ends of the breakage
                prev_unbroken_timestamps.pop_back();

                bool trace_split = prev_unbroken_timestamps.size() < 1;

                // use temporal information to determine a split if available
                if (trace_timestamps.size() > 0)
                {
                    trace_split =
                        trace_split ||
                        (trace_timestamps[t] - trace_timestamps[prev_unbroken_timestamps.back()] >
                         Matching::MAX_BROKEN_TIME);
                }
                else
                {
                    trace_split = trace_split || (t - prev_unbroken_timestamps.back() >
                                                  Matching::MAX_BROKEN_STATES);
                }

                // we reached the beginning of the trace and it is still broken
                // -> split the trace here
                if (trace_split)
                {
                    split_points.push_back(breakage_begin);
                    // note: this preserves everything before breakage_begin
                    model.clear(breakage_begin);
                    unsigned new_start = model.initialize(breakage_begin);
                    // no new start was found -> stop viterbi calculation
                    if (new_start == Matching::INVALID_STATE)
                    {
                        break;
                    }
                    prev_unbroken_timestamps.clear();
                    prev_unbroken_timestamps.push_back(new_start);
                    // Important: We potentially go back here!
                    // However since t+1 > new_start >= breakge_begin
                    // we can only reset trace_coordindates.size() times.
                    t = new_start;
                    breakage_begin = std::numeric_limits<unsigned>::max();
                }
            }
            else
            {
                prev_unbroken_timestamps.push_back(t);
            }
        }

        if (prev_unbroken_timestamps.size() > 0)
        {
            split_points.push_back(prev_unbroken_timestamps.back() + 1);
        }

        unsigned sub_matching_begin = initial_timestamp;
        for (const unsigned sub_matching_end : split_points)
        {
            Matching::SubMatching matching;

            // find real end of trace
            // not sure if this is really needed
            unsigned parent_timestamp_index = sub_matching_end - 1;
            while (parent_timestamp_index >= sub_matching_begin &&
                   model.breakage[parent_timestamp_index])
            {
                parent_timestamp_index--;
            }

            // matchings that only consist of one candidate are invalid
            if (parent_timestamp_index - sub_matching_begin + 1 < 2)
            {
                sub_matching_begin = sub_matching_end;
                continue;
            }

            // loop through the columns, and only compare the last entry
            auto max_element_iter = std::max_element(model.viterbi[parent_timestamp_index].begin(),
                                                     model.viterbi[parent_timestamp_index].end());

            unsigned parent_candidate_index =
                std::distance(model.viterbi[parent_timestamp_index].begin(), max_element_iter);

            std::deque<std::pair<unsigned, unsigned>> reconstructed_indices;
            while (parent_timestamp_index > sub_matching_begin)
            {
                if (model.breakage[parent_timestamp_index])
                {
                    continue;
                }

                reconstructed_indices.emplace_front(parent_timestamp_index, parent_candidate_index);
                const auto &next = model.parents[parent_timestamp_index][parent_candidate_index];
                parent_timestamp_index = next.first;
                parent_candidate_index = next.second;
            }
            reconstructed_indices.emplace_front(parent_timestamp_index, parent_candidate_index);
            if (reconstructed_indices.size() < 2)
            {
                sub_matching_begin = sub_matching_end;
                continue;
            }

            matching.length = 0.0f;
            matching.nodes.resize(reconstructed_indices.size());
            matching.indices.resize(reconstructed_indices.size());
            for (auto i = 0u; i < reconstructed_indices.size(); ++i)
            {
                auto timestamp_index = reconstructed_indices[i].first;
                auto location_index = reconstructed_indices[i].second;

                matching.indices[i] = timestamp_index;
                matching.nodes[i] = candidates_list[timestamp_index][location_index].first;
                matching.length += model.path_lengths[timestamp_index][location_index];

                debug.add_chosen(timestamp_index, location_index);
            }

            sub_matchings.push_back(matching);

            sub_matching_begin = sub_matching_end;
        }

        debug.add_breakage(model.breakage);
    }
};

//[1] "Hidden Markov Map Matching Through Noise and Sparseness"; P. Newson and J. Krumm; 2009; ACM
//GIS

#endif /* MAP_MATCHING_HPP */
