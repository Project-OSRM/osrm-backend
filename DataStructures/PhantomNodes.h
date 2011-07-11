/*
    open source routing machine
    Copyright (C) Dennis Luxen, 2010

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

#ifndef PHANTOMNODES_H_
#define PHANTOMNODES_H_

#include "ExtractorStructs.h"

struct PhantomNode {
    PhantomNode() : startNode(UINT_MAX), targetNode(UINT_MAX), ratio(1.) {}

    NodeID startNode;
    NodeID targetNode;
    double ratio;
    _Coordinate location;
    void Reset() {
        startNode = UINT_MAX;
        targetNode = UINT_MAX;
        ratio = 1.;
        location.Reset();
    }

};

struct PhantomNodes {
    PhantomNode startPhantom;
    PhantomNode targetPhantom;
    void Reset() {
        startPhantom.Reset();
        targetPhantom.Reset();
    }
};

std::ostream& operator<<(std::ostream &out, const PhantomNodes & pn){
    out << "startNode1: " << pn.startPhantom.startNode << std::endl;
    out << "startNode2: " << pn.startPhantom.targetNode << std::endl;
    out << "targetNode1: " << pn.targetPhantom.startNode << std::endl;
    out << "targetNode2: " << pn.targetPhantom.targetNode << std::endl;
    out << "startCoord: " << pn.startPhantom.location << std::endl;
    out << "targetCoord: " << pn.targetPhantom.location << std::endl;
    return out;
}

struct NodesOfEdge {
    NodeID startID;
    NodeID destID;
    double ratio;
    _Coordinate projectedPoint;
};

#endif /* PHANTOMNODES_H_ */
