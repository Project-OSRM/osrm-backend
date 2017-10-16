#ifndef OSRM_ENGINE_API_WAYPOINT_HPP
#define OSRM_ENGINE_API_WAYPOINT_HPP

#include "util/coordinate.hpp"
#include "util/typedefs.hpp"

#include <array>

namespace osrm
{
namespace engine
{
namespace api
{
struct Waypoint
{
    double distance;
    const char *name;
    util::Coordinate location;
    std::array<OSMNodeID, 2> nodes = {{SPECIAL_OSM_NODEID, SPECIAL_OSM_NODEID}};
};
}
}
}

#endif
