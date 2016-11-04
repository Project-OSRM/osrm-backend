#include "engine/guidance/assemble_route.hpp"

#include <numeric>

namespace osrm
{
namespace engine
{
namespace guidance
{

Route assembleRoute(std::vector<RouteLeg> route_legs)
{
    Route route;

    route.legs = std::move(route_legs);
    route.distance = std::accumulate(
        route.legs.begin(), route.legs.end(), 0., [](const double sum, const RouteLeg &leg) {
            return sum + leg.distance;
        });
    route.duration = std::accumulate(
        route.legs.begin(), route.legs.end(), 0., [](const double sum, const RouteLeg &leg) {
            return sum + leg.duration;
        });

    return route;
}

} // namespace guidance
} // namespace engine
} // namespace osrm
