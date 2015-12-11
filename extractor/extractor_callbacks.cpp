/*

Copyright (c) 2015, Project OSRM contributors
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

#include "extractor_callbacks.hpp"
#include "extraction_containers.hpp"
#include "extraction_node.hpp"
#include "extraction_way.hpp"

#include "../data_structures/external_memory_node.hpp"
#include "../data_structures/restriction.hpp"
#include "../util/container.hpp"
#include "../util/simple_logger.hpp"

#include <boost/optional/optional.hpp>

#include <osmium/osm.hpp>

#include <osrm/coordinate.hpp>

#include <limits>
#include <string>
#include <vector>

ExtractorCallbacks::ExtractorCallbacks(ExtractionContainers &extraction_containers)
    : external_memory(extraction_containers)
{
    string_map[""] = 0;
}

/**
 * Takes the node position from osmium and the filtered properties from the lua
 * profile and saves them to external memory.
 *
 * warning: caller needs to take care of synchronization!
 */
void ExtractorCallbacks::ProcessNode(const osmium::Node &input_node,
                                     const ExtractionNode &result_node)
{
    external_memory.all_nodes_list.push_back(
        {static_cast<int>(input_node.location().lat() * COORDINATE_PRECISION),
         static_cast<int>(input_node.location().lon() * COORDINATE_PRECISION),
         OSMNodeID(input_node.id()),
         result_node.barrier,
         result_node.traffic_lights});
}

void ExtractorCallbacks::ProcessRestriction(
    const boost::optional<InputRestrictionContainer> &restriction)
{
    if (restriction)
    {
        external_memory.restrictions_list.push_back(restriction.get());
        // SimpleLogger().Write() << "from: " << restriction.get().restriction.from.node <<
        //                           ",via: " << restriction.get().restriction.via.node <<
        //                           ", to: " << restriction.get().restriction.to.node <<
        //                           ", only: " << (restriction.get().restriction.flags.is_only ?
        //                           "y" : "n");
    }
}
/**
 * Takes the geometry contained in the ```input_way``` and the tags computed
 * by the lua profile inside ```parsed_way``` and computes all edge segments.
 *
 * Depending on the forward/backwards weights the edges are split into forward
 * and backward edges.
 *
 * warning: caller needs to take care of synchronization!
 */
