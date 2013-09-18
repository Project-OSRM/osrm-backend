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


//TODO: Umbauen in Private Data Facade

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
