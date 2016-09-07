#ifndef MAP_MATCHING_HPP
#define MAP_MATCHING_HPP

#include "engine/routing_algorithms/routing_base.hpp"

#include "engine/map_matching/hidden_markov_model.hpp"
#include "engine/map_matching/matching_confidence.hpp"
#include "engine/map_matching/sub_matching.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/for_each_pair.hpp"

#include <cstddef>

#include <algorithm>
#include <deque>
#include <iomanip>
#include <numeric>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

using CandidateList = std::vector<PhantomNodeWithDistance>;
using CandidateLists = std::vector<CandidateList>;
using HMM = map_matching::HiddenMarkovModel<CandidateLists>;
using SubMatchingList = std::vector<map_matching::SubMatching>;

constexpr static const unsigned MAX_BROKEN_STATES = 10;
constexpr static const double MAX_SPEED = 180 / 3.6; // 180km -> m/s
static const constexpr double MATCHING_BETA = 10;
constexpr static const double MAX_DISTANCE_DELTA = 2000.;

// implements a hidden markov model map matching algorithm
template <class DataFacadeT>
class MapMatching final : public BasicRoutingInterface<DataFacadeT, MapMatching<DataFacadeT>>
{
    using super = BasicRoutingInterface<DataFacadeT, MapMatching<DataFacadeT>>;
    using QueryHeap = SearchEngineData::QueryHeap;
    SearchEngineData &engine_working_data;
    map_matching::EmissionLogProbability default_emission_log_probability;
    map_matching::TransitionLogProbability transition_log_probability;
    map_matching::MatchingConfidence confidence;

    unsigned GetMedianSampleTime(const std::vector<unsigned> &timestamps) const
    {
        BOOST_ASSERT(timestamps.size() > 1);

        std::vector<unsigned> sample_times(timestamps.size());

        std::adjacent_difference(timestamps.begin(), timestamps.end(), sample_times.begin());

        // don't use first element of sample_times -> will not be a difference.
        auto first_elem = std::next(sample_times.begin());
        auto median = first_elem + std::distance(first_elem, sample_times.end()) / 2;
        std::nth_element(first_elem, median, sample_times.end());
        return *median;
    }

  public:
    MapMatching(DataFacadeT *facade,
                SearchEngineData &engine_working_data,
                const double default_gps_precision)
        : super(facade), engine_working_data(engine_working_data),
          default_emission_log_probability(default_gps_precision),
          transition_log_probability(MATCHING_BETA)
    {
    }

