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

#include "DescriptionFactory.h"

DescriptionFactory::DescriptionFactory() : entireLength(0) { }

DescriptionFactory::~DescriptionFactory() { }

inline double DescriptionFactory::DegreeToRadian(const double degree) const {
        return degree * (M_PI/180);
}

inline double DescriptionFactory::RadianToDegree(const double radian) const {
        return radian * (180/M_PI);
}

double DescriptionFactory::GetBearing(const _Coordinate& A, const _Coordinate& B) const {
    double deltaLong = DegreeToRadian(B.lon/100000. - A.lon/100000.);

    double lat1 = DegreeToRadian(A.lat/100000.);
    double lat2 = DegreeToRadian(B.lat/100000.);

    double y = sin(deltaLong) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(deltaLong);
    double result = RadianToDegree(atan2(y, x));
    while(result <= 0.)
        result += 360.;
    while(result >= 360.)
        result -= 360.;

    return result;
}

void DescriptionFactory::SetStartSegment(const PhantomNode & _startPhantom) {
    startPhantom = _startPhantom;
    AppendSegment(_startPhantom.location, _PathData(0, _startPhantom.nodeBasedEdgeNameID, 10, _startPhantom.weight1, _startPhantom.mode1));
}

void DescriptionFactory::SetEndSegment(const PhantomNode & _targetPhantom) {
    targetPhantom = _targetPhantom;
    pathDescription.push_back(SegmentInformation(_targetPhantom.location, _targetPhantom.nodeBasedEdgeNameID, 0, _targetPhantom.weight1, 0, true, _targetPhantom.mode1) );
}

void DescriptionFactory::AppendSegment(const _Coordinate & coordinate, const _PathData & data ) {
    if(1 == pathDescription.size() && pathDescription.back().location == coordinate) {
        pathDescription.back().nameID = data.nameID;
        pathDescription.back().mode = data.mode;
    } else {
        pathDescription.push_back(SegmentInformation(coordinate, data.nameID, 0, data.durationOfSegment, data.turnInstruction, data.mode) );
    }
}

void DescriptionFactory::AppendEncodedPolylineString(std::string & output, bool isEncoded) {
    if(isEncoded)
        pc.printEncodedString(pathDescription, output);
    else
        pc.printUnencodedString(pathDescription, output);
}

void DescriptionFactory::AppendEncodedPolylineString(std::string &output) {
    pc.printEncodedString(pathDescription, output);
}

void DescriptionFactory::AppendUnencodedPolylineString(std::string &output) {
    pc.printUnencodedString(pathDescription, output);
}

