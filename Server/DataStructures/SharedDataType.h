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

#ifndef SHARED_DATA_TYPE_H_
#define SHARED_DATA_TYPE_H_

#include "BaseDataFacade.h"

#include "../../DataStructures/QueryEdge.h"
#include "../../DataStructures/StaticGraph.h"
#include "../../DataStructures/StaticRTree.h"
#include "../../DataStructures/TurnInstructions.h"

#include "../../typedefs.h"

#include <osrm/Coordinate.h>

#include <cstdint>

typedef BaseDataFacade<QueryEdge::EdgeData>::RTreeLeaf RTreeLeaf;
typedef StaticRTree<RTreeLeaf, ShM<FixedPointCoordinate, true>::vector, true>::TreeNode RTreeNode;
typedef StaticGraph<QueryEdge::EdgeData> QueryGraph;

struct SharedDataLayout
{
    uint64_t name_index_list_size;
    uint64_t name_char_list_size;
    uint64_t name_id_list_size;
    uint64_t via_node_list_size;
    uint64_t graph_node_list_size;
    uint64_t graph_edge_list_size;
    uint64_t coordinate_list_size;
    uint64_t turn_instruction_list_size;
    uint64_t r_search_tree_size;
    uint64_t geometries_index_list_size;
    uint64_t geometries_list_size;
    uint64_t geometries_indicators;

    unsigned checksum;
    unsigned timestamp_length;

    char ram_index_file_name[1024];

    SharedDataLayout()
        : name_index_list_size(0), name_char_list_size(0), name_id_list_size(0),
          via_node_list_size(0), graph_node_list_size(0), graph_edge_list_size(0),
          coordinate_list_size(0), turn_instruction_list_size(0), r_search_tree_size(0),
          geometries_index_list_size(0), geometries_list_size(0), geometries_indicators(0),
          checksum(0), timestamp_length(0)

    {
        ram_index_file_name[0] = '\0';
    }

    void PrintInformation() const
    {
        SimpleLogger().Write(logDEBUG) << "-";
        SimpleLogger().Write(logDEBUG) << "name_index_list_size:       " << name_index_list_size;
        SimpleLogger().Write(logDEBUG) << "name_char_list_size:        " << name_char_list_size;
        SimpleLogger().Write(logDEBUG) << "name_id_list_size:          " << name_id_list_size;
        SimpleLogger().Write(logDEBUG) << "via_node_list_size:         " << via_node_list_size;
        SimpleLogger().Write(logDEBUG) << "graph_node_list_size:       " << graph_node_list_size;
        SimpleLogger().Write(logDEBUG) << "graph_edge_list_size:       " << graph_edge_list_size;
        SimpleLogger().Write(logDEBUG) << "timestamp_length:           " << timestamp_length;
        SimpleLogger().Write(logDEBUG) << "coordinate_list_size:       " << coordinate_list_size;
        SimpleLogger().Write(logDEBUG)
            << "turn_instruction_list_size: " << turn_instruction_list_size;
        SimpleLogger().Write(logDEBUG) << "r_search_tree_size:         " << r_search_tree_size;
        SimpleLogger().Write(logDEBUG) << "geometries_indicators:      " << geometries_indicators
                                       << "/" << ((geometries_indicators / 8) + 1);
        SimpleLogger().Write(logDEBUG)
            << "geometries_index_list_size: " << geometries_index_list_size;
        SimpleLogger().Write(logDEBUG) << "geometries_list_size:       " << geometries_list_size;
        SimpleLogger().Write(logDEBUG) << "sizeof(checksum):           " << sizeof(checksum);
        SimpleLogger().Write(logDEBUG) << "ram index file name:        " << ram_index_file_name;
    }

