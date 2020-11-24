#ifndef PROFILE_PROPERTIES_HPP
#define PROFILE_PROPERTIES_HPP

#include "extractor/class_data.hpp"

#include "util/typedefs.hpp"

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/optional.hpp>

#include <algorithm>
#include <array>
#include <cstdint>

namespace osrm
{
namespace extractor
{

const constexpr auto DEFAULT_MAX_SPEED = 180 / 3.6; // 180kmph -> m/s

struct ProfileProperties
{
    static constexpr int MAX_WEIGHT_NAME_LENGTH = 255;
    static constexpr int MAX_CLASS_NAME_LENGTH = 255;

    ProfileProperties()
        : traffic_signal_penalty(0), u_turn_penalty(0),
          max_speed_for_map_matching(DEFAULT_MAX_SPEED), continue_straight_at_waypoint(true),
          use_turn_restrictions(false), left_hand_driving(false), fallback_to_duration(true),
          weight_name{"duration"}, class_names{{}}, excludable_classes{{}},
          call_tagless_node_function(true)
    {
        std::fill(excludable_classes.begin(), excludable_classes.end(), INAVLID_CLASS_DATA);
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

    // Mark this combination of classes as excludable
    void SetExcludableClasses(std::size_t index, ClassData classes)
    {
        excludable_classes[index] = classes;
    }

    // Check if this classes are excludable
    boost::optional<std::size_t> ClassesAreExcludable(ClassData classes) const
    {
        auto iter = std::find(excludable_classes.begin(), excludable_classes.end(), classes);
        if (iter != excludable_classes.end())
        {
            return std::distance(excludable_classes.begin(), iter);
        }

        return {};
    }

    void SetClassName(std::size_t index, const std::string &name)
    {
        char *name_ptr = class_names[index];
        auto count = std::min<std::size_t>(name.length(), MAX_CLASS_NAME_LENGTH) + 1;
        std::copy_n(name.c_str(), count, name_ptr);
        // Make sure this is always zero terminated
        BOOST_ASSERT(class_names[index][count - 1] == '\0');
        BOOST_ASSERT(class_names[index][MAX_CLASS_NAME_LENGTH] == '\0');
    }

    std::string GetClassName(std::size_t index) const
    {
        BOOST_ASSERT(index <= MAX_CLASS_INDEX);
        const auto &name_it = std::begin(class_names) + index;
        return std::string(*name_it);
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
    bool left_hand_driving; // DEPRECATED: property value is local to edges from API version 2
    bool fallback_to_duration;
    //! stores the name of the weight (e.g. 'duration', 'distance', 'safety')
    char weight_name[MAX_WEIGHT_NAME_LENGTH + 1];
    //! stores the names of each class
    std::array<char[MAX_CLASS_NAME_LENGTH + 1], MAX_CLASS_INDEX + 1> class_names;
    //! stores the masks of excludable class combinations
    std::array<ClassData, MAX_EXCLUDABLE_CLASSES> excludable_classes;
    unsigned weight_precision = 1;
    bool force_split_edges = false;
    bool call_tagless_node_function = true;
};
} // namespace extractor
} // namespace osrm

#endif
