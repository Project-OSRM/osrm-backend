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
#include "SharedDataType.h"

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

    SharedDataLayout * data_layout;
    char             * shared_memory;

    unsigned                                m_check_sum;
    unsigned                                m_number_of_nodes;
    QueryGraph                            * m_query_graph;
    std::string                             m_timestamp;

    ShM<FixedPointCoordinate, true>::vector m_coordinate_list;
    ShM<NodeID, true>::vector               m_via_node_list;
    ShM<unsigned, true>::vector             m_name_ID_list;
    ShM<TurnInstruction, true>::vector      m_turn_instruction_list;
    ShM<char, true>::vector                 m_names_char_list;
    ShM<unsigned, true>::vector             m_name_begin_indices;
    StaticRTree<RTreeLeaf, true>          * m_static_rtree;

    SharedDataFacade() { }

    void LoadTimestamp() {
        char * timestamp_ptr = shared_memory + data_layout->GetTimeStampOffset();
        m_timestamp.resize(data_layout->timestamp_length);
        std::copy(
            timestamp_ptr,
            timestamp_ptr+data_layout->timestamp_length,
            m_timestamp.begin()
        );
    }

    void LoadRTree(
        const boost::filesystem::path & file_index_path
    ) {
        RTreeNode * tree_ptr = (RTreeNode *)(
            shared_memory + data_layout->GetRSearchTreeOffset()
        );
        m_static_rtree = new StaticRTree<RTreeLeaf, true>(
            tree_ptr,
            data_layout->r_search_tree_size,
            file_index_path
        );
    }

    void LoadGraph() {
        m_number_of_nodes = data_layout->graph_node_list_size;
        GraphNode * graph_nodes_ptr = (GraphNode *)(
            shared_memory + data_layout->GetGraphNodeListOffset()
        );

        GraphEdge * graph_edges_ptr = (GraphEdge *)(
            shared_memory + data_layout->GetGraphEdgeListOffsett()
        );

        typename ShM<GraphNode, true>::vector node_list(
            graph_nodes_ptr,
            data_layout->graph_node_list_size
        );
        typename ShM<GraphEdge, true>::vector edge_list(
            graph_edges_ptr,
            data_layout->graph_edge_list_size
        );
        m_query_graph = new QueryGraph(
            node_list ,
            edge_list
        );

    }

    void LoadNodeAndEdgeInformation() {

        FixedPointCoordinate * coordinate_list_ptr = (FixedPointCoordinate *)(
            shared_memory + data_layout->GetCoordinateListOffset()
        );
        typename ShM<FixedPointCoordinate, true>::vector coordinate_list(
                coordinate_list_ptr,
                data_layout->coordinate_list_size
        );
        m_coordinate_list.swap( coordinate_list );

        TurnInstruction * turn_instruction_list_ptr = (TurnInstruction *)(
            shared_memory + data_layout->GetTurnInstructionListOffset()
        );
        typename ShM<TurnInstruction, true>::vector turn_instruction_list(
            turn_instruction_list_ptr,
            data_layout->turn_instruction_list_size
        );
        m_turn_instruction_list.swap(turn_instruction_list);

        unsigned * name_id_list_ptr = (unsigned *)(
            shared_memory + data_layout->GetNameIDListOffset()
        );
        typename ShM<unsigned, true>::vector name_id_list(
            name_id_list_ptr,
            data_layout->name_id_list_size
        );
        m_name_ID_list.swap(name_id_list);
    }

    void LoadViaNodeList() {
        NodeID * via_node_list_ptr = (NodeID *)(
            shared_memory + data_layout->GetViaNodeListOffset()
        );
        typename ShM<NodeID, true>::vector via_node_list(
            via_node_list_ptr,
            data_layout->via_node_list_size
        );
        m_via_node_list.swap(via_node_list);
    }

    void LoadNames() {
        unsigned * street_names_index_ptr = (unsigned *)(
            shared_memory + data_layout->GetNameIndexOffset()
        );
        typename ShM<unsigned, true>::vector name_begin_indices(
            street_names_index_ptr,
            data_layout->name_index_list_size
        );
        m_name_begin_indices.swap(name_begin_indices);

        char * names_list_ptr = (char *)(
            shared_memory + data_layout->GetNameListOffset()
        );
        typename ShM<char, true>::vector names_char_list(
            names_list_ptr,
            data_layout->name_char_list_size
        );
        m_names_char_list.swap(names_char_list);
    }

public:
    SharedDataFacade(
        const IniFile & server_config,
        const boost::filesystem::path base_path
     ) {
        //check contents of config file
        if ( !server_config.Holds("fileIndex") ) {
            throw OSRMException("no nodes file name in server ini");
        }

        //generate paths of data files
        boost::filesystem::path ram_index_path = boost::filesystem::absolute(
                server_config.GetParameter("fileIndex"),
                base_path
        );

        data_layout = (SharedDataLayout *)(
            SharedMemoryFactory::Get(LAYOUT_1)->Ptr()
        );

        shared_memory = (char *)(
            SharedMemoryFactory::Get(DATA_1)->Ptr()
        );

        data_layout->PrintInformation();

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

    void GetName( const unsigned name_id, std::string & result ) const {
        if(UINT_MAX == name_id) {
            result = "";
            return;
        }
        BOOST_ASSERT_MSG(
            name_id < m_name_begin_indices.size(),
            "name id too high"
        );
        unsigned begin_index = m_name_begin_indices[name_id];
        unsigned end_index = m_name_begin_indices[name_id+1];
        BOOST_ASSERT_MSG(
            begin_index < m_names_char_list.size(),
            "begin index of name too high"
        );
        BOOST_ASSERT_MSG(
            end_index < m_names_char_list.size(),
            "end index of name too high"
        );

        BOOST_ASSERT_MSG(begin_index <= end_index, "string ends before begin");
        result.clear();
        result.resize(end_index - begin_index);
        std::copy(
            m_names_char_list.begin() + begin_index,
            m_names_char_list.begin() + end_index,
            result.begin()
        );
    }

    std::string GetTimestamp() const {
        return m_timestamp;
    }
};

#endif  // SHARED_DATA_FACADE
