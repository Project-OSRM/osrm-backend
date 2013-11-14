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

#include "DescriptionFactory.h"

DescriptionFactory::DescriptionFactory() : entireLength(0) { }

DescriptionFactory::~DescriptionFactory() { }

inline double DescriptionFactory::DegreeToRadian(const double degree) const {
        return degree * (M_PI/180);
}

inline double DescriptionFactory::RadianToDegree(const double radian) const {
        return radian * (180/M_PI);
}

double DescriptionFactory::GetBearing(
    const FixedPointCoordinate & A,
    const FixedPointCoordinate & B
) const {
    double deltaLong = DegreeToRadian(B.lon/COORDINATE_PRECISION - A.lon/COORDINATE_PRECISION);

    const double lat1 = DegreeToRadian(A.lat/COORDINATE_PRECISION);
    const double lat2 = DegreeToRadian(B.lat/COORDINATE_PRECISION);

    const double y = sin(deltaLong) * cos(lat2);
    const double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(deltaLong);
    double result = RadianToDegree(atan2(y, x));
    while(result < 0.) {
        result += 360.;
    }
    while(result >= 360.) {
        result -= 360.;
    }
    return result;
}

void DescriptionFactory::SetStartSegment(const PhantomNode & sph) {
    start_phantom = sph;
    AppendSegment(
        sph.location,
        _PathData(0, sph.nodeBasedEdgeNameID, 10, sph.weight1)
    );
}

void DescriptionFactory::SetEndSegment(const PhantomNode & tph) {
    target_phantom = tph;
    pathDescription.push_back(
        SegmentInformation(
            tph.location,
            tph.nodeBasedEdgeNameID,
            0,
            tph.weight1,
            0,
            true
        )
    );
}

void DescriptionFactory::AppendSegment(
    const FixedPointCoordinate & coordinate,
    const _PathData & data
) {
    if(1 == pathDescription.size() && pathDescription.back().location == coordinate) {
        pathDescription.back().nameID = data.nameID;
    } else {
        pathDescription.push_back(SegmentInformation(coordinate, data.nameID, 0, data.durationOfSegment, data.turnInstruction) );
    }
}

void DescriptionFactory::AppendEncodedPolylineString(
    const bool return_encoded,
    std::vector<std::string> & output
) {
    std::string temp;
    if(return_encoded) {
        polyline_compressor.printEncodedString(pathDescription, temp);
    } else {
        polyline_compressor.printUnencodedString(pathDescription, temp);
    }
    output.push_back(temp);
}

void DescriptionFactory::AppendEncodedPolylineString(
    std::vector<std::string> &output
) const {
    std::string temp;
    polyline_compressor.printEncodedString(pathDescription, temp);
    output.push_back(temp);
}

void DescriptionFactory::AppendUnencodedPolylineString(
    std::vector<std::string>& output
) const {
    std::string temp;
    polyline_compressor.printUnencodedString(pathDescription, temp);
    output.push_back(temp);
}

// void DescriptionFactory::Run(const SearchEngine &sEngine, const unsigned zoomLevel) {

//     if(0 == pathDescription.size())
//         return;

// //    unsigned entireLength = 0;
//     /** starts at index 1 */
//     pathDescription[0].length = 0;
//     for(unsigned i = 1; i < pathDescription.size(); ++i) {
//         pathDescription[i].length = ApproximateEuclideanDistance(pathDescription[i-1].location, pathDescription[i].location);
//     }

//     double lengthOfSegment = 0;
//     unsigned durationOfSegment = 0;
//     unsigned indexOfSegmentBegin = 0;

//     std::string string0 = sEngine.GetEscapedNameForNameID(pathDescription[0].nameID);
//     std::string string1;


//     /*Simplify turn instructions
//     Input :
//     10. Turn left on B 36 for 20 km
//     11. Continue on B 35; B 36 for 2 km
//     12. Continue on B 36 for 13 km

//     becomes:
//     10. Turn left on B 36 for 35 km
//     */
// //TODO: rework to check only end and start of string.
// //		stl string is way to expensive