void ExtractorCallbacks::ProcessWay(const osmium::Way &input_way, const ExtractionWay &parsed_way)
{
    if (((0 >= parsed_way.forward_speed) ||
         (TRAVEL_MODE_INACCESSIBLE == parsed_way.forward_travel_mode)) &&
        ((0 >= parsed_way.backward_speed) ||
         (TRAVEL_MODE_INACCESSIBLE == parsed_way.backward_travel_mode)) &&
        (0 >= parsed_way.duration))
    { // Only true if the way is specified by the speed profile
        return;
    }

    if (input_way.nodes().size() <= 1)
    { // safe-guard against broken data
        return;
    }

    if (std::numeric_limits<decltype(input_way.id())>::max() == input_way.id())
    {
        SimpleLogger().Write(logDEBUG) << "found bogus way with id: " << input_way.id()
                                       << " of size " << input_way.nodes().size();
        return;
    }

    InternalExtractorEdge::WeightData forward_weight_data;
    InternalExtractorEdge::WeightData backward_weight_data;

    if (0 < parsed_way.duration)
    {
        const unsigned num_edges = (input_way.nodes().size() - 1);
        // FIXME We devide by the numer of nodes here, but should rather consider
        // the length of each segment. We would eigther have to compute the length
        // of the whole way here (we can't: no node coordinates) or push that back
        // to the container and keep a reference to the way.
        forward_weight_data.duration = parsed_way.duration / num_edges;
        forward_weight_data.type = InternalExtractorEdge::WeightType::WAY_DURATION;
        backward_weight_data.duration = parsed_way.duration / num_edges;
        backward_weight_data.type = InternalExtractorEdge::WeightType::WAY_DURATION;
    }
    else
    {
        if (parsed_way.forward_speed > 0 &&
            parsed_way.forward_travel_mode != TRAVEL_MODE_INACCESSIBLE)
        {
            forward_weight_data.speed = parsed_way.forward_speed;
            forward_weight_data.type = InternalExtractorEdge::WeightType::SPEED;
        }
        if (parsed_way.backward_speed > 0 &&
            parsed_way.backward_travel_mode != TRAVEL_MODE_INACCESSIBLE)
        {
            backward_weight_data.speed = parsed_way.backward_speed;
            backward_weight_data.type = InternalExtractorEdge::WeightType::SPEED;
        }
    }

    if (forward_weight_data.type == InternalExtractorEdge::WeightType::INVALID &&
        backward_weight_data.type == InternalExtractorEdge::WeightType::INVALID)
    {
        SimpleLogger().Write(logDEBUG) << "found way with bogus speed, id: " << input_way.id();
        return;
    }

    // Get the unique identifier for the street name
    const auto &string_map_iterator = string_map.find(parsed_way.name);
    unsigned name_id = external_memory.name_lengths.size();
    if (string_map.end() == string_map_iterator)
    {
        auto name_length = std::min<unsigned>(255u, parsed_way.name.size());
        std::copy(parsed_way.name.c_str(), parsed_way.name.c_str() + name_length, std::back_inserter(external_memory.name_char_data));
        external_memory.name_lengths.push_back(name_length);
        string_map.insert(std::make_pair(parsed_way.name, name_id));
    }
    else
    {
        name_id = string_map_iterator->second;
    }

    const bool split_edge = (parsed_way.forward_speed > 0) &&
                            (TRAVEL_MODE_INACCESSIBLE != parsed_way.forward_travel_mode) &&
                            (parsed_way.backward_speed > 0) &&
                            (TRAVEL_MODE_INACCESSIBLE != parsed_way.backward_travel_mode) &&
                            ((parsed_way.forward_speed != parsed_way.backward_speed) ||
                             (parsed_way.forward_travel_mode != parsed_way.backward_travel_mode));

    std::transform(input_way.nodes().begin(), input_way.nodes().end(),
                   std::back_inserter(external_memory.used_node_id_list),
                   [](const osmium::NodeRef &ref)
                   {
                       return OSMNodeID(ref.ref());
                   });

    const bool is_opposite_way = TRAVEL_MODE_INACCESSIBLE == parsed_way.forward_travel_mode;

    // traverse way in reverse in this case
    if (is_opposite_way)
    {
        BOOST_ASSERT(split_edge == false);
        BOOST_ASSERT(parsed_way.backward_travel_mode != TRAVEL_MODE_INACCESSIBLE);
        osrm::for_each_pair(input_way.nodes().crbegin(), input_way.nodes().crend(),
                            [&](const osmium::NodeRef &first_node, const osmium::NodeRef &last_node)
                            {
                                external_memory.all_edges_list.push_back(InternalExtractorEdge(
                                    OSMNodeID(first_node.ref()), OSMNodeID(last_node.ref()), name_id,
                                    backward_weight_data, true, false, parsed_way.roundabout,
                                    parsed_way.is_access_restricted, parsed_way.is_startpoint,
                                    parsed_way.backward_travel_mode, false));
                            });

        external_memory.way_start_end_id_list.push_back(
            {OSMWayID(input_way.id()),
             OSMNodeID(input_way.nodes().back().ref()),
             OSMNodeID(input_way.nodes()[input_way.nodes().size() - 2].ref()),
             OSMNodeID(input_way.nodes()[1].ref()),
             OSMNodeID(input_way.nodes()[0].ref())});
    }
    else
    {
        const bool forward_only =
            split_edge || TRAVEL_MODE_INACCESSIBLE == parsed_way.backward_travel_mode;
        osrm::for_each_pair(input_way.nodes().cbegin(), input_way.nodes().cend(),
                            [&](const osmium::NodeRef &first_node, const osmium::NodeRef &last_node)
                            {
                                external_memory.all_edges_list.push_back(InternalExtractorEdge(
                                    OSMNodeID(first_node.ref()), OSMNodeID(last_node.ref()), name_id, forward_weight_data,
                                    true, !forward_only, parsed_way.roundabout,
                                    parsed_way.is_access_restricted, parsed_way.is_startpoint, parsed_way.forward_travel_mode,
                                    split_edge));
                            });
        if (split_edge)
        {
            BOOST_ASSERT(parsed_way.backward_travel_mode != TRAVEL_MODE_INACCESSIBLE);
            osrm::for_each_pair(
                input_way.nodes().cbegin(), input_way.nodes().cend(),
                [&](const osmium::NodeRef &first_node, const osmium::NodeRef &last_node)
                {
                    external_memory.all_edges_list.push_back(InternalExtractorEdge(
                        OSMNodeID(first_node.ref()), OSMNodeID(last_node.ref()), name_id, backward_weight_data, false,
                        true, parsed_way.roundabout, parsed_way.is_access_restricted,
                        parsed_way.is_startpoint, parsed_way.backward_travel_mode, true));
                });
        }

        external_memory.way_start_end_id_list.push_back(
            {OSMWayID(input_way.id()),
             OSMNodeID(input_way.nodes().back().ref()),
             OSMNodeID(input_way.nodes()[input_way.nodes().size() - 2].ref()),
             OSMNodeID(input_way.nodes()[1].ref()),
             OSMNodeID(input_way.nodes()[0].ref())});
    }
}
