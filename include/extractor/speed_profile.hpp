#ifndef SPEED_PROFILE_PROPERTIES_HPP
#define SPEED_PROFILE_PROPERTIES_HPP

namespace osrm
{
namespace extractor
{

struct SpeedProfileProperties
{
    SpeedProfileProperties()
        : traffic_signal_penalty(0), u_turn_penalty(0), has_turn_penalty_function(false)
    {
    }

    int traffic_signal_penalty;
    int u_turn_penalty;
    bool has_turn_penalty_function;
};

}
}

#endif
