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

#ifndef INTERNAL_DATA_FACADE
#define INTERNAL_DATA_FACADE

//implements all data storage when shared memory is _NOT_ used

#include "BaseDataFacade.h"

#include "../../DataStructures/Coordinate.h"
#include "../../DataStructures/QueryNode.h"
#include "../../DataStructures/QueryEdge.h"
#include "../../DataStructures/SharedMemoryVectorWrapper.h"
#include "../../DataStructures/StaticGraph.h"
#include "../../DataStructures/StaticRTree.h"
#include "../../Util/BoostFileSystemFix.h"
#include "../../Util/GraphLoader.h"
#include "../../Util/IniFile.h"
#include "../../Util/SimpleLogger.h"

template<class EdgeDataT>
class InternalDataFacade : public BaseDataFacade<EdgeDataT> {

private:
    typedef BaseDataFacade<EdgeDataT>             super;
    typedef StaticGraph<typename super::EdgeData> QueryGraph;
    typedef typename QueryGraph::InputEdge        InputEdge;
    typedef typename super::RTreeLeaf             RTreeLeaf;

    InternalDataFacade() { }

    unsigned                          m_check_sum;
    unsigned                          m_number_of_nodes;
    QueryGraph                      * m_query_graph;
    std::string                       m_timestamp;

    ShM<FixedPointCoordinate, false>::vector m_coordinate_list;
    ShM<NodeID, false>::vector               m_via_node_list;
    ShM<unsigned, false>::vector             m_name_ID_list;
    ShM<TurnInstruction, false>::vector      m_turn_instruction_list;
    ShM<char, false>::vector                 m_names_char_list;
    ShM<unsigned, false>::vector             m_name_begin_indices;

    StaticRTree<RTreeLeaf, false>                   * m_static_rtree;


    void LoadTimestamp(const boost::filesystem::path & timestamp_path) {
        if( boost::filesystem::exists(timestamp_path) ) {
            SimpleLogger().Write() << "Loading Timestamp";
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
    }

    void LoadGraph(const boost::filesystem::path & hsgr_path) {
        typename ShM<typename QueryGraph::_StrNode, false>::vector node_list;
        typename ShM<typename QueryGraph::_StrEdge, false>::vector edge_list;

        SimpleLogger().Write() << "loading graph from " << hsgr_path.string();

        m_number_of_nodes = readHSGRFromStream(
            hsgr_path,
            node_list,
            edge_list,
            &m_check_sum
        );

        BOOST_ASSERT_MSG(0 != node_list.size(), "node list empty");
        BOOST_ASSERT_MSG(0 != edge_list.size(), "edge list empty");
        SimpleLogger().Write() << "loaded " << node_list.size() << " nodes and " << edge_list.size() << " edges";
        m_query_graph = new QueryGraph(node_list, edge_list);

        BOOST_ASSERT_MSG(0 == node_list.size(), "node list not flushed");
        BOOST_ASSERT_MSG(0 == edge_list.size(), "edge list not flushed");
        SimpleLogger().Write() << "Data checksum is " << m_check_sum;
    }

    void LoadNodeAndEdgeInformation(
        const boost::filesystem::path nodes_file,
        const boost::filesystem::path edges_file
    ) {
        boost::filesystem::ifstream nodes_input_stream(
            nodes_file,
            std::ios::binary
        );

        SimpleLogger().Write(logDEBUG) << "Loading node data";
        NodeInfo current_node;
        while(!nodes_input_stream.eof()) {
            nodes_input_stream.read((char *)&current_node, sizeof(NodeInfo));
            m_coordinate_list.push_back(
                FixedPointCoordinate(
                    current_node.lat,
                    current_node.lon
                )
            );
        }
        std::vector<FixedPointCoordinate>(m_coordinate_list).swap(m_coordinate_list);
        nodes_input_stream.close();

        SimpleLogger().Write(logDEBUG) << "Loading edge data";
        boost::filesystem::ifstream edges_input_stream(
            edges_file,
            std::ios::binary
        );
        unsigned number_of_edges = 0;
        edges_input_stream.read((char*)&number_of_edges, sizeof(unsigned));
        m_via_node_list.resize(number_of_edges);
        m_name_ID_list.resize(number_of_edges);
        m_turn_instruction_list.resize(number_of_edges);

        OriginalEdgeData current_edge_data;
        for(unsigned i = 0; i < number_of_edges; ++i) {
            edges_input_stream.read(
                (char*)&(current_edge_data),
                sizeof(OriginalEdgeData)
            );
            m_via_node_list[i] = current_edge_data.viaNode;
            m_name_ID_list[i]  = current_edge_data.nameID;
            m_turn_instruction_list[i] = current_edge_data.turnInstruction;
        }
        edges_input_stream.close();
    }

    void LoadRTree(
        const boost::filesystem::path & ram_index_path,
        const boost::filesystem::path & file_index_path
    ) {
        m_static_rtree = new StaticRTree<RTreeLeaf>(
            ram_index_path,
            file_index_path
        );
    }

    void LoadStreetNames(
        const boost::filesystem::path & names_file
    ) {
        boost::filesystem::ifstream name_stream(names_file, std::ios::binary);
        unsigned size = 0;
        name_stream.read((char *)&size, sizeof(unsigned));
        BOOST_ASSERT_MSG(0 != size, "name file broken");

        m_name_begin_indices.resize(size);
        name_stream.read((char*)&m_name_begin_indices[0], size*sizeof(unsigned));
        name_stream.read((char *)&size, sizeof(unsigned));
        BOOST_ASSERT_MSG(0 != size, "name file broken");

        m_names_char_list.resize(size+1); //+1 is sentinel/dummy element
        name_stream.read((char *)&m_names_char_list[0], size*sizeof(char));
        BOOST_ASSERT_MSG(0 != m_names_char_list.size(), "could not load any names");

        name_stream.close();
    }
public:
    ~InternalDataFacade() {
        delete m_query_graph;
        delete m_static_rtree;
    }

    InternalDataFacade(
        const IniFile & server_config,
        const boost::filesystem::path & base_path
    ) {
        //check contents of config file
        if ( !server_config.Holds("hsgrData")) {
            throw OSRMException("no ram index file name in server ini");
        }
        if ( !server_config.Holds("ramIndex") ) {
            throw OSRMException("no mem index file name in server ini");
        }
        if ( !server_config.Holds("fileIndex") ) {
            throw OSRMException("no nodes file name in server ini");
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
        boost::filesystem::path file_index_path = boost::filesystem::absolute(
                server_config.GetParameter("fileIndex"),
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

        //load data
        SimpleLogger().Write() << "loading graph data";
        LoadGraph(hsgr_path);
        SimpleLogger().Write() << "loading egde information";
        LoadNodeAndEdgeInformation(node_data_path, edge_data_path);
        SimpleLogger().Write() << "loading r-tree";
        LoadRTree(ram_index_path, file_index_path);
        SimpleLogger().Write() << "loading timestamp";
        LoadTimestamp(timestamp_path);
        SimpleLogger().Write() << "loading street names";
        LoadStreetNames(name_data_path);
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

    const EdgeDataT &GetEdgeData( const EdgeID e ) const {
        return m_query_graph->GetEdgeData(e);
    }

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

    unsigned GetNameIndexFromEdgeID(const unsigned id) const {
        return m_name_ID_list.at(id);
    };

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

#endif  // INTERNAL_DATA_FACADE
