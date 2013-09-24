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

#ifndef SHARED_DATA_FACADE
#define SHARED_DATA_FACADE

//implements all data storage when shared memory is _NOT_ used

#include "BaseDataFacade.h"

#include "../../DataStructures/StaticGraph.h"
#include "../../DataStructures/StaticRTree.h"
#include "../../Util/BoostFileSystemFix.h"
#include "../../Util/IniFile.h"
#include "../../Util/SimpleLogger.h"

#include <algorithm>

template<class EdgeDataT>
class SharedDataFacade : public BaseDataFacade<EdgeDataT> {

private:
    typedef EdgeDataT EdgeData;
    typedef BaseDataFacade<EdgeData>                        super;
    typedef StaticGraph<EdgeData, true>                     QueryGraph;
    typedef typename StaticGraph<EdgeData, true>::_StrNode  GraphNode;
    typedef typename StaticGraph<EdgeData, true>::_StrEdge  GraphEdge;
    typedef typename QueryGraph::InputEdge                  InputEdge;
    typedef typename super::RTreeLeaf                       RTreeLeaf;
    typedef typename StaticRTree<RTreeLeaf, true>::TreeNode RTreeNode;

    unsigned                                m_check_sum;
    unsigned                                m_number_of_nodes;
    QueryGraph                            * m_query_graph;
    std::string                             m_timestamp;

    ShM<FixedPointCoordinate, true>::vector m_coordinate_list;
    ShM<NodeID, true>::vector               m_via_node_list;
    ShM<unsigned, true>::vector             m_name_ID_list;
    ShM<TurnInstruction, true>::vector      m_turn_instruction_list;
    ShM<char, false>::vector                m_names_char_list;
    ShM<unsigned, false>::vector            m_name_begin_indices;
    StaticRTree<RTreeLeaf, true>          * m_static_rtree;

    SharedDataFacade() { }

    void LoadTimestamp() {
        uint32_t timestamp_size = *static_cast<unsigned *>(
            SharedMemoryFactory::Get(TIMESTAMP_SIZE)->Ptr()
        );
        timestamp_size = std::min(timestamp_size, 25u);
        SharedMemory * search_tree = SharedMemoryFactory::Get(TIMESTAMP);
        char * tree_ptr = static_cast<char *>( search_tree->Ptr() );
        m_timestamp.resize(timestamp_size);
        std::copy(tree_ptr, tree_ptr+timestamp_size, m_timestamp.begin());
    }

    void LoadRTree(
        const boost::filesystem::path & file_index_path
    ) {
        uint32_t tree_size = *static_cast<unsigned *>(
            SharedMemoryFactory::Get(R_SEARCH_TREE_SIZE)->Ptr()
        );
        SharedMemory * search_tree = SharedMemoryFactory::Get(R_SEARCH_TREE);
        RTreeNode * tree_ptr = static_cast<RTreeNode *>( search_tree->Ptr() );
        m_static_rtree = new StaticRTree<RTreeLeaf, true>(
            tree_ptr,
            tree_size,
            file_index_path
        );
    }

    void LoadGraph() {
        m_number_of_nodes = *static_cast<unsigned *>(
            SharedMemoryFactory::Get(GRAPH_NODE_LIST_SIZE)->Ptr()
        );
        SharedMemory * graph_nodes = SharedMemoryFactory::Get(GRAPH_NODE_LIST);
        GraphNode * graph_nodes_ptr = static_cast<GraphNode *>( graph_nodes->Ptr() );

        uint32_t number_of_edges = *static_cast<unsigned *>(
            SharedMemoryFactory::Get(GRAPH_EDGE_LIST_SIZE)->Ptr()
        );
        SharedMemory * graph_edges = SharedMemoryFactory::Get(GRAPH_EDGE_LIST);
        GraphEdge * graph_edges_ptr = static_cast<GraphEdge *>( graph_edges->Ptr() );

        typename ShM<GraphNode, true>::vector node_list(graph_nodes_ptr, m_number_of_nodes);
        typename ShM<GraphEdge, true>::vector edge_list(graph_edges_ptr, number_of_edges);
        m_query_graph = new QueryGraph(
            node_list ,
            edge_list
        );

    }

    void LoadNodeAndEdgeInformation() {
        uint32_t number_of_coordinates = *static_cast<unsigned *>(
            SharedMemoryFactory::Get(COORDINATE_LIST_SIZE)->Ptr()
        );
        FixedPointCoordinate * coordinate_list_ptr = static_cast<FixedPointCoordinate *>(
            SharedMemoryFactory::Get(COORDINATE_LIST)->Ptr()
        );
        typename ShM<FixedPointCoordinate, true>::vector coordinate_list(
                coordinate_list_ptr,
                number_of_coordinates
        );
        m_coordinate_list.swap( coordinate_list );

        uint32_t number_of_turn_instructions = *static_cast<unsigned *>(
            SharedMemoryFactory::Get(TURN_INSTRUCTION_LIST_SIZE)->Ptr()
        );

        TurnInstruction * turn_instruction_list_ptr = static_cast<TurnInstruction *>(
            SharedMemoryFactory::Get(TURN_INSTRUCTION_LIST)->Ptr()
        );

        typename ShM<TurnInstruction, true>::vector turn_instruction_list(
            turn_instruction_list_ptr,
            number_of_turn_instructions
        );

        m_turn_instruction_list.swap(turn_instruction_list);
    }

