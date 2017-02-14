#ifndef EXTRACTION_WAY_HPP
#define EXTRACTION_WAY_HPP

#include "extractor/guidance/road_classification.hpp"
#include "extractor/travel_mode.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/typedefs.hpp"

#include <string>
#include <vector>

namespace osrm
{
namespace extractor
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
}

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
        roundabout = false;
        circular = false;
        is_startpoint = true;
        name.clear();
        ref.clear();
        pronunciation.clear();
        destinations.clear();
        forward_travel_mode = TRAVEL_MODE_INACCESSIBLE;
        backward_travel_mode = TRAVEL_MODE_INACCESSIBLE;
        turn_lanes_forward.clear();
        turn_lanes_backward.clear();
        road_classification = guidance::RoadClassification();
        backward_restricted = false;
        forward_restricted = false;
    }

    // These accessors exists because it's not possible to take the address of a bitfield,
    // and LUA therefore cannot read/write the mode attributes directly.
    void set_forward_mode(const TravelMode m) { forward_travel_mode = m; }
    TravelMode get_forward_mode() const { return forward_travel_mode; }
    void set_backward_mode(const TravelMode m) { backward_travel_mode = m; }
    TravelMode get_backward_mode() const { return backward_travel_mode; }

    // wrappers to allow assigning nil (nullptr) to string values
    void SetName(const char *value) { detail::maybeSetString(name, value); }
    const char *GetName() const { return name.c_str(); }
    void SetRef(const char *value) { detail::maybeSetString(ref, value); }
    const char *GetRef() const { return ref.c_str(); }
    void SetDestinations(const char *value) { detail::maybeSetString(destinations, value); }
    const char *GetDestinations() const { return destinations.c_str(); }
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
    std::string ref;
    std::string pronunciation;
    std::string destinations;
    std::string turn_lanes_forward;
    std::string turn_lanes_backward;
    bool roundabout;
    bool circular;
    bool is_startpoint;
    bool backward_restricted;
    bool forward_restricted;
    TravelMode forward_travel_mode : 4;
    TravelMode backward_travel_mode : 4;
    guidance::RoadClassification road_classification;
};
}
}

#endif // EXTRACTION_WAY_HPP
