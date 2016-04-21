#ifndef OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_SCENARIO_THREE_WAY_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_SCENARIO_THREE_WAY_HPP_

#include "extractor/guidance/intersection.hpp"

namespace osrm
{
namespace extractor
{
namespace guidance
{

// possible fork
bool isFork(const ConnectedRoad &uturn,
            const ConnectedRoad &possible_right_fork,
            const ConnectedRoad &possible_left_fork);

// Ending in a T-Intersection
bool isEndOfRoad(const ConnectedRoad &uturn,
                 const ConnectedRoad &possible_right_turn,
                 const ConnectedRoad &possible_left_turn);

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /*OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_SCENARIO_THREE_WAY_HPP_*/
