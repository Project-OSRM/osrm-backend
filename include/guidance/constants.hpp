#ifndef OSRM_GUIDANCE_CONSTANTS_HPP_
#define OSRM_GUIDANCE_CONSTANTS_HPP_

#include "extractor/intersection/constants.hpp"

namespace osrm
{
namespace guidance
{

// what angle is interpreted as going straight
using extractor::intersection::STRAIGHT_ANGLE;
// if a turn deviates this much from going straight, it will be kept straight
using extractor::intersection::MAXIMAL_ALLOWED_NO_TURN_DEVIATION;
// angle that lies between two nearly indistinguishable roads
using extractor::intersection::GROUP_ANGLE;
using extractor::intersection::NARROW_TURN_ANGLE;
// angle difference that can be classified as straight, if its the only narrow turn
using extractor::intersection::FUZZY_ANGLE_DIFFERENCE;

const double constexpr DISTINCTION_RATIO = 2;

// Named roundabouts with radii larger then than this are seen as rotary
const double constexpr MAX_ROUNDABOUT_RADIUS = 15;
// Unnamed small roundabouts that look like intersections are announced as turns,
// guard against data issues or such roundabout intersections getting too large.
const double constexpr MAX_ROUNDABOUT_INTERSECTION_RADIUS = 15;

const double constexpr INCREASES_BY_FOURTY_PERCENT = 1.4;

const int constexpr MAX_SLIPROAD_THRESHOLD = 250;

} // namespace guidance
} // namespace osrm

#endif // OSRM_GUIDANCE_CONSTANTS_HPP_
