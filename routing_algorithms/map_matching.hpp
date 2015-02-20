/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef MAP_MATCHING_H
#define MAP_MATCHING_H

#include "routing_base.hpp"

#include "../data_structures/coordinate_calculation.hpp"
#include "../util/simple_logger.hpp"

#include <variant/variant.hpp>
#include <osrm/json_container.hpp>

#include <algorithm>
#include <iomanip>
#include <numeric>

#include <fstream>

using JSONVariantArray = mapbox::util::recursive_wrapper<JSON::Array>;
using JSONVariantObject = mapbox::util::recursive_wrapper<JSON::Object>;

template<typename T>
T makeJSONSafe(T d)
{
    if (std::isnan(d) || std::numeric_limits<T>::infinity() == d) {
        return std::numeric_limits<T>::max();
    }
    if (-std::numeric_limits<T>::infinity() == d) {
        return -std::numeric_limits<T>::max();
    }

    return d;
}

void appendToJSONArray(JSON::Array& a) { }

template<typename T, typename... Args>
void appendToJSONArray(JSON::Array& a, T value, Args... args)
{
    a.values.emplace_back(value);
    appendToJSONArray(a, args...);
}

template<typename... Args>
JSON::Array makeJSONArray(Args... args)
{
    JSON::Array a;
    appendToJSONArray(a, args...);
    return a;
}

