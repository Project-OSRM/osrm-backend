#ifndef OSRM_UTIL_DEBUG_HPP_
#define OSRM_UTIL_DEBUG_HPP_

#include "extractor/edge_based_edge.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/query_node.hpp"
#include "guidance/intersection.hpp"
#include "guidance/turn_instruction.hpp"
#include "guidance/turn_lane_data.hpp"
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
inline std::ostream &operator<<(std::ostream &out, const Coordinate &coordinate)
{
    out << std::setprecision(12) << "{" << toFloating(coordinate.lon) << ", "
        << toFloating(coordinate.lat) << "}";
    return out;
}
}

namespace engine
{
namespace guidance
{
inline std::ostream &operator<<(std::ostream &out, const RouteStep &step)
{
    out << "RouteStep {" << static_cast<int>(step.maneuver.instruction.type) << " "
        << static_cast<int>(step.maneuver.instruction.direction_modifier) << "  "
        << static_cast<int>(step.maneuver.waypoint_type) << " " << step.maneuver.location << " "
        << " Duration: " << step.duration << " Distance: " << step.distance
        << " Geometry: " << step.geometry_begin << " " << step.geometry_end
        << " Exit: " << step.maneuver.exit << " Mode: " << (int)step.mode
        << "\n\tIntersections: " << step.intersections.size() << " [";

    for (const auto &intersection : step.intersections)
    {
        out << "(Lanes: " << static_cast<int>(intersection.lanes.lanes_in_turn) << " "
            << static_cast<int>(intersection.lanes.first_lane_from_the_right) << " ["
            << intersection.in << "," << intersection.out << "]"
            << " bearings:";
        for (auto bearing : intersection.bearings)
            out << " " << bearing;
        out << ", entry: ";
        for (auto entry : intersection.entry)
            out << " " << (entry ? "true" : "false");
        out << ")";
    }
    out << "] name[" << step.name_id << "]: " << step.name << " Ref: " << step.ref
        << " Pronunciation: " << step.pronunciation << "Destination: " << step.destinations
        << "Exits: " << step.exits << "}";

    return out;
}
}
}

namespace guidance
{
inline std::ostream &operator<<(std::ostream &out, const ConnectedRoad &road)
{
    out << "ConnectedRoad {" << road.eid << " allows entry: " << road.entry_allowed
        << " angle: " << road.angle << " bearing: " << road.perceived_bearing
        << " instruction: " << static_cast<std::int32_t>(road.instruction.type) << " "
        << static_cast<std::int32_t>(road.instruction.direction_modifier) << " "
        << static_cast<std::int32_t>(road.lane_data_id) << "}";
    return out;
}
}

namespace extractor
{
namespace intersection
{
inline std::ostream &operator<<(std::ostream &out, const IntersectionEdgeGeometry &shape)
{
    out << "IntersectionEdgeGeometry { " << shape.eid << " bearing: " << shape.perceived_bearing
        << "}";
    return out;
}

inline std::ostream &operator<<(std::ostream &out, const IntersectionViewData &view)
{
    out << "IntersectionViewData {" << view.eid << " allows entry: " << view.entry_allowed
        << " angle: " << view.angle << " bearing: " << view.perceived_bearing << "}";
    return out;
}
}

namespace TurnLaneType
{
inline std::ostream &operator<<(std::ostream &out, const Mask lane_type)
{
    if (lane_type == 0)
    {
        out << "none";
        return out;
    }

    bool first = true;
    std::bitset<8 * sizeof(Mask)> mask(lane_type);
    for (auto index : util::irange<std::size_t>(0, NUM_TYPES))
    {
        if (!first)
        {
            out << ";";
        }

        if (mask[index])
        {
            out << laneTypeToName(index);
            first = false;
        }
    }

    return out;
}
}
}
}

namespace std
{
inline std::ostream &operator<<(std::ostream &out,
                                const std::vector<osrm::engine::guidance::RouteStep> &steps)
{
    out << "{";
    int segment = 0;
    for (const auto &step : steps)
    {
        if (segment > 0)
            out << ", ";
        out << step;
        segment++;
    }
    out << "}";

    return out;
}
}

namespace osrm
{
namespace guidance
{
inline std::ostream &operator<<(std::ostream &out, const Intersection &intersection)
{
    out << "Intersection {";
    int segment = 0;
    for (const auto &road : intersection)
    {
        if (segment > 0)
            out << ", ";
        out << road;
        segment++;
    }
    out << "}";
    return out;
}

namespace lanes
{
inline std::ostream &operator<<(std::ostream &out, const LaneDataVector &turn_lane_data)
{
    out << " LaneDataVector {";
    int segment = 0;
    for (auto entry : turn_lane_data)
    {
        if (segment > 0)
            out << ", ";
        out << entry.tag << "(" << entry.tag << ") from: " << static_cast<int>(entry.from)
            << " to: " << static_cast<int>(entry.to);
        segment++;
    }
    out << "}";

    return out;
}
}
}

namespace extractor
{
inline std::ostream &operator<<(std::ostream &out, const EdgeBasedEdge &edge)
{
    out << " EdgeBasedEdge {";
    out << " source " << edge.source << ", target: " << edge.target;
    out << " EdgeBasedEdgeData data {";
    out << " turn_id: " << edge.data.turn_id << ", weight: " << edge.data.weight;
    out << " distance: " << edge.data.distance << ", duration: " << edge.data.duration;
    out << " forward: " << (edge.data.forward == 0 ? "false" : "true")
        << ", backward: " << (edge.data.backward == 0 ? "false" : "true");
    out << " }";
    out << "}";
    return out;
}
}
}

#endif /*OSRM_ENGINE_GUIDANCE_DEBUG_HPP_*/
