#ifndef OSRM_ENGINE_GUIDANCE_DEBUG_HPP_
#define OSRM_ENGINE_GUIDANCE_DEBUG_HPP_

#include "engine/guidance/route_step.hpp"

#include <iostream>
#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{
inline void print(const RouteStep &step)
{
    std::cout << static_cast<int>(step.maneuver.instruction.type) << " "
              << static_cast<int>(step.maneuver.instruction.direction_modifier) << "  "
              << static_cast<int>(step.maneuver.waypoint_type) << " Duration: " << step.duration
              << " Distance: " << step.distance << " Geometry: " << step.geometry_begin << " "
              << step.geometry_end << " exit: " << step.maneuver.exit
              << " Intersections: " << step.intersections.size() << " [";

    for (const auto &intersection : step.intersections)
    {
        std::cout << "(bearings:";
        for (auto bearing : intersection.bearings)
            std::cout << " " << bearing;
        std::cout << ", entry: ";
        for (auto entry : intersection.entry)
            std::cout << " " << entry;
        std::cout << ")";
    }
    std::cout << "] name[" << step.name_id << "]: " << step.name;
}

inline void print(const std::vector<RouteStep> &steps)
{
    std::cout << "Path\n";
    int segment = 0;
    for (const auto &step : steps)
    {
        std::cout << "\t[" << segment++ << "]: ";
        print(step);
        std::cout << std::endl;
    }
}
} // namespace guidance
} // namespace engine
} // namespace osrm

#endif /*OSRM_ENGINE_GUIDANCE_DEBUG_HPP_*/