namespace Matching
{

struct SubMatching
{
    std::vector<PhantomNode> nodes;
    std::vector<unsigned> indices;
    double length;
    double confidence;
};

using CandidateList =  std::vector<std::pair<PhantomNode, double>>;
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
template <class DataFacadeT> class MapMatching final : public BasicRoutingInterface<DataFacadeT>
{
    typedef BasicRoutingInterface<DataFacadeT> super;
    typedef typename SearchEngineData::QueryHeap QueryHeap;
    SearchEngineData &engine_working_data;

    // FIXME this value should be a table based on samples/meter (or samples/min)
    constexpr static const double beta = 10.0;
    constexpr static const double sigma_z = 4.07;
    constexpr static const double log_sigma_z = std::log(sigma_z);
    constexpr static const double log_2_pi = std::log(2 * M_PI);

    constexpr static double emission_probability(const double distance)
    {
        return (1. / (std::sqrt(2. * M_PI) * sigma_z)) *
               std::exp(-0.5 * std::pow((distance / sigma_z), 2.));
    }

    constexpr static double transition_probability(const float d_t, const float beta)
    {
        return (1. / beta) * std::exp(-d_t / beta);
    }

    constexpr static double log_emission_probability(const double distance)
    {
        return -0.5 * (log_2_pi + (distance / sigma_z) * (distance / sigma_z)) - log_sigma_z;
    }

    constexpr static double log_transition_probability(const float d_t, const float beta)
    {
        return -std::log(beta) - d_t / beta;
    }

    // TODO: needs to be estimated from the input locations
    // FIXME These values seem wrong. Higher beta for more samples/minute? Should be inverse proportional.
    //constexpr static const double beta = 1.;
    // samples/min and beta
    // 1 0.49037673
    // 2 0.82918373
    // 3 1.24364564
    // 4 1.67079581
    // 5 2.00719298
    // 6 2.42513007
    // 7 2.81248831
    // 8 3.15745473
    // 9 3.52645392
    // 10 4.09511775
    // 11 4.67319795
    // 21 12.55107715
    // 12 5.41088180
    // 13 6.47666590
    // 14 6.29010734
    // 15 7.80752112
    // 16 8.09074504
    // 17 8.08550528
    // 18 9.09405065
    // 19 11.09090603
    // 20 11.87752824
    // 21 12.55107715
    // 22 15.82820829
    // 23 17.69496773
    // 24 18.07655652
    // 25 19.63438911
    // 26 25.40832185
    // 27 23.76001877
    // 28 28.43289797
    // 29 32.21683062
    // 30 34.56991141

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

        QueryHeap &forward_heap = *(engine_working_data.forwardHeap);
        QueryHeap &reverse_heap = *(engine_working_data.backwardHeap);

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
            for (const auto& p : unpacked_path)
            {
                current_coordinate = super::facade->GetCoordinateOfNode(p.node);
                distance += coordinate_calculation::great_circle_distance(previous_coordinate, current_coordinate);
                previous_coordinate = current_coordinate;
            }
            distance += coordinate_calculation::great_circle_distance(previous_coordinate, target_phantom.location);
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

        const Matching::CandidateLists& candidates_list;


        HiddenMarkovModel(const Matching::CandidateLists& candidates_list)
        : breakage(candidates_list.size())
        , candidates_list(candidates_list)
        {
            for (const auto& l : candidates_list)
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
            BOOST_ASSERT(viterbi.size() == parents.size()
                      && parents.size() == path_lengths.size()
                      && path_lengths.size() == pruned.size()
                      && pruned.size() == breakage.size());

            for (unsigned t = initial_timestamp; t < viterbi.size(); t++)
            {
                std::fill(viterbi[t].begin(), viterbi[t].end(), Matching::IMPOSSIBLE_LOG_PROB);
                std::fill(parents[t].begin(), parents[t].end(), std::make_pair(0u, 0u));
                std::fill(path_lengths[t].begin(), path_lengths[t].end(), 0);
                std::fill(pruned[t].begin(), pruned[t].end(), true);
            }
            std::fill(breakage.begin()+initial_timestamp, breakage.end(), true);
        }

        unsigned initialize(unsigned initial_timestamp)
        {
            BOOST_ASSERT(initial_timestamp < candidates_list.size());

            do
            {
                for (auto s = 0u; s < viterbi[initial_timestamp].size(); ++s)
                {
                    viterbi[initial_timestamp][s] = log_emission_probability(candidates_list[initial_timestamp][s].second);
                    parents[initial_timestamp][s] = std::make_pair(initial_timestamp, s);
                    pruned[initial_timestamp][s] = viterbi[initial_timestamp][s] < Matching::MINIMAL_LOG_PROB;

                    breakage[initial_timestamp] = breakage[initial_timestamp] && pruned[initial_timestamp][s];

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

  public:
    MapMatching(DataFacadeT *facade, SearchEngineData &engine_working_data)
        : super(facade), engine_working_data(engine_working_data)
    {
    }


    void operator()(const Matching::CandidateLists &candidates_list,
                    const std::vector<FixedPointCoordinate>& trace_coordinates,
                    const std::vector<unsigned>& trace_timestamps,
                    Matching::SubMatchingList& sub_matchings,
                    JSON::Object& _debug_info) const
    {
        BOOST_ASSERT(candidates_list.size() > 0);

        HiddenMarkovModel model(candidates_list);

        unsigned initial_timestamp = model.initialize(0);
        if (initial_timestamp == Matching::INVALID_STATE)
        {
            return;
        }

        JSON::Array _debug_states;
        for (unsigned t = 0; t < candidates_list.size(); t++)
        {
            JSON::Array _debug_timestamps;
            for (unsigned s = 0; s < candidates_list[t].size(); s++)
            {
                JSON::Object _debug_state;
                _debug_state.values["transitions"] = JSON::Array();
                _debug_state.values["coordinate"] = makeJSONArray(candidates_list[t][s].first.location.lat / COORDINATE_PRECISION,
                                                                  candidates_list[t][s].first.location.lon / COORDINATE_PRECISION);
                if (t < initial_timestamp)
                {
                    _debug_state.values["viterbi"] = makeJSONSafe(Matching::IMPOSSIBLE_LOG_PROB);
                    _debug_state.values["pruned"] = 0u;
                }
                else if (t == initial_timestamp)
                {
                    _debug_state.values["viterbi"] = makeJSONSafe(model.viterbi[t][s]);
                    _debug_state.values["pruned"] = static_cast<unsigned>(model.pruned[t][s]);
                }
                _debug_timestamps.values.push_back(_debug_state);
            }
            _debug_states.values.push_back(_debug_timestamps);
        }

        unsigned breakage_begin = std::numeric_limits<unsigned>::max();
        std::vector<unsigned> split_points;
        std::vector<unsigned> prev_unbroken_timestamps;
        prev_unbroken_timestamps.reserve(candidates_list.size());
        prev_unbroken_timestamps.push_back(initial_timestamp);
        for (auto t = initial_timestamp + 1; t < candidates_list.size(); ++t)
        {
            unsigned prev_unbroken_timestamp = prev_unbroken_timestamps.back();
            const auto& prev_viterbi = model.viterbi[prev_unbroken_timestamp];
            const auto& prev_pruned = model.pruned[prev_unbroken_timestamp];
            const auto& prev_unbroken_timestamps_list = candidates_list[prev_unbroken_timestamp];
            const auto& prev_coordinate = trace_coordinates[prev_unbroken_timestamp];

            auto& current_viterbi = model.viterbi[t];
            auto& current_pruned = model.pruned[t];
            auto& current_parents = model.parents[t];
            auto& current_lengths = model.path_lengths[t];
            const auto& current_timestamps_list = candidates_list[t];
            const auto& current_coordinate = trace_coordinates[t];

            // compute d_t for this timestamp and the next one
            for (auto s = 0u; s < prev_viterbi.size(); ++s)
            {
                if (prev_pruned[s])
                    continue;

                for (auto s_prime = 0u; s_prime < current_viterbi.size(); ++s_prime)
                {
                    // how likely is candidate s_prime at time t to be emitted?
                    const double emission_pr = log_emission_probability(candidates_list[t][s_prime].second);
                    double new_value = prev_viterbi[s] + emission_pr;
                    if (current_viterbi[s_prime] > new_value)
                        continue;

                    // get distance diff between loc1/2 and locs/s_prime
                    const auto network_distance = get_network_distance(prev_unbroken_timestamps_list[s].first,
                                                                       current_timestamps_list[s_prime].first);
                    const auto great_circle_distance =
                        coordinate_calculation::great_circle_distance(prev_coordinate,
                                                                      current_coordinate);

                    const auto d_t = std::abs(network_distance - great_circle_distance);

                    // very low probability transition -> prune
                    if (d_t > 500)
                        continue;

                    const double transition_pr = log_transition_probability(d_t, beta);
                    new_value += transition_pr;

                    JSON::Object _debug_transistion;
                    _debug_transistion.values["to"] = makeJSONArray(t, s_prime);
                    _debug_transistion.values["properties"] = makeJSONArray(
                        makeJSONSafe(prev_viterbi[s]),
                        makeJSONSafe(emission_pr),
                        makeJSONSafe(transition_pr),
                        network_distance,
                        great_circle_distance
                    );
                    _debug_states.values[prev_unbroken_timestamp]
                        .get<JSONVariantArray>().get().values[s]
                        .get<JSONVariantObject>().get().values["transitions"]
                        .get<JSONVariantArray>().get().values.push_back(_debug_transistion);

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

            for (auto s_prime = 0u; s_prime < current_viterbi.size(); ++s_prime)
            {
                _debug_states.values[t]
                    .get<JSONVariantArray>().get().values[s_prime]
                    .get<JSONVariantObject>().get().values["viterbi"] = makeJSONSafe(current_viterbi[s_prime]);
                _debug_states.values[t]
                    .get<JSONVariantArray>().get().values[s_prime]
                    .get<JSONVariantObject>().get().values["pruned"]  = static_cast<unsigned>(current_pruned[s_prime]);
            }

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
                    trace_split = trace_split || (trace_timestamps[t] - trace_timestamps[prev_unbroken_timestamps.back()] > Matching::MAX_BROKEN_TIME);
                }
                else
                {
                    trace_split = trace_split || (t - prev_unbroken_timestamps.back() > Matching::MAX_BROKEN_STATES);
                }

                // we reached the beginning of the trace and it is still broken
                // -> split the trace here
                if (trace_split)
                {
                    split_points.push_back(breakage_begin);
                    // note this preserves everything before t
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
            split_points.push_back(prev_unbroken_timestamps.back()+1);
        }

        unsigned sub_matching_begin = initial_timestamp;
        for (const unsigned sub_matching_end : split_points)
        {
            Matching::SubMatching matching;

            // find real end of trace
            // not sure if this is really needed
            unsigned parent_timestamp_index = sub_matching_end-1;
            while (parent_timestamp_index >= sub_matching_begin && model.breakage[parent_timestamp_index])
            {
                parent_timestamp_index--;
            }

            // matchings that only consist of one candidate are invalid
            if (parent_timestamp_index - sub_matching_begin < 2)
            {
                sub_matching_begin = sub_matching_end;
                continue;
            }

            // loop through the columns, and only compare the last entry
            auto max_element_iter = std::max_element(model.viterbi[parent_timestamp_index].begin(),
                                                     model.viterbi[parent_timestamp_index].end());

            unsigned parent_candidate_index = std::distance(model.viterbi[parent_timestamp_index].begin(), max_element_iter);

            std::deque<std::pair<unsigned, unsigned>> reconstructed_indices;
            while (parent_timestamp_index > sub_matching_begin)
            {
                if (model.breakage[parent_timestamp_index])
                {
                    continue;
                }

                reconstructed_indices.emplace_front(parent_timestamp_index, parent_candidate_index);
                const auto& next = model.parents[parent_timestamp_index][parent_candidate_index];
                parent_timestamp_index = next.first;
                parent_candidate_index = next.second;
            }
            reconstructed_indices.emplace_front(parent_timestamp_index, parent_candidate_index);
            if (reconstructed_indices.size() < 2)
            {
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

                _debug_states.values[timestamp_index]
                    .get<JSONVariantArray>().get().values[location_index]
                    .get<JSONVariantObject>().get().values["chosen"] = true;
            }

            sub_matchings.push_back(matching);

            sub_matching_begin = sub_matching_end;
        }

        JSON::Array _debug_breakage;
        for (auto b : model.breakage) {
            _debug_breakage.values.push_back(static_cast<unsigned>(b));
        }

        _debug_info.values["breakage"] = _debug_breakage;
        _debug_info.values["states"] = _debug_states;
    }
};

//[1] "Hidden Markov Map Matching Through Noise and Sparseness"; P. Newson and J. Krumm; 2009; ACM GIS

#endif /* MAP_MATCHING_H */
