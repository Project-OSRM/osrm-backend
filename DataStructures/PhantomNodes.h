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

#ifndef PHANTOMNODES_H_
#define PHANTOMNODES_H_

#include "Coordinate.h"

struct PhantomNode {
    PhantomNode() :
        edgeBasedNode(UINT_MAX),
        nodeBasedEdgeNameID(UINT_MAX),
        weight1(INT_MAX),
        weight2(INT_MAX),
        ratio(0.)
    { }

    NodeID edgeBasedNode;
    unsigned nodeBasedEdgeNameID;
    int weight1;
    int weight2;
    double ratio;
    FixedPointCoordinate location;
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

    bool operator==(const PhantomNode & other) const {
        return location == other.location;
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

    bool PhantomNodesHaveEqualLocation() const {
        return startPhantom == targetPhantom;
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

#endif /* PHANTOMNODES_H_ */