void DescriptionFactory::Run(const SearchEngineT &sEngine, const unsigned zoomLevel) {
    if(0 == pathDescription.size())
        return;

//    unsigned entireLength = 0;
    /** starts at index 1 */
    pathDescription[0].length = 0;
    for(unsigned i = 1; i < pathDescription.size(); ++i) {
        pathDescription[i].length = ApproximateDistanceByEuclid(pathDescription[i-1].location, pathDescription[i].location);
    }

    double lengthOfSegment = 0;
    unsigned durationOfSegment = 0;
    unsigned indexOfSegmentBegin = 0;

    std::string string0 = sEngine.GetEscapedNameForNameID(pathDescription[0].nameID);
    std::string string1;


    /*Simplify turn instructions
    Input :
    10. Turn left on B 36 for 20 km
    11. Continue on B 35; B 36 for 2 km
    12. Continue on B 36 for 13 km

    becomes:
    10. Turn left on B 36 for 35 km
    */
//TODO: rework to check only end and start of string.
//		stl string is way to expensive

//    unsigned lastTurn = 0;
//    for(unsigned i = 1; i < pathDescription.size(); ++i) {
//        string1 = sEngine.GetEscapedNameForNameID(pathDescription[i].nameID);
//        if(TurnInstructionsClass::GoStraight == pathDescription[i].turnInstruction) {
//            if(std::string::npos != string0.find(string1+";")
//            		|| std::string::npos != string0.find(";"+string1)
//            		|| std::string::npos != string0.find(string1+" ;")
//                    || std::string::npos != string0.find("; "+string1)
//                    ){
//                INFO("->next correct: " << string0 << " contains " << string1);
//                for(; lastTurn != i; ++lastTurn)
//                    pathDescription[lastTurn].nameID = pathDescription[i].nameID;
//                pathDescription[i].turnInstruction = TurnInstructionsClass::NoTurn;
//            } else if(std::string::npos != string1.find(string0+";")
//            		|| std::string::npos != string1.find(";"+string0)
//                    || std::string::npos != string1.find(string0+" ;")
//                    || std::string::npos != string1.find("; "+string0)
//                    ){
//                INFO("->prev correct: " << string1 << " contains " << string0);
//                pathDescription[i].nameID = pathDescription[i-1].nameID;
//                pathDescription[i].turnInstruction = TurnInstructionsClass::NoTurn;
//            }
//        }
//        if (TurnInstructionsClass::NoTurn != pathDescription[i].turnInstruction) {
//            lastTurn = i;
//        }
//        string0 = string1;
//    }


    for(unsigned i = 1; i < pathDescription.size(); ++i) {
        entireLength += pathDescription[i].length;
        lengthOfSegment += pathDescription[i].length;
        durationOfSegment += pathDescription[i].duration;
        pathDescription[indexOfSegmentBegin].length = lengthOfSegment;
        pathDescription[indexOfSegmentBegin].duration = durationOfSegment;


        if(TurnInstructionsClass::NoTurn != pathDescription[i].turnInstruction) {
            //INFO("Turn after " << lengthOfSegment << "m into way with name id " << segment.nameID);
            assert(pathDescription[i].necessary);
            lengthOfSegment = 0;
            durationOfSegment = 0;
            indexOfSegmentBegin = i;
        }
    }
    //    INFO("#segs: " << pathDescription.size());

    //Post-processing to remove empty or nearly empty path segments
    if(FLT_EPSILON > pathDescription.back().length) {
        //        INFO("#segs: " << pathDescription.size() << ", last ratio: " << targetPhantom.ratio << ", length: " << pathDescription.back().length);
        if(pathDescription.size() > 2){
            pathDescription.pop_back();
            pathDescription.back().necessary = true;
            pathDescription.back().turnInstruction = TurnInstructions.NoTurn;
            targetPhantom.nodeBasedEdgeNameID = (pathDescription.end()-2)->nameID;
            targetPhantom.mode1 = (pathDescription.end()-2)->mode;
            //            INFO("Deleting last turn instruction");
        }
    } else {
        pathDescription[indexOfSegmentBegin].duration *= (1.-targetPhantom.ratio);
    }
    if(FLT_EPSILON > pathDescription[0].length) {
        //TODO: this is never called actually?
        if(pathDescription.size() > 2) {
            pathDescription.erase(pathDescription.begin());
            pathDescription[0].turnInstruction = TurnInstructions.HeadOn;
            pathDescription[0].necessary = true;
            startPhantom.nodeBasedEdgeNameID = pathDescription[0].nameID;
            startPhantom.mode1 = pathDescription[0].mode;
            //           INFO("Deleting first turn instruction, ratio: " << startPhantom.ratio << ", length: " << pathDescription[0].length);
        }
    } else {
        pathDescription[0].duration *= startPhantom.ratio;
    }

    //Generalize poly line
    dp.Run(pathDescription, zoomLevel);

    //fix what needs to be fixed else
    for(unsigned i = 0; i < pathDescription.size()-1 && pathDescription.size() >= 2; ++i){
        if(pathDescription[i].necessary) {
            double angle = GetBearing(pathDescription[i].location, pathDescription[i+1].location);
            pathDescription[i].bearing = angle;
        }
    }

//    BuildRouteSummary(entireLength, duration);
    return;
}

void DescriptionFactory::BuildRouteSummary(const double distance, const unsigned time) {
    summary.startName = startPhantom.nodeBasedEdgeNameID;
    summary.destName = targetPhantom.nodeBasedEdgeNameID;
    summary.BuildDurationAndLengthStrings(distance, time);
}
