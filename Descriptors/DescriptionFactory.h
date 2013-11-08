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
#include "../DataStructures/SearchEngine.h"
#include "../DataStructures/SegmentInformation.h"
#include "../DataStructures/TurnInstructions.h"
#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

#include <limits>
#include <vector>

/* This class is fed with all way segments in consecutive order
 *  and produces the description plus the encoded polyline */

class DescriptionFactory {
    DouglasPeucker<SegmentInformation> dp;
    PolylineCompressor pc;
    PhantomNode startPhantom, targetPhantom;

    double DegreeToRadian(const double degree) const;
    double RadianToDegree(const double degree) const;
public:
    struct _RouteSummary {
        std::string lengthString;
        std::string durationString;
        unsigned startName;
        unsigned destName;
        _RouteSummary() : lengthString("0"), durationString("0"), startName(0), destName(0) {}
        void BuildDurationAndLengthStrings(const double distance, const unsigned time) {
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
    void AppendEncodedPolylineString(std::string &output);
    void AppendUnencodedPolylineString(std::string &output);
    void AppendSegment(const FixedPointCoordinate & coordinate, const _PathData & data);
    void BuildRouteSummary(const double distance, const unsigned time);
    void SetStartSegment(const PhantomNode & startPhantom);
    void SetEndSegment(const PhantomNode & startPhantom);
    void AppendEncodedPolylineString(std::string & output, bool isEncoded);
    void Run(const SearchEngine &sEngine, const unsigned zoomLevel);
};

#endif /* DESCRIPTIONFACTORY_H_ */
