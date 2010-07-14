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

#include <kdtree++/kdtree.hpp>

typedef KDTree::KDTree<2, NodeInfo > KDTreeType;

class NodeInformationHelpDesk{
public:
    ~NodeInformationHelpDesk();
    NodeInformationHelpDesk() { int2ExtNodeMap = new vector<KDTreeType::iterator>();}
    KDTreeType * initKDTree(ifstream& input);

    NodeID getExternalNodeID(const NodeID node);
    const NodeInfo& getExternalNodeInfo(const NodeID node) const;
    int getLatitudeOfNode(const NodeID node) const;
    int getLongitudeOfNode(const NodeID node) const;

    NodeID getNumberOfNodes() const { return int2ExtNodeMap->size(); }

    inline NodeID findNearestNodeIDForLatLon(const int lat, const int lon, NodeCoords<NodeID> * data) const
    {

       NodeInfo nearestNeighbor = *(kdtree->find_nearest(NodeInfo(lat, lon, 0)).first);
       data->id = nearestNeighbor.id;
       data->lat = nearestNeighbor.lat;
       data->lon = nearestNeighbor.lon;
       return data->id;
    }
private:
    vector<KDTreeType::iterator> * int2ExtNodeMap;
    KDTreeType * kdtree;
};

//////////////////
//implementation//
//////////////////

NodeInformationHelpDesk::~NodeInformationHelpDesk(){
    //	delete graph;
    //	delete calc;
    //	delete c;
}

/* @brief: initialize kd-tree and internal->external node id map
 *
 */
KDTreeType * NodeInformationHelpDesk::initKDTree(ifstream& in)
{
    kdtree = new KDTreeType();
    NodeID id = 0;
    while(!in.eof())
    {
        NodeInfo b;
        in.read((char *)&b, sizeof(b));
        b.id = id;
        KDTreeType::iterator kdit = kdtree->insert(b);
        int2ExtNodeMap->push_back(kdit);
        id++;
    }
    in.close();
//    kdtree->optimise();
    return kdtree;
}

NodeID NodeInformationHelpDesk::getExternalNodeID(const NodeID node)
{

    return int2ExtNodeMap->at(node)->id;
}

const NodeInfo& NodeInformationHelpDesk::getExternalNodeInfo(const NodeID node) const
{
    return *(int2ExtNodeMap->at(node) );
}

int NodeInformationHelpDesk::getLatitudeOfNode(const NodeID node) const
{
    return int2ExtNodeMap->at(node)->lat;
}

int NodeInformationHelpDesk::getLongitudeOfNode(const NodeID node) const
{
    return int2ExtNodeMap->at(node)->lon;
}

#endif /*NODEINFORMATIONHELPDESK_H_*/
