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

typedef NNGrid::NNGrid<false> ReadOnlyGrid;

class NodeInformationHelpDesk{
public:
	NodeInformationHelpDesk(const char* ramIndexInput, const char* fileIndexInput) { numberOfNodes = 0; int2ExtNodeMap = new vector<_Coordinate>(); g = new ReadOnlyGrid(ramIndexInput,fileIndexInput); }
	~NodeInformationHelpDesk() { delete int2ExtNodeMap; delete g; }
	void initNNGrid(ifstream& in)
	{
	    while(!in.eof())
	    {
	        NodeInfo b;
	        in.read((char *)&b, sizeof(b));
	        int2ExtNodeMap->push_back(_Coordinate(b.lat, b.lon));
	        numberOfNodes++;
	    }
	    in.close();
	    g->OpenIndexFiles();
	}

	inline int getLatitudeOfNode(const NodeID node) const { return int2ExtNodeMap->at(node).lat; }

	inline int getLongitudeOfNode(const NodeID node) const { return int2ExtNodeMap->at(node).lon; }

	NodeID getNumberOfNodes() const { return numberOfNodes; }

	inline void findNearestNodeIDForLatLon(const _Coordinate coord, _Coordinate& result) { result = g->FindNearestPointOnEdge(coord); }

	inline bool FindRoutingStarts(const _Coordinate start, const _Coordinate target, PhantomNodes * phantomNodes) {
	    g->FindRoutingStarts(start, target, phantomNodes);
	}
private:
	vector<_Coordinate> * int2ExtNodeMap;
	ReadOnlyGrid * g;
	unsigned numberOfNodes;
};

#endif /*NODEINFORMATIONHELPDESK_H_*/
