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

#include "NodeCoords.h"
#include "PhantomNodes.h"
#include "StaticRTree.h"
#include "../Contractor/EdgeBasedGraphFactory.h"
#include "../typedefs.h"

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>

#include <fstream>

#include <iostream>
#include <vector>

typedef EdgeBasedGraphFactory::EdgeBasedNode RTreeLeaf;

class NodeInformationHelpDesk : boost::noncopyable{
public:
    NodeInformationHelpDesk(
        const char* ramIndexInput,
        const char* fileIndexInput,
        const unsigned number_of_nodes,
        const unsigned crc) : number_of_nodes(number_of_nodes), checkSum(crc) {
        read_only_rtree = new StaticRTree<RTreeLeaf>(
            ramIndexInput,
            fileIndexInput
        );
        BOOST_ASSERT_MSG(
            0 == coordinateVector.size(),
            "Coordinate vector not empty"
        );
    }

    //Todo: Shared memory mechanism
	~NodeInformationHelpDesk() {
		delete read_only_rtree;
	}

	void initNNGrid(
        std::ifstream& nodesInstream,
        std::ifstream& edgesInStream
    ) {
	    DEBUG("Loading node data");
		NodeInfo b;
	    while(!nodesInstream.eof()) {
			nodesInstream.read((char *)&b, sizeof(NodeInfo));
			coordinateVector.push_back(_Coordinate(b.lat, b.lon));
		}
	    std::vector<_Coordinate>(coordinateVector).swap(coordinateVector);
	    nodesInstream.close();

        DEBUG("Loading edge data");
        unsigned numberOfOrigEdges(0);
        edgesInStream.read((char*)&numberOfOrigEdges, sizeof(unsigned));
        origEdgeData_viaNode.resize(numberOfOrigEdges);
        origEdgeData_nameID.resize(numberOfOrigEdges);
        origEdgeData_turnInstruction.resize(numberOfOrigEdges);

        OriginalEdgeData deserialized_originalEdgeData;
        for(unsigned i = 0; i < numberOfOrigEdges; ++i) {
        	edgesInStream.read(
                (char*)&(deserialized_originalEdgeData),
                sizeof(OriginalEdgeData)
            );
            origEdgeData_viaNode[i] = deserialized_originalEdgeData.viaNode;
            origEdgeData_nameID[i] 	= deserialized_originalEdgeData.nameID;
            origEdgeData_turnInstruction[i] = deserialized_originalEdgeData.turnInstruction;
        }
        edgesInStream.close();
        DEBUG("Loaded " << numberOfOrigEdges << " orig edges");
	    DEBUG("Opening NN indices");
	}

	inline int getLatitudeOfNode(const unsigned id) const {
	    const NodeID node = origEdgeData_viaNode.at(id);
	    return coordinateVector.at(node).lat;
	}

	inline int getLongitudeOfNode(const unsigned id) const {
        const NodeID node = origEdgeData_viaNode.at(id);
	    return coordinateVector.at(node).lon;
	}

	inline unsigned getNameIndexFromEdgeID(const unsigned id) const {
	    return origEdgeData_nameID.at(id);
	}

    inline TurnInstruction getTurnInstructionFromEdgeID(const unsigned id) const {
        return origEdgeData_turnInstruction.at(id);
    }

    inline NodeID getNumberOfNodes() const {
        return number_of_nodes;
    }

	inline NodeID getNumberOfNodes2() const {
        return coordinateVector.size();
    }

    inline bool FindNearestNodeCoordForLatLon(
            const _Coordinate& input_coordinate,
            _Coordinate& result,
            const unsigned zoom_level = 18
    ) const {
        PhantomNode resulting_phantom_node;
        bool foundNode = FindPhantomNodeForCoordinate(
            input_coordinate,
            resulting_phantom_node, zoom_level
        );
        result = resulting_phantom_node.location;
        return foundNode;
    }

    inline bool FindPhantomNodeForCoordinate(
            const _Coordinate & input_coordinate,
            PhantomNode & resulting_phantom_node,
            const unsigned zoom_level
    ) const {
        return read_only_rtree->FindPhantomNodeForCoordinate(
                input_coordinate,
                resulting_phantom_node,
                zoom_level
        );
    }

	inline unsigned GetCheckSum() const {
	    return checkSum;
	}

private:
	std::vector<_Coordinate> coordinateVector;
	std::vector<NodeID> origEdgeData_viaNode;
	std::vector<unsigned> origEdgeData_nameID;
	std::vector<TurnInstruction> origEdgeData_turnInstruction;

	StaticRTree<EdgeBasedGraphFactory::EdgeBasedNode> * read_only_rtree;
	const unsigned number_of_nodes;
	const unsigned checkSum;
};

#endif /*NODEINFORMATIONHELPDESK_H_*/
