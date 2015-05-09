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

#ifndef GRAPH_LOADER_HPP
#define GRAPH_LOADER_HPP

#include "fingerprint.hpp"
#include "osrm_exception.hpp"
#include "simple_logger.hpp"
#include "../data_structures/external_memory_node.hpp"
#include "../data_structures/import_edge.hpp"
#include "../data_structures/query_node.hpp"
#include "../data_structures/restriction.hpp"
#include "../typedefs.h"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <tbb/parallel_sort.h>

#include <cmath>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <vector>

/**
 * Reads the .restrictions file and loads it to a vector.
 * The since the restrictions reference nodes using their external node id,
 * we need to renumber it to the new internal id.
*/
unsigned loadRestrictionsFromFile(std::istream& input_stream,
                                  const std::unordered_map<NodeID, NodeID>& ext_to_int_id_map,
                                  std::vector<TurnRestriction>& restriction_list)
{
    const FingerPrint fingerprint_orig;
    FingerPrint fingerprint_loaded;
    unsigned number_of_usable_restrictions = 0;
    input_stream.read((char *)&fingerprint_loaded, sizeof(FingerPrint));
    if (!fingerprint_loaded.TestPrepare(fingerprint_orig))
    {
        SimpleLogger().Write(logWARNING) << ".restrictions was prepared with different build.\n"
                                            "Reprocess to get rid of this warning.";
    }

    input_stream.read((char *)&number_of_usable_restrictions, sizeof(unsigned));
    restriction_list.resize(number_of_usable_restrictions);
    if (number_of_usable_restrictions > 0)
    {
        input_stream.read((char *) restriction_list.data(),
                                number_of_usable_restrictions * sizeof(TurnRestriction));
    }

    // renumber ids referenced in restrictions
    for (TurnRestriction &current_restriction : restriction_list)
    {
        auto internal_id_iter = ext_to_int_id_map.find(current_restriction.from.node);
        if (internal_id_iter == ext_to_int_id_map.end())
        {
            SimpleLogger().Write(logDEBUG) << "Unmapped from node " << current_restriction.from.node
                                           << " of restriction";
            continue;
        }
        current_restriction.from.node = internal_id_iter->second;

        internal_id_iter = ext_to_int_id_map.find(current_restriction.via.node);
        if (internal_id_iter == ext_to_int_id_map.end())
        {
            SimpleLogger().Write(logDEBUG) << "Unmapped via node " << current_restriction.via.node
                                           << " of restriction";
            continue;
        }

        current_restriction.via.node = internal_id_iter->second;

        internal_id_iter = ext_to_int_id_map.find(current_restriction.to.node);
        if (internal_id_iter == ext_to_int_id_map.end())
        {
            SimpleLogger().Write(logDEBUG) << "Unmapped to node " << current_restriction.to.node
                                           << " of restriction";
            continue;
        }
        current_restriction.to.node = internal_id_iter->second;
    }

    return number_of_usable_restrictions;
}


/**
 * Reads the beginning of an .osrm file and produces:
 *  - list of barrier nodes
 *  - list of traffic lights
 *  - index to original node id map
 *  - original node id to index map
 */
NodeID loadNodesFromFile(std::istream &input_stream,
                         std::vector<NodeID> &barrier_node_list,
                         std::vector<NodeID> &traffic_light_node_list,
                         std::vector<QueryNode> &int_to_ext_node_id_map,
                         std::unordered_map<NodeID, NodeID> &ext_to_int_id_map)
{
    const FingerPrint fingerprint_orig;
    FingerPrint fingerprint_loaded;
    input_stream.read(reinterpret_cast<char *>(&fingerprint_loaded), sizeof(FingerPrint));

    if (!fingerprint_loaded.TestGraphUtil(fingerprint_orig))
    {
        SimpleLogger().Write(logWARNING) << ".osrm was prepared with different build.\n"
                                            "Reprocess to get rid of this warning.";
    }

    NodeID n;
    input_stream.read(reinterpret_cast<char *>(&n), sizeof(NodeID));
    SimpleLogger().Write() << "Importing n = " << n << " nodes ";

    ExternalMemoryNode current_node;
    for (NodeID i = 0; i < n; ++i)
    {
        input_stream.read(reinterpret_cast<char *>(&current_node), sizeof(ExternalMemoryNode));
        int_to_ext_node_id_map.emplace_back(current_node.lat, current_node.lon,
                                             current_node.node_id);
        ext_to_int_id_map.emplace(current_node.node_id, i);
        if (current_node.barrier)
        {
            barrier_node_list.emplace_back(i);
        }
        if (current_node.traffic_lights)
        {
            traffic_light_node_list.emplace_back(i);
        }
    }

    // tighten vector sizes
    barrier_node_list.shrink_to_fit();
    traffic_light_node_list.shrink_to_fit();

    return n;
}

