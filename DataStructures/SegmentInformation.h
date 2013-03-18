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

#ifndef SEGMENTINFORMATION_H_
#define SEGMENTINFORMATION_H_

#include <climits>

#include "TurnInstructions.h"

struct SegmentInformation {
    _Coordinate location;
    NodeID nameID;
    double length;
    unsigned duration;
    double bearing;
    TurnInstruction turnInstruction;
    bool necessary;
    char mode;
    SegmentInformation(const _Coordinate & loc, const NodeID nam, const double len, const unsigned dur, const TurnInstruction tInstr, const bool nec, const char mod) :
            location(loc), nameID(nam), length(len), duration(dur), bearing(0.), turnInstruction(tInstr), necessary(nec), mode(mod) {}
    SegmentInformation(const _Coordinate & loc, const NodeID nam, const double len, const unsigned dur, const TurnInstruction tInstr, const char mod) :
        location(loc), nameID(nam), length(len), duration(dur), bearing(0.), turnInstruction(tInstr), necessary(tInstr != 0), mode(mod) {}
};

#endif /* SEGMENTINFORMATION_H_ */
