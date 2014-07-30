/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
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

#include "ExtractorCallbacks.h"
#include "ExtractionContainers.h"
#include "ExtractionWay.h"
#include "ScriptingEnvironment.h"

#include "../DataStructures/Restriction.h"
#include "../DataStructures/ImportNode.h"
#include "../Util/SimpleLogger.h"

#include <osrm/Coordinate.h>

#include <limits>
#include <string>
#include <vector>

ExtractorCallbacks::ExtractorCallbacks(ExtractionContainers &extraction_containers,
                                       std::unordered_map<std::string, NodeID> &string_map)
    : string_map(string_map), external_memory(extraction_containers)
{
}

/** warning: caller needs to take care of synchronization! */

void ExtractorCallbacks::ProcessNode(const osmium::Node &current_node, ExtractionNode &result_node)
{
  ExternalMemoryNode node;
  node.bollard = result_node.barrier;
  node.trafficLight = result_node.traffic_lights;
  node.lat = current_node.location().lat()*COORDINATE_PRECISION;
  node.lon = current_node.location().lon()*COORDINATE_PRECISION;
  node.node_id = current_node.id();
  external_memory.all_nodes_list.push_back(node);
}

bool ExtractorCallbacks::ProcessRestriction(const InputRestrictionContainer &restriction)
{
    external_memory.restrictions_list.push_back(restriction);
    return true;
}

/** warning: caller needs to take care of synchronization! */
void ExtractorCallbacks::ProcessWay(const osmium::Way &current_way, ExtractionWay &parsed_way)
{
    SimpleLogger().Write() << "processing way with speed: " << parsed_way.speed << ", name: " << parsed_way.name;
    if ((0 >= parsed_way.speed) && (0 >= parsed_way.duration))
    { // Only true if the way is specified by the speed profile
        SimpleLogger().Write(logDEBUG) << "returning early, speed";
        return;
    }

    if (current_way.nodes().size() <= 1)
    { // safe-guard against broken data
        SimpleLogger().Write(logDEBUG) << "returning early, size";
        return;
    }

    if (std::numeric_limits<unsigned>::max() == current_way.id())
    {
        SimpleLogger().Write(logDEBUG) << "found bogus way with id: " << parsed_way.id
                                       << " of size " << current_way.nodes().size();
        return;
    }

    if (0 < parsed_way.duration)
    {
        // TODO: iterate all way segments and set duration corresponding to the length of each
        // segment
        parsed_way.speed = parsed_way.duration / (current_way.nodes().size() - 1);
    }

    if (std::numeric_limits<double>::epsilon() >= std::abs(-1. - parsed_way.speed))
    {
        SimpleLogger().Write(logDEBUG) << "found way with bogus speed, id: " << parsed_way.id;
        return;
    }

    // Get the unique identifier for the street name
    const auto &string_map_iterator = string_map.find(parsed_way.name);
    if (string_map.end() == string_map_iterator)
    {
        parsed_way.nameID = external_memory.name_list.size();
        external_memory.name_list.push_back(parsed_way.name);
        string_map.insert(std::make_pair(parsed_way.name, parsed_way.nameID));
        SimpleLogger().Write() << "inserted name " << parsed_way.name << " at " << parsed_way.nameID;
    }
    else
    {
        parsed_way.nameID = string_map_iterator->second;
    }

    if (ExtractionWay::opposite == parsed_way.direction)
    {
        // std::reverse(current_way.nodes().begin(), current_way.nodes().end());
        parsed_way.direction = ExtractionWay::oneway;
    }

    const bool split_edge =
        (parsed_way.backward_speed > 0) && (parsed_way.speed != parsed_way.backward_speed);

    for (unsigned n = 0; n < (current_way.nodes().size() - 1); ++n)
    {
        SimpleLogger().Write() << "adding edge (" << current_way.nodes()[n].ref() << "," << current_way.nodes()[n+1].ref() << ")";
        external_memory.all_edges_list.push_back(InternalExtractorEdge(
            current_way.nodes()[n].ref(),
            current_way.nodes()[n + 1].ref(),
            parsed_way.type,
            (split_edge ? ExtractionWay::oneway : parsed_way.direction),
            parsed_way.speed,
            parsed_way.nameID,
            parsed_way.roundabout,
            parsed_way.ignoreInGrid,
            (0 < parsed_way.duration),
            parsed_way.isAccessRestricted,
            false,
            split_edge));
        external_memory.used_node_id_list.push_back(current_way.nodes()[n].ref());
    }
    external_memory.used_node_id_list.push_back(current_way.nodes().back().ref());

    // The following information is needed to identify start and end segments of restrictions
    external_memory.way_start_end_id_list.push_back(
        WayIDStartAndEndEdge(parsed_way.id,
                             current_way.nodes()[0].ref(),
                             current_way.nodes()[1].ref(),
                             current_way.nodes()[current_way.nodes().size() - 2].ref(),
                             current_way.nodes().back().ref()));

    if (split_edge)
    { // Only true if the way should be split
        // std::reverse(current_way.nodes().begin(), current_way.nodes().end());
        for (std::vector<NodeID>::size_type n = 0; n < current_way.nodes().size() - 1; ++n)
        {
            external_memory.all_edges_list.push_back(
                InternalExtractorEdge(current_way.nodes()[n].ref(),
                                      current_way.nodes()[n + 1].ref(),
                                      parsed_way.type,
                                      ExtractionWay::oneway,
                                      parsed_way.backward_speed,
                                      parsed_way.nameID,
                                      parsed_way.roundabout,
                                      parsed_way.ignoreInGrid,
                                      (0 < parsed_way.duration),
                                      parsed_way.isAccessRestricted,
                                      (ExtractionWay::oneway == parsed_way.direction),
                                      split_edge));
        }
        external_memory.way_start_end_id_list.push_back(
            WayIDStartAndEndEdge(parsed_way.id,
                                 current_way.nodes()[0].ref(),
                                 current_way.nodes()[1].ref(),
                                 current_way.nodes()[current_way.nodes().size() - 2].ref(),
                                 current_way.nodes().back().ref()));
    }
}
