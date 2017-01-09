#ifndef MAP_MATCHING_HPP
#define MAP_MATCHING_HPP

#include "engine/routing_algorithms/routing_base.hpp"

#include "engine/algorithm.hpp"
#include "engine/map_matching/hidden_markov_model.hpp"
#include "engine/map_matching/matching_confidence.hpp"
#include "engine/map_matching/sub_matching.hpp"

#include "extractor/profile_properties.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/for_each_pair.hpp"

#include <cstddef>

#include <algorithm>
#include <deque>
#include <iomanip>
#include <memory>
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
static const constexpr double MATCHING_BETA = 10;
constexpr static const double MAX_DISTANCE_DELTA = 2000.;
static const constexpr double DEFAULT_GPS_PRECISION = 5;

template <typename AlgorithmT> class MapMatching;

// implements a hidden markov model map matching algorithm
template <> class MapMatching<algorithm::CH> final : public BasicRouting<algorithm::CH>
{
    using super = BasicRouting<algorithm::CH>;
    using FacadeT = datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH>;
    using QueryHeap = SearchEngineData::QueryHeap;
    SearchEngineData &engine_working_data;
    map_matching::EmissionLogProbability default_emission_log_probability;
    map_matching::TransitionLogProbability transition_log_probability;
    map_matching::MatchingConfidence confidence;
    extractor::ProfileProperties m_profile_properties;

    unsigned GetMedianSampleTime(const std::vector<unsigned> &timestamps) const;

  public:
    MapMatching(SearchEngineData &engine_working_data)
        : engine_working_data(engine_working_data),
          default_emission_log_probability(DEFAULT_GPS_PRECISION),
          transition_log_probability(MATCHING_BETA)
    {
    }

    SubMatchingList
    operator()(const FacadeT &facade,
               const CandidateLists &candidates_list,
               const std::vector<util::Coordinate> &trace_coordinates,
               const std::vector<unsigned> &trace_timestamps,
               const std::vector<boost::optional<double>> &trace_gps_precision) const;
};
}
}
}

//[1] "Hidden Markov Map Matching Through Noise and Sparseness"; P. Newson and J. Krumm; 2009; ACM
// GIS

#endif /* MAP_MATCHING_HPP */
