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
