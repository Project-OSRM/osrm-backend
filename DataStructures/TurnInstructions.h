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
#include <iostream>
#include <fstream>
using namespace std;

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
    const static short StartAtEndOfStreet = 14;

    std::string TurnStrings[15];
    std::string Ordinals[11];

	TurnInstructionsClass(){
	};

    //This is a hack until c++0x is available enough to use initializer lists.
    TurnInstructionsClass(string nameFile){
        // Declare an input file stream
	ifstream fread;
	// Open a file for only read
	fread.open(nameFile.c_str(), ifstream::in);
	// Check if there have been any error
	if (!fread){
		cout << "Fault to read: " << nameFile.c_str() << endl;
	}
	// Create a buffer to use for read
	char buffer[128];
	// Read from file to buffer
	fread.getline(buffer, 128);
	int i = 0;
	while (!fread.eof()){
		/* Write the result in an array */
		std::string str(buffer);
		if(i<15){
			TurnStrings[i] = str;	
		}else if(i>=15 && i<26){
			Ordinals[i-15] = str;	
		}
		/* Read the next line */
		i++;
		fread.getline(buffer, 128);
	}
	fread.close();
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

};

static TurnInstructionsClass TurnInstructions;

#endif /* TURNINSTRUCTIONS_H_ */
