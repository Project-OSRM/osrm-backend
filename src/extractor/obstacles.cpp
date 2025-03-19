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
                                         {"turning_circle", Obstacle::Type::TurningCircle}};

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

    // build a map to speed up the next step
    // multimap of OSMNodeId to index into way_node_offsets
    std::unordered_multimap<OSMNodeID, size_t> node2start_index;

    for (size_t i = 0, j = 1; j < way_node_offsets.size(); ++i, ++j)
    {
        for (size_t k = way_node_offsets[i]; k < way_node_offsets[j]; ++k)
        {
            node2start_index.emplace(node_ids[k], i);
        }
    }

    // for each unidirectional obstacle
    //      for each way that crosses the obstacle
    //          for each node of the crossing way
    //              find the node immediately before or after the obstacle
    //              add the unidirectional obstacle
    //              note that we don't remove the bidirectional obstacle here,
    //              but in fixupNodes()

    for (auto &[from_id, to_id, obstacle] : osm_obstacles)
    {
        if (obstacle.direction == Obstacle::Direction::Forward ||
            obstacle.direction == Obstacle::Direction::Backward)
        {
            bool forward = obstacle.direction == Obstacle::Direction::Forward;
            auto [wno_begin, wno_end] = node2start_index.equal_range(to_id);
            // for each way that crosses the obstacle
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
                    osm_obstacles.emplace_back(*(node_iter + (forward ? -1 : 1)), to_id, obstacle);
                }
            }
        }
    }

    TIMER_STOP(preProcess);
    log << "ok, after " << TIMER_SEC(preProcess) << "s";
}

void ObstacleMap::fixupNodes(const NodeIDVector &node_ids)
{
    const auto begin = node_ids.cbegin();
    const auto end = node_ids.cend();

    auto osm_to_internal = [&](const OSMNodeID &osm_node) -> NodeID
    {
        if (osm_node == SPECIAL_OSM_NODEID)
        {
            return SPECIAL_NODEID;
        }
        const auto it = std::lower_bound(begin, end, osm_node);
        return (it == end || osm_node < *it) ? SPECIAL_NODEID
                                             : static_cast<NodeID>(std::distance(begin, it));
    };

    for (const auto &[osm_from, osm_to, obstacle] : osm_obstacles)
    {
        if ((obstacle.direction == Obstacle::Direction::Forward ||
             obstacle.direction == Obstacle::Direction::Backward) &&
            osm_from == SPECIAL_OSM_NODEID)
            // drop these bidirectional entries because we have generated an
            // unidirectional copy of them
            continue;
        emplace(osm_to_internal(osm_from), osm_to_internal(osm_to), obstacle);
    }
    osm_obstacles.clear();
}

void ObstacleMap::compress(NodeID node1, NodeID delendus, NodeID node2)
{
    auto comp = [this, delendus](NodeID first, NodeID last)
    {
        const auto &[begin, end] = obstacles.equal_range(last);
        for (auto i = begin; i != end; ++i)
        {
            auto &[from, to, obstacle] = i->second;
            if (from == delendus)
                from = first;
        }
    };

    comp(node1, node2);
    comp(node2, node1);
}

} // namespace osrm::extractor