/**
 * Reads a .osrm file and produces the edges. Edges reference nodes in the old
 * OSM based format, we need to renumber it here.
 */
NodeID loadEdgesFromFile(std::istream &input_stream,
                         const std::unordered_map<NodeID, NodeID> &ext_to_int_id_map,
                         std::vector<NodeBasedEdge> &edge_list)
{
    EdgeID m;
    input_stream.read(reinterpret_cast<char *>(&m), sizeof(unsigned));
    edge_list.reserve(m);
    SimpleLogger().Write() << " and " << m << " edges ";

    NodeBasedEdge edge(0, 0, 0, 0, false, false, false, false, false, TRAVEL_MODE_INACCESSIBLE, false);
    for (EdgeID i = 0; i < m; ++i)
    {
        input_stream.read(reinterpret_cast<char *>(&edge), sizeof(NodeBasedEdge));
        BOOST_ASSERT_MSG(edge.weight > 0, "loaded null weight");
        BOOST_ASSERT_MSG(edge.forward || edge.backward, "loaded null weight");
        BOOST_ASSERT_MSG(edge.travel_mode != TRAVEL_MODE_INACCESSIBLE, "loaded null weight");

        // translate the external NodeIDs to internal IDs
        auto internal_id_iter = ext_to_int_id_map.find(edge.source);
        if (ext_to_int_id_map.find(edge.source) == ext_to_int_id_map.end())
        {
#ifndef NDEBUG
            SimpleLogger().Write(logWARNING) << " unresolved source NodeID: " << edge.source;
#endif
            continue;
        }
        edge.source = internal_id_iter->second;
        internal_id_iter = ext_to_int_id_map.find(edge.target);
        if (ext_to_int_id_map.find(edge.target) == ext_to_int_id_map.end())
        {
#ifndef NDEBUG
            SimpleLogger().Write(logWARNING) << "unresolved target NodeID : " << edge.target;
#endif
            continue;
        }
        edge.target = internal_id_iter->second;
        BOOST_ASSERT_MSG(edge.source != SPECIAL_NODEID && edge.target != SPECIAL_NODEID,
                         "nonexisting source or target");

        if (edge.source > edge.target)
        {
            std::swap(edge.source, edge.target);

            // std::swap does not work with bit-fields
            bool temp = edge.forward;
            edge.forward = edge.backward;
            edge.backward = temp;
        }

        edge_list.push_back(edge);
    }

    tbb::parallel_sort(edge_list.begin(), edge_list.end());

    // Removes multi-edges between nodes
    for (unsigned i = 1; i < edge_list.size(); ++i)
    {
        if ((edge_list[i - 1].target == edge_list[i].target) &&
            (edge_list[i - 1].source == edge_list[i].source))
        {
            const bool edge_flags_equivalent = (edge_list[i - 1].forward == edge_list[i].forward) &&
                                               (edge_list[i - 1].backward == edge_list[i].backward);
            const bool edge_flags_are_superset1 =
                (edge_list[i - 1].forward && edge_list[i - 1].backward) &&
                (edge_list[i].forward != edge_list[i].backward);
            const bool edge_flags_are_superset_2 =
                (edge_list[i].forward && edge_list[i].backward) &&
                (edge_list[i - 1].forward != edge_list[i - 1].backward);

            if (edge_flags_equivalent)
            {
                edge_list[i].weight = std::min(edge_list[i - 1].weight, edge_list[i].weight);
                edge_list[i - 1].source = SPECIAL_NODEID;
            }
            else if (edge_flags_are_superset1)
            {
                if (edge_list[i - 1].weight <= edge_list[i].weight)
                {
                    // edge i-1 is smaller and goes in both directions. Throw away the other edge
                    edge_list[i].source = SPECIAL_NODEID;
                }
                else
                {
                    // edge i-1 is open in both directions, but edge i is smaller in one direction.
                    // Close edge i-1 in this direction
                    edge_list[i - 1].forward = !edge_list[i].forward;
                    edge_list[i - 1].backward = !edge_list[i].backward;
                }
            }
            else if (edge_flags_are_superset_2)
            {
                if (edge_list[i - 1].weight <= edge_list[i].weight)
                {
                    // edge i-1 is smaller for one direction. edge i is open in both. close edge i
                    // in the other direction
                    edge_list[i].forward = !edge_list[i - 1].forward;
                    edge_list[i].backward = !edge_list[i - 1].backward;
                }
                else
                {
                    // edge i is smaller and goes in both direction. Throw away edge i-1
                    edge_list[i - 1].source = SPECIAL_NODEID;
                }
            }
        }
    }
    const auto new_end_iter =
        std::remove_if(edge_list.begin(), edge_list.end(), [](const NodeBasedEdge &edge)
                       {
                           return edge.source == SPECIAL_NODEID || edge.target == SPECIAL_NODEID;
                       });
    edge_list.erase(new_end_iter, edge_list.end()); // remove excess candidates.
    edge_list.shrink_to_fit();
    SimpleLogger().Write() << "Graph loaded ok and has " << edge_list.size() << " edges";

    return m;
}

