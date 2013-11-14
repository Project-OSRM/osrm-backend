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

#ifndef DESCRIPTIONFACTORY_H_
#define DESCRIPTIONFACTORY_H_

#include "../Algorithms/DouglasPeucker.h"
#include "../Algorithms/PolylineCompressor.h"
#include "../DataStructures/Coordinate.h"
#include "../DataStructures/PhantomNodes.h"
#include "../DataStructures/RawRouteData.h"
#include "../DataStructures/SegmentInformation.h"
#include "../DataStructures/TurnInstructions.h"
#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

#include <limits>
#include <vector>

/* This class is fed with all way segments in consecutive order
 *  and produces the description plus the encoded polyline */

class DescriptionFactory {
    DouglasPeucker polyline_generalizer;
    PolylineCompressor polyline_compressor;
    PhantomNode start_phantom, target_phantom;

    double DegreeToRadian(const double degree) const;
    double RadianToDegree(const double degree) const;
public:
    struct RouteSummary {
        std::string lengthString;
        std::string durationString;
        unsigned startName;
        unsigned destName;
        RouteSummary() :
            lengthString("0"),
            durationString("0"),
            startName(0),
            destName(0)
        {}

        void BuildDurationAndLengthStrings(
            const double distance,
            const unsigned time
        ) {
            //compute distance/duration for route summary
            intToString(round(distance), lengthString);
            int travelTime = time/10 + 1;
            intToString(travelTime, durationString);
        }
    } summary;

    double entireLength;

    //I know, declaring this public is considered bad. I'm lazy
    std::vector <SegmentInformation> pathDescription;
    DescriptionFactory();
    virtual ~DescriptionFactory();
    double GetBearing(const FixedPointCoordinate& C, const FixedPointCoordinate& B) const;
    void AppendEncodedPolylineString(std::vector<std::string> &output) const;
    void AppendUnencodedPolylineString(std::vector<std::string> &output) const;
    void AppendSegment(const FixedPointCoordinate & coordinate, const _PathData & data);
    void BuildRouteSummary(const double distance, const unsigned time);
    void SetStartSegment(const PhantomNode & start_phantom);
    void SetEndSegment(const PhantomNode & start_phantom);
    void AppendEncodedPolylineString(
        const bool return_encoded,
        std::vector<std::string> & output
        );

    template<class DataFacadeT>
    void Run(const DataFacadeT * facade, const unsigned zoomLevel) {

        if( pathDescription.empty() ) {
            return;
        }

    //    unsigned entireLength = 0;
        /** starts at index 1 */
        pathDescription[0].length = 0;
        for(unsigned i = 1; i < pathDescription.size(); ++i) {
            pathDescription[i].length = ApproximateEuclideanDistance(pathDescription[i-1].location, pathDescription[i].location);
        }

        double lengthOfSegment = 0;
        unsigned durationOfSegment = 0;
        unsigned indexOfSegmentBegin = 0;

        // std::string string0 = facade->GetEscapedNameForNameID(pathDescription[0].nameID);
        // std::string string1;


        /*Simplify turn instructions
        Input :
        10. Turn left on B 36 for 20 km
        11. Continue on B 35; B 36 for 2 km
        12. Continue on B 36 for 13 km

        becomes:
        10. Turn left on B 36 for 35 km
        */
    //TODO: rework to check only end and start of string.
    //      stl string is way to expensive

    //    unsigned lastTurn = 0;
    //    for(unsigned i = 1; i < pathDescription.size(); ++i) {
    //        string1 = sEngine.GetEscapedNameForNameID(pathDescription[i].nameID);
    //        if(TurnInstructionsClass::GoStraight == pathDescription[i].turnInstruction) {
    //            if(std::string::npos != string0.find(string1+";")
    //                  || std::string::npos != string0.find(";"+string1)
    //                  || std::string::npos != string0.find(string1+" ;")
    //                    || std::string::npos != string0.find("; "+string1)
    //                    ){
    //                SimpleLogger().Write() << "->next correct: " << string0 << " contains " << string1;
    //                for(; lastTurn != i; ++lastTurn)
    //                    pathDescription[lastTurn].nameID = pathDescription[i].nameID;
    //                pathDescription[i].turnInstruction = TurnInstructionsClass::NoTurn;
    //            } else if(std::string::npos != string1.find(string0+";")
    //                  || std::string::npos != string1.find(";"+string0)
    //                    || std::string::npos != string1.find(string0+" ;")
    //                    || std::string::npos != string1.find("; "+string0)
    //                    ){
    //                SimpleLogger().Write() << "->prev correct: " << string1 << " contains " << string0;
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
                //SimpleLogger().Write() << "Turn after " << lengthOfSegment << "m into way with name id " << pathDescription[i].nameID;
                assert(pathDescription[i].necessary);
                lengthOfSegment = 0;
                durationOfSegment = 0;
                indexOfSegmentBegin = i;
            }
        }
        //    SimpleLogger().Write() << "#segs: " << pathDescription.size();

        //Post-processing to remove empty or nearly empty path segments
        if(std::numeric_limits<double>::epsilon() > pathDescription.back().length) {
            //        SimpleLogger().Write() << "#segs: " << pathDescription.size() << ", last ratio: " << target_phantom.ratio << ", length: " << pathDescription.back().length;
            if(pathDescription.size() > 2){
                pathDescription.pop_back();
                pathDescription.back().necessary = true;
                pathDescription.back().turnInstruction = TurnInstructions.NoTurn;
                target_phantom.nodeBasedEdgeNameID = (pathDescription.end()-2)->nameID;
                //            SimpleLogger().Write() << "Deleting last turn instruction";
            }
        } else {
            pathDescription[indexOfSegmentBegin].duration *= (1.-target_phantom.ratio);
        }
        if(std::numeric_limits<double>::epsilon() > pathDescription[0].length) {
            //TODO: this is never called actually?
            if(pathDescription.size() > 2) {
                pathDescription.erase(pathDescription.begin());
                pathDescription[0].turnInstruction = TurnInstructions.HeadOn;
                pathDescription[0].necessary = true;
                start_phantom.nodeBasedEdgeNameID = pathDescription[0].nameID;
                //            SimpleLogger().Write() << "Deleting first turn instruction, ratio: " << start_phantom.ratio << ", length: " << pathDescription[0].length;
            }
        } else {
            pathDescription[0].duration *= start_phantom.ratio;
        }

        //Generalize poly line
        polyline_generalizer.Run(pathDescription, zoomLevel);

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
};

#endif /* DESCRIPTIONFACTORY_H_ */
