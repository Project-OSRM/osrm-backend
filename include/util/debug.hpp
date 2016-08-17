#ifndef OSRM_ENGINE_GUIDANCE_DEBUG_HPP_
#define OSRM_ENGINE_GUIDANCE_DEBUG_HPP_

#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/turn_lane_data.hpp"
#include "extractor/query_node.hpp"
#include "engine/guidance/route_step.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace osrm
{
namespace util
{

namespace guidance
{
inline void print(const engine::guidance::RouteStep &step)
{
    std::cout << static_cast<int>(step.maneuver.instruction.type) << " "
              << static_cast<int>(step.maneuver.instruction.direction_modifier) << "  "
              << static_cast<int>(step.maneuver.waypoint_type) << " "
              << " Duration: " << step.duration << " Distance: " << step.distance
              << " Geometry: " << step.geometry_begin << " " << step.geometry_end
              << "\n\tIntersections: " << step.intersections.size() << " [";

    for (const auto &intersection : step.intersections)
    {
        std::cout << "(Lanes: " << static_cast<int>(intersection.lanes.lanes_in_turn) << " "
                  << static_cast<int>(intersection.lanes.first_lane_from_the_right) << " ["
                  << intersection.in << "," << intersection.out << "]"
                  << " bearings:";
        for (auto bearing : intersection.bearings)
            std::cout << " " << bearing;
        std::cout << ", entry: ";
        for (auto entry : intersection.entry)
            std::cout << " " << (entry ? "true" : "false");
        std::cout << ")";
    }
    std::cout << "] name[" << step.name_id << "]: " << step.name;
}

inline void print(const std::vector<engine::guidance::RouteStep> &steps)
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

inline void print(const extractor::guidance::Intersection &intersection)
{
    std::cout << "  Intersection:\n";
    for (const auto &road : intersection)
        std::cout << "\t" << toString(road) << "\n";
    std::cout << std::flush;
}

inline void print(const NodeBasedDynamicGraph &node_based_graph,
                  const extractor::guidance::Intersection &intersection)
{
    std::cout << "  Intersection:\n";
    for (const auto &road : intersection)
    {
        std::cout << "\t" << toString(road) << "\n";
        std::cout << "\t\t"
                  << node_based_graph.GetEdgeData(road.turn.eid).road_classification.ToString()
                  << "\n";
    }
    std::cout << std::flush;
}

inline void print(const extractor::guidance::lanes::LaneDataVector &turn_lane_data)
{
    std::cout << " Tags:\n";
    for (auto entry : turn_lane_data)
        std::cout << "\t" << entry.tag << "("
                  << extractor::guidance::TurnLaneType::toString(entry.tag)
                  << ") from: " << static_cast<int>(entry.from)
                  << " to: " << static_cast<int>(entry.to)
                  << " Can Be Suppresssed: " << (entry.suppress_assignment ? "true" : "false")
                  << "\n";
    std::cout << std::flush;
}

inline void
printTurnAssignmentData(const NodeID at,
                        const extractor::guidance::lanes::LaneDataVector &turn_lane_data,
                        const extractor::guidance::Intersection &intersection,
                        const std::vector<extractor::QueryNode> &node_info_list)
{
    std::cout << "[Turn Assignment Progress]\nLocation:";
    auto coordinate = node_info_list[at];
    std::cout << std::setprecision(12) << toFloating(coordinate.lat) << " "
              << toFloating(coordinate.lon) << "\n";

    print(intersection);
    // flushes as well
    print(turn_lane_data);
}

} // namespace guidance
} // namespace util
} // namespace osrm

#endif /*OSRM_ENGINE_GUIDANCE_DEBUG_HPP_*/
