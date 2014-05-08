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

#include "DataStructures/OriginalEdgeData.h"
#include "DataStructures/QueryEdge.h"
#include "DataStructures/SharedMemoryFactory.h"
#include "DataStructures/SharedMemoryVectorWrapper.h"
#include "DataStructures/StaticGraph.h"
#include "DataStructures/StaticRTree.h"
#include "Server/DataStructures/BaseDataFacade.h"
#include "Server/DataStructures/SharedDataType.h"
#include "Server/DataStructures/SharedBarriers.h"
#include "Util/BoostFileSystemFix.h"
#include "Util/DataStoreOptions.h"
#include "Util/SimpleLogger.h"
#include "Util/UUID.h"
#include "typedefs.h"

#ifdef __linux__
#include <sys/mman.h>
#endif

#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/seek.hpp>

#include <cstdint>

#include <fstream>
#include <string>
#include <vector>

int main(const int argc, const char *argv[])
{
    LogPolicy::GetInstance().Unmute();
    SharedBarriers barrier;

#ifdef __linux__
    const bool lock_flags = MCL_CURRENT | MCL_FUTURE;
    if (-1 == mlockall(lock_flags))
    {
        SimpleLogger().Write(logWARNING) << "Process " << argv[0] << " could not request RAM lock";
    }
#endif

    try
    {
        try
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> pending_lock(
                barrier.pending_update_mutex);
        }
        catch (...)
        {
            // hard unlock in case of any exception.
            barrier.pending_update_mutex.unlock();
        }
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    }

    try
    {
        SimpleLogger().Write(logDEBUG) << "Checking input parameters";

        ServerPaths server_paths;
        if (!GenerateDataStoreOptions(argc, argv, server_paths))
        {
            return 0;
        }
        if (server_paths.find("hsgrdata") == server_paths.end())
        {
            throw OSRMException("no hsgr file found");
        }
        if (server_paths.find("ramindex") == server_paths.end())
        {
            throw OSRMException("no ram index file found");
        }
        if (server_paths.find("fileindex") == server_paths.end())
        {
            throw OSRMException("no leaf index file found");
        }
        if (server_paths.find("nodesdata") == server_paths.end())
        {
            throw OSRMException("no nodes file found");
        }
        if (server_paths.find("edgesdata") == server_paths.end())
        {
            throw OSRMException("no edges file found");
        }
        if (server_paths.find("namesdata") == server_paths.end())
        {
            throw OSRMException("no names file found");
        }
        if (server_paths.find("geometry") == server_paths.end())
        {
            throw OSRMException("no geometry file found");
        }

        ServerPaths::const_iterator paths_iterator = server_paths.find("hsgrdata");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &hsgr_path = paths_iterator->second;
        paths_iterator = server_paths.find("timestamp");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &timestamp_path = paths_iterator->second;
        paths_iterator = server_paths.find("ramindex");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &ram_index_path = paths_iterator->second;
        paths_iterator = server_paths.find("fileindex");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path index_file_path_absolute =
            boost::filesystem::portable_canonical(paths_iterator->second);
        const std::string &file_index_file_name = index_file_path_absolute.string();
        paths_iterator = server_paths.find("nodesdata");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &nodes_data_path = paths_iterator->second;
        paths_iterator = server_paths.find("edgesdata");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &edges_data_path = paths_iterator->second;
        paths_iterator = server_paths.find("namesdata");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &names_data_path = paths_iterator->second;
        paths_iterator = server_paths.find("geometry");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &geometries_data_path = paths_iterator->second;

        // get the shared memory segment to use
        bool use_first_segment = SharedMemory::RegionExists(LAYOUT_2);
        SharedDataType LAYOUT = (use_first_segment ? LAYOUT_1 : LAYOUT_2);
        SharedDataType DATA = (use_first_segment ? DATA_1 : DATA_2);

        // Allocate a memory layout in shared memory, deallocate previous
        SharedMemory *layout_memory = SharedMemoryFactory::Get(LAYOUT, sizeof(SharedDataLayout));
        SharedDataLayout *shared_layout_ptr = static_cast<SharedDataLayout *>(layout_memory->Ptr());
        shared_layout_ptr = new (layout_memory->Ptr()) SharedDataLayout();

        std::copy(file_index_file_name.begin(),
                  (file_index_file_name.length() <= 1024 ? file_index_file_name.end()
                                                         : file_index_file_name.begin() + 1023),
                  shared_layout_ptr->ram_index_file_name);
        // add zero termination
        unsigned end_of_string_index = std::min((std::size_t)1023, file_index_file_name.length());
        shared_layout_ptr->ram_index_file_name[end_of_string_index] = '\0';

        // collect number of elements to store in shared memory object
        SimpleLogger().Write() << "load names from: " << names_data_path;
        // number of entries in name index
        boost::filesystem::ifstream name_stream(names_data_path, std::ios::binary);
        unsigned name_index_size = 0;
        name_stream.read((char *)&name_index_size, sizeof(unsigned));
        shared_layout_ptr->name_index_list_size = name_index_size;
        SimpleLogger().Write() << "size: " << name_index_size;
        BOOST_ASSERT_MSG(0 != shared_layout_ptr->name_index_list_size, "name file broken");

        unsigned number_of_chars = 0;
        name_stream.read((char *)&number_of_chars, sizeof(unsigned));
        shared_layout_ptr->name_char_list_size = number_of_chars;

        // Loading information for original edges
        boost::filesystem::ifstream edges_input_stream(edges_data_path, std::ios::binary);
        unsigned number_of_original_edges = 0;
        edges_input_stream.read((char *)&number_of_original_edges, sizeof(unsigned));

        shared_layout_ptr->via_node_list_size = number_of_original_edges;
        shared_layout_ptr->name_id_list_size = number_of_original_edges;
        shared_layout_ptr->turn_instruction_list_size = number_of_original_edges;
        shared_layout_ptr->geometries_indicators = number_of_original_edges;

        boost::filesystem::ifstream hsgr_input_stream(hsgr_path, std::ios::binary);

        UUID uuid_loaded, uuid_orig;
        hsgr_input_stream.read((char *)&uuid_loaded, sizeof(UUID));
        if (uuid_loaded.TestGraphUtil(uuid_orig))
        {
            SimpleLogger().Write(logDEBUG) << "UUID checked out ok";
        }
        else
        {
            SimpleLogger().Write(logWARNING) << ".hsgr was prepared with different build. "
                                                "Reprocess to get rid of this warning.";
        }

        // load checksum
        unsigned checksum = 0;
        hsgr_input_stream.read((char *)&checksum, sizeof(unsigned));
        shared_layout_ptr->checksum = checksum;
        // load graph node size
        unsigned number_of_graph_nodes = 0;
        hsgr_input_stream.read((char *)&number_of_graph_nodes, sizeof(unsigned));

        BOOST_ASSERT_MSG((0 != number_of_graph_nodes), "number of nodes is zero");
        shared_layout_ptr->graph_node_list_size = number_of_graph_nodes;

        // load graph edge size
        unsigned number_of_graph_edges = 0;
        hsgr_input_stream.read((char *)&number_of_graph_edges, sizeof(unsigned));
        BOOST_ASSERT_MSG(0 != number_of_graph_edges, "number of graph edges is zero");
        shared_layout_ptr->graph_edge_list_size = number_of_graph_edges;

        // load rsearch tree size
        boost::filesystem::ifstream tree_node_file(ram_index_path, std::ios::binary);

        uint32_t tree_size = 0;
        tree_node_file.read((char *)&tree_size, sizeof(uint32_t));
        shared_layout_ptr->r_search_tree_size = tree_size;

        // load timestamp size
        std::string m_timestamp;
        if (boost::filesystem::exists(timestamp_path))
        {
            boost::filesystem::ifstream timestamp_stream(timestamp_path);
            if (!timestamp_stream)
            {
                SimpleLogger().Write(logWARNING) << timestamp_path
                                                 << " not found. setting to default";
            }
            else
            {
                getline(timestamp_stream, m_timestamp);
                timestamp_stream.close();
            }
        }
        if (m_timestamp.empty())
        {
            m_timestamp = "n/a";
        }
        if (25 < m_timestamp.length())
        {
            m_timestamp.resize(25);
        }
        shared_layout_ptr->timestamp_length = m_timestamp.length();

        // load coordinate size
        boost::filesystem::ifstream nodes_input_stream(nodes_data_path, std::ios::binary);
        unsigned coordinate_list_size = 0;
        nodes_input_stream.read((char *)&coordinate_list_size, sizeof(unsigned));
        shared_layout_ptr->coordinate_list_size = coordinate_list_size;

        // load geometries sizes
        std::ifstream geometry_input_stream(geometries_data_path.c_str(), std::ios::binary);
        unsigned number_of_geometries_indices = 0;
        unsigned number_of_compressed_geometries = 0;

        geometry_input_stream.read((char *)&number_of_geometries_indices, sizeof(unsigned));
        shared_layout_ptr->geometries_index_list_size = number_of_geometries_indices;
        boost::iostreams::seek(
            geometry_input_stream, number_of_geometries_indices * sizeof(unsigned), BOOST_IOS::cur);
        geometry_input_stream.read((char *)&number_of_compressed_geometries, sizeof(unsigned));
        shared_layout_ptr->geometries_list_size = number_of_compressed_geometries;

        // allocate shared memory block
        SimpleLogger().Write() << "allocating shared memory of "
                               << shared_layout_ptr->GetSizeOfLayout() << " bytes";
        SharedMemory *shared_memory =
            SharedMemoryFactory::Get(DATA, shared_layout_ptr->GetSizeOfLayout());
        char *shared_memory_ptr = static_cast<char *>(shared_memory->Ptr());

        // read actual data into shared memory object //
        // Loading street names
        unsigned *name_index_ptr =
            (unsigned *)(shared_memory_ptr + shared_layout_ptr->GetNameIndexOffset());

        name_stream.read((char *)name_index_ptr,
                         shared_layout_ptr->name_index_list_size * sizeof(unsigned));

        char *name_char_ptr = shared_memory_ptr + shared_layout_ptr->GetNameListOffset();
        name_stream.read(name_char_ptr, shared_layout_ptr->name_char_list_size * sizeof(char));
        name_stream.close();

        // load original edge information
        NodeID *via_node_ptr =
            (NodeID *)(shared_memory_ptr + shared_layout_ptr->GetViaNodeListOffset());

        unsigned *name_id_ptr =
            (unsigned *)(shared_memory_ptr + shared_layout_ptr->GetNameIDListOffset());

        TurnInstruction *turn_instructions_ptr =
            (TurnInstruction *)(shared_memory_ptr +
                                shared_layout_ptr->GetTurnInstructionListOffset());

        unsigned *geometries_indicator_ptr =
            (unsigned *)(shared_memory_ptr + shared_layout_ptr->GetGeometriesIndicatorOffset());

        OriginalEdgeData current_edge_data;
        for (unsigned i = 0; i < number_of_original_edges; ++i)
        {
            edges_input_stream.read((char *)&(current_edge_data), sizeof(OriginalEdgeData));
            via_node_ptr[i] = current_edge_data.via_node;
            name_id_ptr[i] = current_edge_data.name_id;
            turn_instructions_ptr[i] = current_edge_data.turn_instruction;

            const unsigned bucket = i / 32;
            const unsigned offset = i % 32;
            unsigned value = ((0 == offset) ? 0 : geometries_indicator_ptr[bucket]);
            if (current_edge_data.compressed_geometry)
            {
                geometries_indicator_ptr[bucket] = (value | (1 << offset));
            }
        }
        edges_input_stream.close();

        // load compressed geometry
        unsigned temporary_value;
        unsigned *geometries_index_ptr =
            (unsigned *)(shared_memory_ptr + shared_layout_ptr->GetGeometriesIndexListOffset());
        geometry_input_stream.seekg(0, geometry_input_stream.beg);
        geometry_input_stream.read((char *)&temporary_value, sizeof(unsigned));
        BOOST_ASSERT(temporary_value == shared_layout_ptr->geometries_index_list_size);

        geometry_input_stream.read((char *)geometries_index_ptr,
                                   shared_layout_ptr->geometries_index_list_size *
                                       sizeof(unsigned));

        unsigned *geometries_list_ptr =
            (unsigned *)(shared_memory_ptr + shared_layout_ptr->GetGeometryListOffset());

        geometry_input_stream.read((char *)&temporary_value, sizeof(unsigned));
        BOOST_ASSERT(temporary_value == shared_layout_ptr->geometries_list_size);

        geometry_input_stream.read((char *)geometries_list_ptr,
                                   shared_layout_ptr->geometries_list_size * sizeof(unsigned));

        // Loading list of coordinates
        FixedPointCoordinate *coordinates_ptr =
            (FixedPointCoordinate *)(shared_memory_ptr +
                                     shared_layout_ptr->GetCoordinateListOffset());

        NodeInfo current_node;
        for (unsigned i = 0; i < coordinate_list_size; ++i)
        {
            nodes_input_stream.read((char *)&current_node, sizeof(NodeInfo));
            coordinates_ptr[i] = FixedPointCoordinate(current_node.lat, current_node.lon);
        }
        nodes_input_stream.close();

        // store timestamp
        char *timestamp_ptr =
            static_cast<char *>(shared_memory_ptr + shared_layout_ptr->GetTimeStampOffset());
        std::copy(m_timestamp.c_str(), m_timestamp.c_str() + m_timestamp.length(), timestamp_ptr);

        // store search tree portion of rtree
        char *rtree_ptr =
            static_cast<char *>(shared_memory_ptr + shared_layout_ptr->GetRSearchTreeOffset());

        tree_node_file.read(rtree_ptr, sizeof(RTreeNode) * tree_size);
        tree_node_file.close();

        // load the nodes of the search graph
        QueryGraph::NodeArrayEntry *graph_node_list_ptr =
            (QueryGraph::NodeArrayEntry *)(shared_memory_ptr +
                                           shared_layout_ptr->GetGraphNodeListOffset());
        hsgr_input_stream.read((char *)graph_node_list_ptr,
                               shared_layout_ptr->graph_node_list_size *
                                   sizeof(QueryGraph::NodeArrayEntry));

        // load the edges of the search graph
        QueryGraph::EdgeArrayEntry *graph_edge_list_ptr =
            (QueryGraph::EdgeArrayEntry *)(shared_memory_ptr +
                                           shared_layout_ptr->GetGraphEdgeListOffset());
        hsgr_input_stream.read((char *)graph_edge_list_ptr,
                               shared_layout_ptr->graph_edge_list_size *
                                   sizeof(QueryGraph::EdgeArrayEntry));
        hsgr_input_stream.close();

        // acquire lock
        SharedMemory *data_type_memory =
            SharedMemoryFactory::Get(CURRENT_REGIONS, sizeof(SharedDataTimestamp), true, false);
        SharedDataTimestamp *data_timestamp_ptr =
            static_cast<SharedDataTimestamp *>(data_type_memory->Ptr());

        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> query_lock(
            barrier.query_mutex);

        // notify all processes that were waiting for this condition
        if (0 < barrier.number_of_queries)
        {
            barrier.no_running_queries_condition.wait(query_lock);
        }

        data_timestamp_ptr->layout = LAYOUT;
        data_timestamp_ptr->data = DATA;
        data_timestamp_ptr->timestamp += 1;
        if (use_first_segment)
        {
            BOOST_ASSERT(DATA == DATA_1);
            BOOST_ASSERT(LAYOUT == LAYOUT_1);
            if (!SharedMemory::Remove(DATA_2))
            {
                SimpleLogger().Write(logWARNING) << "could not delete DATA_2";
            }
            if (!SharedMemory::Remove(LAYOUT_2))
            {
                SimpleLogger().Write(logWARNING) << "could not delete LAYOUT_2";
            }
        }
        else
        {
            BOOST_ASSERT(DATA == DATA_2);
            BOOST_ASSERT(LAYOUT == LAYOUT_2);
            if (!SharedMemory::Remove(DATA_1))
            {
                SimpleLogger().Write(logWARNING) << "could not delete DATA_1";
            }
            if (!SharedMemory::Remove(LAYOUT_1))
            {
                SimpleLogger().Write(logWARNING) << "could not delete LAYOUT_1";
            }
        }
        SimpleLogger().Write() << "all data loaded";

        shared_layout_ptr->PrintInformation();
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "caught exception: " << e.what();
    }

    return 0;
}
