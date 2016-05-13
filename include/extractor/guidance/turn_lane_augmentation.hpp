#ifndef OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_AUGMENTATION_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_AUGMENTATION_HPP_

#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/turn_lane_data.hpp"

namespace osrm
{
namespace extractor
{
namespace guidance
{
namespace lanes
{

LaneDataVector handleNoneValueAtSimpleTurn(LaneDataVector lane_data,
                                           const Intersection &intersection);

} // namespace lanes
} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /* OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_AUGMENTATION_HPP_ */
