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
    PhantomNode() : edgeBasedNode(UINT_MAX), nodeBasedEdgeNameID(UINT_MAX), weight1(INT_MAX), weight2(INT_MAX), ratio(0.) {}
    NodeID edgeBasedNode;
    unsigned nodeBasedEdgeNameID;
    int weight1;
    int weight2;
    double ratio;
    _Coordinate location;
    void Reset() {
        edgeBasedNode = UINT_MAX;
        nodeBasedEdgeNameID = UINT_MAX;
        weight1 = INT_MAX;
        weight2 = INT_MAX;
        ratio = 0.;
        location.Reset();
    }
    bool isBidirected() const {
        return weight2 != INT_MAX;
    }
    bool isValid(const unsigned numberOfNodes) const {
        return location.isValid() && (edgeBasedNode < numberOfNodes) && (weight1 != INT_MAX) && (ratio >= 0.) && (ratio <= 1.) && (nodeBasedEdgeNameID != UINT_MAX);
    }
};

struct PhantomNodes {
    PhantomNode startPhantom;
    PhantomNode targetPhantom;
    void Reset() {
        startPhantom.Reset();
        targetPhantom.Reset();
    }

    bool PhantomsAreOnSameNodeBasedEdge() const {
        return (startPhantom.edgeBasedNode == targetPhantom.edgeBasedNode);
    }

    bool AtLeastOnePhantomNodeIsUINTMAX() const {
        return !(startPhantom.edgeBasedNode == UINT_MAX || targetPhantom.edgeBasedNode == UINT_MAX);
    }
};

inline std::ostream& operator<<(std::ostream &out, const PhantomNodes & pn){
    out << "Node1: " << pn.startPhantom.edgeBasedNode << std::endl;
    out << "Node2: " << pn.targetPhantom.edgeBasedNode << std::endl;
    out << "startCoord: " << pn.startPhantom.location << std::endl;
    out << "targetCoord: " << pn.targetPhantom.location << std::endl;
    return out;
}

inline std::ostream& operator<<(std::ostream &out, const PhantomNode & pn){
    out << "node: " << pn.edgeBasedNode << ", name: " << pn.nodeBasedEdgeNameID << ", w1: " << pn.weight1 << ", w2: " << pn.weight2 << ", ratio: " << pn.ratio << ", loc: " << pn.location;
    return out;
}

struct NodesOfEdge {
    NodeID edgeBasedNode;
    double ratio;
    _Coordinate projectedPoint;
};

#endif /* PHANTOMNODES_H_ */
