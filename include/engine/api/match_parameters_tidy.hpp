#ifndef COORDINATE_TIDY
#define COORDINATE_TIDY

#include <algorithm>
#include <cstdint>
#include <iterator>

#include "engine/api/match_parameters.hpp"
#include "util/coordinate_calculation.hpp"

#include <boost/assert.hpp>
#include <boost/dynamic_bitset.hpp>

namespace osrm
{
namespace engine
{
namespace api
{
namespace tidy
{

struct Thresholds
{
    double distance_in_meters;
    std::int32_t duration_in_seconds;
};

using Mask = boost::dynamic_bitset<>;
using Mapping = std::vector<std::size_t>;

struct Result
{
    // Tidied parameters
    MatchParameters parameters;
    // Masking the MatchParameter parallel arrays for items which should be removed.
    Mask can_be_removed;
    // Maps the MatchParameter's original items to items which should not be removed.
    Mapping original_to_tidied;
};

inline Result tidy(const MatchParameters &params, Thresholds cfg = {15., 5})
{
    Result result;

    result.can_be_removed.reserve(params.coordinates.size());
    result.original_to_tidied.reserve(params.coordinates.size());

    result.can_be_removed.push_back(false);
    result.original_to_tidied.push_back(0);
    std::size_t last_good = 0;

    const auto uses_timestamps = !params.timestamps.empty();

    Thresholds running{0., 0};

    // Walk over adjacent (coord, ts)-pairs, with rhs being the candidate to discard or keep
    for (std::size_t current = 0; current < params.coordinates.size() - 1; ++current)
    {
        const auto next = current + 1;

        auto distance_delta = util::coordinate_calculation::haversineDistance(
            params.coordinates[current], params.coordinates[next]);
        running.distance_in_meters += distance_delta;
        const auto over_distance = running.distance_in_meters >= cfg.distance_in_meters;

        if (uses_timestamps)
        {
            auto duration_delta = params.timestamps[next] - params.timestamps[current];
            running.duration_in_seconds += duration_delta;
            const auto over_duration = running.duration_in_seconds >= cfg.duration_in_seconds;

            if (over_distance && over_duration)
            {
                result.can_be_removed.push_back(false);
                last_good = next;
                running = {0., 0}; // reset running distance and time
            }
            else
            {
                result.can_be_removed.push_back(true);
            }
        }
        else
        {
            if (over_distance)
            {
                result.can_be_removed.push_back(false);
                last_good = next;
                running = {0., 0}; // reset running distance and time
            }
            else
            {
                result.can_be_removed.push_back(true);
            }
        }

        result.original_to_tidied.push_back(last_good);
    }

    BOOST_ASSERT(result.original_to_tidied.size() == params.coordinates.size());
    BOOST_ASSERT(result.can_be_removed.size() == params.coordinates.size());
    BOOST_ASSERT(std::is_sorted(begin(result.original_to_tidied), end(result.original_to_tidied)));

    // We have to filter parallel arrays that may be empty or the exact same size.
    // result.parameters contains an empty MatchParameters at this point: conditionally fill.

    const auto to_keep = result.can_be_removed.size() - result.can_be_removed.count();

    for (std::size_t i = 0; i < result.can_be_removed.size(); ++i)
    {
        if (!result.can_be_removed[i])
        {
            result.parameters.coordinates.push_back(params.coordinates[i]);

            if (!params.hints.empty())
                result.parameters.hints.push_back(params.hints[i]);

            if (!params.radiuses.empty())
                result.parameters.radiuses.push_back(params.radiuses[i]);

            if (!params.bearings.empty())
                result.parameters.bearings.push_back(params.bearings[i]);

            if (!params.timestamps.empty())
                result.parameters.timestamps.push_back(params.timestamps[i]);
        }
    }

    return result;
}

} // ns tidy
} // ns api
} // ns engine
} // ns osrm

#endif
