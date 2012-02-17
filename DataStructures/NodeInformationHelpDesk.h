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
#include "NNGrid.h"
#include "PhantomNodes.h"

class NodeInformationHelpDesk{
public:
	NodeInformationHelpDesk(const char* ramIndexInput, const char* fileIndexInput, const unsigned _numberOfNodes, const unsigned crc) : numberOfNodes(_numberOfNodes), checkSum(crc) {
		readOnlyGrid = new ReadOnlyGrid(ramIndexInput,fileIndexInput);
		coordinateVector.reserve(numberOfNodes);
	    assert(0 == coordinateVector.size());
	}

	~NodeInformationHelpDesk() {
		delete readOnlyGrid;
	}
	void initNNGrid(ifstream& in) {
	    while(!in.eof()) {
			NodeInfo b;
			in.read((char *)&b, sizeof(b));
			coordinateVector.push_back(_Coordinate(b.lat, b.lon));
		}
		in.close();
		readOnlyGrid->OpenIndexFiles();
	}

	inline int getLatitudeOfNode(const NodeID node) const { return coordinateVector.at(node).lat; }

	inline int getLongitudeOfNode(const NodeID node) const { return coordinateVector.at(node).lon; }

	inline NodeID getNumberOfNodes() const { return numberOfNodes; }
	inline NodeID getNumberOfNodes2() const { return coordinateVector.size(); }

	inline void FindNearestNodeCoordForLatLon(const _Coordinate& coord, _Coordinate& result) const {
		readOnlyGrid->FindNearestCoordinateOnEdgeInNodeBasedGraph(coord, result);
	}
	inline void FindPhantomNodeForCoordinate( const _Coordinate & location, PhantomNode & resultNode) const {
	    readOnlyGrid->FindPhantomNodeForCoordinate(location, resultNode);
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
	ReadOnlyGrid * readOnlyGrid;
	unsigned numberOfNodes;
	unsigned checkSum;
};

#endif /*NODEINFORMATIONHELPDESK_H_*/
