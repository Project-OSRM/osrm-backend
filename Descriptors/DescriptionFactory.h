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

#ifndef DESCRIPTIONFACTORY_H_
#define DESCRIPTIONFACTORY_H_

#include <vector>

#include "../typedefs.h"
#include "../Algorithms/DouglasPeucker.h"
#include "../Algorithms/PolylineCompressor.h"
#include "../DataStructures/ExtractorStructs.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/SearchEngine.h"
#include "../DataStructures/SegmentInformation.h"
#include "../DataStructures/TurnInstructions.h"

/* This class is fed with all way segments in consecutive order
 *  and produces the description plus the encoded polyline */

class DescriptionFactory {
    DouglasPeucker<SegmentInformation> dp;
    PolylineCompressor pc;
    PhantomNode startPhantom, targetPhantom;

    typedef SearchEngine<QueryEdge::EdgeData, StaticGraph<QueryEdge::EdgeData> > SearchEngineT;

    double DegreeToRadian(const double degree) const;
    double RadianToDegree(const double degree) const;
public:
    struct _RouteSummary {
        std::string lengthString;
        std::string durationString;
        unsigned startName;
        unsigned destName;
        _RouteSummary() : lengthString("0"), durationString("0"), startName(0), destName(0) {}
        void BuildDurationAndLengthStrings(unsigned distance, unsigned time) {
            //compute distance/duration for route summary
            std::ostringstream s;
            s << 10*(round(distance/10.));
            lengthString = s.str();
            int travelTime = time/10 + 1;
            s.str("");
            s << travelTime;
            durationString = s.str();
        }
    } summary;

    unsigned entireLength;

    //I know, declaring this public is considered bad. I'm lazy
    std::vector <SegmentInformation> pathDescription;
    DescriptionFactory();
    virtual ~DescriptionFactory();
    double GetBearing(const _Coordinate& C, const _Coordinate& B) const;
    void AppendEncodedPolylineString(std::string &output);
    void AppendUnencodedPolylineString(std::string &output);
    void AppendSegment(const _Coordinate & coordinate, const _PathData & data);
    void BuildRouteSummary(const unsigned distance, const unsigned time);
    void SetStartSegment(const PhantomNode & startPhantom);
    void SetEndSegment(const PhantomNode & startPhantom);
    void AppendEncodedPolylineString(std::string & output, bool isEncoded);
    void Run(const SearchEngineT &sEngine, const unsigned zoomLevel, const unsigned duration);
};

#endif /* DESCRIPTIONFACTORY_H_ */
