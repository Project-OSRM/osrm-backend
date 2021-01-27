#ifndef OSRM_EXTRACTOR_CLASSIFICATION_DATA_HPP_
#define OSRM_EXTRACTOR_CLASSIFICATION_DATA_HPP_

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "extractor/intersection/constants.hpp"

namespace osrm
{
namespace extractor
{

// Priorities are used to distinguish between how likely a turn is in comparison to a different
// road. The priorities here are used to distinguish between obvious turns (e.g. following a primary
// road next to a residential one is obvious). The decision what is obvious is described in the
// guidance constants.
namespace RoadPriorityClass
{
typedef std::uint8_t Enum;
// Top priority Road
const constexpr Enum MOTORWAY = 0;
const constexpr Enum MOTORWAY_LINK = 1;
// Second highest priority
const constexpr Enum TRUNK = 2;
const constexpr Enum TRUNK_LINK = 3;
// Main roads
const constexpr Enum PRIMARY = 4;
const constexpr Enum PRIMARY_LINK = 5;
const constexpr Enum SECONDARY = 6;
const constexpr Enum SECONDARY_LINK = 7;
const constexpr Enum TERTIARY = 8;
const constexpr Enum TERTIARY_LINK = 9;
// Residential Categories
const constexpr Enum MAIN_RESIDENTIAL = 10;
const constexpr Enum SIDE_RESIDENTIAL = 11;
const constexpr Enum ALLEY = 12;
const constexpr Enum PARKING = 13;
// Link Category
const constexpr Enum LINK_ROAD = 14;
const constexpr Enum UNCLASSIFIED = 15;
// Bike Accessible
const constexpr Enum BIKE_PATH = 16;
// Walk Accessible
const constexpr Enum FOOT_PATH = 18;
// Link types are usually not considered in forks, unless amongst each other.
// a road simply offered for connectivity. Will be ignored in forks/other decisions. Always
// considered non-obvious to continue on
const constexpr Enum CONNECTIVITY = 31;
} // namespace RoadPriorityClass

#pragma pack(push, 1)
class RoadClassification
{
    // a class that behaves like a motorway (separated directions)
    std::uint8_t motorway_class : 1;
    // all types of link classes
    std::uint8_t link_class : 1;
    // a low priority class is a pure connectivity way. It can be ignored in multiple decisions
    // (e.g. fork on a primary vs service will not happen)
    std::uint8_t may_be_ignored : 1;
    // the road priority is used as an indicator for forks. If the roads are of similar priority
    // (difference <=1), we can see the road as a fork. Else one of the road classes is seen as
    // obvious choice
    RoadPriorityClass::Enum road_priority_class : 5;
    // the number of lanes in the road
    std::uint8_t number_of_lanes;

  public:
    // default construction
    RoadClassification()
        : motorway_class(0), link_class(0), may_be_ignored(0),
          road_priority_class(RoadPriorityClass::CONNECTIVITY), number_of_lanes(0)
    {
    }

    RoadClassification(bool motorway_class,
                       bool link_class,
                       bool may_be_ignored,
                       RoadPriorityClass::Enum road_priority_class,
                       std::uint8_t number_of_lanes)
        : motorway_class(motorway_class), link_class(link_class), may_be_ignored(may_be_ignored),
          road_priority_class(road_priority_class), number_of_lanes(number_of_lanes)
    {
    }

    bool IsMotorwayClass() const { return (0 != motorway_class) && (0 == link_class); }
    void SetMotorwayFlag(const bool new_value) { motorway_class = new_value; }

    bool IsRampClass() const { return (0 != motorway_class) && (0 != link_class); }

    bool IsLinkClass() const { return (0 != link_class); }
    void SetLinkClass(const bool new_value) { link_class = new_value; }

    bool IsLowPriorityRoadClass() const { return (0 != may_be_ignored); }
    void SetLowPriorityFlag(const bool new_value) { may_be_ignored = new_value; }

    std::uint8_t GetNumberOfLanes() const { return number_of_lanes; }
    void SetNumberOfLanes(const std::uint8_t new_value) { number_of_lanes = new_value; }

    std::uint32_t GetPriority() const { return static_cast<std::uint32_t>(road_priority_class); }

    RoadPriorityClass::Enum GetClass() const { return road_priority_class; }
    void SetClass(const RoadPriorityClass::Enum new_value) { road_priority_class = new_value; }

