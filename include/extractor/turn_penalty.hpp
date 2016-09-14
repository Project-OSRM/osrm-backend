#ifndef OSRM_TURN_PENALTY_HPP_
#define OSRM_TURN_PENALTY_HPP_

/* The turn functions offered by osrm come in two variations. One uses a basic angle. The other one
 * offers a list of additional entries to improve the turn penalty */

namespace osrm
{
namespace extractor
{

// properties describing the turn
struct TurnProperties
{
    // the turn angle between the ingoing/outgoing segments
    double angle;
    // the radius of the turn at the curve (approximated)
    double radius;
    // the turn is crossing through opposing traffic
    bool crossing_through_traffic;
    // indicate if the turn needs to be announced or if it is a turn following the obvious road
    bool requires_announcement;
};

// properties describing intersection related penalties
struct IntersectionProperties
{
    // is the turn at a traffic light
    bool traffic_light;
    // is the turn at a stop-light
    bool stop_sign;
    // is the turn following a right-of-way situation
    bool right_of_way;
};

// information about the segments coming in/out of the intersection
struct TurnSegment
{
    double length_in_meters;
    double speed_in_meters_per_second;
};

} // namespace extractor
} // namespace osrm

#endif // OSRM_TURN_PENALTY_HPP_
