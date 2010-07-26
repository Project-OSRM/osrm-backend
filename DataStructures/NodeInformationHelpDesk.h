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

#include <omp.h>

#include <climits>
#include <cstdlib>

#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>
#include <limits>
#include <list>
#include <stack>
#include <string>
#include <vector>

#include "../typedefs.h"
#include "StaticKDTree.h"

typedef KDTree::StaticKDTree<2, int, NodeID> KDTreeType;

class NodeInformationHelpDesk{
public:
	NodeInformationHelpDesk() { int2ExtNodeMap = new vector<KDTreeType::InputPoint>();}
	KDTreeType * initKDTree(ifstream& input);

	NodeID getExternalNodeID(const NodeID node);
	void getExternalNodeInfo(const NodeID node, NodeInfo * info) const;
	int getLatitudeOfNode(const NodeID node) const;
	int getLongitudeOfNode(const NodeID node) const;

	NodeID getNumberOfNodes() const { return int2ExtNodeMap->size(); }

	inline NodeID findNearestNodeIDForLatLon(const int lat, const int lon, NodeCoords<NodeID> * data) const
	{
		KDTreeType::InputPoint i;
		KDTreeType::InputPoint o;
		i.coordinates[0] = lat;
		i.coordinates[1] = lon;
		kdtree->NearestNeighbor(&o, i);
		data->id = o.data;
		data->lat = o.coordinates[0];
		data->lon = o.coordinates[1];
		return data->id;
	}
private:
	vector<KDTreeType::InputPoint> * int2ExtNodeMap;
	KDTreeType * kdtree;
};

//////////////////
//implementation//
//////////////////

/* @brief: initialize kd-tree and internal->external node id map
 *
 */
KDTreeType * NodeInformationHelpDesk::initKDTree(ifstream& in)
{
	NodeID id = 0;
	while(!in.eof())
	{
		NodeInfo b;
		in.read((char *)&b, sizeof(b));
		b.id = id;
		KDTreeType::InputPoint p;
		p.coordinates[0] = b.lat;
		p.coordinates[1] = b.lon;
		p.data = id;
		int2ExtNodeMap->push_back(p);
		id++;
	}
	in.close();
	kdtree = new KDTreeType(int2ExtNodeMap);
	return kdtree;
}

NodeID NodeInformationHelpDesk::getExternalNodeID(const NodeID node)
{

	return int2ExtNodeMap->at(node).data;
}

void NodeInformationHelpDesk::getExternalNodeInfo(const NodeID node, NodeInfo * info) const
{
	info->id = int2ExtNodeMap->at(node).data;
	info->lat = int2ExtNodeMap->at(node).coordinates[0];
	info->lon = int2ExtNodeMap->at(node).coordinates[1];
}

int NodeInformationHelpDesk::getLatitudeOfNode(const NodeID node) const
{
	return int2ExtNodeMap->at(node).coordinates[0];
}

int NodeInformationHelpDesk::getLongitudeOfNode(const NodeID node) const
{
	return int2ExtNodeMap->at(node).coordinates[1];
}

#endif /*NODEINFORMATIONHELPDESK_H_*/
