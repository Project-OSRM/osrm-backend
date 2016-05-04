#ifndef EXTRACTION_WAY_HPP
#define EXTRACTION_WAY_HPP

#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

#include <string>
#include <vector>

namespace osrm
{
namespace extractor
{

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
        duration = -1;
        roundabout = false;
        is_startpoint = true;
        is_access_restricted = false;
        name.clear();
        destination.clear();
        forward_travel_mode = TRAVEL_MODE_INACCESSIBLE;
        backward_travel_mode = TRAVEL_MODE_INACCESSIBLE;
    }

    // These accessors exists because it's not possible to take the address of a bitfield,
    // and LUA therefore cannot read/write the mode attributes directly.
    void set_forward_mode(const TravelMode m) { forward_travel_mode = m; }
    TravelMode get_forward_mode() const { return forward_travel_mode; }
    void set_backward_mode(const TravelMode m) { backward_travel_mode = m; }
    TravelMode get_backward_mode() const { return backward_travel_mode; }

    double forward_speed;
    double backward_speed;
    double duration;
    std::string name;
    std::string destination;
    bool roundabout;
    bool is_access_restricted;
    bool is_startpoint;
    TravelMode forward_travel_mode : 4;
    TravelMode backward_travel_mode : 4;
};
}
}

#endif // EXTRACTION_WAY_HPP
