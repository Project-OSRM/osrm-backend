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
#include "../DataStructures/PhantomNodes.h"
#include "../DataStructures/RawRouteData.h"
#include "../DataStructures/SegmentInformation.h"
#include "../DataStructures/TurnInstructions.h"
#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

#include <osrm/Coordinate.h>

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
            int travel_time = time/10;
            intToString(std::max(travel_time, 1), durationString);
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
    void AppendSegment(const FixedPointCoordinate & coordinate, const PathData & data);
    void BuildRouteSummary(const double distance, const unsigned time);
    void SetStartSegment(const PhantomNode & start_phantom, const bool source_traversed_in_reverse);
    void SetEndSegment(const PhantomNode & start_phantom, const bool target_traversed_in_reverse);
    void AppendEncodedPolylineString(
        const bool return_encoded,
        std::vector<std::string> & output
    );

    template<class DataFacadeT>
    void Run(
        const DataFacadeT * facade,
        const unsigned zoomLevel
    ) {
        if( pathDescription.empty() ) {
            return;
        }

        /** starts at index 1 */
        pathDescription[0].length = 0;
        for (unsigned i = 1; i < pathDescription.size(); ++i)
        {
            //move down names by one, q&d hack
            pathDescription[i-1].name_id = pathDescription[i].name_id;
            pathDescription[i].length = FixedPointCoordinate::ApproximateEuclideanDistance(pathDescription[i-1].location, pathDescription[i].location);
        }

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
    //        string1 = sEngine.GetEscapedNameForNameID(pathDescription[i].name_id);
    //        if(TurnInstructionsClass::GoStraight == pathDescription[i].turn_instruction) {
    //            if(std::string::npos != string0.find(string1+";")
    //                  || std::string::npos != string0.find(";"+string1)
    //                  || std::string::npos != string0.find(string1+" ;")
    //                    || std::string::npos != string0.find("; "+string1)
    //                    ){
    //                SimpleLogger().Write() << "->next correct: " << string0 << " contains " << string1;
    //                for(; lastTurn != i; ++lastTurn)
    //                    pathDescription[lastTurn].name_id = pathDescription[i].name_id;
    //                pathDescription[i].turn_instruction = TurnInstructionsClass::NoTurn;
    //            } else if(std::string::npos != string1.find(string0+";")
    //                  || std::string::npos != string1.find(";"+string0)
    //                    || std::string::npos != string1.find(string0+" ;")
    //                    || std::string::npos != string1.find("; "+string0)
    //                    ){
    //                SimpleLogger().Write() << "->prev correct: " << string1 << " contains " << string0;
    //                pathDescription[i].name_id = pathDescription[i-1].name_id;
    //                pathDescription[i].turn_instruction = TurnInstructionsClass::NoTurn;
    //            }
    //        }
    //        if (TurnInstructionsClass::NoTurn != pathDescription[i].turn_instruction) {
    //            lastTurn = i;
    //        }
    //        string0 = string1;
    //    }

        double segment_length = 0.;
        unsigned segment_duration = 0;
        unsigned segment_start_index = 0;

        for(unsigned i = 1; i < pathDescription.size(); ++i) {
            entireLength += pathDescription[i].length;
            segment_length += pathDescription[i].length;
            segment_duration += pathDescription[i].duration;
            pathDescription[segment_start_index].length = segment_length;
            pathDescription[segment_start_index].duration = segment_duration;


            if(TurnInstructionsClass::NoTurn != pathDescription[i].turn_instruction) {
                BOOST_ASSERT(pathDescription[i].necessary);
                segment_length = 0;
                segment_duration = 0;
                segment_start_index = i;
            }
        }

        //Post-processing to remove empty or nearly empty path segments
        if(std::numeric_limits<double>::epsilon() > pathDescription.back().length) {
            if(pathDescription.size() > 2){
                pathDescription.pop_back();
                pathDescription.back().necessary = true;
                pathDescription.back().turn_instruction = TurnInstructionsClass::NoTurn;
                target_phantom.name_id = (pathDescription.end()-2)->name_id;
            }
        }
        if(std::numeric_limits<double>::epsilon() > pathDescription[0].length) {
            if(pathDescription.size() > 2) {
                pathDescription.erase(pathDescription.begin());
                pathDescription[0].turn_instruction = TurnInstructionsClass::HeadOn;
                pathDescription[0].necessary = true;
                start_phantom.name_id = pathDescription[0].name_id;
            }
        }

        //Generalize poly line
        polyline_generalizer.Run(pathDescription, zoomLevel);

        //fix what needs to be fixed else
        for(unsigned i = 0; i < pathDescription.size()-1 && pathDescription.size() >= 2; ++i){
            if(pathDescription[i].necessary) {
                double angle = GetBearing(pathDescription[i].location, pathDescription[i+1].location);
                pathDescription[i].bearing = angle*10;
            }
        }
        return;
    }
};

#endif /* DESCRIPTIONFACTORY_H_ */
