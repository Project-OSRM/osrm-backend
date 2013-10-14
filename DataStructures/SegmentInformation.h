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

#ifndef SEGMENTINFORMATION_H_
#define SEGMENTINFORMATION_H_

#include "Coordinate.h"
#include "TurnInstructions.h"
#include "../typedefs.h"

#include <climits>

struct SegmentInformation {
    FixedPointCoordinate location;
    NodeID nameID;
    double length;
    unsigned duration;
    double bearing;
    TurnInstruction turnInstruction;
    bool necessary;
    SegmentInformation(const FixedPointCoordinate & loc, const NodeID nam, const double len, const unsigned dur, const TurnInstruction tInstr, const bool nec) :
            location(loc), nameID(nam), length(len), duration(dur), bearing(0.), turnInstruction(tInstr), necessary(nec) {}
    SegmentInformation(const FixedPointCoordinate & loc, const NodeID nam, const double len, const unsigned dur, const TurnInstruction tInstr) :
        location(loc), nameID(nam), length(len), duration(dur), bearing(0.), turnInstruction(tInstr), necessary(tInstr != 0) {}
};

#endif /* SEGMENTINFORMATION_H_ */
