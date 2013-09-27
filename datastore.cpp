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
#include "Util/BoostFilesystemFix.h"
#include "Util/GraphLoader.h"
#include "Util/IniFile.h"
#include "Util/SimpleLogger.h"
#include "typedefs.h"

#include <boost/integer.hpp>
#include <boost/filesystem/fstream.hpp>

#include <string>
#include <vector>

int main(int argc, char * argv[]) {
    try {
        LogPolicy::GetInstance().Unmute();
        SimpleLogger().Write() << "Checking input parameters";

        boost::filesystem::path base_path = boost::filesystem::absolute(
            (argc > 1 ? argv[1] : "server.ini")
        ).parent_path();
        IniFile server_config((argc > 1 ? argv[1] : "server.ini"));
        //check contents of config file
        if ( !server_config.Holds("hsgrData")) {
            throw OSRMException("no ram index file name in server ini");
        }
        if ( !server_config.Holds("ramIndex") ) {
            throw OSRMException("no mem index file name in server ini");
        }
        if ( !server_config.Holds("nodesData") ) {
            throw OSRMException("no nodes file name in server ini");
        }
        if ( !server_config.Holds("edgesData") ) {
            throw OSRMException("no edges file name in server ini");
        }

        //generate paths of data files
        boost::filesystem::path hsgr_path = boost::filesystem::absolute(
                server_config.GetParameter("hsgrData"),
                base_path
        );
        boost::filesystem::path ram_index_path = boost::filesystem::absolute(
                server_config.GetParameter("ramIndex"),
                base_path
        );
        boost::filesystem::path node_data_path = boost::filesystem::absolute(
                server_config.GetParameter("nodesData"),
                base_path
        );
        boost::filesystem::path edge_data_path = boost::filesystem::absolute(
                server_config.GetParameter("edgesData"),
                base_path
        );
        boost::filesystem::path name_data_path = boost::filesystem::absolute(
                server_config.GetParameter("namesData"),
                base_path
        );
        boost::filesystem::path timestamp_path = boost::filesystem::absolute(
                server_config.GetParameter("timestamp"),
                base_path
        );

        // check if data files empty
        if ( 0 == boost::filesystem::file_size( node_data_path ) ) {
            throw OSRMException("nodes file is empty");
        }
        if ( 0 == boost::filesystem::file_size( edge_data_path ) ) {
            throw OSRMException("edges file is empty");
        }

        // Allocate a memory layout in shared memory //
          SharedMemory * layout_memory = SharedMemoryFactory::Get(
            LAYOUT_1,
            sizeof(SharedDataLayout)
        );
        SharedDataLayout * shared_layout_ptr = static_cast<SharedDataLayout *>(
            layout_memory->Ptr()
        );

        //                                                             //
        // collect number of elements to store in shared memory object //
        //                                                             //
        SimpleLogger().Write() << "Collecting files sizes";
        // number of entries in name index
        boost::filesystem::ifstream name_stream(
            name_data_path, std::ios::binary
        );
        unsigned name_index_size = 0;
        name_stream.read((char *)&name_index_size, sizeof(unsigned));
        shared_layout_ptr->name_index_list_size = name_index_size;
        BOOST_ASSERT_MSG(0 != shared_layout_ptr->name_index_list_size, "name file broken");

        unsigned number_of_chars = 0;
        name_stream.read((char *)&number_of_chars, sizeof(unsigned));
        shared_layout_ptr->name_char_list_size = number_of_chars;

        //Loading information for original edges
        boost::filesystem::ifstream edges_input_stream(
            edge_data_path,
            std::ios::binary
        );
        unsigned number_of_edges = 0;
        edges_input_stream.read((char*)&number_of_edges, sizeof(unsigned));
        SimpleLogger().Write() << "number of edges: " << number_of_edges;

        shared_layout_ptr->via_node_list_size = number_of_edges;
        shared_layout_ptr->name_id_list_size = number_of_edges;
        shared_layout_ptr->turn_instruction_list_size = number_of_edges;

        //TODO load graph node size
        //TODO load graph edge size
        //TODO load search tree size
        //TODO load checksum
        //TODO load rsearch tree size
        //TODO load timestamp size

        //load coordinate size
        SimpleLogger().Write(logDEBUG) << "Loading coordinates list";
        boost::filesystem::ifstream nodes_input_stream(
            node_data_path,
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
        SimpleLogger().Write() << "Loading names index and chars from: " << name_data_path.string();
        unsigned * name_index_ptr = (unsigned*)(
            shared_memory_ptr + shared_layout_ptr->GetNameIndexOffset()
        );
        SimpleLogger().Write(logDEBUG) << "Bytes: " << shared_layout_ptr->name_index_list_size*sizeof(unsigned);

        name_stream.read(
            (char*)name_index_ptr,
            shared_layout_ptr->name_index_list_size*sizeof(unsigned)
        );

        SimpleLogger().Write() << "Loading names char list";
        SimpleLogger().Write(logDEBUG) << "Bytes: " << shared_layout_ptr->name_char_list_size*sizeof(char);
        char * name_char_ptr = shared_memory_ptr + shared_layout_ptr->GetNameListOffset();
        name_stream.read(
            name_char_ptr,
            shared_layout_ptr->name_char_list_size*sizeof(char)
        );
        name_stream.close();

        //load original edge information
        SimpleLogger().Write() << "Loading via node, coordinates and turn instruction list";

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
        for(unsigned i = 0; i < number_of_edges; ++i) {
            SimpleLogger().Write() << i << "/" << number_of_edges;
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

















        std::vector<QueryGraph::_StrNode> node_list;
        std::vector<QueryGraph::_StrEdge> edge_list;


        //TODO BEGIN:
        //Read directly into shared memory
        unsigned m_check_sum = 0;
        SimpleLogger().Write() << "Loading graph node list";
        uint64_t m_number_of_nodes = readHSGRFromStream(
            hsgr_path,
            node_list,
            edge_list,
            &m_check_sum
        );
        SimpleLogger().Write() << "number of nodes: " << m_number_of_nodes;
        //TODO END

        SharedMemory * graph_node_memory  = SharedMemoryFactory::Get(
            GRAPH_NODE_LIST,
            sizeof(QueryGraph::_StrNode)*node_list.size()
        );
        QueryGraph::_StrNode * graph_node_ptr = static_cast<QueryGraph::_StrNode *>(
            graph_node_memory->Ptr()
        );

        std::copy(node_list.begin(), node_list.end(), graph_node_ptr);

        SimpleLogger().Write() << "Loading graph edge list";
        SharedMemory * graph_edge_memory  = SharedMemoryFactory::Get(
            GRAPH_EDGE_LIST,
            sizeof(QueryGraph::_StrEdge)*edge_list.size()
        );
        QueryGraph::_StrEdge * graph_edge_ptr = static_cast<QueryGraph::_StrEdge *>(
            graph_edge_memory->Ptr()
        );
        std::copy(edge_list.begin(), edge_list.end(), graph_edge_ptr);

        // load checksum
        SimpleLogger().Write() << "Loading check sum";

        SimpleLogger().Write() << "Loading timestamp";
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
        SharedMemory * timestamp_memory  = SharedMemoryFactory::Get(
            TIMESTAMP, m_timestamp.length()
        );
        char * timestamp_ptr = static_cast<char *>( timestamp_memory->Ptr() );
        std::copy(
            m_timestamp.c_str(),
            m_timestamp.c_str()+m_timestamp.length(),
            timestamp_ptr
        );

        //Loading information for original edges
        boost::filesystem::ifstream edges_input_stream(
            edge_data_path,
            std::ios::binary
        );
        unsigned number_of_edges = 0;
        edges_input_stream.read((char*)&number_of_edges, sizeof(unsigned));
        SimpleLogger().Write() << "number of edges: " << number_of_edges;

        // Loading r-tree search data structure
        SimpleLogger().Write() << "loading r-tree search list";
        boost::filesystem::ifstream tree_node_file(
            ram_index_path,
            std::ios::binary
        );

        uint32_t tree_size = 0;
        tree_node_file.read((char*)&tree_size, sizeof(uint32_t));
        StoreIntegerInSharedMemory(tree_size, R_SEARCH_TREE_SIZE);
        //SimpleLogger().Write() << "reading " << tree_size << " tree nodes in " << (sizeof(TreeNode)*tree_size) << " bytes";
        SharedMemory * rtree_memory  = SharedMemoryFactory::Get(
            R_SEARCH_TREE,
            tree_size*sizeof(RTreeNode)
        );
        char * rtree_ptr = static_cast<char *>( rtree_memory->Ptr() );

        tree_node_file.read(rtree_ptr, sizeof(RTreeNode)*tree_size);
        tree_node_file.close();

    } catch(const std::exception & e) {
        SimpleLogger().Write(logWARNING) << "caught exception: " << e.what();
    }

    return 0;
}
