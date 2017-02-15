#ifndef OSRM_ENGINE_GUIDANCE_COLLAPSE_SCENARIO_DETECTION_HPP_
#define OSRM_ENGINE_GUIDANCE_COLLAPSE_SCENARIO_DETECTION_HPP_

#include "engine/guidance/collapsing_utility.hpp"
#include "engine/guidance/route_step.hpp"

namespace osrm
{
namespace engine
{
namespace guidance
{

// check basic collapse preconditions (mode ok, no roundabout types);
bool basicCollapsePreconditions(const RouteStepIterator first, const RouteStepIterator second);

bool basicCollapsePreconditions(const RouteStepIterator first,
                                const RouteStepIterator second,
                                const RouteStepIterator third);

// Staggered intersection are very short zig-zags of a few meters.
// We do not want to announce these short left-rights or right-lefts:
// 
//      * -> b      a -> *
//      |       or       |       becomes  a   ->   b
// a -> *                * -> b
bool isStaggeredIntersection(const RouteStepIterator step_prior_to_intersection,
                             const RouteStepIterator step_entering_intersection,
                             const RouteStepIterator step_leaving_intersection);

// Two two turns following close after another, we can announce them as a U-Turn if both end up
// involving the same (segregated) road.
// 
// b < - y
//       |      will be represented by at x, turn around instead of turn left at x, turn left at y
// a - > x
bool isUTurn(const RouteStepIterator step_prior_to_intersection,
             const RouteStepIterator step_entering_intersection,
             const RouteStepIterator step_leaving_intersection);

// detect oscillating names where a name switch A->B->A occurs. This is often the case due to
// bridges or tunnels. Any such oszillation is not supposed to show up
bool isNameOszillation(const RouteStepIterator step_prior_to_intersection,
                       const RouteStepIterator step_entering_intersection,
                       const RouteStepIterator step_leaving_intersection);

// Sometimes, segments names don't match the perceived turns. We try to detect these additional
// name changes and issue a combined turn.
// 
//  |  e  |
// a - b - c
//         d
// 
// can have `a-b` as one name, `b-c-d` as a second. At `b` we would issue a new name, even though
// the road turns right after. The offset would only be there due to the broad road at `e`
bool maneuverPreceededByNameChange(const RouteStepIterator step_prior_to_intersection,
                                   const RouteStepIterator step_entering_intersection,
                                   const RouteStepIterator step_leaving_intersection);
bool maneuverPreceededBySuppressedDirection(const RouteStepIterator step_entering_intersection,
                                            const RouteStepIterator step_leaving_intersection);
bool suppressedStraightBetweenTurns(const RouteStepIterator step_entering_intersection,
                                    const RouteStepIterator step_at_center_of_intersection,
                                    const RouteStepIterator step_leaving_intersection);

bool maneuverSucceededByNameChange(const RouteStepIterator step_entering_intersection,
                                   const RouteStepIterator step_leaving_intersection);
bool maneuverSucceededBySuppressedDirection(const RouteStepIterator step_entering_intersection,
                                            const RouteStepIterator step_leaving_intersection);
bool nameChangeImmediatelyAfterSuppressed(const RouteStepIterator step_entering_intersection,
                                          const RouteStepIterator step_leaving_intersection);
bool closeChoicelessTurnAfterTurn(const RouteStepIterator step_entering_intersection,
                                  const RouteStepIterator step_leaving_intersection);
// if modelled turn roads meet in the center of a segregated intersection, we can end up with double
// choiceless turns
bool doubleChoiceless(const RouteStepIterator step_entering_intersection,
                      const RouteStepIterator step_leaving_intersection);

// Due to obvious detection, sometimes we can have straight turns followed by a different turn right
// next to each other. We combine both turns into one, if the second turn is without choice
// 
//         e
// a - b - c
//       ' d
// 
// with a main road `abd`, the turn `continue straight` at `b` and `turn left at `c` will become a
// `turn left` at `b`
bool straightTurnFollowedByChoiceless(const RouteStepIterator step_entering_intersection,
                                      const RouteStepIterator step_leaving_intersection);

} /* namespace guidance */
} /* namespace engine */
} /* namespace osrm */

#endif /* OSRM_ENGINE_GUIDANCE_COLLAPSE_SCENARIO_DETECTION_HPP_ */
