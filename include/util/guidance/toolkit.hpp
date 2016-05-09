#ifndef OSRM_UTIL_GUIDANCE_TOOLKIT_HPP_
#define OSRM_UTIL_GUIDANCE_TOOLKIT_HPP_

/* A set of tools required for guidance in both pre and post-processing */

#include "extractor/guidance/turn_instruction.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/simple_logger.hpp"

#include <algorithm>
#include <vector>

namespace osrm
{
namespace util
{
namespace guidance
{

inline double angularDeviation(const double angle, const double from)
{
    const double deviation = std::abs(angle - from);
    return std::min(360 - deviation, deviation);
}

inline extractor::guidance::DirectionModifier getTurnDirection(const double angle)
{
    // An angle of zero is a u-turn
    // 180 goes perfectly straight
    // 0-180 are right turns
    // 180-360 are left turns
    if (angle > 0 && angle < 60)
        return extractor::guidance::DirectionModifier::SharpRight;
    if (angle >= 60 && angle < 140)
        return extractor::guidance::DirectionModifier::Right;
    if (angle >= 140 && angle < 170)
        return extractor::guidance::DirectionModifier::SlightRight;
    if (angle >= 165 && angle <= 195)
        return extractor::guidance::DirectionModifier::Straight;
    if (angle > 190 && angle <= 220)
        return extractor::guidance::DirectionModifier::SlightLeft;
    if (angle > 220 && angle <= 300)
        return extractor::guidance::DirectionModifier::Left;
    if (angle > 300 && angle < 360)
        return extractor::guidance::DirectionModifier::SharpLeft;
    return extractor::guidance::DirectionModifier::UTurn;
}

inline double getMatchingDiscreteBearing(const bool requires_entry,
                                  const double bearing,
                                  const EntryClass entry_class,
                                  const std::vector<double> existing_bearings)
{
    if (existing_bearings.empty())
        return 0;

    const double discrete_bearing =
        BearingClass::discreteIDToAngle(BearingClass::angleToDiscreteID(bearing));
    // it they are very close to the turn, the discrete bearing should be fine
    if (std::abs(bearing - discrete_bearing) < 0.25 * BearingClass::discrete_angle_step_size)
    {
        const auto isValidEntry = [&]() {
            const auto bound = std::upper_bound(existing_bearings.begin(), existing_bearings.end(),
                                                (discrete_bearing - 0.001));
            const auto index = bound == existing_bearings.end()
                                   ? 0
                                   : std::distance(existing_bearings.begin(), bound);

            return entry_class.allowsEntry(index);
        };
        BOOST_ASSERT(!requires_entry || isValidEntry());
        return discrete_bearing;
    }
    else
    {
        // the next larger bearing or first if we are wrapping around at zero
        const auto next_index =
            std::distance(existing_bearings.begin(),
                          std::lower_bound(existing_bearings.begin(), existing_bearings.end(),
                                           discrete_bearing)) %
            existing_bearings.size();

        // next smaller bearing or last if we are wrapping around at zero
        const auto previous_index =
            (next_index + existing_bearings.size() - 1) % existing_bearings.size();

        const auto difference = [](const double first, const double second) {
            return std::min(std::abs(first - second), 360.0 - std::abs(first - second));
        };

        const auto next_bearing = existing_bearings[next_index];
        const auto previous_bearing = existing_bearings[previous_index];

        const auto decideOnBearing = [&](
            const std::size_t preferred_index, const double preferred_bearing,
            const std::size_t alternative_index, const double alternative_bearing) {
            if (!requires_entry || entry_class.allowsEntry(preferred_index))
                return preferred_bearing;
            else if (entry_class.allowsEntry(alternative_index))
                return alternative_bearing;
            else
            {
                SimpleLogger().Write(logDEBUG)
                    << "Cannot find a valid entry for a discrete Bearing in Turn Classificiation";
                return 0.;
            }
        };

        if (difference(bearing, next_bearing) < difference(bearing, previous_index))
            return decideOnBearing(next_index, next_bearing, previous_index, previous_bearing);
        else
            return decideOnBearing(previous_index, previous_bearing, next_index, next_bearing);
    }

    return 0;
}
} // namespace guidance
} // namespace util
} // namespace osrm

#endif /* OSRM_UTIL_GUIDANCE_TOOLKIT_HPP_ */
