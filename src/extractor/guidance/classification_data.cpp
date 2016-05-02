#include "extractor/guidance/classification_data.hpp"
#include "util/simple_logger.hpp"

#include <unordered_map>

namespace osrm
{
namespace extractor
{
namespace guidance
{

FunctionalRoadClass functionalRoadClassFromTag(std::string const &value)
{
    // FIXME at some point this should be part of the profiles
    const static auto class_hash = [] {
        std::unordered_map<std::string, FunctionalRoadClass> hash;
        hash["motorway"] = FunctionalRoadClass::MOTORWAY;
        hash["motorway_link"] = FunctionalRoadClass::MOTORWAY_LINK;
        hash["trunk"] = FunctionalRoadClass::TRUNK;
        hash["trunk_link"] = FunctionalRoadClass::TRUNK_LINK;
        hash["primary"] = FunctionalRoadClass::PRIMARY;
        hash["primary_link"] = FunctionalRoadClass::PRIMARY_LINK;
        hash["secondary"] = FunctionalRoadClass::SECONDARY;
        hash["secondary_link"] = FunctionalRoadClass::SECONDARY_LINK;
        hash["tertiary"] = FunctionalRoadClass::TERTIARY;
        hash["tertiary_link"] = FunctionalRoadClass::TERTIARY_LINK;
        hash["unclassified"] = FunctionalRoadClass::UNCLASSIFIED;
        hash["residential"] = FunctionalRoadClass::RESIDENTIAL;
        hash["service"] = FunctionalRoadClass::SERVICE;
        hash["living_street"] = FunctionalRoadClass::LIVING_STREET;
        hash["track"] = FunctionalRoadClass::LOW_PRIORITY_ROAD;
        hash["road"] = FunctionalRoadClass::LOW_PRIORITY_ROAD;
        hash["path"] = FunctionalRoadClass::LOW_PRIORITY_ROAD;
        hash["driveway"] = FunctionalRoadClass::LOW_PRIORITY_ROAD;
        return hash;
    }();

    if (class_hash.find(value) != class_hash.end())
    {
        return class_hash.find(value)->second;
    }
    else
    {
        // TODO activate again, when road classes are moved to the profile
        // util::SimpleLogger().Write(logDEBUG) << "Unknown road class encountered: " << value;
        return FunctionalRoadClass::UNKNOWN;
    }
}

} // ns guidance
} // ns extractor
} // ns osrm
