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

#include "extractor/obstacles.hpp"

#include "util/log.hpp"
#include "util/timing_util.hpp"

#include <unordered_set>

namespace osrm::extractor
{

const std::initializer_list<std::pair<std::string_view, Obstacle::Type>>
    Obstacle::enum_type_initializer_list{{"none", Obstacle::Type::None},
                                         {"barrier", Obstacle::Type::Barrier},
                                         {"traffic_signals", Obstacle::Type::TrafficSignals},
                                         {"stop", Obstacle::Type::Stop},
                                         {"stop_minor", Obstacle::Type::StopMinor},
                                         {"give_way", Obstacle::Type::GiveWay},
                                         {"crossing", Obstacle::Type::Crossing},
                                         {"traffic_calming", Obstacle::Type::TrafficCalming},
                                         {"mini_roundabout", Obstacle::Type::MiniRoundabout},
                                         {"turning_loop", Obstacle::Type::TurningLoop},
                                         {"turning_circle", Obstacle::Type::TurningCircle},
                                         {"gate", Obstacle::Type::Gate}};

const std::initializer_list<std::pair<std::string_view, Obstacle::Direction>>
    Obstacle::enum_direction_initializer_list{{"none", Obstacle::Direction::None},
                                              {"forward", Obstacle::Direction::Forward},
                                              {"backward", Obstacle::Direction::Backward},
                                              {"both", Obstacle::Direction::Both}};

void ObstacleMap::preProcess(const NodeIDVector &node_ids, const WayNodeIDOffsets &way_node_offsets)
{
    util::UnbufferedLog log;
    log << "Collecting information on " << osm_obstacles.size() << " obstacles...";
    TIMER_START(preProcess);

    // collect the node IDs of unidirectional obstacles
    std::unordered_set<OSMNodeID> directional_obstacle_nodes;
    for (const auto &[from_id, to_id, obstacle] : osm_obstacles)
    {
        if (obstacle.direction == Obstacle::Direction::Forward ||
            obstacle.direction == Obstacle::Direction::Backward)
        {
            directional_obstacle_nodes.insert(to_id);
        }
    }

    if (!directional_obstacle_nodes.empty())
    {
        // build a multimap of OSMNodeId to way index, but only for obstacle nodes
        std::unordered_multimap<OSMNodeID, size_t> node2start_index;

        for (size_t i = 0, j = 1; j < way_node_offsets.size(); ++i, ++j)
        {
            for (size_t k = way_node_offsets[i]; k < way_node_offsets[j]; ++k)
            {
                if (directional_obstacle_nodes.contains(node_ids[k]))
                {
                    node2start_index.emplace(node_ids[k], i);
                }
            }
        }

        // For each unidirectional obstacle, find the node immediately before
        // or after the obstacle on each way that crosses it.  Collect new
        // entries separately to avoid mutating osm_obstacles during iteration.
        std::vector<OsmFromToObstacle> new_entries;

        for (const auto &[from_id, to_id, obstacle] : osm_obstacles)
        {
            if (obstacle.direction == Obstacle::Direction::Forward ||
                obstacle.direction == Obstacle::Direction::Backward)
            {
                bool forward = obstacle.direction == Obstacle::Direction::Forward;
                auto [wno_begin, wno_end] = node2start_index.equal_range(to_id);
                for (auto wno_iter = wno_begin; wno_iter != wno_end; ++wno_iter)
                {
                    using NodeIdIterator = NodeIDVector::const_iterator;

                    NodeIdIterator begin = node_ids.cbegin() + way_node_offsets[wno_iter->second];
                    NodeIdIterator end = node_ids.cbegin() + way_node_offsets[wno_iter->second + 1];
                    if (forward)
                        ++begin;
                    else
                        --end;

                    NodeIdIterator node_iter = find(begin, end, to_id);
                    if (node_iter != end)
                    {
                        new_entries.emplace_back(
                            *(node_iter + (forward ? -1 : 1)), to_id, obstacle);
                    }
                }
            }
        }

        for (auto &entry : new_entries)
            osm_obstacles.push_back(std::move(entry));
    }

    TIMER_STOP(preProcess);
    log << "ok, after " << TIMER_SEC(preProcess) << "s";
}

void ObstacleMap::fixupNodes(const NodeIDVector &node_ids)
{
    util::UnbufferedLog log;
    log << "Renumbering " << osm_obstacles.size() << " obstacles...";
    TIMER_START(obstacle_renumbering);

    const auto should_skip = [](const auto &osm_from, const auto &, const auto &obstacle)
    {
        return (obstacle.direction == Obstacle::Direction::Forward ||
                obstacle.direction == Obstacle::Direction::Backward) &&
               osm_from == SPECIAL_OSM_NODEID;
    };

    std::vector<OSMNodeID> used_osm_node_ids;
    used_osm_node_ids.reserve(osm_obstacles.size() * 2);

    for (const auto &[osm_from, osm_to, obstacle] : osm_obstacles)
    {
        if (should_skip(osm_from, osm_to, obstacle))
            continue;

        if (osm_from != SPECIAL_OSM_NODEID)
            used_osm_node_ids.push_back(osm_from);

        if (osm_to != SPECIAL_OSM_NODEID)
            used_osm_node_ids.push_back(osm_to);
    }

    std::sort(used_osm_node_ids.begin(), used_osm_node_ids.end());
    used_osm_node_ids.erase(std::unique(used_osm_node_ids.begin(), used_osm_node_ids.end()),
                            used_osm_node_ids.end());

    std::vector<NodeID> internal_node_ids(used_osm_node_ids.size(), SPECIAL_NODEID);

    std::size_t node_index = 0;
    std::size_t used_index = 0;

    while (node_index < node_ids.size() && used_index < used_osm_node_ids.size())
    {
        const auto node_id = node_ids[node_index];
        const auto used_id = used_osm_node_ids[used_index];

        if (node_id < used_id)
        {
            ++node_index;
        }
        else if (used_id < node_id)
        {
            ++used_index;
        }
        else
        {
            internal_node_ids[used_index] = static_cast<NodeID>(node_index);
            ++used_index;
        }
    }

    auto osm_to_internal = [&](const OSMNodeID osm_node) -> NodeID
    {
        if (osm_node == SPECIAL_OSM_NODEID)
            return SPECIAL_NODEID;

        const auto it =
            std::lower_bound(used_osm_node_ids.begin(), used_osm_node_ids.end(), osm_node);

        if (it == used_osm_node_ids.end() || *it != osm_node)
            return SPECIAL_NODEID;

        return internal_node_ids[static_cast<std::size_t>(
            std::distance(used_osm_node_ids.begin(), it))];
    };

    obstacles.reserve(osm_obstacles.size());

    for (const auto &[osm_from, osm_to, obstacle] : osm_obstacles)
    {
        if (should_skip(osm_from, osm_to, obstacle))
            continue;

        const auto from = osm_to_internal(osm_from);
        const auto to = osm_to_internal(osm_to);

        obstacles.push_back({to, from, obstacle});
    }

    osm_obstacles.clear();
    sort();
    TIMER_STOP(obstacle_renumbering);
    log << "ok, after " << TIMER_SEC(obstacle_renumbering) << "s";
}

void ObstacleMap::sort()
{
    std::sort(obstacles.begin(), obstacles.end());
    sorted = true;
}

void ObstacleMap::compress(const NodeID node1, const NodeID delendus, const NodeID node2)
{
    auto comp = [this, delendus](const NodeID first, const NodeID last)
    {
        auto [begin, end] = find_range(last);
        for (auto it = begin; it != end; ++it)
        {
            if (it->from == delendus)
                it->from = first;
        }
    };

    comp(node1, node2);
    comp(node2, node1);
}

} // namespace osrm::extractor
