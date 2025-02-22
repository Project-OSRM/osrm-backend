#ifndef OSRM_UNIT_TEST_WAYPOINT_CHECK
#define OSRM_UNIT_TEST_WAYPOINT_CHECK

#include "engine/api/flatbuffers/fbresult_generated.h"
#include "osrm/coordinate.hpp"
#include "osrm/json_container.hpp"
#include "util/exception.hpp"
#include <variant>

inline bool waypoint_check(osrm::json::Value waypoint)
{
    using namespace osrm;

    if (!std::holds_alternative<util::json::Object>(waypoint))
    {
        throw util::exception("Must pass in a waypoint object");
    }
    const auto waypoint_object = std::get<json::Object>(waypoint);
    const auto waypoint_location =
        std::get<json::Array>(waypoint_object.values.at("location")).values;
    util::FloatLongitude lon{std::get<json::Number>(waypoint_location[0]).value};
    util::FloatLatitude lat{std::get<json::Number>(waypoint_location[1]).value};
    util::Coordinate location_coordinate(lon, lat);
    return location_coordinate.IsValid();
}

inline bool waypoint_check(const osrm::engine::api::fbresult::Waypoint *const waypoint)
{
    using namespace osrm;

    util::FloatLongitude lon{waypoint->location()->longitude()};
    util::FloatLatitude lat{waypoint->location()->latitude()};
    util::Coordinate location_coordinate(lon, lat);
    return location_coordinate.IsValid();
}

#endif
