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

#include "../Algorithms/DouglasPeucker.h"
#include "../Algorithms/PolylineCompressor.h"
#include "../DataStructures/Coordinate.h"
#include "../DataStructures/PhantomNodes.h"
#include "../DataStructures/RawRouteData.h"
#include "../DataStructures/SegmentInformation.h"
#include "../DataStructures/TurnInstructions.h"
#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

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
        if(FLT_EPSILON > pathDescription.back().length) {
            //        SimpleLogger().Write() << "#segs: " << pathDescription.size() << ", last ratio: " << targetPhantom.ratio << ", length: " << pathDescription.back().length;
            if(pathDescription.size() > 2){
                pathDescription.pop_back();
                pathDescription.back().necessary = true;
                pathDescription.back().turnInstruction = TurnInstructions.NoTurn;
                targetPhantom.nodeBasedEdgeNameID = (pathDescription.end()-2)->nameID;
                //            SimpleLogger().Write() << "Deleting last turn instruction";
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
                //            SimpleLogger().Write() << "Deleting first turn instruction, ratio: " << startPhantom.ratio << ", length: " << pathDescription[0].length;
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
};

#endif /* DESCRIPTIONFACTORY_H_ */
