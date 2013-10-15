/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#include "DataStructures/QueryEdge.h"
#include "DataStructures/SharedMemoryFactory.h"
#include "DataStructures/SharedMemoryVectorWrapper.h"
#include "DataStructures/StaticGraph.h"
#include "DataStructures/StaticRTree.h"
#include "Server/DataStructures/BaseDataFacade.h"
#include "Server/DataStructures/SharedDataType.h"
#include "Util/BoostFileSystemFix.h"
#include "Util/ProgramOptions.h"
#include "Util/SimpleLogger.h"
#include "Util/UUID.h"
#include "typedefs.h"

#include <boost/integer.hpp>
#include <boost/filesystem/fstream.hpp>

#include <string>
#include <vector>

int main( const int argc, const char * argv[] ) {
    try {
        LogPolicy::GetInstance().Unmute();
        SimpleLogger().Write() << "Checking input parameters";

        bool use_shared_memory = false;
        std::string ip_address;
        int ip_port, requested_num_threads;

        ServerPaths server_paths;
        if( !GenerateServerProgramOptions(
                argc,
                argv,
                server_paths,
                ip_address,
                ip_port,
                requested_num_threads,
                use_shared_memory
             )
        ) {
            return 0;
        }
        if( server_paths.find("hsgrdata") == server_paths.end() ) {
            throw OSRMException("no hsgr file given in ini file");
        }
        if( server_paths.find("ramindex") == server_paths.end() ) {
            throw OSRMException("no ram index file given in ini file");
        }
        if( server_paths.find("fileindex") == server_paths.end() ) {
            throw OSRMException("no leaf index file given in ini file");
        }
        if( server_paths.find("nodesdata") == server_paths.end() ) {
            throw OSRMException("no nodes file given in ini file");
        }
        if( server_paths.find("edgesdata") == server_paths.end() ) {
            throw OSRMException("no edges file given in ini file");
        }
        if( server_paths.find("namesdata") == server_paths.end() ) {
            throw OSRMException("no names file given in ini file");
        }

        ServerPaths::const_iterator paths_iterator = server_paths.find("hsgrdata");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        const boost::filesystem::path & hsgr_path = paths_iterator->second;
        paths_iterator = server_paths.find("timestamp");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        const boost::filesystem::path & timestamp_path = paths_iterator->second;
        paths_iterator = server_paths.find("ramindex");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        const boost::filesystem::path & ram_index_path = paths_iterator->second;
        // paths_iterator = server_paths.find("fileindex");
        // BOOST_ASSERT(server_paths.end() != paths_iterator);
        // const boost::filesystem::path & file_index_path = paths_iterator->second;
        paths_iterator = server_paths.find("nodesdata");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        const boost::filesystem::path & nodes_data_path = paths_iterator->second;
        paths_iterator = server_paths.find("edgesdata");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        const boost::filesystem::path & edges_data_path = paths_iterator->second;
        paths_iterator = server_paths.find("namesdata");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        const boost::filesystem::path & names_data_path = paths_iterator->second;


        // Allocate a memory layout in shared memory //
          SharedMemory * layout_memory = SharedMemoryFactory::Get(
            LAYOUT_1,
            sizeof(SharedDataLayout)
        );
        SharedDataLayout * shared_layout_ptr = static_cast<SharedDataLayout *>(
            layout_memory->Ptr()
        );
        shared_layout_ptr = new(layout_memory->Ptr()) SharedDataLayout();

        //                                                             //
        // collect number of elements to store in shared memory object //
        //                                                             //
        SimpleLogger().Write() << "Collecting files sizes";
        // number of entries in name index
        boost::filesystem::ifstream name_stream(
            names_data_path, std::ios::binary
        );
        unsigned name_index_size = 0;
        name_stream.read((char *)&name_index_size, sizeof(unsigned));
        shared_layout_ptr->name_index_list_size = name_index_size;
        // SimpleLogger().Write() << "name index size: " << shared_layout_ptr->name_index_list_size;
        BOOST_ASSERT_MSG(0 != shared_layout_ptr->name_index_list_size, "name file broken");

        unsigned number_of_chars = 0;
        name_stream.read((char *)&number_of_chars, sizeof(unsigned));
        shared_layout_ptr->name_char_list_size = number_of_chars;
        // SimpleLogger().Write() << "name char size: " << shared_layout_ptr->name_char_list_size;

        //Loading information for original edges
        boost::filesystem::ifstream edges_input_stream(
            edges_data_path,
            std::ios::binary
        );
        unsigned number_of_original_edges = 0;
        edges_input_stream.read((char*)&number_of_original_edges, sizeof(unsigned));
        SimpleLogger().Write(logDEBUG) <<
            "number of edges: " << number_of_original_edges;

        shared_layout_ptr->via_node_list_size = number_of_original_edges;
        shared_layout_ptr->name_id_list_size = number_of_original_edges;
        shared_layout_ptr->turn_instruction_list_size = number_of_original_edges;


        SimpleLogger().Write(logDEBUG) << "noted number of edges";

        SimpleLogger().Write(logDEBUG) << "loading hsgr from " << hsgr_path.string();
        boost::filesystem::ifstream hsgr_input_stream(
            hsgr_path,
            std::ios::binary
        );

        UUID uuid_loaded, uuid_orig;
        hsgr_input_stream.read((char *)&uuid_loaded, sizeof(UUID));
        if( !uuid_loaded.TestGraphUtil(uuid_orig) ) {
            SimpleLogger().Write(logWARNING) <<
                ".hsgr was prepared with different build. "
                "Reprocess to get rid of this warning.";
        } else {
            SimpleLogger().Write() << "UUID checked out ok";
        }

        // load checksum
        unsigned checksum = 0;
        hsgr_input_stream.read((char*)&checksum, sizeof(unsigned) );
        SimpleLogger().Write() << "checksum: " << checksum;
        shared_layout_ptr->checksum = checksum;
        SimpleLogger().Write(logDEBUG) << "noted checksum";
        // load graph node size
        unsigned number_of_graph_nodes = 0;
        hsgr_input_stream.read(
            (char*) &number_of_graph_nodes,
            sizeof(unsigned)
        );
        SimpleLogger().Write(logDEBUG) << "number of nodes: " << number_of_graph_nodes;
        BOOST_ASSERT_MSG(
            (0 != number_of_graph_nodes),
            "number of nodes is zero"
        );
        shared_layout_ptr->graph_node_list_size = number_of_graph_nodes;

        // load graph edge size
        unsigned number_of_graph_edges = 0;
        hsgr_input_stream.read( (char*) &number_of_graph_edges, sizeof(unsigned) );
        SimpleLogger().Write() << "number of graph edges: " << number_of_graph_edges;
        BOOST_ASSERT_MSG( 0 != number_of_graph_edges, "number of graph edges is zero");
        shared_layout_ptr->graph_edge_list_size = number_of_graph_edges;

        // load rsearch tree size
        SimpleLogger().Write(logDEBUG) << "loading r-tree search list size";
        boost::filesystem::ifstream tree_node_file(
            ram_index_path,
            std::ios::binary
        );

        uint32_t tree_size = 0;
        tree_node_file.read((char*)&tree_size, sizeof(uint32_t));
        shared_layout_ptr->r_search_tree_size = tree_size;

        //load timestamp size
        SimpleLogger().Write(logDEBUG) << "Loading timestamp";
        std::string m_timestamp;
        if( boost::filesystem::exists(timestamp_path) ) {
            boost::filesystem::ifstream timestampInStream( timestamp_path );
            if(!timestampInStream) {
                SimpleLogger().Write(logWARNING) << timestamp_path << " not found";
            }
            getline(timestampInStream, m_timestamp);
            timestampInStream.close();
        }
        if(m_timestamp.empty()) {
            m_timestamp = "n/a";
        }
        if(25 < m_timestamp.length()) {
            m_timestamp.resize(25);
        }
        shared_layout_ptr->timestamp_length = m_timestamp.length();

        //load coordinate size
        SimpleLogger().Write() <<
            "Loading coordinates list from " << nodes_data_path.string();
        boost::filesystem::ifstream nodes_input_stream(
            nodes_data_path,
            std::ios::binary
        );
        unsigned coordinate_list_size = 0;
        nodes_input_stream.read((char *)&coordinate_list_size, sizeof(unsigned));
        shared_layout_ptr->coordinate_list_size = coordinate_list_size;


        // allocate shared memory block
        SimpleLogger().Write() << "allocating shared memory of " << shared_layout_ptr->GetSizeOfLayout() << " bytes";
        SharedMemory * shared_memory = SharedMemoryFactory::Get(
            DATA_1,
            shared_layout_ptr->GetSizeOfLayout()
        );
        char * shared_memory_ptr = static_cast<char *>(shared_memory->Ptr());

        // read actual data into shared memory object //
        // Loading street names
        SimpleLogger().Write() << "Loading names index and chars from: " << names_data_path.string();
        unsigned * name_index_ptr = (unsigned*)(
            shared_memory_ptr + shared_layout_ptr->GetNameIndexOffset()
        );
        SimpleLogger().Write(logDEBUG) << "Bytes: " << shared_layout_ptr->name_index_list_size*sizeof(unsigned);

        name_stream.read(
            (char*)name_index_ptr,
            shared_layout_ptr->name_index_list_size*sizeof(unsigned)
        );

        SimpleLogger().Write(logDEBUG) << "Loading names char list";
        SimpleLogger().Write(logDEBUG) << "Bytes: " << shared_layout_ptr->name_char_list_size*sizeof(char);
        char * name_char_ptr = shared_memory_ptr + shared_layout_ptr->GetNameListOffset();
        name_stream.read(
            name_char_ptr,
            shared_layout_ptr->name_char_list_size*sizeof(char)
        );
        name_stream.close();

        //load original edge information
        SimpleLogger().Write() <<
            "Loading via node, coordinates and turn instruction lists from: " <<
            edges_data_path.string();

        NodeID * via_node_ptr = (NodeID *)(
            shared_memory_ptr + shared_layout_ptr->GetViaNodeListOffset()
        );

        unsigned * name_id_ptr = (unsigned *)(
            shared_memory_ptr + shared_layout_ptr->GetNameIDListOffset()
        );

        TurnInstruction * turn_instructions_ptr = (TurnInstruction *)(
            shared_memory_ptr + shared_layout_ptr->GetTurnInstructionListOffset()
        );

        OriginalEdgeData current_edge_data;
        for(unsigned i = 0; i < number_of_original_edges; ++i) {
            // SimpleLogger().Write() << i << "/" << number_of_edges;
            edges_input_stream.read(
                (char*)&(current_edge_data),
                sizeof(OriginalEdgeData)
            );
            via_node_ptr[i] = current_edge_data.viaNode;
            name_id_ptr[i]  = current_edge_data.nameID;
            turn_instructions_ptr[i] = current_edge_data.turnInstruction;
        }
        edges_input_stream.close();

        // Loading list of coordinates
        FixedPointCoordinate * coordinates_ptr = (FixedPointCoordinate *)(
            shared_memory_ptr + shared_layout_ptr->GetCoordinateListOffset()
        );

        NodeInfo current_node;
        for(unsigned i = 0; i < coordinate_list_size; ++i) {
            nodes_input_stream.read((char *)&current_node, sizeof(NodeInfo));
            coordinates_ptr[i] = FixedPointCoordinate(current_node.lat, current_node.lon);
        }
        nodes_input_stream.close();

        //store timestamp
        char * timestamp_ptr = static_cast<char *>(
            shared_memory_ptr + shared_layout_ptr->GetTimeStampOffset()
        );
        std::copy(
            m_timestamp.c_str(),
            m_timestamp.c_str()+m_timestamp.length(),
            timestamp_ptr
        );

        // store search tree portion of rtree
        char * rtree_ptr = static_cast<char *>(
            shared_memory_ptr + shared_layout_ptr->GetRSearchTreeOffset()
        );

        tree_node_file.read(rtree_ptr, sizeof(RTreeNode)*tree_size);
        tree_node_file.close();

        // load the nodes of the search graph
        QueryGraph::_StrNode * graph_node_list_ptr = (QueryGraph::_StrNode*)(
            shared_memory_ptr + shared_layout_ptr->GetGraphNodeListOffset()
        );
        hsgr_input_stream.read(
            (char*) graph_node_list_ptr,
            shared_layout_ptr->graph_node_list_size*sizeof(QueryGraph::_StrNode)
        );

        // load the edges of the search graph
        QueryGraph::_StrEdge * graph_edge_list_ptr = (QueryGraph::_StrEdge *)(
            shared_memory_ptr + shared_layout_ptr->GetGraphEdgeListOffsett()
        );
        hsgr_input_stream.read(
            (char*) graph_edge_list_ptr,
            shared_layout_ptr->graph_edge_list_size*sizeof(QueryGraph::_StrEdge)
        );
        hsgr_input_stream.close();

        SimpleLogger().Write() << "all data loaded. pressing a key deallocates memory";
        std::cin.get();

    } catch(const std::exception & e) {
        SimpleLogger().Write(logWARNING) << "caught exception: " << e.what();
    }

    return 0;
}
