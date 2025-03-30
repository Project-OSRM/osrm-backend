/*

Copyright (c) 2025, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef OSRM_EXTRACTOR_OBSTACLES_DATA_HPP_
#define OSRM_EXTRACTOR_OBSTACLES_DATA_HPP_

#include "util/typedefs.hpp"

#include <osmium/osm/node.hpp>
#include <tbb/concurrent_vector.h>

#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

namespace osrm::extractor
{
// A class that represents an obstacle on the road or a place where you can turn around.
//
// This may be a completely impassable obstacle like a barrier, a temporary obstacle
// like a traffic light or a stop sign, or an obstacle that just slows you down like a
// traffic calming bump.  The obstacle may be present in both directions or in one
// direction only.
//
// This also represents a good turning point like a mini_roundabout, turning_loop, or
// turning_circle.
//
// Note: A better name would have been 'WayPoint', but that name is ambiguous in the
// OSRM-context.
//
struct Obstacle
{
    // The type of an obstacle
    // Note: must be kept in sync with the initializer_list in obstacles.cpp
    enum class Type : uint16_t
    {
        None = 0x0000,
        Barrier = 0x0001,
        TrafficSignals = 0x0002,
        Stop = 0x0004,
        GiveWay = 0x0008,
        Crossing = 0x0010,
        TrafficCalming = 0x0020,
        MiniRoundabout = 0x0040,
        TurningLoop = 0x0080,
        TurningCircle = 0x0100,
        StopMinor = 0x0200,

        Turning = MiniRoundabout | TurningLoop | TurningCircle,
        Incompressible = Barrier | Turning,
        All = 0xFFFF
    };

    static const std::initializer_list<std::pair<std::string_view, Type>>
        enum_type_initializer_list;

    // The directions in which an obstacle applies.
    // Note: must be kept in sync with the initializer_list in obstacles.cpp
    enum class Direction : uint8_t
    {
        None = 0x0,
        Forward = 0x1,
        Backward = 0x2,
        Both = 0x3
    };

    static const std::initializer_list<std::pair<std::string_view, Direction>>
        enum_direction_initializer_list;

    // use overloading instead of default arguments for sol::constructors
    Obstacle(Type t_type) : type{t_type} {};
    Obstacle(Type t_type, Direction t_direction) : type{t_type}, direction{t_direction} {};
    Obstacle(Type t_type, Direction t_direction, float t_duration_penalty, float t_weight_penalty)
        : type{t_type}, direction{t_direction}, duration{t_duration_penalty},
          weight{t_weight_penalty} {};

    Type type;
    Direction direction{Direction::None};
    float duration{0}; // in seconds
    float weight{0};   // unit: profile.weight_multiplier
};

// A class that holds all known nodes with obstacles.
//
// This class holds all obstacles, bidirectional and unidirectional ones.  For
// unidirectional obstacle nodes it also stores the node leading to it.
//
// Notes:
//
// Before fixupNodes() it uses the external node id (aka. OSMNodeID).
// After fixupNodes() it uses the internal node id (aka. NodeID).
//
class ObstacleMap
{
    using NodeIDVector = std::vector<OSMNodeID>;
    using WayNodeIDOffsets = std::vector<size_t>;

    using OsmFromToObstacle = std::tuple<OSMNodeID, OSMNodeID, Obstacle>;
    using FromToObstacle = std::tuple<NodeID, NodeID, Obstacle>;

  public:
    ObstacleMap() = default;

    // Insert an obstacle using the OSM node id.
    //
    // This function is thread-safe.
    void emplace(OSMNodeID osm_node_id, const Obstacle &obstacle)
    {
        osm_obstacles.emplace_back(SPECIAL_OSM_NODEID, osm_node_id, obstacle);
    };

    // Insert an obstacle using internal node ids.
    //
    // The obstacle is at 'to'. For bidirectional obstacles set 'from' to SPECIAL_NODEID.
    // Convenient for testing.
    void emplace(NodeID from, NodeID to, const Obstacle &obstacle)
    {
        obstacles.emplace(to, std::make_tuple(from, to, obstacle));
    };

    // get all obstacles at node 'to' when coming from node 'from'
    // pass SPECIAL_NODEID as 'from' to get all obstacles at 'to'
    // 'type' can be a bitwise-or combination of Obstacle::Type
    std::vector<Obstacle>
    get(NodeID from, NodeID to, Obstacle::Type type = Obstacle::Type::All) const
    {
        std::vector<Obstacle> result;

        auto [begin, end] = obstacles.equal_range(to);
        for (auto &[key, value] : std::ranges::subrange(begin, end))
        {
            auto &[from_id, to_id, obstacle] = value;
            if ((from_id == SPECIAL_NODEID || from_id == from) &&
                (static_cast<uint16_t>(obstacle.type) & static_cast<uint16_t>(type)))
            {
                result.push_back(obstacle);
            }
        }
        return result; // vector has move semantics
    }

    std::vector<Obstacle> get(NodeID to) const
    {
        return get(SPECIAL_NODEID, to, Obstacle::Type::All);
    }

    // is there any obstacle at node 'to'?
    // inexpensive general test
    bool any(NodeID to) const { return obstacles.contains(to); }

    // is there any obstacle of type 'type' at node 'to' when coming from node 'from'?
    // pass SPECIAL_NODEID as 'from' to query all obstacles at 'to'
    // 'type' can be a bitwise-or combination of Obstacle::Type
    bool any(NodeID from, NodeID to, Obstacle::Type type = Obstacle::Type::All) const
    {
        return any(to) && !get(from, to, type).empty();
    }

    // Preprocess the obstacles
    //
    // For all unidirectional obstacles find the node that leads up to them.  This
    // function must be called while the vector 'node_ids' is still unsorted /
    // uncompressed.
    void preProcess(const NodeIDVector &, const WayNodeIDOffsets &);

    // Convert all external OSMNodeIDs into internal NodeIDs.
    //
    // This function must be called when the vector 'node_ids' is already sorted and
    // compressed.  Call this function once only.
    void fixupNodes(const NodeIDVector &);

    // Replace a leading node that is going to be deleted during graph
    // compression with the leading node of the leading node.
    void compress(NodeID from, NodeID delendus, NodeID to);

  private:
    // obstacles according to external id
    tbb::concurrent_vector<OsmFromToObstacle> osm_obstacles;

    // obstacles according to internal id, with a leading node if unidirectional
    // bidirectional obstacles are stored with a leading node id == SPECIAL_NODE_ID
    std::unordered_multimap<NodeID, FromToObstacle> obstacles;
};

} // namespace osrm::extractor
#endif // OSRM_EXTRACTOR_OBSTACLES_DATA_HPP_
