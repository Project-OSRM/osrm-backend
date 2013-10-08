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

#ifndef SHARED_DATA_TYPE_H_
#define SHARED_DATA_TYPE_H_

#include "BaseDataFacade.h"

#include "../../DataStructures/Coordinate.h"
#include "../../DataStructures/QueryEdge.h"
#include "../../DataStructures/StaticGraph.h"
#include "../../DataStructures/StaticRTree.h"
#include "../../DataStructures/TurnInstructions.h"

#include "../../typedefs.h"

#include <boost/integer.hpp>

typedef BaseDataFacade<QueryEdge::EdgeData>::RTreeLeaf RTreeLeaf;
typedef StaticRTree<RTreeLeaf, true>::TreeNode RTreeNode;
typedef StaticGraph<QueryEdge::EdgeData> QueryGraph;

struct SharedDataLayout {
    uint64_t name_index_list_size;
    uint64_t name_char_list_size;
    uint64_t name_id_list_size;
    uint64_t via_node_list_size;
    uint64_t graph_node_list_size;
    uint64_t graph_edge_list_size;
    uint64_t coordinate_list_size;
    uint64_t turn_instruction_list_size;
    uint64_t r_search_tree_size;

    unsigned checksum;
    unsigned timestamp_length;

    SharedDataLayout() :
        name_index_list_size(0),
        name_char_list_size(0),
        name_id_list_size(0),
        via_node_list_size(0),
        graph_node_list_size(0),
        graph_edge_list_size(0),
        coordinate_list_size(0),
        turn_instruction_list_size(0),
        r_search_tree_size(0),
        checksum(0),
        timestamp_length(0)
    { }

    void PrintInformation() const {
        SimpleLogger().Write(logDEBUG) << "-";
        SimpleLogger().Write(logDEBUG) << "name_index_list_size:       " << name_index_list_size;
        SimpleLogger().Write(logDEBUG) << "name_char_list_size:        " << name_char_list_size;
        SimpleLogger().Write(logDEBUG) << "name_id_list_size:          " << name_id_list_size;
        SimpleLogger().Write(logDEBUG) << "via_node_list_size:         " << via_node_list_size;
        SimpleLogger().Write(logDEBUG) << "graph_node_list_size:       " << graph_node_list_size;
        SimpleLogger().Write(logDEBUG) << "graph_edge_list_size:       " << graph_edge_list_size;
        SimpleLogger().Write(logDEBUG) << "timestamp_length:           " << timestamp_length;
        SimpleLogger().Write(logDEBUG) << "coordinate_list_size:       " << coordinate_list_size;
        SimpleLogger().Write(logDEBUG) << "turn_instruction_list_size: " << turn_instruction_list_size;
        SimpleLogger().Write(logDEBUG) << "r_search_tree_size:         " << r_search_tree_size;
        SimpleLogger().Write(logDEBUG) << "sizeof(checksum):           " << sizeof(checksum);
    }

    uint64_t GetSizeOfLayout() const {
        uint64_t result =
            (name_index_list_size       * sizeof(unsigned)            ) +
            (name_char_list_size        * sizeof(char)                ) +
            (name_id_list_size          * sizeof(unsigned)            ) +
            (via_node_list_size         * sizeof(NodeID)              ) +
            (graph_node_list_size       * sizeof(QueryGraph::_StrNode)) +
            (graph_edge_list_size       * sizeof(QueryGraph::_StrEdge)) +
            (timestamp_length           * sizeof(char)                ) +
            (coordinate_list_size       * sizeof(FixedPointCoordinate)) +
            (turn_instruction_list_size * sizeof(TurnInstructions)    ) +
            (r_search_tree_size         * sizeof(RTreeNode)           ) +
            sizeof(checksum);
        return result;
    }

