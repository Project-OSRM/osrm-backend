/*

Copyright (c) 2015, Project OSRM, Dennis Luxen, others
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

#include "processing_chain.hpp"

#include "contractor.hpp"
#include "../data_structures/deallocating_vector.hpp"
#include "../data_structures/static_rtree.hpp"
#include "../data_structures/restriction_map.hpp"
#include "../data_structures/range_table.hpp"

#include "../util/git_sha.hpp"
#include "../util/graph_loader.hpp"
#include "../util/integer_range.hpp"
#include "../util/lua_util.hpp"
#include "../util/osrm_exception.hpp"
#include "../util/simple_logger.hpp"
#include "../util/string_util.hpp"
#include "../util/timing_util.hpp"
#include "../typedefs.h"

#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>

#include <tbb/parallel_sort.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

Prepare::~Prepare() {}

int Prepare::Run()
{
#ifdef WIN32
#pragma message("Memory consumption on Windows can be higher due to different bit packing")
#else
    static_assert(sizeof(NodeBasedEdge) == 32,
                  "changing NodeBasedEdge type has influence on memory consumption!");
    static_assert(sizeof(EdgeBasedEdge) == 40,
                  "changing EdgeBasedEdge type has influence on memory consumption!");
#endif

    TIMER_START(preparing);

    // Load the edge-expanded graph from disk
    //

    unsigned edges_crc32;
    DeallocatingVector<EdgeBasedEdge> edge_based_edge_list;
    size_t max_edge_id = LoadEdgeExpandedGraph(config.edge_based_graph_filename, edges_crc32, edge_based_edge_list);
    // Contracting the edge-expanded graph

    TIMER_START(contraction);
    std::vector<bool> is_core_node;
    DeallocatingVector<QueryEdge> contracted_edge_list;
    ContractGraph(max_edge_id, edge_based_edge_list, contracted_edge_list, is_core_node);
    TIMER_STOP(contraction);

    SimpleLogger().Write() << "Contraction took " << TIMER_SEC(contraction) << " sec";
    std::size_t number_of_used_edges = WriteContractedGraph(max_edge_id,
                                                            edges_crc32,
                                                            std::move(contracted_edge_list));
    WriteCoreNodeMarker(std::move(is_core_node));

    TIMER_STOP(preparing);

    SimpleLogger().Write() << "Preprocessing : " << TIMER_SEC(preparing) << " seconds";

    SimpleLogger().Write() << "Contraction: " << ((max_edge_id + 1) / TIMER_SEC(contraction))
                           << " nodes/sec and " << number_of_used_edges / TIMER_SEC(contraction)
                           << " edges/sec";

    SimpleLogger().Write() << "finished preprocessing";

    return 0;
}

std::size_t Prepare::LoadEdgeExpandedGraph(
        std::string const &edge_based_graph_filename,
        unsigned &edges_crc32,
        DeallocatingVector<EdgeBasedEdge> &edge_based_edge_list)
{
    boost::filesystem::ifstream input_stream(edge_based_graph_filename, std::ios::in | std::ios::binary);
    unsigned number_of_edges = 0;
    size_t max_edge_id = SPECIAL_EDGEID;
    input_stream.read((char *)&number_of_edges, sizeof(unsigned));
    input_stream.read((char *)&max_edge_id, sizeof(size_t));
    input_stream.read((char *)&edges_crc32, sizeof(unsigned));

    edge_based_edge_list.resize(number_of_edges);

    // TODO: can we read this in bulk?  DeallocatingVector isn't necessarily
    // all stored contiguously
    for (;number_of_edges > 0; --number_of_edges) {
        EdgeBasedEdge inbuffer;
        input_stream.read((char *) &inbuffer, sizeof(EdgeBasedEdge));
        edge_based_edge_list.emplace_back(std::move(inbuffer));
    }

    return max_edge_id;
}

std::size_t Prepare::WriteContractedGraph(unsigned max_node_id,
                                          const unsigned edges_crc32,
                                          DeallocatingVector<QueryEdge> contracted_edge_list)
{
    // Sorting contracted edges in a way that the static query graph can read some in in-place.
    tbb::parallel_sort(contracted_edge_list.begin(), contracted_edge_list.end());
    const unsigned contracted_edge_count = contracted_edge_list.size();
    SimpleLogger().Write() << "Serializing compacted graph of " << contracted_edge_count
                           << " edges";

    const FingerPrint fingerprint = FingerPrint::GetValid();
    boost::filesystem::ofstream hsgr_output_stream(config.graph_output_path, std::ios::binary);
    hsgr_output_stream.write((char *)&fingerprint, sizeof(FingerPrint));
    const unsigned max_used_node_id = [&contracted_edge_list]
    {
        unsigned tmp_max = 0;
        for (const QueryEdge &edge : contracted_edge_list)
        {
            BOOST_ASSERT(SPECIAL_NODEID != edge.source);
            BOOST_ASSERT(SPECIAL_NODEID != edge.target);
            tmp_max = std::max(tmp_max, edge.source);
            tmp_max = std::max(tmp_max, edge.target);
        }
        return tmp_max;
    }();

    SimpleLogger().Write(logDEBUG) << "input graph has " << (max_node_id + 1) << " nodes";
    SimpleLogger().Write(logDEBUG) << "contracted graph has " << (max_used_node_id + 1) << " nodes";

    std::vector<StaticGraph<EdgeData>::NodeArrayEntry> node_array;
    // make sure we have at least one sentinel
    node_array.resize(max_node_id + 2);

    SimpleLogger().Write() << "Building node array";
    StaticGraph<EdgeData>::EdgeIterator edge = 0;
    StaticGraph<EdgeData>::EdgeIterator position = 0;
    StaticGraph<EdgeData>::EdgeIterator last_edge;

    // initializing 'first_edge'-field of nodes:
    for (const auto node : osrm::irange(0u, max_used_node_id + 1))
    {
        last_edge = edge;
        while ((edge < contracted_edge_count) && (contracted_edge_list[edge].source == node))
        {
            ++edge;
        }
        node_array[node].first_edge = position; //=edge
        position += edge - last_edge;           // remove
    }

    for (const auto sentinel_counter :
         osrm::irange<unsigned>(max_used_node_id + 1, node_array.size()))
    {
        // sentinel element, guarded against underflow
        node_array[sentinel_counter].first_edge = contracted_edge_count;
    }

    SimpleLogger().Write() << "Serializing node array";

    const unsigned node_array_size = node_array.size();
    // serialize crc32, aka checksum
    hsgr_output_stream.write((char *)&edges_crc32, sizeof(unsigned));
    // serialize number of nodes
    hsgr_output_stream.write((char *)&node_array_size, sizeof(unsigned));
    // serialize number of edges
    hsgr_output_stream.write((char *)&contracted_edge_count, sizeof(unsigned));
    // serialize all nodes
    if (node_array_size > 0)
    {
        hsgr_output_stream.write((char *)&node_array[0],
                                 sizeof(StaticGraph<EdgeData>::NodeArrayEntry) * node_array_size);
    }

    // serialize all edges
    SimpleLogger().Write() << "Building edge array";
    int number_of_used_edges = 0;

    StaticGraph<EdgeData>::EdgeArrayEntry current_edge;
    for (const auto edge : osrm::irange<std::size_t>(0, contracted_edge_list.size()))
    {
        // no eigen loops
        BOOST_ASSERT(contracted_edge_list[edge].source != contracted_edge_list[edge].target);
        current_edge.target = contracted_edge_list[edge].target;
        current_edge.data = contracted_edge_list[edge].data;

        // every target needs to be valid
        BOOST_ASSERT(current_edge.target <= max_used_node_id);
#ifndef NDEBUG
        if (current_edge.data.distance <= 0)
        {
            SimpleLogger().Write(logWARNING) << "Edge: " << edge
                                             << ",source: " << contracted_edge_list[edge].source
                                             << ", target: " << contracted_edge_list[edge].target
                                             << ", dist: " << current_edge.data.distance;

            SimpleLogger().Write(logWARNING) << "Failed at adjacency list of node "
                                             << contracted_edge_list[edge].source << "/"
                                             << node_array.size() - 1;
            return 1;
        }
#endif
        hsgr_output_stream.write((char *)&current_edge,
                                 sizeof(StaticGraph<EdgeData>::EdgeArrayEntry));

        ++number_of_used_edges;
    }

    return number_of_used_edges;
}

/**
 \brief Build contracted graph.
 */
void Prepare::ContractGraph(const unsigned max_edge_id,
                            DeallocatingVector<EdgeBasedEdge> &edge_based_edge_list,
                            DeallocatingVector<QueryEdge> &contracted_edge_list,
                            std::vector<bool> &is_core_node)
{
    Contractor contractor(max_edge_id + 1, edge_based_edge_list);
    contractor.Run(config.core_factor);
    contractor.GetEdges(contracted_edge_list);
    contractor.GetCoreMarker(is_core_node);
}

