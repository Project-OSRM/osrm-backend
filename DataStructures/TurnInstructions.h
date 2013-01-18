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

#ifndef TURNINSTRUCTIONS_H_
#define TURNINSTRUCTIONS_H_

#include <boost/noncopyable.hpp>

typedef unsigned char TurnInstruction;

//This is a hack until c++0x is available enough to use scoped enums
struct TurnInstructionsClass : boost::noncopyable {

    const static TurnInstruction NoTurn = 0;          //Give no instruction at all
    const static TurnInstruction GoStraight = 1;      //Tell user to go straight!
    const static TurnInstruction TurnSlightRight = 2;
    const static TurnInstruction TurnRight = 3;
    const static TurnInstruction TurnSharpRight = 4;
    const static TurnInstruction UTurn = 5;
    const static TurnInstruction TurnSharpLeft = 6;
    const static TurnInstruction TurnLeft = 7;
    const static TurnInstruction TurnSlightLeft = 8;
    const static TurnInstruction ReachViaPoint = 9;
    const static TurnInstruction HeadOn = 10;
    const static TurnInstruction EnterRoundAbout = 11;
    const static TurnInstruction LeaveRoundAbout = 12;
    const static TurnInstruction StayOnRoundAbout = 13;
    const static TurnInstruction StartAtEndOfStreet = 14;
    const static TurnInstruction ReachedYourDestination = 15;
    const static TurnInstruction EnterAgainstAllowedDirection = 16;
    const static TurnInstruction LeaveAgainstAllowedDirection = 17;

    const static TurnInstruction AccessRestrictionFlag = 128;
    const static TurnInstruction InverseAccessRestrictionFlag = 0x7f; // ~128 does not work without a warning.

    const static int AccessRestrictionPenalty = 1 << 15; //unrelated to the bit set in the restriction flag

    static inline TurnInstruction GetTurnDirectionOfInstruction( const double angle ) {
        if(angle >= 23 && angle < 67) {
            return TurnSharpRight;
        }
        if (angle >= 67 && angle < 113) {
            return TurnRight;
        }
        if (angle >= 113 && angle < 158) {
            return TurnSlightRight;
        }
        if (angle >= 158 && angle < 202) {
            return GoStraight;
        }
        if (angle >= 202 && angle < 248) {
            return TurnSlightLeft;
        }
        if (angle >= 248 && angle < 292) {
            return TurnLeft;
        }
        if (angle >= 292 && angle < 336) {
            return TurnSharpLeft;
        }
        return UTurn;
    }

    static inline bool TurnIsNecessary ( const short turnInstruction ) {
        if(NoTurn == turnInstruction || StayOnRoundAbout == turnInstruction)
            return false;
        return true;
    }

};

static TurnInstructionsClass TurnInstructions;

#endif /* TURNINSTRUCTIONS_H_ */
