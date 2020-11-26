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
    Mapping tidied_to_original;
    // Masking the MatchParameter coordinates for items whose indices were present in the
    // `waypoints` parameter.
    Mask was_waypoint;
};

inline Result keep_all(const MatchParameters &params)
{
    Result result;

    result.can_be_removed.resize(params.coordinates.size(), false);
    result.was_waypoint.resize(params.coordinates.size(), true);
    // by default all input coordinates are treated as waypoints
    if (!params.waypoints.empty())
    {
        for (const auto p : params.waypoints)
        {
            result.was_waypoint.set(p, false);
        }
        // logic is a little funny, uses inversion to set the bitfield
        result.was_waypoint.flip();
    }
    result.tidied_to_original.reserve(params.coordinates.size());
    for (std::size_t current = 0; current < params.coordinates.size(); ++current)
    {
        result.tidied_to_original.push_back(current);
    }

    BOOST_ASSERT(result.can_be_removed.size() == params.coordinates.size());

    // We have to filter parallel arrays that may be empty or the exact same size.
    // result.parameters contains an empty MatchParameters at this point: conditionally fill.

    for (std::size_t i = 0; i < result.can_be_removed.size(); ++i)
    {
        if (!result.can_be_removed[i])
        {
            result.parameters.coordinates.push_back(params.coordinates[i]);

            if (result.was_waypoint[i])
                result.parameters.waypoints.push_back(result.parameters.coordinates.size() - 1);
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
    if (params.waypoints.empty())
        result.parameters.waypoints.clear();

    return result;
}

inline Result tidy(const MatchParameters &params, Thresholds cfg = {15., 5})
{
    BOOST_ASSERT(!params.coordinates.empty());

    Result result;

    result.can_be_removed.resize(params.coordinates.size(), false);
    result.was_waypoint.resize(params.coordinates.size(), true);
    if (!params.waypoints.empty())
    {
        for (const auto p : params.waypoints)
        {
            result.was_waypoint.set(p, false);
        }
        result.was_waypoint.flip();
    }

    result.tidied_to_original.push_back(0);

    const auto uses_timestamps = !params.timestamps.empty();

    Thresholds running{0., 0};

    // Walk over adjacent (coord, ts)-pairs, with rhs being the candidate to discard or keep
    for (std::size_t current = 0, next = 1; next < params.coordinates.size() - 1; ++current, ++next)
    {
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
                result.tidied_to_original.push_back(next);
                running = {0., 0}; // reset running distance and time
            }
            else
            {
                result.can_be_removed.set(next, true);
            }
        }
        else
        {
            if (over_distance)
            {
                result.tidied_to_original.push_back(next);
                running = {0., 0}; // reset running distance and time
            }
            else
            {
                result.can_be_removed.set(next, true);
            }
        }
    }

    // Always use the last coordinate if more than two original coordinates
    if (params.coordinates.size() > 1)
    {
        result.tidied_to_original.push_back(params.coordinates.size() - 1);
    }

    // We have to filter parallel arrays that may be empty or the exact same size.
    // result.parameters contains an empty MatchParameters at this point: conditionally fill.
    for (std::size_t i = 0; i < result.can_be_removed.size(); ++i)
    {
        if (!result.can_be_removed[i])
        {
            result.parameters.coordinates.push_back(params.coordinates[i]);

            if (result.was_waypoint[i])
                result.parameters.waypoints.push_back(result.parameters.coordinates.size() - 1);
            if (!params.hints.empty())
                result.parameters.hints.push_back(params.hints[i]);

            if (!params.radiuses.empty())
                result.parameters.radiuses.push_back(params.radiuses[i]);

            if (!params.bearings.empty())
                result.parameters.bearings.push_back(params.bearings[i]);

            if (!params.timestamps.empty())
                result.parameters.timestamps.push_back(params.timestamps[i]);
        }
        else
        {
            // one of the coordinates meant to be used as a waypoint was marked for removal
            // update the original waypoint index to the new representative coordinate
            const auto last_idx = result.parameters.coordinates.size() - 1;
            if (result.was_waypoint[i] && (result.parameters.waypoints.back() != last_idx))
            {
                result.parameters.waypoints.push_back(last_idx);
            }
        }
    }

    return result;
}

} // namespace tidy
} // namespace api
} // namespace engine
} // namespace osrm

#endif