    SubMatchingList
    operator()(const CandidateLists &candidates_list,
               const std::vector<util::Coordinate> &trace_coordinates,
               const std::vector<unsigned> &trace_timestamps,
               const std::vector<boost::optional<double>> &trace_gps_precision) const
    {
        SubMatchingList sub_matchings;

        BOOST_ASSERT(candidates_list.size() == trace_coordinates.size());
        BOOST_ASSERT(candidates_list.size() > 1);

        const bool use_timestamps = trace_timestamps.size() > 1;

        const auto median_sample_time = [&] {
            if (use_timestamps)
            {
                return std::max(1u, GetMedianSampleTime(trace_timestamps));
            }
            else
            {
                return 1u;
            }
        }();
        const auto max_broken_time = median_sample_time * MAX_BROKEN_STATES;
        const auto max_distance_delta = [&] {
            if (use_timestamps)
            {
                return median_sample_time * MAX_SPEED;
            }
            else
            {
                return MAX_DISTANCE_DELTA;
            }
        }();

        std::vector<std::vector<double>> emission_log_probabilities(trace_coordinates.size());
        if (trace_gps_precision.empty())
        {
            for (auto t = 0UL; t < candidates_list.size(); ++t)
            {
                emission_log_probabilities[t].resize(candidates_list[t].size());
                std::transform(candidates_list[t].begin(),
                               candidates_list[t].end(),
                               emission_log_probabilities[t].begin(),
                               [this](const PhantomNodeWithDistance &candidate) {
                                   return default_emission_log_probability(candidate.distance);
                               });
            }
        }
        else
        {
            for (auto t = 0UL; t < candidates_list.size(); ++t)
            {
                emission_log_probabilities[t].resize(candidates_list[t].size());
                if (trace_gps_precision[t])
                {
                    map_matching::EmissionLogProbability emission_log_probability(
                        *trace_gps_precision[t]);
                    std::transform(
                        candidates_list[t].begin(),
                        candidates_list[t].end(),
                        emission_log_probabilities[t].begin(),
                        [&emission_log_probability](const PhantomNodeWithDistance &candidate) {
                            return emission_log_probability(candidate.distance);
                        });
                }
                else
                {
                    std::transform(candidates_list[t].begin(),
                                   candidates_list[t].end(),
                                   emission_log_probabilities[t].begin(),
                                   [this](const PhantomNodeWithDistance &candidate) {
                                       return default_emission_log_probability(candidate.distance);
                                   });
                }
            }
        }

        HMM model(candidates_list, emission_log_probabilities);

        std::size_t initial_timestamp = model.initialize(0);
        if (initial_timestamp == map_matching::INVALID_STATE)
        {
            return sub_matchings;
        }

        engine_working_data.InitializeOrClearFirstThreadLocalStorage(
            super::facade->GetNumberOfNodes());
        engine_working_data.InitializeOrClearSecondThreadLocalStorage(
            super::facade->GetNumberOfNodes());

        QueryHeap &forward_heap = *(engine_working_data.forward_heap_1);
        QueryHeap &reverse_heap = *(engine_working_data.reverse_heap_1);
        QueryHeap &forward_core_heap = *(engine_working_data.forward_heap_2);
        QueryHeap &reverse_core_heap = *(engine_working_data.reverse_heap_2);

        std::size_t breakage_begin = map_matching::INVALID_STATE;
        std::vector<std::size_t> split_points;
        std::vector<std::size_t> prev_unbroken_timestamps;
        prev_unbroken_timestamps.reserve(candidates_list.size());
        prev_unbroken_timestamps.push_back(initial_timestamp);
        for (auto t = initial_timestamp + 1; t < candidates_list.size(); ++t)
        {
            // breakage recover has removed all previous good points
            bool trace_split = prev_unbroken_timestamps.empty();

            // use temporal information if available to determine a split
            if (use_timestamps)
            {
                trace_split =
                    trace_split ||
                    (trace_timestamps[t] - trace_timestamps[prev_unbroken_timestamps.back()] >
                     max_broken_time);
            }
            else
            {
                trace_split =
                    trace_split || (t - prev_unbroken_timestamps.back() > MAX_BROKEN_STATES);
            }

            if (trace_split)
            {
                std::size_t split_index = t;
                if (breakage_begin != map_matching::INVALID_STATE)
                {
                    split_index = breakage_begin;
                    breakage_begin = map_matching::INVALID_STATE;
                }
                split_points.push_back(split_index);

                // note: this preserves everything before split_index
                model.Clear(split_index);
                std::size_t new_start = model.initialize(split_index);
                // no new start was found -> stop viterbi calculation
                if (new_start == map_matching::INVALID_STATE)
                {
                    break;
                }

                prev_unbroken_timestamps.clear();
                prev_unbroken_timestamps.push_back(new_start);
                // Important: We potentially go back here!
                // However since t > new_start >= breakge_begin
                // we can only reset trace_coordindates.size() times.
                t = new_start + 1;
            }

            BOOST_ASSERT(!prev_unbroken_timestamps.empty());
            const std::size_t prev_unbroken_timestamp = prev_unbroken_timestamps.back();

            const auto &prev_viterbi = model.viterbi[prev_unbroken_timestamp];
            const auto &prev_pruned = model.pruned[prev_unbroken_timestamp];
            const auto &prev_unbroken_timestamps_list = candidates_list[prev_unbroken_timestamp];
            const auto &prev_coordinate = trace_coordinates[prev_unbroken_timestamp];

            auto &current_viterbi = model.viterbi[t];
            auto &current_pruned = model.pruned[t];
            auto &current_parents = model.parents[t];
            auto &current_lengths = model.path_distances[t];
            const auto &current_timestamps_list = candidates_list[t];
            const auto &current_coordinate = trace_coordinates[t];

            const auto haversine_distance = util::coordinate_calculation::haversineDistance(
                prev_coordinate, current_coordinate);
            // assumes minumum of 0.1 m/s
            const int duration_uppder_bound =
                ((haversine_distance + max_distance_delta) * 0.25) * 10;

            // compute d_t for this timestamp and the next one
            for (const auto s : util::irange<std::size_t>(0UL, prev_viterbi.size()))
            {
                if (prev_pruned[s])
                {
                    continue;
                }

                for (const auto s_prime : util::irange<std::size_t>(0UL, current_viterbi.size()))
                {
                    const double emission_pr = emission_log_probabilities[t][s_prime];
                    double new_value = prev_viterbi[s] + emission_pr;
                    if (current_viterbi[s_prime] > new_value)
                    {
                        continue;
                    }

                    forward_heap.Clear();
                    reverse_heap.Clear();

                    double network_distance;
                    if (super::facade->GetCoreSize() > 0)
                    {
                        forward_core_heap.Clear();
                        reverse_core_heap.Clear();
                        network_distance = super::GetNetworkDistanceWithCore(
                            forward_heap,
                            reverse_heap,
                            forward_core_heap,
                            reverse_core_heap,
                            prev_unbroken_timestamps_list[s].phantom_node,
                            current_timestamps_list[s_prime].phantom_node,
                            duration_uppder_bound);
                    }
                    else
                    {
                        network_distance = super::GetNetworkDistance(
                            forward_heap,
                            reverse_heap,
                            prev_unbroken_timestamps_list[s].phantom_node,
                            current_timestamps_list[s_prime].phantom_node);
                    }

                    // get distance diff between loc1/2 and locs/s_prime
                    const auto d_t = std::abs(network_distance - haversine_distance);

                    // very low probability transition -> prune
                    if (d_t >= max_distance_delta)
                    {
                        continue;
                    }

                    const double transition_pr = transition_log_probability(d_t);
                    new_value += transition_pr;

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

            if (model.breakage[t])
            {
                // save start of breakage -> we need this as split point
                if (t < breakage_begin)
                {
                    breakage_begin = t;
                }

                BOOST_ASSERT(prev_unbroken_timestamps.size() > 0);
                // remove both ends of the breakage
                prev_unbroken_timestamps.pop_back();
            }
            else
            {
                prev_unbroken_timestamps.push_back(t);
            }
        }

        if (!prev_unbroken_timestamps.empty())
        {
            split_points.push_back(prev_unbroken_timestamps.back() + 1);
        }

        std::size_t sub_matching_begin = initial_timestamp;
        for (const auto sub_matching_end : split_points)
        {
            map_matching::SubMatching matching;

            std::size_t parent_timestamp_index = sub_matching_end - 1;
            while (parent_timestamp_index >= sub_matching_begin &&
                   model.breakage[parent_timestamp_index])
            {
                --parent_timestamp_index;
            }
            while (sub_matching_begin < sub_matching_end && model.breakage[sub_matching_begin])
            {
                ++sub_matching_begin;
            }

            // matchings that only consist of one candidate are invalid
            if (parent_timestamp_index - sub_matching_begin + 1 < 2)
            {
                sub_matching_begin = sub_matching_end;
                continue;
            }

            // loop through the columns, and only compare the last entry
            const auto max_element_iter =
                std::max_element(model.viterbi[parent_timestamp_index].begin(),
                                 model.viterbi[parent_timestamp_index].end());

            std::size_t parent_candidate_index =
                std::distance(model.viterbi[parent_timestamp_index].begin(), max_element_iter);

            std::deque<std::pair<std::size_t, std::size_t>> reconstructed_indices;
            while (parent_timestamp_index > sub_matching_begin)
            {
                if (model.breakage[parent_timestamp_index])
                {
                    continue;
                }

                reconstructed_indices.emplace_front(parent_timestamp_index, parent_candidate_index);
                const auto &next = model.parents[parent_timestamp_index][parent_candidate_index];
                // make sure we can never get stuck in this loop
                if (parent_timestamp_index == next.first)
                {
                    break;
                }
                parent_timestamp_index = next.first;
                parent_candidate_index = next.second;
            }
            reconstructed_indices.emplace_front(parent_timestamp_index, parent_candidate_index);
            if (reconstructed_indices.size() < 2)
            {
                sub_matching_begin = sub_matching_end;
                continue;
            }

            auto matching_distance = 0.0;
            auto trace_distance = 0.0;
            matching.nodes.reserve(reconstructed_indices.size());
            matching.indices.reserve(reconstructed_indices.size());
            for (const auto idx : reconstructed_indices)
            {
                const auto timestamp_index = idx.first;
                const auto location_index = idx.second;

                matching.indices.push_back(timestamp_index);
                matching.nodes.push_back(
                    candidates_list[timestamp_index][location_index].phantom_node);
                matching_distance += model.path_distances[timestamp_index][location_index];
            }
            util::for_each_pair(
                reconstructed_indices,
                [&trace_distance,
                 &trace_coordinates](const std::pair<std::size_t, std::size_t> &prev,
                                     const std::pair<std::size_t, std::size_t> &curr) {
                    trace_distance += util::coordinate_calculation::haversineDistance(
                        trace_coordinates[prev.first], trace_coordinates[curr.first]);
                });

            matching.confidence = confidence(trace_distance, matching_distance);

            sub_matchings.push_back(matching);
            sub_matching_begin = sub_matching_end;
        }

        return sub_matchings;
    }
};
}
}
}

//[1] "Hidden Markov Map Matching Through Noise and Sparseness"; P. Newson and J. Krumm; 2009; ACM
// GIS

#endif /* MAP_MATCHING_HPP */
