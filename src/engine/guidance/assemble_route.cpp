#include "engine/guidance/assemble_route.hpp"

#include <numeric>

namespace osrm
{
namespace engine
{
namespace guidance
{

Route assembleRoute(const std::vector<RouteLeg> &route_legs)
{
    auto distance = std::accumulate(
        route_legs.begin(), route_legs.end(), 0., [](const double sum, const RouteLeg &leg) {
            return sum + leg.distance;
        });
    auto duration = std::accumulate(
        route_legs.begin(), route_legs.end(), 0., [](const double sum, const RouteLeg &leg) {
            return sum + leg.duration;
        });

    return Route{duration, distance};
}

} // namespace guidance
} // namespace engine
} // namespace osrm
