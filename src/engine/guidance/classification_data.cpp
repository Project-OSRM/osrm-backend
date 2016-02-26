#include "engine/guidance/classification_data.hpp"

#include <osmium/osm.hpp>

#include <iostream>

namespace osrm
{
namespace engine
{
namespace guidance
{
void RoadClassificationData::invalidate() { road_class = FunctionalRoadClass::UNKNOWN; }

void RoadClassificationData::augment(const osmium::Way &way)
{
    const char *data = way.get_value_by_key("highway");
    if (data)
        road_class = functionalRoadClassFromTag(data);
}
} // namespace guidance
} // namespace engine
} // namespace osrm
