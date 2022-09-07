#ifndef OSRM_EXTRACTOR_TRAFFIC_LIGHTS_DATA_HPP_
#define OSRM_EXTRACTOR_TRAFFIC_LIGHTS_DATA_HPP_

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

} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_TRAFFIC_LIGHTS_DATA_HPP_
