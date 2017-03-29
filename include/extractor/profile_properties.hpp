#ifndef PROFILE_PROPERTIES_HPP
#define PROFILE_PROPERTIES_HPP

#include "util/typedefs.hpp"

#include <algorithm>
#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>

namespace osrm
{
namespace extractor
{

const constexpr auto DEFAULT_MAX_SPEED = 180 / 3.6; // 180kmph -> m/s

struct ProfileProperties
{
    static constexpr int MAX_WEIGHT_NAME_LENGTH = 255;

    ProfileProperties()
        : traffic_signal_penalty(0), u_turn_penalty(0),
          max_speed_for_map_matching(DEFAULT_MAX_SPEED), continue_straight_at_waypoint(true),
          use_turn_restrictions(false), left_hand_driving(false), fallback_to_duration(true),
          weight_name{"duration"}
    {
        BOOST_ASSERT(weight_name[MAX_WEIGHT_NAME_LENGTH] == '\0');
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

    void SetWeightName(const std::string &name)
    {
        auto count = std::min<std::size_t>(name.length(), MAX_WEIGHT_NAME_LENGTH) + 1;
        std::copy_n(name.c_str(), count, weight_name);
        // Make sure this is always zero terminated
        BOOST_ASSERT(weight_name[count - 1] == '\0');
        BOOST_ASSERT(weight_name[MAX_WEIGHT_NAME_LENGTH] == '\0');
        // Set lazy fallback flag
        fallback_to_duration = name == "duration";
    }

    std::string GetWeightName() const
    {
        // Make sure this is always zero terminated
        BOOST_ASSERT(weight_name[MAX_WEIGHT_NAME_LENGTH] == '\0');
        return std::string(weight_name);
    }

    double GetWeightMultiplier() const { return std::pow(10., weight_precision); }

    double GetMaxTurnWeight() const
    {
        return std::numeric_limits<TurnPenalty>::max() / GetWeightMultiplier();
    }

    //! penalty to cross a traffic light in deci-seconds
    std::int32_t traffic_signal_penalty;
    //! penalty to do a uturn in deci-seconds
    std::int32_t u_turn_penalty;
    double max_speed_for_map_matching;
    //! depending on the profile, force the routing to always continue in the same direction
    bool continue_straight_at_waypoint;
    //! flag used for restriction parser (e.g. used for the walk profile)
    bool use_turn_restrictions;
    bool left_hand_driving;
    bool fallback_to_duration;
    //! stores the name of the weight (e.g. 'duration', 'distance', 'safety')
    char weight_name[MAX_WEIGHT_NAME_LENGTH + 1];
    unsigned weight_precision = 1;
    bool force_split_edges = false;
};
}
}

#endif
