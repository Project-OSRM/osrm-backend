#ifndef OSRM_EXTRACTOR_TRAFFIC_LIGHTS_DATA_HPP_
#define OSRM_EXTRACTOR_TRAFFIC_LIGHTS_DATA_HPP_

#include <cstdint>
namespace osrm
{
namespace extractor
{

namespace TrafficLightClass
{
// The traffic light annotation is extracted from node tags.
// The directions in which the traffic light applies are relative to the way containing the node.
enum Direction
{
    NONE = 0,
    DIRECTION_ALL = 1,
    DIRECTION_FORWARD = 2,
    DIRECTION_REVERSE = 3
};
} // namespace TrafficLightClass

// Stop Signs tagged on nodes can be present or not. In addition Stop Signs have
// an optional way direction they apply to. If the direction is unknown from the
// data we have to compute by checking the distance to the next intersection.
//
// Impl. detail: namespace + enum instead of enum class to make Luabind happy
namespace StopSign
{
enum Direction : std::uint8_t
{
    NONE = 0,
    DIRECTION_ALL = 1,
    DIRECTION_FORWARD = 2,
    DIRECTION_REVERSE = 3
};
}

// Give Way is the complement to priority roads. Tagging is the same as Stop Signs.
// See explanation above.
namespace GiveWay
{
enum Direction : std::uint8_t
{
    NONE = 0,
    DIRECTION_ALL = 1,
    DIRECTION_FORWARD = 2,
    DIRECTION_REVERSE = 3
};
}


} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_TRAFFIC_LIGHTS_DATA_HPP_