    uint64_t GetSizeOfLayout() const
    {
        uint64_t result =
            (name_index_list_size * sizeof(unsigned)) + (name_char_list_size * sizeof(char)) +
            (name_id_list_size * sizeof(unsigned)) + (via_node_list_size * sizeof(NodeID)) +
            (graph_node_list_size * sizeof(QueryGraph::NodeArrayEntry)) +
            (graph_edge_list_size * sizeof(QueryGraph::EdgeArrayEntry)) +
            (timestamp_length * sizeof(char)) +
            (coordinate_list_size * sizeof(FixedPointCoordinate)) +
            (turn_instruction_list_size * sizeof(TurnInstructionsClass)) +
            (r_search_tree_size * sizeof(RTreeNode)) +
            (geometries_indicators / 32 + 1) * sizeof(unsigned) +
            (geometries_index_list_size * sizeof(unsigned)) +
            (geometries_list_size * sizeof(unsigned)) + sizeof(checksum) + 1024 * sizeof(char);
        return result;
    }

    uint64_t GetNameIndexOffset() const { return 0; }
    uint64_t GetNameListOffset() const
    {
        uint64_t result = (name_index_list_size * sizeof(unsigned));
        return result;
    }
    uint64_t GetNameIDListOffset() const
    {
        uint64_t result =
            (name_index_list_size * sizeof(unsigned)) + (name_char_list_size * sizeof(char));
        return result;
    }
    uint64_t GetViaNodeListOffset() const
    {
        uint64_t result = (name_index_list_size * sizeof(unsigned)) +
                          (name_char_list_size * sizeof(char)) +
                          (name_id_list_size * sizeof(unsigned));
        return result;
    }
    uint64_t GetGraphNodeListOffset() const
    {
        uint64_t result =
            (name_index_list_size * sizeof(unsigned)) + (name_char_list_size * sizeof(char)) +
            (name_id_list_size * sizeof(unsigned)) + (via_node_list_size * sizeof(NodeID));
        return result;
    }
    uint64_t GetGraphEdgeListOffset() const
    {
        uint64_t result =
            (name_index_list_size * sizeof(unsigned)) + (name_char_list_size * sizeof(char)) +
            (name_id_list_size * sizeof(unsigned)) + (via_node_list_size * sizeof(NodeID)) +
            (graph_node_list_size * sizeof(QueryGraph::NodeArrayEntry));
        return result;
    }
    uint64_t GetTimeStampOffset() const
    {
        uint64_t result =
            (name_index_list_size * sizeof(unsigned)) + (name_char_list_size * sizeof(char)) +
            (name_id_list_size * sizeof(unsigned)) + (via_node_list_size * sizeof(NodeID)) +
            (graph_node_list_size * sizeof(QueryGraph::NodeArrayEntry)) +
            (graph_edge_list_size * sizeof(QueryGraph::EdgeArrayEntry));
        return result;
    }
    uint64_t GetCoordinateListOffset() const
    {
        uint64_t result =
            (name_index_list_size * sizeof(unsigned)) + (name_char_list_size * sizeof(char)) +
            (name_id_list_size * sizeof(unsigned)) + (via_node_list_size * sizeof(NodeID)) +
            (graph_node_list_size * sizeof(QueryGraph::NodeArrayEntry)) +
            (graph_edge_list_size * sizeof(QueryGraph::EdgeArrayEntry)) +
            (timestamp_length * sizeof(char));
        return result;
    }
    uint64_t GetTurnInstructionListOffset() const
    {
        uint64_t result =
            (name_index_list_size * sizeof(unsigned)) + (name_char_list_size * sizeof(char)) +
            (name_id_list_size * sizeof(unsigned)) + (via_node_list_size * sizeof(NodeID)) +
            (graph_node_list_size * sizeof(QueryGraph::NodeArrayEntry)) +
            (graph_edge_list_size * sizeof(QueryGraph::EdgeArrayEntry)) +
            (timestamp_length * sizeof(char)) +
            (coordinate_list_size * sizeof(FixedPointCoordinate));
        return result;
    }
    uint64_t GetRSearchTreeOffset() const
    {
        uint64_t result =
            (name_index_list_size * sizeof(unsigned)) + (name_char_list_size * sizeof(char)) +
            (name_id_list_size * sizeof(unsigned)) + (via_node_list_size * sizeof(NodeID)) +
            (graph_node_list_size * sizeof(QueryGraph::NodeArrayEntry)) +
            (graph_edge_list_size * sizeof(QueryGraph::EdgeArrayEntry)) +
            (timestamp_length * sizeof(char)) +
            (coordinate_list_size * sizeof(FixedPointCoordinate)) +
            (turn_instruction_list_size * sizeof(TurnInstructionsClass));
        return result;
    }
    uint64_t GetGeometriesIndicatorOffset() const
    {
        uint64_t result =
            (name_index_list_size * sizeof(unsigned)) + (name_char_list_size * sizeof(char)) +
            (name_id_list_size * sizeof(unsigned)) + (via_node_list_size * sizeof(NodeID)) +
            (graph_node_list_size * sizeof(QueryGraph::NodeArrayEntry)) +
            (graph_edge_list_size * sizeof(QueryGraph::EdgeArrayEntry)) +
            (timestamp_length * sizeof(char)) +
            (coordinate_list_size * sizeof(FixedPointCoordinate)) +
            (turn_instruction_list_size * sizeof(TurnInstructionsClass)) +
            (r_search_tree_size * sizeof(RTreeNode));
        return result;
    }

