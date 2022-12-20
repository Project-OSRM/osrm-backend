#ifndef OSRM_GUIDANCE_TURN_LANE_AUGMENTATION_HPP_
#define OSRM_GUIDANCE_TURN_LANE_AUGMENTATION_HPP_

#include "guidance/intersection.hpp"
#include "guidance/turn_lane_data.hpp"
#include "util/attributes.hpp"

namespace osrm::guidance::lanes
{

OSRM_ATTR_WARN_UNUSED
LaneDataVector handleNoneValueAtSimpleTurn(LaneDataVector lane_data,
                                           const Intersection &intersection);

} // namespace osrm::guidance::lanes

#endif /* OSRM_GUIDANCE_TURN_LANE_AUGMENTATION_HPP_ */
