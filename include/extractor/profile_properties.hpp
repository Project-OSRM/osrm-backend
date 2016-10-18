#ifndef PROFILE_PROPERTIES_HPP
#define PROFILE_PROPERTIES_HPP

#include <boost/numeric/conversion/cast.hpp>

namespace osrm
{
namespace extractor
{

const constexpr auto DEFAULT_MAX_SPEED = 180 / 3.6; // 180kmph -> m/s

struct ProfileProperties
{
    ProfileProperties()
        : traffic_signal_penalty(0), u_turn_penalty(0),
          max_speed_for_map_matching(DEFAULT_MAX_SPEED), continue_straight_at_waypoint(true),
          use_turn_restrictions(false), left_hand_driving(false)
    {
    }

    double GetUturnPenalty() const { return u_turn_penalty / 10.; }

    void SetUturnPenalty(const double u_turn_penalty_)
    {
        u_turn_penalty = boost::numeric_cast<int>(u_turn_penalty_ * 10.);
    }

    double GetTrafficSignalPenalty() const { return traffic_signal_penalty / 10.; }

    void SetTrafficSignalPenalty(const double traffic_signal_penalty_)
    {
        traffic_signal_penalty = boost::numeric_cast<int>(traffic_signal_penalty_ * 10.);
    }

    double GetMaxSpeedForMapMatching() const { return max_speed_for_map_matching; }

    void SetMaxSpeedForMapMatching(const double max_speed_for_map_matching_)
    {
        max_speed_for_map_matching = max_speed_for_map_matching_;
    }

    //! penalty to cross a traffic light in deci-seconds
    int traffic_signal_penalty;
    //! penalty to do a uturn in deci-seconds
    int u_turn_penalty;
    double max_speed_for_map_matching;
    bool continue_straight_at_waypoint;
    bool use_turn_restrictions;
    bool left_hand_driving;
};
}
}

#endif
