#ifndef OSRM_TURN_PENALTY_HPP_
#define OSRM_TURN_PENALTY_HPP_

/* The turn functions offered by osrm come in two variations. One uses a basic angle. The other one
 * offers a list of additional entries to improve the turn penalty */

#include <string>

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
    // check if we are continuing on the normal road on an obvious turn
    bool is_through_street;
};

// properties describing intersection related penalties
struct IntersectionProperties
{
    // is the turn at a traffic light
    bool traffic_light;
    // is the turn following a right-of-way situation
    bool right_of_way;
    // has to give way describes situations where we have to let traffic pass (e.g. due to stop
    // signs) but also simply due to driveways
    bool give_way;
};

// information about the segments coming in/out of the intersection
struct TurnSegment
{
    double length_in_meters;
    double speed_in_meters_per_second;
};

inline std::string toString(const TurnProperties &props)
{
    std::string result;
    result += "[Angle: " + std::to_string(props.angle) + " Radius: " +
              std::to_string(props.radius) + " | Through Traffic: " +
              (props.crossing_through_traffic ? "true" : "false") + " | Announced: " +
              (props.requires_announcement ? "true" : "false") + " | Through Traffic: " +
              (props.is_through_street ? "true" : "false") + "]";
    return result;
}

inline std::string toString(const IntersectionProperties &props)
{
    std::string result;
    result += std::string("[") + "Traffic Light: " + (props.traffic_light ? "true" : "false") +
              " | Right Of Way: " + (props.right_of_way ? "true" : "false") + " | Give Way: " +
              (props.give_way ? "true" : "false") + "]";
    return result;
}

inline std::string toString(const TurnSegment &segment)
{
    std::string result;
    result += std::string("[") + "Length: " + std::to_string(segment.length_in_meters) +
              " Speed: " + std::to_string(segment.speed_in_meters_per_second) + "]";
    return result;
}

} // namespace extractor
} // namespace osrm

#endif // OSRM_TURN_PENALTY_HPP_
