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

#include <string>

//This is a hack until c++0x is available enough to use scoped enums
struct TurnInstructionsClass {

    const static short NoTurn = 0;          //Give no instruction at all
    const static short GoStraight = 1;      //Tell user to go straight!
    const static short TurnSlightRight = 2;
    const static short TurnRight = 3;
    const static short TurnSharpRight = 4;
    const static short UTurn = 5;
    const static short TurnSharpLeft = 6;
    const static short TurnLeft = 7;
    const static short TurnSlightLeft = 8;
    const static short ReachViaPoint = 9;
    const static short HeadOn = 10;
    const static short EnterRoundAbout = 11;
    const static short LeaveRoundAbout = 12;
    const static short StayOnRoundAbout = 13;

    std::string TurnStrings[14];

    //This is a hack until c++0x is available enough to use initializer lists.
    TurnInstructionsClass(){
        TurnStrings [0] = "";
        TurnStrings [1] = "Continue";
        TurnStrings [2] = "Turn slight right";
        TurnStrings [3] = "Turn right";
        TurnStrings [4] = "Turn sharp right";
        TurnStrings [5] = "U-Turn";
        TurnStrings [6] = "Turn sharp left";
        TurnStrings [7] = "Turn left";
        TurnStrings [8] = "Turn slight left";
        TurnStrings [9] = "Reach via point";
        TurnStrings[10] = "Head";
        TurnStrings[11] = "Enter round-about";
        TurnStrings[12] = "Leave round-about";
        TurnStrings[13] = "Stay on round-about";
    };

    static inline double GetTurnDirectionOfInstruction( const double angle ) {
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
        return 5;
    }

    static inline bool TurnIsNecessary ( const short turnInstruction ) {
        if(NoTurn == turnInstruction || StayOnRoundAbout == turnInstruction)
            return false;
        return true;
    }

    //    static inline void getDirectionOfInstruction(double angle, DirectionOfInstruction & dirInst) {
    //        if(angle >= 23 && angle < 67) {
    //            dirInst.direction = "southeast";
    //            dirInst.shortDirection = "SE";
    //            return;
    //        }
    //        if(angle >= 67 && angle < 113) {
    //            dirInst.direction = "south";
    //            dirInst.shortDirection = "S";
    //            return;
    //        }
    //        if(angle >= 113 && angle < 158) {
    //            dirInst.direction = "southwest";
    //            dirInst.shortDirection = "SW";
    //            return;
    //        }
    //        if(angle >= 158 && angle < 202) {
    //            dirInst.direction = "west";
    //            dirInst.shortDirection = "W";
    //            return;
    //        }
    //        if(angle >= 202 && angle < 248) {
    //            dirInst.direction = "northwest";
    //            dirInst.shortDirection = "NW";
    //            return;
    //        }
    //        if(angle >= 248 && angle < 292) {
    //            dirInst.direction = "north";
    //            dirInst.shortDirection = "N";
    //            return;
    //        }
    //        if(angle >= 292 && angle < 336) {
    //            dirInst.direction = "northeast";
    //            dirInst.shortDirection = "NE";
    //            return;
    //        }
    //        dirInst.direction = "East";
    //        dirInst.shortDirection = "E";
    //        return;
    //    }
    //

};

static TurnInstructionsClass TurnInstructions;

#endif /* TURNINSTRUCTIONS_H_ */
