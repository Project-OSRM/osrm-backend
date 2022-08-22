#ifndef ROUTE_LEG_HPP
#define ROUTE_LEG_HPP

#include "engine/guidance/route_step.hpp"

#include <boost/optional.hpp>

#include <string>
#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

struct RouteLeg
{
    double distance;
    double duration;
    double weight;
    std::string summary;
    std::vector<RouteStep> steps;
};
} // namespace guidance
} // namespace engine
} // namespace osrm

#endif
