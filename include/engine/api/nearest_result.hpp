#ifndef OSRM_ENGINE_API_NEAREST_RESULT_HPP
#define OSRM_ENGINE_API_NEAREST_RESULT_HPP

#include "engine/api/waypoint.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{

struct NearestResult
{
    std::vector<Waypoint> waypoints;
};
}
}
}

#endif
