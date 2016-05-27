#ifndef OSRM_EXTRACTOR_CLASSIFICATION_DATA_HPP_
#define OSRM_EXTRACTOR_CLASSIFICATION_DATA_HPP_

#include <cstdint>

#include <string>

// Forward Declaration to allow usage of external osmium::Way
namespace osmium
{
class Way;
}

namespace osrm
{
namespace extractor
{
namespace guidance
{

enum class FunctionalRoadClass : std::uint8_t
{
    UNKNOWN = 0,
    MOTORWAY,
    MOTORWAY_LINK,
    TRUNK,
    TRUNK_LINK,
    PRIMARY,
    PRIMARY_LINK,
    SECONDARY,
    SECONDARY_LINK,
    TERTIARY,
    TERTIARY_LINK,
    UNCLASSIFIED,
    RESIDENTIAL,
    SERVICE,
    LIVING_STREET,
    LOW_PRIORITY_ROAD // a road simply included for connectivity. Should be avoided at all cost
};

FunctionalRoadClass functionalRoadClassFromTag(std::string const &tag);

inline bool isRampClass(const FunctionalRoadClass road_class)
{
    // Primary Roads and down are usually too small to announce their links as ramps
    return road_class == FunctionalRoadClass::MOTORWAY_LINK ||
           road_class == FunctionalRoadClass::TRUNK_LINK;
}

// Links are usually smaller than ramps, but are sometimes tagged
// as MOTORWAY_LINK if they exit/enter a motorway/trunk road.
inline bool isLinkClass(const FunctionalRoadClass road_class)
{
    return road_class == FunctionalRoadClass::MOTORWAY_LINK ||
           road_class == FunctionalRoadClass::TRUNK_LINK ||
           road_class == FunctionalRoadClass::PRIMARY_LINK ||
           road_class == FunctionalRoadClass::SECONDARY_LINK ||
           road_class == FunctionalRoadClass::TERTIARY_LINK;
}

// TODO augment this with all data required for guidance generation
struct RoadClassificationData
{
    FunctionalRoadClass road_class = FunctionalRoadClass::UNKNOWN;

    void augment(const osmium::Way &way);
};

inline bool operator==(const RoadClassificationData lhs, const RoadClassificationData rhs)
{
    return lhs.road_class == rhs.road_class;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_CLASSIFICATION_DATA_HPP_
