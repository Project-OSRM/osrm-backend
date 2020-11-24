#ifndef HIDDEN_MARKOV_MODEL
#define HIDDEN_MARKOV_MODEL

#include "util/integer_range.hpp"

#include <boost/assert.hpp>
#include <boost/math/constants/constants.hpp>

#include <cmath>

#include <limits>
#include <vector>

namespace osrm
{
namespace engine
{
namespace map_matching
{

static const double log_2_pi = std::log(2. * boost::math::constants::pi<double>());
static const double IMPOSSIBLE_LOG_PROB = -std::numeric_limits<double>::infinity();
static const double MINIMAL_LOG_PROB = std::numeric_limits<double>::lowest();
static const std::size_t INVALID_STATE = std::numeric_limits<std::size_t>::max();

// closures to precompute log -> only simple floating point operations
struct EmissionLogProbability
{
    double sigma_z;
    double log_sigma_z;

    EmissionLogProbability(const double sigma_z) : sigma_z(sigma_z), log_sigma_z(std::log(sigma_z))
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
    TransitionLogProbability(const double beta) : beta(beta), log_beta(std::log(beta)) {}

    double operator()(const double d_t) const { return -log_beta - d_t / beta; }
};

template <class CandidateLists> struct HiddenMarkovModel
{
    std::vector<std::vector<double>> viterbi;
    std::vector<std::vector<bool>> viterbi_reachable;
    std::vector<std::vector<std::pair<unsigned, unsigned>>> parents;
    std::vector<std::vector<float>> path_distances;
    std::vector<std::vector<bool>> pruned;
    std::vector<bool> breakage;

    const CandidateLists &candidates_list;
    const std::vector<std::vector<double>> &emission_log_probabilities;

    HiddenMarkovModel(const CandidateLists &candidates_list,
                      const std::vector<std::vector<double>> &emission_log_probabilities)
        : breakage(candidates_list.size()), candidates_list(candidates_list),
          emission_log_probabilities(emission_log_probabilities)
    {
        viterbi.resize(candidates_list.size());
        viterbi_reachable.resize(candidates_list.size());
        parents.resize(candidates_list.size());
        path_distances.resize(candidates_list.size());
        pruned.resize(candidates_list.size());
        breakage.resize(candidates_list.size());
        for (const auto i : util::irange<std::size_t>(0UL, candidates_list.size()))
        {
            const auto &num_candidates = candidates_list[i].size();
            // add empty vectors
            if (num_candidates > 0)
            {
                viterbi[i].resize(num_candidates);
                viterbi_reachable[i].resize(num_candidates);
                parents[i].resize(num_candidates);
                path_distances[i].resize(num_candidates);
                pruned[i].resize(num_candidates);
            }
        }

        Clear(0);
    }

    void Clear(std::size_t initial_timestamp)
    {
        BOOST_ASSERT(viterbi.size() == parents.size() && parents.size() == path_distances.size() &&
                     path_distances.size() == pruned.size() && pruned.size() == breakage.size());

        for (const auto t : util::irange(initial_timestamp, viterbi.size()))
        {
            std::fill(viterbi[t].begin(), viterbi[t].end(), IMPOSSIBLE_LOG_PROB);
            std::fill(viterbi_reachable[t].begin(), viterbi_reachable[t].end(), false);
            std::fill(parents[t].begin(), parents[t].end(), std::make_pair(0u, 0u));
            std::fill(path_distances[t].begin(), path_distances[t].end(), 0);
            std::fill(pruned[t].begin(), pruned[t].end(), true);
        }
        std::fill(breakage.begin() + initial_timestamp, breakage.end(), true);
    }

    std::size_t initialize(std::size_t initial_timestamp)
    {
        auto num_points = candidates_list.size();
        do
        {
            BOOST_ASSERT(initial_timestamp < num_points);

            for (const auto s : util::irange<std::size_t>(0UL, viterbi[initial_timestamp].size()))
            {
                viterbi[initial_timestamp][s] = emission_log_probabilities[initial_timestamp][s];
                parents[initial_timestamp][s] = std::make_pair(initial_timestamp, s);
                pruned[initial_timestamp][s] = viterbi[initial_timestamp][s] < MINIMAL_LOG_PROB;

                breakage[initial_timestamp] =
                    breakage[initial_timestamp] && pruned[initial_timestamp][s];
            }

            ++initial_timestamp;
        } while (initial_timestamp < num_points && breakage[initial_timestamp - 1]);

        if (initial_timestamp >= num_points)
        {
            return INVALID_STATE;
        }

        BOOST_ASSERT(initial_timestamp > 0);
        --initial_timestamp;

        BOOST_ASSERT(breakage[initial_timestamp] == false);

        return initial_timestamp;
    }
};
} // namespace map_matching
} // namespace engine
} // namespace osrm

#endif // HIDDEN_MARKOV_MODEL
