#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/intersection_scenario_three_way.hpp"
#include "extractor/guidance/toolkit.hpp"

namespace osrm
{
namespace extractor
{
namespace guidance
{

bool isFork(const ConnectedRoad &,
            const ConnectedRoad &possible_right_fork,
            const ConnectedRoad &possible_left_fork)
{
    return angularDeviation(possible_right_fork.turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
           angularDeviation(possible_left_fork.turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE;
}

bool isEndOfRoad(const ConnectedRoad &,
                 const ConnectedRoad &possible_right_turn,
                 const ConnectedRoad &possible_left_turn)
{
    return angularDeviation(possible_right_turn.turn.angle, 90) < NARROW_TURN_ANGLE &&
           angularDeviation(possible_left_turn.turn.angle, 270) < NARROW_TURN_ANGLE &&
           angularDeviation(possible_right_turn.turn.angle, possible_left_turn.turn.angle) >
               2 * NARROW_TURN_ANGLE;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
