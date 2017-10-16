#ifndef OSRM_ENGINE_API_WAYPOINT_HPP
#define OSRM_ENGINE_API_WAYPOINT_HPP

#include "util/coordinate.hpp"

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
};
}
}
}

#endif
