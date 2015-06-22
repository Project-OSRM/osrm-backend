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
                                  std::vector<TurnRestriction>& restriction_list)
{
    const FingerPrint fingerprint_valid = FingerPrint::GetValid();
    FingerPrint fingerprint_loaded;
    unsigned number_of_usable_restrictions = 0;
    input_stream.read((char *)&fingerprint_loaded, sizeof(FingerPrint));
    if (!fingerprint_loaded.TestPrepare(fingerprint_valid))
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

    return number_of_usable_restrictions;
}


/**
 * Reads the beginning of an .osrm file and produces:
 *  - list of barrier nodes
 *  - list of traffic lights
 *  - nodes indexed by their internal (non-osm) id
 */
NodeID loadNodesFromFile(std::istream &input_stream,
                         std::vector<NodeID> &barrier_node_list,
                         std::vector<NodeID> &traffic_light_node_list,
                         std::vector<QueryNode> &node_array)
{
    const FingerPrint fingerprint_valid = FingerPrint::GetValid();
    FingerPrint fingerprint_loaded;
    input_stream.read(reinterpret_cast<char *>(&fingerprint_loaded), sizeof(FingerPrint));

    if (!fingerprint_loaded.TestPrepare(fingerprint_valid))
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
        node_array.emplace_back(current_node.lat, current_node.lon, current_node.node_id);
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
 * Reads a .osrm file and produces the edges.
 */
NodeID loadEdgesFromFile(std::istream &input_stream,
                         std::vector<NodeBasedEdge> &edge_list)
{
    EdgeID m;
    input_stream.read(reinterpret_cast<char *>(&m), sizeof(unsigned));
    edge_list.resize(m);
    SimpleLogger().Write() << " and " << m << " edges ";

    input_stream.read((char *) edge_list.data(), m * sizeof(NodeBasedEdge));

    BOOST_ASSERT(edge_list.size() > 0);

#ifndef NDEBUG
    SimpleLogger().Write() << "Validating loaded edges...";
    std::sort(edge_list.begin(), edge_list.end(),
            [](const NodeBasedEdge& lhs, const NodeBasedEdge& rhs)
            {
                return (lhs.source < rhs.source) || (lhs.source == rhs.source && lhs.target < rhs.target);
            });
    for (auto i = 1u; i < edge_list.size(); ++i)
    {
        const auto& edge = edge_list[i];
        const auto& prev_edge = edge_list[i-1];

        BOOST_ASSERT_MSG(edge.weight > 0, "loaded null weight");
        BOOST_ASSERT_MSG(edge.forward, "edge must be oriented in forward direction");
        BOOST_ASSERT_MSG(edge.travel_mode != TRAVEL_MODE_INACCESSIBLE, "loaded non-accessible");

        BOOST_ASSERT_MSG(edge.source != edge.target, "loaded edges contain a loop");
        BOOST_ASSERT_MSG(edge.source != prev_edge.source || edge.target != prev_edge.target, "loaded edges contain a multi edge");
    }
#endif

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

    const FingerPrint fingerprint_valid = FingerPrint::GetValid();
    FingerPrint fingerprint_loaded;
    hsgr_input_stream.read(reinterpret_cast<char *>(&fingerprint_loaded), sizeof(FingerPrint));
    if (!fingerprint_loaded.TestGraphUtil(fingerprint_valid))
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
