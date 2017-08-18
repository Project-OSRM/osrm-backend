#ifndef OSRM_GEOJSON_VALIDATION_HPP
#define OSRM_GEOJSON_VALIDATION_HPP

#include "util/exception.hpp"
#include "util/log.hpp"

#include "rapidjson/document.h"

namespace osrm
{
namespace util
{

inline void validateCoordinate(const rapidjson::Value &coordinate)
{
    if (!coordinate.IsArray())
        throw osrm::util::exception("Feature geometry has a non-array coordinate.");
    if (coordinate.Capacity() != 2)
    {
        throw osrm::util::exception(
            "Feature geometry has a malformed coordinate with more than 2 values.");
    }
    else
    {
        for (rapidjson::SizeType i = 0; i < coordinate.Size(); i++)
        {
            if (!coordinate[i].IsNumber() && !coordinate[i].IsDouble())
                throw osrm::util::exception("Feature geometry has a non-number coordinate.");
        }
    }
}

inline void validateFeature(const rapidjson::Value &feature)
{
    if (!feature.HasMember("type"))
    {
        throw osrm::util::exception("Feature is missing type member.");
    }
    else if (!feature["type"].IsString())
    {
        throw osrm::util::exception("Feature has non-string type member.");
    }
    if (!feature.HasMember("properties"))
    {
        throw osrm::util::exception("Feature is missing properties member.");
    }
    else if (!feature.GetObject()["properties"].IsObject())
    {
        throw osrm::util::exception("Feature has non-object properties member.");
    }
    if (!feature.HasMember("geometry"))
    {
        throw osrm::util::exception("Feature is missing geometry member.");
    }
    else if (!feature.GetObject()["geometry"].IsObject())
    {
        throw osrm::util::exception("Feature non-object geometry member.");
    }

    if (!feature["geometry"].GetObject().HasMember("type"))
    {
        throw osrm::util::exception("Feature geometry is missing type member.");
    }
    else if (!feature["geometry"].GetObject()["type"].IsString())
    {
        throw osrm::util::exception("Feature geometry has non-string type member.");
    }
    if (!feature["geometry"].GetObject().HasMember("coordinates"))
    {
        throw osrm::util::exception("Feature geometry is missing coordinates member.");
    }
    else if (!feature["geometry"].GetObject()["coordinates"].IsArray())
    {
        throw osrm::util::exception("Feature geometry has a non-array coordinates member.");
    }
    const auto coord_array = feature["geometry"].GetObject()["coordinates"].GetArray();
    if (coord_array.Empty())
        throw osrm::util::exception("Feature geometry coordinates member is empty.");
}
}
}
#endif // OSRM_GEOJSON_VALIDATION_HPP
