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
#include "ExtractionNode.h"
#include "ExtractionWay.h"

#include "../DataStructures/Restriction.h"
#include "../DataStructures/ImportNode.h"
#include "../Util/container.hpp"
#include "../Util/simple_logger.hpp"

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
void ExtractorCallbacks::ProcessNode(const osmium::Node &osm_input_node, const ExtractionNode &result_node)
{
    //TODO: sort out datatype issues
    ExternalMemoryNode node;
    node.bollard = result_node.barrier;
    node.trafficLight = result_node.traffic_lights;
    node.lat = osm_input_node.location().lat() * COORDINATE_PRECISION;
    node.lon = osm_input_node.location().lon() * COORDINATE_PRECISION;
    node.node_id = osm_input_node.id();
    external_memory.all_nodes_list.push_back(node);
}

void ExtractorCallbacks::ProcessRestriction(const boost::optional<InputRestrictionContainer> &restriction)
{
    if (!restriction.is_initialized())
    {
      return;
    }
    external_memory.restrictions_list.push_back(restriction.get());
}
/** warning: caller needs to take care of synchronization! */
void ExtractorCallbacks::ProcessWay(const osmium::Way &current_way, ExtractionWay &parsed_way)
{
    if (((0 >= parsed_way.forward_speed) ||
            (TRAVEL_MODE_INACCESSIBLE == parsed_way.forward_travel_mode)) &&
        ((0 >= parsed_way.backward_speed) ||
            (TRAVEL_MODE_INACCESSIBLE == parsed_way.backward_travel_mode)) &&
        (0 >= parsed_way.duration))
    { // Only true if the way is specified by the speed profile
        return;
    }

    if (current_way.nodes().size() <= 1)
    { // safe-guard against broken data
        return;
    }

    if (std::numeric_limits<unsigned>::max() == current_way.id())
    {
        SimpleLogger().Write(logDEBUG) << "found bogus way with id: " << current_way.id()
                                       << " of size " << current_way.nodes().size();
        return;
    }

    if (0 < parsed_way.duration)
    {
        // TODO: iterate all way segments and set duration corresponding to the length of each
        // segment
        parsed_way.forward_speed = parsed_way.duration / (current_way.nodes().size() - 1);
        parsed_way.backward_speed = parsed_way.duration / (current_way.nodes().size() - 1);
    }

    if (std::numeric_limits<double>::epsilon() >= std::abs(-1. - parsed_way.forward_speed))
    {
        SimpleLogger().Write(logDEBUG) << "found way with bogus speed, id: " << current_way.id();
        return;
    }

    // Get the unique identifier for the street name
    const auto &string_map_iterator = string_map.find(parsed_way.name);
    unsigned name_id = external_memory.name_list.size();
    if (string_map.end() == string_map_iterator)
    {
        external_memory.name_list.push_back(parsed_way.name);
        string_map.insert(std::make_pair(parsed_way.name, name_id));
    }
    else
    {
        name_id = string_map_iterator->second;
    }

    const bool split_edge =
      (parsed_way.forward_speed>0) && (TRAVEL_MODE_INACCESSIBLE != parsed_way.forward_travel_mode) &&
      (parsed_way.backward_speed>0) && (TRAVEL_MODE_INACCESSIBLE != parsed_way.backward_travel_mode) &&
      ((parsed_way.forward_speed != parsed_way.backward_speed) ||
      (parsed_way.forward_travel_mode != parsed_way.backward_travel_mode));

    auto pair_wise_segment_split = [&](const osmium::NodeRef &first_node,
                                       const osmium::NodeRef &last_node) {
        // SimpleLogger().Write() << "adding edge (" << first_node.ref() << "," <<
        // last_node.ref() << "), speed: " << parsed_way.speed;
        external_memory.all_edges_list.push_back(
            InternalExtractorEdge(first_node.ref(),
                                  last_node.ref(),
            ((split_edge || TRAVEL_MODE_INACCESSIBLE == parsed_way.backward_travel_mode) ? ExtractionWay::oneway
                                                                                 : ExtractionWay::bidirectional),
            parsed_way.forward_speed,
            name_id,
            parsed_way.roundabout,
            parsed_way.ignore_in_grid,
            (0 < parsed_way.duration),
            parsed_way.is_access_restricted,
            parsed_way.forward_travel_mode,
            split_edge));
        external_memory.used_node_id_list.push_back(first_node.ref());
    };

    const bool is_opposite_way = TRAVEL_MODE_INACCESSIBLE == parsed_way.forward_travel_mode;
    if (is_opposite_way)
    {
        parsed_way.forward_travel_mode = parsed_way.backward_travel_mode;
        parsed_way.backward_travel_mode = TRAVEL_MODE_INACCESSIBLE;
        osrm::for_each_pair(current_way.nodes().crbegin(), current_way.nodes().crend(), pair_wise_segment_split);
        external_memory.used_node_id_list.push_back(current_way.nodes().front().ref());
    }
    else
    {
        osrm::for_each_pair(current_way.nodes().cbegin(), current_way.nodes().cend(), pair_wise_segment_split);
        external_memory.used_node_id_list.push_back(current_way.nodes().back().ref());
    }

    // The following information is needed to identify start and end segments of restrictions
     // The following information is needed to identify start and end segments of restrictions
    external_memory.way_start_end_id_list.push_back(
        {(EdgeID)current_way.id(),
         (NodeID)current_way.nodes()[0].ref(),
         (NodeID)current_way.nodes()[1].ref(),
         (NodeID)current_way.nodes()[current_way.nodes().size() - 2].ref(),
         (NodeID)current_way.nodes().back().ref()});

    if (split_edge)
    { // Only true if the way should be split
        BOOST_ASSERT(parsed_way.backward_travel_mode>0);
        auto pair_wise_segment_split_2 = [&](const osmium::NodeRef &first_node,
                                             const osmium::NodeRef &last_node)
        {
            // SimpleLogger().Write() << "adding edge (" << last_node.ref() << "," <<
            // first_node.ref() << "), speed: " << parsed_way.backward_speed;
            external_memory.all_edges_list.push_back(
                InternalExtractorEdge(last_node.ref(),
                                      first_node.ref(),
                                      ExtractionWay::oneway,
                                      parsed_way.backward_speed,
                                      name_id,
                                      parsed_way.roundabout,
                                      parsed_way.ignore_in_grid,
                                      (0 < parsed_way.duration),
                                      parsed_way.is_access_restricted,
                                      parsed_way.backward_travel_mode,
                                      split_edge));
        };

        if (is_opposite_way)
        {
            // SimpleLogger().Write() << "opposite2";
            osrm::for_each_pair(current_way.nodes().crbegin(), current_way.nodes().crend(), pair_wise_segment_split_2);
            external_memory.used_node_id_list.push_back(current_way.nodes().front().ref());
        }
        else
        {
            osrm::for_each_pair(current_way.nodes().cbegin(), current_way.nodes().cend(), pair_wise_segment_split_2);
            external_memory.used_node_id_list.push_back(current_way.nodes().back().ref());
        }


        external_memory.way_start_end_id_list.push_back(
            {(EdgeID)current_way.id(),
             (NodeID)current_way.nodes()[1].ref(),
             (NodeID)current_way.nodes()[0].ref(),
             (NodeID)current_way.nodes().back().ref(),
             (NodeID)current_way.nodes()[current_way.nodes().size() - 2].ref()});
    }
}