    uint64_t GetGeometriesIndexListOffset() const
    {
        uint64_t result =
            (name_index_list_size * sizeof(unsigned)) + (name_char_list_size * sizeof(char)) +
            (name_id_list_size * sizeof(unsigned)) + (via_node_list_size * sizeof(NodeID)) +
            (graph_node_list_size * sizeof(QueryGraph::NodeArrayEntry)) +
            (graph_edge_list_size * sizeof(QueryGraph::EdgeArrayEntry)) +
            (timestamp_length * sizeof(char)) +
            (coordinate_list_size * sizeof(FixedPointCoordinate)) +
            (turn_instruction_list_size * sizeof(TurnInstructionsClass)) +
            (r_search_tree_size * sizeof(RTreeNode)) +
            (geometries_indicators / 32 + 1) * sizeof(unsigned);
        return result;
    }

    uint64_t GetGeometryListOffset() const
    {
        uint64_t result =
            (name_index_list_size * sizeof(unsigned)) + (name_char_list_size * sizeof(char)) +
            (name_id_list_size * sizeof(unsigned)) + (via_node_list_size * sizeof(NodeID)) +
            (graph_node_list_size * sizeof(QueryGraph::NodeArrayEntry)) +
            (graph_edge_list_size * sizeof(QueryGraph::EdgeArrayEntry)) +
            (timestamp_length * sizeof(char)) +
            (coordinate_list_size * sizeof(FixedPointCoordinate)) +
            (turn_instruction_list_size * sizeof(TurnInstructionsClass)) +
            (r_search_tree_size * sizeof(RTreeNode)) +
            (geometries_indicators / 32 + 1) * sizeof(unsigned) +
            (geometries_index_list_size * sizeof(unsigned));
        return result;
    }
    uint64_t GetChecksumOffset() const
    {
        uint64_t result =
            (name_index_list_size * sizeof(unsigned)) + (name_char_list_size * sizeof(char)) +
            (name_id_list_size * sizeof(unsigned)) + (via_node_list_size * sizeof(NodeID)) +
            (graph_node_list_size * sizeof(QueryGraph::NodeArrayEntry)) +
            (graph_edge_list_size * sizeof(QueryGraph::EdgeArrayEntry)) +
            (timestamp_length * sizeof(char)) +
            (coordinate_list_size * sizeof(FixedPointCoordinate)) +
            (turn_instruction_list_size * sizeof(TurnInstructionsClass)) +
            (r_search_tree_size * sizeof(RTreeNode)) +
            (geometries_indicators / 32 + 1) * sizeof(unsigned) +
            (geometries_index_list_size * sizeof(unsigned)) +
            (geometries_list_size * sizeof(unsigned));
        return result;
    }
};

enum SharedDataType
{ CURRENT_REGIONS,
  LAYOUT_1,
  DATA_1,
  LAYOUT_2,
  DATA_2,
  LAYOUT_NONE,
  DATA_NONE };

struct SharedDataTimestamp
{
    SharedDataType layout;
    SharedDataType data;
    unsigned timestamp;
};

#endif /* SHARED_DATA_TYPE_H_ */
