#ifndef OSRM_ENGINE_ISOCHRONE_DURATION_HPP
#define OSRM_ENGINE_ISOCHRONE_DURATION_HPP

#include "util/typedefs.hpp"

#include <cmath>
#include <limits>
#include <optional>

namespace osrm::engine::isochrone
{

inline constexpr double INTERNAL_DURATION_UNITS_PER_SECOND = 10.;

inline std::optional<EdgeDuration> durationCutoffFromSeconds(const double seconds)
{
    using DurationValue = EdgeDuration::value_type;
    constexpr auto invalid_duration = std::numeric_limits<DurationValue>::max();

    if (!std::isfinite(seconds) || seconds < 0.)
        return std::nullopt;

    const auto internal_duration = std::floor(seconds * INTERNAL_DURATION_UNITS_PER_SECOND);
    if (!std::isfinite(internal_duration) || internal_duration < 0. ||
        internal_duration >= invalid_duration)
    {
        return std::nullopt;
    }

    return EdgeDuration{static_cast<DurationValue>(internal_duration)};
}

inline double durationToSeconds(const EdgeDuration duration)
{ return from_alias<double>(duration) / INTERNAL_DURATION_UNITS_PER_SECOND; }

inline double durationToSeconds(const long double duration)
{ return static_cast<double>(duration / INTERNAL_DURATION_UNITS_PER_SECOND); }

} // namespace osrm::engine::isochrone

#endif // OSRM_ENGINE_ISOCHRONE_DURATION_HPP