    void LoadViaNodeList() {
        uint32_t number_of_via_nodes = * static_cast<unsigned *> (
            SharedMemoryFactory::Get(VIA_NODE_LIST_SIZE)->Ptr()
        );
        NodeID * via_node_list_ptr = static_cast<NodeID *>(
            SharedMemoryFactory::Get(VIA_NODE_LIST)->Ptr()
        );
        typename ShM<NodeID, true>::vector via_node_list(
            via_node_list_ptr,
            number_of_via_nodes
        );
        m_via_node_list.swap(via_node_list);
    }

    void LoadNames() {

    }

public:
    SharedDataFacade(
        const IniFile & server_config,
        const boost::filesystem::path base_path
     ) {
        //check contents of config file
        if ( !server_config.Holds("ramIndex") ) {
            throw OSRMException("no nodes file name in server ini");
        }

        //generate paths of data files
        boost::filesystem::path ram_index_path = boost::filesystem::absolute(
                server_config.GetParameter("ramIndex"),
                base_path
        );

        //load data
        SimpleLogger().Write() << "loading graph data";
        LoadGraph();
        LoadNodeAndEdgeInformation();
        LoadRTree(ram_index_path);
        LoadTimestamp();
        LoadViaNodeList();
        LoadNames();
    }

    //search graph access
    unsigned GetNumberOfNodes() const {
        return m_query_graph->GetNumberOfNodes();
    }

    unsigned GetNumberOfEdges() const {
        return m_query_graph->GetNumberOfEdges();
    }

    unsigned GetOutDegree( const NodeID n ) const {
        return m_query_graph->GetOutDegree(n);
    }

    NodeID GetTarget( const EdgeID e ) const {
        return m_query_graph->GetTarget(e); }

    EdgeDataT &GetEdgeData( const EdgeID e ) {
        return m_query_graph->GetEdgeData(e);
    }

    // const EdgeDataT &GetEdgeData( const EdgeID e ) const {
    //     return m_query_graph->GetEdgeData(e);
    // }

    EdgeID BeginEdges( const NodeID n ) const {
        return m_query_graph->BeginEdges(n);
    }

    EdgeID EndEdges( const NodeID n ) const {
        return m_query_graph->EndEdges(n);
    }

    //searches for a specific edge
    EdgeID FindEdge( const NodeID from, const NodeID to ) const {
        return m_query_graph->FindEdge(from, to);
    }

    EdgeID FindEdgeInEitherDirection(
        const NodeID from,
        const NodeID to
    ) const {
        return m_query_graph->FindEdgeInEitherDirection(from, to);
    }

    EdgeID FindEdgeIndicateIfReverse(
        const NodeID from,
        const NodeID to,
        bool & result
    ) const {
        return m_query_graph->FindEdgeIndicateIfReverse(from, to, result);
    }

    //node and edge information access
    FixedPointCoordinate GetCoordinateOfNode(
        const unsigned id
    ) const {
        const NodeID node = m_via_node_list.at(id);
        return m_coordinate_list.at(node);
    };

    TurnInstruction GetTurnInstructionForEdgeID(
        const unsigned id
    ) const {
        return m_turn_instruction_list.at(id);
    }

    bool LocateClosestEndPointForCoordinate(
        const FixedPointCoordinate& input_coordinate,
        FixedPointCoordinate& result,
        const unsigned zoom_level = 18
    ) const {
        return  m_static_rtree->LocateClosestEndPointForCoordinate(
                    input_coordinate,
                    result,
                    zoom_level
                );
    }

    bool FindPhantomNodeForCoordinate(
        const FixedPointCoordinate & input_coordinate,
        PhantomNode & resulting_phantom_node,
        const unsigned zoom_level
    ) const {
        return  m_static_rtree->FindPhantomNodeForCoordinate(
                    input_coordinate,
                    resulting_phantom_node,
                    zoom_level
                );
    }

    unsigned GetCheckSum() const { return m_check_sum; }

    unsigned GetNameIndexFromEdgeID(const unsigned id) const { return 0; };

    void GetName(
        const unsigned name_id,
        std::string & result
    ) const { return; };

    std::string GetTimestamp() const {
        return m_timestamp;
    }
};

#endif  // SHARED_DATA_FACADE
