#ifndef ROUTE_LEG_HPP
#define ROUTE_LEG_HPP

#include "engine/guidance/route_step.hpp"

#include <optional>

#include <string>
#include <vector>

namespace osrm::engine::guidance
{

struct RouteLeg
{
    double distance;
    double duration;
    double weight;
    std::string summary;
    std::vector<RouteStep> steps;
};
} // namespace osrm::engine::guidance

#endif
