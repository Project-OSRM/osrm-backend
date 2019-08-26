#ifndef OSRM_UNIT_TEST_WAYPOINT_CHECK
#define OSRM_UNIT_TEST_WAYPOINT_CHECK

#include "engine/api/flatbuffers/fbresult_generated.h"
#include "osrm/coordinate.hpp"
#include "osrm/json_container.hpp"
#include "util/exception.hpp"

using namespace osrm;

inline bool waypoint_check(json::Value waypoint)
{
    if (!waypoint.is<mapbox::util::recursive_wrapper<util::json::Object>>())
    {
        throw util::exception("Must pass in a waypoint object");
    }
    const auto waypoint_object = waypoint.get<json::Object>();
    const auto waypoint_location = waypoint_object.values.at("location").get<json::Array>().values;
    util::FloatLongitude lon{waypoint_location[0].get<json::Number>().value};
    util::FloatLatitude lat{waypoint_location[1].get<json::Number>().value};
    util::Coordinate location_coordinate(lon, lat);
    return location_coordinate.IsValid();
}

inline bool waypoint_check(const osrm::engine::api::fbresult::Waypoint *const waypoint)
{
    util::FloatLongitude lon{waypoint->location()->longitude()};
    util::FloatLatitude lat{waypoint->location()->latitude()};
    util::Coordinate location_coordinate(lon, lat);
    return location_coordinate.IsValid();
}

#endif
