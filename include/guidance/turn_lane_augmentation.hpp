#ifndef OSRM_GUIDANCE_TURN_LANE_AUGMENTATION_HPP_
#define OSRM_GUIDANCE_TURN_LANE_AUGMENTATION_HPP_

#include "guidance/intersection.hpp"
#include "guidance/turn_lane_data.hpp"
#include "util/attributes.hpp"

namespace osrm
{
namespace guidance
{
namespace lanes
{

OSRM_ATTR_WARN_UNUSED
LaneDataVector handleNoneValueAtSimpleTurn(LaneDataVector lane_data,
                                           const Intersection &intersection);

} // namespace lanes
} // namespace guidance
} // namespace osrm

#endif /* OSRM_GUIDANCE_TURN_LANE_AUGMENTATION_HPP_ */
