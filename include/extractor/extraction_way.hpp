#ifndef EXTRACTION_WAY_HPP
#define EXTRACTION_WAY_HPP

#include "extractor/road_classification.hpp"
#include "extractor/travel_mode.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/typedefs.hpp"

#include <string>
#include <vector>

namespace osrm::extractor
{
namespace detail
{
inline void maybeSetString(std::string &str, const char *value)
{
    if (value == nullptr)
    {
        str.clear();
    }
    else
    {
        str = std::string(value);
    }
}
} // namespace detail

/**
 * This struct is the direct result of the call to ```way_function```
 * in the lua based profile.
 *
 * It is split into multiple edge segments in the ExtractorCallback.
 */
struct ExtractionWay
{
    ExtractionWay() { clear(); }

    void clear()
    {
        forward_speed = -1;
        backward_speed = -1;
        forward_rate = -1;
        backward_rate = -1;
        duration = -1;
        weight = -1;
        name.clear();
        forward_ref.clear();
        backward_ref.clear();
        pronunciation.clear();
        destinations.clear();
        exits.clear();
        turn_lanes_forward.clear();
        turn_lanes_backward.clear();
        road_classification = RoadClassification();
        forward_travel_mode = TRAVEL_MODE_INACCESSIBLE;
        backward_travel_mode = TRAVEL_MODE_INACCESSIBLE;
        roundabout = false;
        circular = false;
        is_startpoint = true;
        forward_restricted = false;
        backward_restricted = false;
        is_left_hand_driving = false;
        highway_turn_classification = 0;
        access_turn_classification = 0;
    }

    // wrappers to allow assigning nil (nullptr) to string values
    void SetName(const char *value) { detail::maybeSetString(name, value); }
    const char *GetName() const { return name.c_str(); }
    void SetForwardRef(const char *value) { detail::maybeSetString(forward_ref, value); }
    const char *GetForwardRef() const { return forward_ref.c_str(); }
    void SetBackwardRef(const char *value) { detail::maybeSetString(backward_ref, value); }
    const char *GetBackwardRef() const { return backward_ref.c_str(); }
    void SetDestinations(const char *value) { detail::maybeSetString(destinations, value); }
    const char *GetDestinations() const { return destinations.c_str(); }
    void SetExits(const char *value) { detail::maybeSetString(exits, value); }
    const char *GetExits() const { return exits.c_str(); }
    void SetPronunciation(const char *value) { detail::maybeSetString(pronunciation, value); }
    const char *GetPronunciation() const { return pronunciation.c_str(); }
    void SetTurnLanesForward(const char *value)
    {
        detail::maybeSetString(turn_lanes_forward, value);
    }
    const char *GetTurnLanesForward() const { return turn_lanes_forward.c_str(); }
    void SetTurnLanesBackward(const char *value)
    {
        detail::maybeSetString(turn_lanes_backward, value);
    }
    const char *GetTurnLanesBackward() const { return turn_lanes_backward.c_str(); }

    // markers for determining user-defined classes for each way
    std::unordered_map<std::string, bool> forward_classes;
    std::unordered_map<std::string, bool> backward_classes;

    // speed in km/h
    double forward_speed;
    double backward_speed;
    // weight per meter
    double forward_rate;
    double backward_rate;
    // duration of the whole way in both directions
    double duration;
    // weight of the whole way in both directions
    double weight;
    std::string name;
    std::string forward_ref;
    std::string backward_ref;
    std::string pronunciation;
    std::string destinations;
    std::string exits;
    std::string turn_lanes_forward;
    std::string turn_lanes_backward;
    RoadClassification road_classification;
    TravelMode forward_travel_mode : 4;
    TravelMode backward_travel_mode : 4;

    // Boolean flags
    bool roundabout : 1;
    bool circular : 1;
    bool is_startpoint : 1;
    bool forward_restricted : 1;
    bool backward_restricted : 1;
    bool is_left_hand_driving : 1;
    bool : 2;

    // user classifications for turn penalties
    std::uint8_t highway_turn_classification : 4;
    std::uint8_t access_turn_classification : 4;
};
} // namespace osrm::extractor

#endif // EXTRACTION_WAY_HPP
