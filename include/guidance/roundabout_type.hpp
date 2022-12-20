#ifndef OSRM_GUIDANCE_ROUNDABOUT_TYPES_HPP_
#define OSRM_GUIDANCE_ROUNDABOUT_TYPES_HPP_

namespace osrm::guidance
{
enum class RoundaboutType
{
    None,                  // not a roundabout
    Roundabout,            // standard roundabout
    Rotary,                // traffic circle (large roundabout) with dedicated name
    RoundaboutIntersection // small roundabout with distinct turns, handled as intersection
};
} // namespace osrm::guidance

#endif /* OSRM_EXTRACTOR_GUIDANCE_ROUNDABOUT_TYPES_HPP_ */