// //    unsigned lastTurn = 0;
// //    for(unsigned i = 1; i < pathDescription.size(); ++i) {
// //        string1 = sEngine.GetEscapedNameForNameID(pathDescription[i].nameID);
// //        if(TurnInstructionsClass::GoStraight == pathDescription[i].turnInstruction) {
// //            if(std::string::npos != string0.find(string1+";")
// //            		|| std::string::npos != string0.find(";"+string1)
// //            		|| std::string::npos != string0.find(string1+" ;")
// //                    || std::string::npos != string0.find("; "+string1)
// //                    ){
// //                SimpleLogger().Write() << "->next correct: " << string0 << " contains " << string1;
// //                for(; lastTurn != i; ++lastTurn)
// //                    pathDescription[lastTurn].nameID = pathDescription[i].nameID;
// //                pathDescription[i].turnInstruction = TurnInstructionsClass::NoTurn;
// //            } else if(std::string::npos != string1.find(string0+";")
// //            		|| std::string::npos != string1.find(";"+string0)
// //                    || std::string::npos != string1.find(string0+" ;")
// //                    || std::string::npos != string1.find("; "+string0)
// //                    ){
// //                SimpleLogger().Write() << "->prev correct: " << string1 << " contains " << string0;
// //                pathDescription[i].nameID = pathDescription[i-1].nameID;
// //                pathDescription[i].turnInstruction = TurnInstructionsClass::NoTurn;
// //            }
// //        }
// //        if (TurnInstructionsClass::NoTurn != pathDescription[i].turnInstruction) {
// //            lastTurn = i;
// //        }
// //        string0 = string1;
// //    }


//     for(unsigned i = 1; i < pathDescription.size(); ++i) {
//         entireLength += pathDescription[i].length;
//         lengthOfSegment += pathDescription[i].length;
//         durationOfSegment += pathDescription[i].duration;
//         pathDescription[indexOfSegmentBegin].length = lengthOfSegment;
//         pathDescription[indexOfSegmentBegin].duration = durationOfSegment;


//         if(TurnInstructionsClass::NoTurn != pathDescription[i].turnInstruction) {
//             //SimpleLogger().Write() << "Turn after " << lengthOfSegment << "m into way with name id " << segment.nameID;
//             assert(pathDescription[i].necessary);
//             lengthOfSegment = 0;
//             durationOfSegment = 0;
//             indexOfSegmentBegin = i;
//         }
//     }
//     //    SimpleLogger().Write() << "#segs: " << pathDescription.size();

//     //Post-processing to remove empty or nearly empty path segments
//     if(FLT_EPSILON > pathDescription.back().length) {
//         //        SimpleLogger().Write() << "#segs: " << pathDescription.size() << ", last ratio: " << target_phantom.ratio << ", length: " << pathDescription.back().length;
//         if(pathDescription.size() > 2){
//             pathDescription.pop_back();
//             pathDescription.back().necessary = true;
//             pathDescription.back().turnInstruction = TurnInstructions.NoTurn;
//             target_phantom.nodeBasedEdgeNameID = (pathDescription.end()-2)->nameID;
//             //            SimpleLogger().Write() << "Deleting last turn instruction";
//         }
//     } else {
//         pathDescription[indexOfSegmentBegin].duration *= (1.-target_phantom.ratio);
//     }
//     if(FLT_EPSILON > pathDescription[0].length) {
//         //TODO: this is never called actually?
//         if(pathDescription.size() > 2) {
//             pathDescription.erase(pathDescription.begin());
//             pathDescription[0].turnInstruction = TurnInstructions.HeadOn;
//             pathDescription[0].necessary = true;
//             start_phantom.nodeBasedEdgeNameID = pathDescription[0].nameID;
//             //            SimpleLogger().Write() << "Deleting first turn instruction, ratio: " << start_phantom.ratio << ", length: " << pathDescription[0].length;
//         }
//     } else {
//         pathDescription[0].duration *= start_phantom.ratio;
//     }

//     //Generalize poly line
//     dp.Run(pathDescription, zoomLevel);

//     //fix what needs to be fixed else
//     for(unsigned i = 0; i < pathDescription.size()-1 && pathDescription.size() >= 2; ++i){
//         if(pathDescription[i].necessary) {
//             double angle = GetBearing(pathDescription[i].location, pathDescription[i+1].location);
//             pathDescription[i].bearing = angle;
//         }
//     }

// //    BuildRouteSummary(entireLength, duration);
//     return;
// }

void DescriptionFactory::BuildRouteSummary(
    const double distance,
    const unsigned time
) {
    summary.startName = start_phantom.nodeBasedEdgeNameID;
    summary.destName = target_phantom.nodeBasedEdgeNameID;
    summary.BuildDurationAndLengthStrings(distance, time);
}
