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
#include "../DataStructures/SegmentInformation.h"

/* This class is fed with all way segments in consecutive order
 *  and produces the description plus the encoded polyline */

class DescriptionFactory {
    DouglasPeucker<SegmentInformation> dp;
    PolylineCompressor pc;
    PhantomNode startPhantom, targetPhantom;
public:
    //I know, declaring this public is considered bad. I'm lazy
    std::vector <SegmentInformation> pathDescription;
    DescriptionFactory();
    virtual ~DescriptionFactory();
    double GetAngleBetweenCoordinates() const;
    void AppendEncodedPolylineString(std::string &output);
    void AppendUnencodedPolylineString(std::string &output);
    void AppendSegment(const _Coordinate & coordinate, const _PathData & data);
    void SetStartSegment(const PhantomNode & startPhantom);
    void SetEndSegment(const PhantomNode & startPhantom);
    void AppendEncodedPolylineString(std::string & output, bool isEncoded);
    unsigned Run(const unsigned zoomLevel);

};

#endif /* DESCRIPTIONFACTORY_H_ */

//private:
//    void appendInstructionNameToString(const std::string & nameOfStreet, const std::string & instructionOrDirection, std::string &output, bool firstAdvice = false) {
//        output += "[";
//        if(config.instructions) {
//            output += "\"";
//            if(firstAdvice) {
//                output += "Head ";
//            }
//            output += instructionOrDirection;
//            output += "\",\"";
//            output += nameOfStreet;
//            output += "\",";
//        }
//    }
//
//    void appendInstructionLengthToString(unsigned length, std::string &output) {
//        if(config.instructions){
//            std::string tmpDistance;
//            intToString(10*(round(length/10.)), tmpDistance);
//            output += tmpDistance;
//            output += ",";
//            intToString(descriptionFactory.startIndexOfGeometry, tmp);
//            output += tmp;
//            output += ",";
//            intToString(descriptionFactory.durationOfInstruction, tmp);
//            output += tmp;
//            output += ",";
//            output += "\"";
//            output += tmpDistance;
//            output += "\",";
//            double angle = descriptionFactory.GetAngleBetweenCoordinates();
//            DirectionOfInstruction direction;
//            getDirectionOfInstruction(angle, direction);
//            output += "\"";
//            output += direction.shortDirection;
//            output += "\",";
//            std::stringstream numberString;
//            numberString << fixed << setprecision(2) << angle;
//            output += numberString.str();
//        }
//        output += "]";
//    }
