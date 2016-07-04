#include "extractor/guidance/intersection_scenario_three_way.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/toolkit.hpp"

#include "util/guidance/toolkit.hpp"

using osrm::util::guidance::angularDeviation;

namespace osrm
{
namespace extractor
{
namespace guidance
{

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