    bool operator==(const RoadClassification &other) const
    {
        return motorway_class == other.motorway_class && link_class == other.link_class &&
               may_be_ignored == other.may_be_ignored &&
               road_priority_class == other.road_priority_class;
    }

    bool operator!=(const RoadClassification &other) const { return !(*this == other); }

    std::string ToString() const
    {
        return std::string() + (motorway_class ? "motorway" : "normal") +
               (link_class ? "_link" : "") + (may_be_ignored ? " ignorable " : " important ") +
               std::to_string(road_priority_class);
    }
};
#pragma pack(pop)

static_assert(
    sizeof(RoadClassification) == 2,
    "Road Classification should fit two bytes. Increasing this has a severe impact on memory.");

inline bool canBeSeenAsFork(const RoadClassification first, const RoadClassification second)
{
    return std::abs(static_cast<int>(first.GetPriority()) -
                    static_cast<int>(second.GetPriority())) <= 1;
}

// priority groups are road classes that can be categoriesed as somewhat similar
inline std::uint8_t getRoadGroup(const RoadClassification classification)
{
    const constexpr std::uint8_t groups[RoadPriorityClass::CONNECTIVITY + 1] = {
        0, // MOTORWAY
        0, // MOTORWAY_LINK
        1, // TRUNK
        1, // TRUNK_LINK
        2, // PRIMARY
        2, // PRIMARY_LINK
        2, // SECONDARY
        2, // SECONDARY_LINK
        2, // TERTIARY
        2, // TERTIARY_LINK
        3, // MAIN_RESIDENTIAL
        3, // SIDE_RESIDENTIAL
        3, // ALLEY
        3, // PARKING
        4, // LINK_ROAD
        4, // UNCLASSIFIED
        5, // BIKE_PATH
        6, // FOOT_PATH
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7 // CONNECTIVITY
    };

    BOOST_ASSERT(groups[RoadPriorityClass::CONNECTIVITY] == 7);

    return groups[classification.GetPriority()];
}

// LHS road classification is strictly less than RHS, if it belongs to a lower general category
// of roads. E.g. normal city roads are strictly less of a priority than a motorway and alleys
// are strictly less than inner-city roads.
inline bool strictlyLess(const RoadClassification lhs, const RoadClassification rhs)
{
    if (!lhs.IsLowPriorityRoadClass() && rhs.IsLowPriorityRoadClass())
        return false;
    if (lhs.IsLowPriorityRoadClass() && !rhs.IsLowPriorityRoadClass())
        return true;

    return getRoadGroup(lhs) > getRoadGroup(rhs);
}

// check whether a link class is the fitting link class to a road
inline bool isLinkTo(const RoadClassification link, const RoadClassification road)
{
    // needs to be a link/non-link combination
    if (!link.IsLinkClass() || road.IsLinkClass())
        return false;

    switch (link.GetPriority())
    {
    case RoadPriorityClass::MOTORWAY_LINK:
        return road.GetPriority() == RoadPriorityClass::MOTORWAY;

    case RoadPriorityClass::TRUNK_LINK:
        return road.GetPriority() == RoadPriorityClass::TRUNK;

    case RoadPriorityClass::PRIMARY_LINK:
        return road.GetPriority() == RoadPriorityClass::PRIMARY;

    case RoadPriorityClass::SECONDARY_LINK:
        return road.GetPriority() == RoadPriorityClass::SECONDARY;

    case RoadPriorityClass::TERTIARY_LINK:
        return road.GetPriority() == RoadPriorityClass::TERTIARY;

    default:
        return false;
    }
}

inline bool obviousByRoadClass(const RoadClassification in_classification,
                               const RoadClassification obvious_candidate,
                               const RoadClassification compare_candidate)
{
    // lower numbers are of higher priority
    const bool has_high_priority =
        intersection::PRIORITY_DISTINCTION_FACTOR * obvious_candidate.GetPriority() <
        compare_candidate.GetPriority();

    const bool continues_on_same_class = in_classification == obvious_candidate;
    return (has_high_priority && continues_on_same_class) ||
           (!obvious_candidate.IsLowPriorityRoadClass() &&
            !in_classification.IsLowPriorityRoadClass() &&
            compare_candidate.IsLowPriorityRoadClass());
}

} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_CLASSIFICATION_DATA_HPP_
