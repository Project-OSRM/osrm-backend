#ifndef OSRM_ENGINE_GUIDANCE_RESULT_HPP_
#define OSRM_ENGINE_GUIDANCE_RESULT_HPP_

namespace osrm
{
namespace engine
{
namespace guidance
{
class Waypoint;
class Route;

struct Result
{
    std::vector<Waypoint> waypoints;
    std::vector<Route> routes;
};
}
}
}

#endif
