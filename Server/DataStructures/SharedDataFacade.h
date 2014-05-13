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

#ifndef SHARED_DATA_FACADE_H
#define SHARED_DATA_FACADE_H

// implements all data storage when shared memory _IS_ used

#include "BaseDataFacade.h"
#include "SharedDataType.h"

#include "../../DataStructures/StaticGraph.h"
#include "../../DataStructures/StaticRTree.h"
#include "../../Util/BoostFileSystemFix.h"
#include "../../Util/ProgramOptions.h"
#include "../../Util/SimpleLogger.h"

#include <algorithm>
#include <memory>

template <class EdgeDataT> class SharedDataFacade : public BaseDataFacade<EdgeDataT>
{

  private:
    typedef EdgeDataT EdgeData;
    typedef BaseDataFacade<EdgeData> super;
    typedef StaticGraph<EdgeData, true> QueryGraph;
    typedef typename StaticGraph<EdgeData, true>::NodeArrayEntry GraphNode;
    typedef typename StaticGraph<EdgeData, true>::EdgeArrayEntry GraphEdge;
    typedef typename QueryGraph::InputEdge InputEdge;
    typedef typename super::RTreeLeaf RTreeLeaf;
    typedef typename StaticRTree<RTreeLeaf, ShM<FixedPointCoordinate, true>::vector, true>::TreeNode
    RTreeNode;

    SharedDataLayout *data_layout;
    char *shared_memory;
    SharedDataTimestamp *data_timestamp_ptr;

    SharedDataType CURRENT_LAYOUT;
    SharedDataType CURRENT_DATA;
    unsigned CURRENT_TIMESTAMP;

    unsigned m_check_sum;
    unsigned m_number_of_nodes;
    std::shared_ptr<QueryGraph> m_query_graph;
    std::shared_ptr<SharedMemory> m_layout_memory;
    std::shared_ptr<SharedMemory> m_large_memory;
    std::string m_timestamp;

    std::shared_ptr<ShM<FixedPointCoordinate, true>::vector> m_coordinate_list;
    ShM<NodeID, true>::vector m_via_node_list;
    ShM<unsigned, true>::vector m_name_ID_list;
    ShM<TurnInstruction, true>::vector m_turn_instruction_list;
    ShM<char, true>::vector m_names_char_list;
    ShM<unsigned, true>::vector m_name_begin_indices;
    ShM<bool, true>::vector m_egde_is_compressed;
    ShM<unsigned, true>::vector m_geometry_indices;
    ShM<unsigned, true>::vector m_geometry_list;

    std::shared_ptr<StaticRTree<RTreeLeaf, ShM<FixedPointCoordinate, true>::vector, true>>
    m_static_rtree;

    void LoadTimestamp()
    {
        char *timestamp_ptr = shared_memory + data_layout->GetTimeStampOffset();
        m_timestamp.resize(data_layout->timestamp_length);
        std::copy(
            timestamp_ptr, timestamp_ptr + data_layout->timestamp_length, m_timestamp.begin());
    }

    void LoadRTree(const boost::filesystem::path &file_index_path)
    {
        BOOST_ASSERT_MSG(!m_coordinate_list->empty(), "coordinates must be loaded before r-tree");

        RTreeNode *tree_ptr = (RTreeNode *)(shared_memory + data_layout->GetRSearchTreeOffset());
        m_static_rtree =
            std::make_shared<StaticRTree<RTreeLeaf, ShM<FixedPointCoordinate, true>::vector, true>>(
                tree_ptr, data_layout->r_search_tree_size, file_index_path, m_coordinate_list);
    }

    void LoadGraph()
    {
        m_number_of_nodes = data_layout->graph_node_list_size;
        GraphNode *graph_nodes_ptr =
            (GraphNode *)(shared_memory + data_layout->GetGraphNodeListOffset());

        GraphEdge *graph_edges_ptr =
            (GraphEdge *)(shared_memory + data_layout->GetGraphEdgeListOffset());

        typename ShM<GraphNode, true>::vector node_list(graph_nodes_ptr,
                                                        data_layout->graph_node_list_size);
        typename ShM<GraphEdge, true>::vector edge_list(graph_edges_ptr,
                                                        data_layout->graph_edge_list_size);
        m_query_graph.reset(new QueryGraph(node_list, edge_list));
    }

    void LoadNodeAndEdgeInformation()
    {

        FixedPointCoordinate *coordinate_list_ptr =
            (FixedPointCoordinate *)(shared_memory + data_layout->GetCoordinateListOffset());
        m_coordinate_list = std::make_shared<ShM<FixedPointCoordinate, true>::vector>(
            coordinate_list_ptr, data_layout->coordinate_list_size);

        TurnInstruction *turn_instruction_list_ptr =
            (TurnInstruction *)(shared_memory + data_layout->GetTurnInstructionListOffset());
        typename ShM<TurnInstruction, true>::vector turn_instruction_list(
            turn_instruction_list_ptr, data_layout->turn_instruction_list_size);
        m_turn_instruction_list.swap(turn_instruction_list);

        unsigned *name_id_list_ptr =
            (unsigned *)(shared_memory + data_layout->GetNameIDListOffset());
        typename ShM<unsigned, true>::vector name_id_list(name_id_list_ptr,
                                                          data_layout->name_id_list_size);
        m_name_ID_list.swap(name_id_list);
    }

    void LoadViaNodeList()
    {
        NodeID *via_node_list_ptr = (NodeID *)(shared_memory + data_layout->GetViaNodeListOffset());
        typename ShM<NodeID, true>::vector via_node_list(via_node_list_ptr,
                                                         data_layout->via_node_list_size);
        m_via_node_list.swap(via_node_list);
    }

    void LoadNames()
    {
        unsigned *street_names_index_ptr =
            (unsigned *)(shared_memory + data_layout->GetNameIndexOffset());
        typename ShM<unsigned, true>::vector name_begin_indices(street_names_index_ptr,
                                                                data_layout->name_index_list_size);
        m_name_begin_indices.swap(name_begin_indices);

        char *names_list_ptr = (char *)(shared_memory + data_layout->GetNameListOffset());
        typename ShM<char, true>::vector names_char_list(names_list_ptr,
                                                         data_layout->name_char_list_size);
        m_names_char_list.swap(names_char_list);
    }

    void LoadGeometries()
    {
        unsigned *geometries_compressed_ptr =
            (unsigned *)(shared_memory + data_layout->GetGeometriesIndicatorOffset());
        typename ShM<bool, true>::vector egde_is_compressed(geometries_compressed_ptr,
                                                            data_layout->geometries_indicators);
        m_egde_is_compressed.swap(egde_is_compressed);

        unsigned *geometries_index_ptr =
            (unsigned *)(shared_memory + data_layout->GetGeometriesIndexListOffset());
        typename ShM<unsigned, true>::vector geometry_begin_indices(
            geometries_index_ptr, data_layout->geometries_index_list_size);
        m_geometry_indices.swap(geometry_begin_indices);

        unsigned *geometries_list_ptr =
            (unsigned *)(shared_memory + data_layout->GetGeometryListOffset());
        typename ShM<unsigned, true>::vector geometry_list(geometries_list_ptr,
                                                           data_layout->geometries_list_size);
        m_geometry_list.swap(geometry_list);
    }

  public:
    SharedDataFacade()
    {
        data_timestamp_ptr = (SharedDataTimestamp *)SharedMemoryFactory::Get(
                                 CURRENT_REGIONS, sizeof(SharedDataTimestamp), false, false)->Ptr();

        CURRENT_LAYOUT = LAYOUT_NONE;
        CURRENT_DATA = DATA_NONE;
        CURRENT_TIMESTAMP = 0;

        // load data
        CheckAndReloadFacade();
    }

    void CheckAndReloadFacade()
    {
        if (CURRENT_LAYOUT != data_timestamp_ptr->layout ||
            CURRENT_DATA != data_timestamp_ptr->data ||
            CURRENT_TIMESTAMP != data_timestamp_ptr->timestamp)
        {
            // release the previous shared memory segments
            SharedMemory::Remove(CURRENT_LAYOUT);
            SharedMemory::Remove(CURRENT_DATA);

            CURRENT_LAYOUT = data_timestamp_ptr->layout;
            CURRENT_DATA = data_timestamp_ptr->data;
            CURRENT_TIMESTAMP = data_timestamp_ptr->timestamp;

            m_layout_memory.reset(SharedMemoryFactory::Get(CURRENT_LAYOUT));

            data_layout = (SharedDataLayout *)(m_layout_memory->Ptr());
            boost::filesystem::path ram_index_path(data_layout->ram_index_file_name);
            if (!boost::filesystem::exists(ram_index_path))
            {
                throw OSRMException("no leaf index file given. "
                                    "Is any data loaded into shared memory?");
            }

            m_large_memory.reset(SharedMemoryFactory::Get(CURRENT_DATA));
            shared_memory = (char *)(m_large_memory->Ptr());

            LoadGraph();
            LoadNodeAndEdgeInformation();
            LoadGeometries();
            LoadRTree(ram_index_path);
            LoadTimestamp();
            LoadViaNodeList();
            LoadNames();

            data_layout->PrintInformation();
        }
    }

    // search graph access
    unsigned GetNumberOfNodes() const { return m_query_graph->GetNumberOfNodes(); }

    unsigned GetNumberOfEdges() const { return m_query_graph->GetNumberOfEdges(); }

    unsigned GetOutDegree(const NodeID n) const { return m_query_graph->GetOutDegree(n); }

    NodeID GetTarget(const EdgeID e) const { return m_query_graph->GetTarget(e); }

    EdgeDataT &GetEdgeData(const EdgeID e) { return m_query_graph->GetEdgeData(e); }

    // const EdgeDataT &GetEdgeData( const EdgeID e ) const {
    //     return m_query_graph->GetEdgeData(e);
    // }

    EdgeID BeginEdges(const NodeID n) const { return m_query_graph->BeginEdges(n); }

    EdgeID EndEdges(const NodeID n) const { return m_query_graph->EndEdges(n); }

    EdgeRange GetAdjacentEdgeRange(const NodeID node) const { return m_query_graph->GetAdjacentEdgeRange(node); };

    // searches for a specific edge
    EdgeID FindEdge(const NodeID from, const NodeID to) const
    {
        return m_query_graph->FindEdge(from, to);
    }

    EdgeID FindEdgeInEitherDirection(const NodeID from, const NodeID to) const
    {
        return m_query_graph->FindEdgeInEitherDirection(from, to);
    }

    EdgeID FindEdgeIndicateIfReverse(const NodeID from, const NodeID to, bool &result) const
    {
        return m_query_graph->FindEdgeIndicateIfReverse(from, to, result);
    }

    // node and edge information access
    FixedPointCoordinate GetCoordinateOfNode(const unsigned id) const
    {
        return m_coordinate_list->at(id);
    };

    virtual bool EdgeIsCompressed(const unsigned id) const { return m_egde_is_compressed.at(id); }

    virtual void GetUncompressedGeometry(const unsigned id, std::vector<unsigned> &result_nodes)
        const
    {
        const unsigned begin = m_geometry_indices.at(id);
        const unsigned end = m_geometry_indices.at(id + 1);

        result_nodes.clear();
        result_nodes.insert(
            result_nodes.begin(), m_geometry_list.begin() + begin, m_geometry_list.begin() + end);
    }

    virtual unsigned GetGeometryIndexForEdgeID(const unsigned id) const
    {
        return m_via_node_list.at(id);
    }

    TurnInstruction GetTurnInstructionForEdgeID(const unsigned id) const
    {
        return m_turn_instruction_list.at(id);
    }

    bool LocateClosestEndPointForCoordinate(const FixedPointCoordinate &input_coordinate,
                                            FixedPointCoordinate &result,
                                            const unsigned zoom_level = 18) const
    {
        return m_static_rtree->LocateClosestEndPointForCoordinate(
            input_coordinate, result, zoom_level);
    }

    bool FindPhantomNodeForCoordinate(const FixedPointCoordinate &input_coordinate,
                                      PhantomNode &resulting_phantom_node,
                                      const unsigned zoom_level) const
    {
        const bool found = m_static_rtree->FindPhantomNodeForCoordinate(
            input_coordinate, resulting_phantom_node, zoom_level);
        return found;
    }

    unsigned GetCheckSum() const { return m_check_sum; }

    unsigned GetNameIndexFromEdgeID(const unsigned id) const
    {
        return m_name_ID_list.at(id);
    };

    void GetName(const unsigned name_id, std::string &result) const
    {
        if (UINT_MAX == name_id)
        {
            result = "";
            return;
        }
        BOOST_ASSERT_MSG(name_id < m_name_begin_indices.size(), "name id too high");
        const unsigned begin_index = m_name_begin_indices[name_id];
        const unsigned end_index = m_name_begin_indices[name_id + 1];
        BOOST_ASSERT_MSG(begin_index <= m_names_char_list.size(), "begin index of name too high");
        BOOST_ASSERT_MSG(end_index <= m_names_char_list.size(), "end index of name too high");

        BOOST_ASSERT_MSG(begin_index <= end_index, "string ends before begin");
        result.clear();
        result.resize(end_index - begin_index);
        std::copy(m_names_char_list.begin() + begin_index,
                  m_names_char_list.begin() + end_index,
                  result.begin());
    }

    std::string GetTimestamp() const { return m_timestamp; }
};

#endif // SHARED_DATA_FACADE_H
