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

#ifndef NODEINFORMATIONHELPDESK_H_
#define NODEINFORMATIONHELPDESK_H_

#include "QueryNode.h"
#include "PhantomNodes.h"
#include "StaticRTree.h"
#include "../Contractor/EdgeBasedGraphFactory.h"
#include "../Util/OSRMException.h"
#include "../typedefs.h"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/noncopyable.hpp>

#include <iostream>
#include <string>
#include <vector>

typedef EdgeBasedGraphFactory::EdgeBasedNode RTreeLeaf;

class NodeInformationHelpDesk : boost::noncopyable {

public:
    NodeInformationHelpDesk(
        const std::string & ram_index_filename,
        const std::string & mem_index_filename,
        const std::string & nodes_filename,
        const std::string & edges_filename,
        const unsigned m_number_of_nodes,
        const unsigned m_check_sum
    ) :
        m_number_of_nodes(m_number_of_nodes),
        m_check_sum(m_check_sum)
    {
        if ( ram_index_filename.empty() ) {
            throw OSRMException("no ram index file name in server ini");
        }
        if ( mem_index_filename.empty() ) {
            throw OSRMException("no mem index file name in server ini");
        }
        if ( nodes_filename.empty()     ) {
            throw OSRMException("no nodes file name in server ini");
        }
        if ( edges_filename.empty()     ) {
            throw OSRMException("no edges file name in server ini");
        }

        m_ro_rtree_ptr = new StaticRTree<RTreeLeaf>(
            ram_index_filename,
            mem_index_filename
        );
        BOOST_ASSERT_MSG(
            0 == m_coordinate_list.size(),
            "Coordinate vector not empty"
        );

        LoadNodesAndEdges(nodes_filename, edges_filename);
    }

	~NodeInformationHelpDesk() {
		delete m_ro_rtree_ptr;
	}

    inline FixedPointCoordinate GetCoordinateOfNode(const unsigned id) const {
        const NodeID node = m_via_node_list.at(id);
        return m_coordinate_list.at(node);
    }

	inline unsigned GetNameIndexFromEdgeID(const unsigned id) const {
	    return m_name_ID_list.at(id);
	}

    inline TurnInstruction GetTurnInstructionForEdgeID(const unsigned id) const {
        return m_turn_instruction_list.at(id);
    }

    inline NodeID GetNumberOfNodes() const {
        return m_number_of_nodes;
    }

    inline bool LocateClosestEndPointForCoordinate(
            const FixedPointCoordinate& input_coordinate,
            FixedPointCoordinate& result,
            const unsigned zoom_level = 18
    ) const {
        bool found_node = m_ro_rtree_ptr->LocateClosestEndPointForCoordinate(
                            input_coordinate,
                            result, zoom_level
                          );
        return found_node;
    }

    inline bool FindPhantomNodeForCoordinate(
            const FixedPointCoordinate & input_coordinate,
            PhantomNode & resulting_phantom_node,
            const unsigned zoom_level
    ) const {
        return m_ro_rtree_ptr->FindPhantomNodeForCoordinate(
                input_coordinate,
                resulting_phantom_node,
                zoom_level
        );
    }

	inline unsigned GetCheckSum() const {
	    return m_check_sum;
	}

private:
    void LoadNodesAndEdges(
        const std::string & nodes_filename,
        const std::string & edges_filename
    ) {
        boost::filesystem::path nodes_file(nodes_filename);
        if ( !boost::filesystem::exists( nodes_file ) ) {
            throw OSRMException("nodes file does not exist");
        }
        if ( 0 == boost::filesystem::file_size( nodes_file ) ) {
            throw OSRMException("nodes file is empty");
        }

        boost::filesystem::path edges_file(edges_filename);
        if ( !boost::filesystem::exists( edges_file ) ) {
            throw OSRMException("edges file does not exist");
        }
        if ( 0 == boost::filesystem::file_size( edges_file ) ) {
            throw OSRMException("edges file is empty");
        }

        boost::filesystem::ifstream nodes_input_stream(
            nodes_file,
            std::ios::binary
        );

        boost::filesystem::ifstream edges_input_stream(
            edges_file, std::ios::binary
        );

        SimpleLogger().Write(logDEBUG)
            << "Loading node data";
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

        SimpleLogger().Write(logDEBUG)
            << "Loading edge data";
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
        SimpleLogger().Write(logDEBUG)
            << "Loaded " << number_of_edges << " orig edges";
        SimpleLogger().Write(logDEBUG)
            << "Opening NN indices";
    }

	std::vector<FixedPointCoordinate>  m_coordinate_list;
	std::vector<NodeID>                m_via_node_list;
	std::vector<unsigned>              m_name_ID_list;
	std::vector<TurnInstruction>       m_turn_instruction_list;

	StaticRTree<EdgeBasedGraphFactory::EdgeBasedNode> * m_ro_rtree_ptr;
	const unsigned m_number_of_nodes;
	const unsigned m_check_sum;
};

#endif /*NODEINFORMATIONHELPDESK_H_*/
