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

#include <boost/noncopyable.hpp>

#include "../typedefs.h"
#include "../DataStructures/QueryEdge.h"
#include "NNGrid.h"
#include "PhantomNodes.h"
#include "NodeCoords.h"
#include "TravelMode.h"

class QueryGraph;

class NodeInformationHelpDesk : boost::noncopyable{
public:
    NodeInformationHelpDesk(const char* ramIndexInput, const char* fileIndexInput, const unsigned _numberOfNodes, const unsigned crc, StaticGraph<QueryEdge::EdgeData>* graph) : numberOfNodes(_numberOfNodes), checkSum(crc) {
        readOnlyGrid = new ReadOnlyGrid(ramIndexInput,fileIndexInput, this, graph);
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
	    nodesInstream.close();

        DEBUG("Loading edge data");
        unsigned numberOfOrigEdges(0);
        edgesInStream.read((char*)&numberOfOrigEdges, sizeof(unsigned));
        origEdgeData_viaNode.resize(numberOfOrigEdges);
        origEdgeData_nameID.resize(numberOfOrigEdges);
        origEdgeData_turnInstruction.resize(numberOfOrigEdges);
        origEdgeData_mode.resize(numberOfOrigEdges);

        OriginalEdgeData deserialized_originalEdgeData;
        for(unsigned i = 0; i < numberOfOrigEdges; ++i) {
        	edgesInStream.read((char*)&(deserialized_originalEdgeData), sizeof(OriginalEdgeData));
            origEdgeData_viaNode[i] 		= deserialized_originalEdgeData.viaNode;
            origEdgeData_nameID[i] 			= deserialized_originalEdgeData.nameID;
            origEdgeData_turnInstruction[i] = deserialized_originalEdgeData.turnInstruction;
            origEdgeData_mode[i]            = deserialized_originalEdgeData.mode;
        }
        edgesInStream.close();
        DEBUG("Loaded " << numberOfOrigEdges << " orig edges");
	    DEBUG("Opening NN indices");
	    readOnlyGrid->OpenIndexFiles();
	}

//	void initNNGrid() {
//	    readOnlyGrid->OpenIndexFiles();
//	}

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

    inline TravelMode getModeFromEdgeID(const unsigned id) const {
        return origEdgeData_mode.at(id);
    }

    inline NodeID getNumberOfNodes() const { return numberOfNodes; }
	inline NodeID getNumberOfNodes2() const { return coordinateVector.size(); }

	inline bool FindNearestNodeCoordForLatLon(const _Coordinate& coord, _Coordinate& result) const {
		return readOnlyGrid->FindNearestCoordinateOnEdgeInNodeBasedGraph(coord, result);
	}

	inline bool FindPhantomNodeForCoordinate( const _Coordinate & location, PhantomNode & resultNode, const unsigned zoomLevel) {
	    return readOnlyGrid->FindPhantomNodeForCoordinate(location, resultNode, zoomLevel);
	}

	inline void FindRoutingStarts(const _Coordinate &start, const _Coordinate &target, PhantomNodes & phantomNodes, const unsigned zoomLevel) const {
		readOnlyGrid->FindRoutingStarts(start, target, phantomNodes, zoomLevel);
	}

	inline void FindNearestPointOnEdge(const _Coordinate & input, _Coordinate& output){
	    readOnlyGrid->FindNearestPointOnEdge(input, output);
	}

	inline unsigned GetCheckSum() const {
	    return checkSum;
	}

private:
	std::vector<_Coordinate> coordinateVector;
	std::vector<NodeID> origEdgeData_viaNode;
	std::vector<unsigned> origEdgeData_nameID;
	std::vector<TurnInstruction> origEdgeData_turnInstruction;
	std::vector<TravelMode> origEdgeData_mode;

	ReadOnlyGrid * readOnlyGrid;
	const unsigned numberOfNodes;
	const unsigned checkSum;
};

#endif /*NODEINFORMATIONHELPDESK_H_*/