template <typename NodeT, typename EdgeT>
unsigned readHSGRFromStream(const boost::filesystem::path &hsgr_file,
                            std::vector<NodeT> &node_list,
                            std::vector<EdgeT> &edge_list,
                            unsigned *check_sum)
{
    if (!boost::filesystem::exists(hsgr_file))
    {
        throw osrm::exception("hsgr file does not exist");
    }
    if (0 == boost::filesystem::file_size(hsgr_file))
    {
        throw osrm::exception("hsgr file is empty");
    }

    boost::filesystem::ifstream hsgr_input_stream(hsgr_file, std::ios::binary);

    FingerPrint fingerprint_loaded, fingerprint_orig;
    hsgr_input_stream.read(reinterpret_cast<char *>(&fingerprint_loaded), sizeof(FingerPrint));
    if (!fingerprint_loaded.TestGraphUtil(fingerprint_orig))
    {
        SimpleLogger().Write(logWARNING) << ".hsgr was prepared with different build.\n"
                                            "Reprocess to get rid of this warning.";
    }

    unsigned number_of_nodes = 0;
    unsigned number_of_edges = 0;
    hsgr_input_stream.read(reinterpret_cast<char *>(check_sum), sizeof(unsigned));
    hsgr_input_stream.read(reinterpret_cast<char *>(&number_of_nodes), sizeof(unsigned));
    BOOST_ASSERT_MSG(0 != number_of_nodes, "number of nodes is zero");
    hsgr_input_stream.read(reinterpret_cast<char *>(&number_of_edges), sizeof(unsigned));

    SimpleLogger().Write() << "number_of_nodes: " << number_of_nodes
                           << ", number_of_edges: " << number_of_edges;

    // BOOST_ASSERT_MSG( 0 != number_of_edges, "number of edges is zero");
    node_list.resize(number_of_nodes);
    hsgr_input_stream.read(reinterpret_cast<char *>(&node_list[0]),
                           number_of_nodes * sizeof(NodeT));

    edge_list.resize(number_of_edges);
    if (number_of_edges > 0)
    {
        hsgr_input_stream.read(reinterpret_cast<char *>(&edge_list[0]),
                               number_of_edges * sizeof(EdgeT));
    }
    hsgr_input_stream.close();

    return number_of_nodes;
}

#endif // GRAPH_LOADER_HPP
