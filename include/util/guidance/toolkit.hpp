#ifndef OSRM_UTIL_GUIDANCE_TOOLKIT_HPP_
#define OSRM_UTIL_GUIDANCE_TOOLKIT_HPP_

/* A set of tools required for guidance in both pre and post-processing */

#include "engine/guidance/route_step.hpp"
#include "engine/phantom_node.hpp"
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

inline engine::guidance::Intersection
setIntersectionClasses(engine::guidance::Intersection intersection,
                       const engine::PhantomNode &phantom)
{
    BOOST_ASSERT(intersection.bearing_before == 0 || intersection.bearing_after == 0);
    const double bearing = std::max(intersection.bearing_before, intersection.bearing_after);

    intersection.bearing_class = {};
    intersection.entry_class = {};
    if (bearing >= 180.)
    {
        intersection.bearing_class.add(std::round(bearing - 180.));
        if (phantom.forward_segment_id.id != SPECIAL_SEGMENTID &&
            phantom.reverse_segment_id.id != SPECIAL_SEGMENTID)
            intersection.entry_class.activate(0);
        intersection.bearing_class.add(std::round(bearing));
        intersection.entry_class.activate(1);
    }
    else
    {
        intersection.bearing_class.add(std::round(bearing));
        intersection.entry_class.activate(0);
        intersection.bearing_class.add(std::round(bearing + 180.));
        if (phantom.forward_segment_id.id != SPECIAL_SEGMENTID &&
            phantom.reverse_segment_id.id != SPECIAL_SEGMENTID)
            intersection.entry_class.activate(1);
    }
    return intersection;
}

} // namespace guidance
} // namespace util
} // namespace osrm

#endif /* OSRM_UTIL_GUIDANCE_TOOLKIT_HPP_ */
