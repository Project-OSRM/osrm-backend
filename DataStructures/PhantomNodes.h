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

#include <osrm/Coordinate.h>
#include "../typedefs.h"

struct PhantomNode {
    PhantomNode() :
        forward_node_id( SPECIAL_NODEID ),
        reverse_node_id( SPECIAL_NODEID ),
        name_id( std::numeric_limits<unsigned>::max() ),
        forward_weight(INVALID_EDGE_WEIGHT),
        reverse_weight(INVALID_EDGE_WEIGHT),
        forward_offset(0),
        reverse_offset(0),
        ratio(0.)
    { }

    NodeID forward_node_id;
    NodeID reverse_node_id;
    unsigned name_id;
    int forward_weight;
    int reverse_weight;
    int forward_offset;
    int reverse_offset;
    double ratio;
    FixedPointCoordinate location;

    int GetForwardWeightPlusOffset() const {
        return forward_weight + forward_offset;
    }

    int GetReverseWeightPlusOffset() const {
        return reverse_weight + reverse_offset;
    }

    void Reset() {
        forward_node_id = SPECIAL_NODEID;
        name_id = SPECIAL_NODEID;
        forward_weight = INVALID_EDGE_WEIGHT;
        reverse_weight = INVALID_EDGE_WEIGHT;
        forward_offset = 0;
        reverse_offset = 0;
        ratio = 0.;
        location.Reset();
    }

    bool isBidirected() const {
        return ( forward_node_id != SPECIAL_NODEID ) &&
               ( reverse_node_id != SPECIAL_NODEID );
    }

    bool IsCompressed() const {
        return (forward_offset != 0) || (reverse_offset != 0);
    }

    bool isValid(const unsigned numberOfNodes) const {
        return
            location.isValid() &&
            ( (forward_node_id < numberOfNodes) || (reverse_node_id < numberOfNodes) ) &&
            ( (forward_weight != INVALID_EDGE_WEIGHT) || (reverse_weight != INVALID_EDGE_WEIGHT) ) &&
            (ratio >= 0.) &&
            (ratio <= 1.) &&
            (name_id != std::numeric_limits<unsigned>::max());
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
        return (startPhantom.forward_node_id == targetPhantom.forward_node_id);
    }

    bool AtLeastOnePhantomNodeIsUINTMAX() const {
        return !(startPhantom.forward_node_id == SPECIAL_NODEID || targetPhantom.forward_node_id == SPECIAL_NODEID);
    }

    bool PhantomNodesHaveEqualLocation() const {
        return startPhantom == targetPhantom;
    }
};

inline std::ostream& operator<<(std::ostream &out, const PhantomNodes & pn){
    out << "Node1: " << pn.startPhantom.forward_node_id  << std::endl;
    out << "Node2: " << pn.targetPhantom.reverse_node_id << std::endl;
    out << "startCoord: "  << pn.startPhantom.location << std::endl;
    out << "targetCoord: " << pn.targetPhantom.location << std::endl;
    return out;
}

inline std::ostream& operator<<(std::ostream &out, const PhantomNode & pn){
    out << "node1: " << pn.forward_node_id << ", node2: " << pn.reverse_node_id << ", name: " << pn.name_id << ", w1: " << pn.forward_weight << ", w2: " << pn.reverse_weight << ", ratio: " << pn.ratio << ", loc: " << pn.location;
    return out;
}

#endif /* PHANTOMNODES_H_ */
