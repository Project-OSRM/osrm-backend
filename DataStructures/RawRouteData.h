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

#ifndef RAWROUTEDATA_H_
#define RAWROUTEDATA_H_

#include "../DataStructures/Coordinate.h"
#include "../DataStructures/PhantomNodes.h"
#include "../typedefs.h"

#include <vector>

struct _PathData {
    _PathData(NodeID no, unsigned na, unsigned tu, unsigned dur) : node(no), nameID(na), durationOfSegment(dur), turnInstruction(tu) { }
    NodeID node;
    unsigned nameID;
    unsigned durationOfSegment;
    short turnInstruction;
};

struct RawRouteData {
    std::vector< _PathData > computedShortestPath;
    std::vector< _PathData > computedAlternativePath;
    std::vector< PhantomNodes > segmentEndCoordinates;
    std::vector< FixedPointCoordinate > rawViaNodeCoordinates;
    unsigned checkSum;
    int lengthOfShortestPath;
    int lengthOfAlternativePath;
    RawRouteData() : checkSum(UINT_MAX), lengthOfShortestPath(INT_MAX), lengthOfAlternativePath(INT_MAX) {}
};

#endif /* RAWROUTEDATA_H_ */
