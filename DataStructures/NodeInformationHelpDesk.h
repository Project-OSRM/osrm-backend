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

#include <fstream>
#include <iostream>
#include <vector>

#include "../typedefs.h"
#include "../DataStructures/QueryEdge.h"
#include "NNGrid.h"
#include "PhantomNodes.h"

class NodeInformationHelpDesk{
public:
    NodeInformationHelpDesk(const char* ramIndexInput, const char* fileIndexInput, const unsigned _numberOfNodes, const unsigned crc) : numberOfNodes(_numberOfNodes), checkSum(crc) {
        readOnlyGrid = new ReadOnlyGrid(ramIndexInput,fileIndexInput);
        assert(0 == coordinateVector.size());
    }

    //Todo: Shared memory mechanism
//    NodeInformationHelpDesk(const char* ramIndexInput, const char* fileIndexInput, const unsigned crc) : checkSum(crc) {
//        readOnlyGrid = new ReadOnlyGrid(ramIndexInput,fileIndexInput);
//    }

	~NodeInformationHelpDesk() {
		delete readOnlyGrid;
	}
	void initNNGrid(std::ifstream& nodesInstream, std::ifstream& edgesInStream) {
	    DEBUG("Loading node data");
		NodeInfo b;
	    while(!nodesInstream.eof()) {
			nodesInstream.read((char *)&b, sizeof(NodeInfo));
			coordinateVector.push_back(_Coordinate(b.lat, b.lon));
		}
	    std::vector<_Coordinate>(coordinateVector).swap(coordinateVector);
	    numberOfNodes = coordinateVector.size();
	    nodesInstream.close();

        DEBUG("Loading edge data");
        unsigned numberOfOrigEdges(0);
        edgesInStream.read((char*)&numberOfOrigEdges, sizeof(unsigned));
        origEdgeData.resize(numberOfOrigEdges);
        edgesInStream.read((char*)&(origEdgeData[0]), numberOfOrigEdges*sizeof(OriginalEdgeData));
        edgesInStream.close();
        DEBUG("Loaded " << numberOfOrigEdges << " orig edges");
	    DEBUG("Opening NN indices");
	    readOnlyGrid->OpenIndexFiles();
	}

	void initNNGrid() {
	    readOnlyGrid->OpenIndexFiles();
	}

	inline int getLatitudeOfNode(const unsigned id) const {
	    const NodeID node = origEdgeData.at(id).viaNode;
	    return coordinateVector.at(node).lat;
	}

	inline int getLongitudeOfNode(const unsigned id) const {
        const NodeID node = origEdgeData.at(id).viaNode;
	    return coordinateVector.at(node).lon;
	}

	inline unsigned getNameIndexFromEdgeID(const unsigned id) const {
	    return origEdgeData.at(id).nameID;
	}

    inline short getTurnInstructionFromEdgeID(const unsigned id) const {
        return origEdgeData.at(id).turnInstruction;
    }

    inline NodeID getNumberOfNodes() const { return numberOfNodes; }
	inline NodeID getNumberOfNodes2() const { return coordinateVector.size(); }

	inline bool FindNearestNodeCoordForLatLon(const _Coordinate& coord, _Coordinate& result) const {
		return readOnlyGrid->FindNearestCoordinateOnEdgeInNodeBasedGraph(coord, result);
	}
	inline bool FindPhantomNodeForCoordinate( const _Coordinate & location, PhantomNode & resultNode) const {
	    return readOnlyGrid->FindPhantomNodeForCoordinate(location, resultNode);
	}

	inline void FindRoutingStarts(const _Coordinate &start, const _Coordinate &target, PhantomNodes & phantomNodes) const {
		readOnlyGrid->FindRoutingStarts(start, target, phantomNodes);
	}

	inline void FindNearestPointOnEdge(const _Coordinate & input, _Coordinate& output){
	    readOnlyGrid->FindNearestPointOnEdge(input, output);
	}

	inline unsigned GetCheckSum() const {
	    return checkSum;
	}

private:
	std::vector<_Coordinate> coordinateVector;
    std::vector<OriginalEdgeData> origEdgeData;

	ReadOnlyGrid * readOnlyGrid;
	unsigned numberOfNodes;
	unsigned checkSum;
};

#endif /*NODEINFORMATIONHELPDESK_H_*/