    uint64_t GetNameIndexOffset() const {
        return 0;
    }
    uint64_t GetNameListOffset() const {
        uint64_t result =
            (name_index_list_size           * sizeof(unsigned)            );
        return result;
    }
    uint64_t GetNameIDListOffset() const {
        uint64_t result =
            (name_index_list_size           * sizeof(unsigned)            ) +
            (name_char_list_size            * sizeof(char)                );
        return result;
    }
    uint64_t GetViaNodeListOffset() const {
        uint64_t result =
            (name_index_list_size           * sizeof(unsigned)            ) +
            (name_char_list_size            * sizeof(char)                ) +
            (name_id_list_size          * sizeof(unsigned)            );
        return result;
    }
    uint64_t GetGraphNodeListOffset() const {
        uint64_t result =
            (name_index_list_size           * sizeof(unsigned)            ) +
            (name_char_list_size            * sizeof(char)                ) +
            (name_id_list_size          * sizeof(unsigned)            ) +
            (via_node_list_size         * sizeof(NodeID)              );
        return result;
    }
    uint64_t GetGraphEdgeListOffsett() const {
        uint64_t result =
            (name_index_list_size           * sizeof(unsigned)            ) +
            (name_char_list_size            * sizeof(char)                ) +
            (name_id_list_size          * sizeof(unsigned)            ) +
            (via_node_list_size         * sizeof(NodeID)              ) +
            (graph_node_list_size       * sizeof(QueryGraph::_StrNode)) ;
        return result;
    }
    uint64_t GetTimeStampOffset() const {
        uint64_t result =
            (name_index_list_size           * sizeof(unsigned)            ) +
            (name_char_list_size            * sizeof(char)                ) +
            (name_id_list_size          * sizeof(unsigned)            ) +
            (via_node_list_size         * sizeof(NodeID)              ) +
            (graph_node_list_size       * sizeof(QueryGraph::_StrNode)) +
            (graph_edge_list_size       * sizeof(QueryGraph::_StrEdge));
        return result;
    }
    uint64_t GetCoordinateListOffset() const {
        uint64_t result =
            (name_index_list_size           * sizeof(unsigned)            ) +
            (name_char_list_size            * sizeof(char)                ) +
            (name_id_list_size          * sizeof(unsigned)            ) +
            (via_node_list_size         * sizeof(NodeID)              ) +
            (graph_node_list_size       * sizeof(QueryGraph::_StrNode)) +
            (graph_edge_list_size       * sizeof(QueryGraph::_StrEdge)) +
            (timestamp_length           * sizeof(char)                );
        return result;
    }
    uint64_t GetTurnInstructionListOffset() const {
        uint64_t result =
            (name_index_list_size           * sizeof(unsigned)            ) +
            (name_char_list_size            * sizeof(char)                ) +
            (name_id_list_size          * sizeof(unsigned)            ) +
            (via_node_list_size         * sizeof(NodeID)              ) +
            (graph_node_list_size       * sizeof(QueryGraph::_StrNode)) +
            (graph_edge_list_size       * sizeof(QueryGraph::_StrEdge)) +
            (timestamp_length           * sizeof(char)                ) +
            (coordinate_list_size       * sizeof(FixedPointCoordinate));
        return result;
    }
    uint64_t GetRSearchTreeOffset() const {
        uint64_t result =
            (name_index_list_size           * sizeof(unsigned)            ) +
            (name_char_list_size            * sizeof(char)                ) +
            (name_id_list_size          * sizeof(unsigned)            ) +
            (via_node_list_size         * sizeof(NodeID)              ) +
            (graph_node_list_size       * sizeof(QueryGraph::_StrNode)) +
            (graph_edge_list_size       * sizeof(QueryGraph::_StrEdge)) +
            (timestamp_length           * sizeof(char)                ) +
            (coordinate_list_size       * sizeof(FixedPointCoordinate)) +
            (turn_instruction_list_size * sizeof(TurnInstructions)    );
        return result;
    }
    uint64_t GetChecksumOffset() const {
        uint64_t result =
            (name_index_list_size           * sizeof(unsigned)            ) +
            (name_char_list_size            * sizeof(char)                ) +
            (name_id_list_size          * sizeof(unsigned)            ) +
            (via_node_list_size         * sizeof(NodeID)              ) +
            (graph_node_list_size       * sizeof(QueryGraph::_StrNode)) +
            (graph_edge_list_size       * sizeof(QueryGraph::_StrEdge)) +
            (timestamp_length           * sizeof(char)                ) +
            (coordinate_list_size       * sizeof(FixedPointCoordinate)) +
            (turn_instruction_list_size * sizeof(TurnInstructions)    ) +
            (r_search_tree_size         * sizeof(RTreeNode)           );
        return result;
    }
};

enum SharedDataType {
    LAYOUT_1,
    DATA_1,
    LAYOUT_2,
    DATA_2,
    LAYOUT_3,
    DATA_3,
    LAYOUT_LOAD,
    DATA_LOAD
};

#endif /* SHARED_DATA_TYPE_H_ */
