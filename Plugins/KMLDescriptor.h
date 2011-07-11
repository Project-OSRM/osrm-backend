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

#ifndef KML_DESCRIPTOR_H_
#define KML_DESCRIPTOR_H_

#include "BaseDescriptor.h"

template<class SearchEngineT>
class KMLDescriptor : public BaseDescriptor<SearchEngineT>{
private:
    _DescriptorConfig config;
    RouteSummary summary;
    DirectionOfInstruction directionOfInstruction;
    DescriptorState descriptorState;
    std::string tmp;

public:
    KMLDescriptor() {}
    void SetConfig(const _DescriptorConfig & c) { config = c; }

    void Run(http::Reply & reply, RawRouteData *rawRoute, PhantomNodes *phantomNodes, SearchEngineT *sEngine, unsigned  distance) {
        WriteHeaderToOutput(reply.content);

        //We do not need to do much, if there is no route ;-)
        if(distance != UINT_MAX && rawRoute->routeSegments.size() > 0) {

            //Put first segment of route into geometry
            appendCoordinateToString(phantomNodes->startPhantom.location, descriptorState.routeGeometryString);
            descriptorState.startOfSegmentCoordinate = phantomNodes->startPhantom.location;
            //Generate initial instruction for start of route (order of NodeIDs does not matter, its the same name anyway)
            summary.startName = sEngine->GetEscapedNameForOriginDestinationNodeID(phantomNodes->targetPhantom.startNode, phantomNodes->startPhantom.startNode);
            descriptorState.lastNameID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetPhantom.startNode, phantomNodes->startPhantom.startNode);

            //If we have a route, i.e. start and dest not on same edge, than get it
            if(rawRoute->routeSegments[0].size() > 0)
                sEngine->getCoordinatesForNodeID(rawRoute->routeSegments[0].begin()->node, descriptorState.tmpCoord);
            else
                descriptorState.tmpCoord = phantomNodes->targetPhantom.location;

            descriptorState.previousCoordinate = phantomNodes->startPhantom.location;
            descriptorState.currentCoordinate = descriptorState.tmpCoord;
            descriptorState.distanceOfInstruction += ApproximateDistance(descriptorState.previousCoordinate, descriptorState.currentCoordinate);

            if(config.instructions) {
                //Get Heading
                double angle = GetAngleBetweenTwoEdges(_Coordinate(phantomNodes->startPhantom.location.lat, phantomNodes->startPhantom.location.lon), descriptorState.tmpCoord, _Coordinate(descriptorState.tmpCoord.lat, descriptorState.tmpCoord.lon-1000));
                getDirectionOfInstruction(angle, directionOfInstruction);
                appendInstructionNameToString(summary.startName, directionOfInstruction.direction, descriptorState.routeInstructionString, true);
            }
            NodeID lastNodeID = UINT_MAX;
            for(unsigned segmentIdx = 0; segmentIdx < rawRoute->routeSegments.size(); segmentIdx++) {
                const std::vector< _PathData > & path = rawRoute->routeSegments[segmentIdx];
                if( ! path.size() )
                    continue;

                if ( UINT_MAX == lastNodeID) {
                    lastNodeID = (phantomNodes->startPhantom.startNode == (*path.begin()).node ? phantomNodes->targetPhantom.startNode : phantomNodes->startPhantom.startNode);
                }
                //Check, if there is overlap between current and previous route segment
                //if not, than we are fine and can route over this edge without paying any special attention.
                if(lastNodeID == (*path.begin()).node) {
                    appendCoordinateToString(descriptorState.currentCoordinate, descriptorState.routeGeometryString);
                    lastNodeID = (lastNodeID == rawRoute->segmentEndCoordinates[segmentIdx].startPhantom.startNode ? rawRoute->segmentEndCoordinates[segmentIdx].targetPhantom.startNode : rawRoute->segmentEndCoordinates[segmentIdx].startPhantom.startNode);

                    //output of the via nodes coordinates
                    appendCoordinateToString(rawRoute->segmentEndCoordinates[segmentIdx].startPhantom.location, descriptorState.routeGeometryString);
                    descriptorState.currentNameID = sEngine->GetNameIDForOriginDestinationNodeID(rawRoute->segmentEndCoordinates[segmentIdx].startPhantom.startNode, rawRoute->segmentEndCoordinates[segmentIdx].targetPhantom.startNode);
                    //Make a special announement to do a U-Turn.
                    appendInstructionLengthToString(descriptorState.distanceOfInstruction, descriptorState.routeInstructionString);

                    descriptorState.distanceOfInstruction = ApproximateDistance(descriptorState.currentCoordinate, rawRoute->segmentEndCoordinates[segmentIdx].startPhantom.location);
                    getTurnDirectionOfInstruction(descriptorState.GetAngleBetweenCoordinates(), tmp);
                    appendInstructionNameToString(sEngine->GetEscapedNameForNameID(descriptorState.currentNameID), tmp, descriptorState.routeInstructionString);
                    appendInstructionLengthToString(descriptorState.distanceOfInstruction, descriptorState.routeInstructionString);
                    tmp = "U-turn at via point";
                    appendInstructionNameToString(sEngine->GetEscapedNameForNameID(descriptorState.currentNameID), tmp, descriptorState.routeInstructionString);
                    double tmpDistance = descriptorState.distanceOfInstruction;
                    descriptorState.SetStartOfSegment(); //Set start of segment but save distance information.
                    descriptorState.distanceOfInstruction = tmpDistance;
                } else if(segmentIdx > 0) { //We are going straight through an edge which is carrying the via point.
                    assert(segmentIdx != 0);
                    //routeInstructionString += "\nreaching via node: \n";
                    descriptorState.nextCoordinate = rawRoute->segmentEndCoordinates[segmentIdx].startPhantom.location;
                    descriptorState.currentNameID = sEngine->GetNameIDForOriginDestinationNodeID(rawRoute->segmentEndCoordinates[segmentIdx].startPhantom.startNode, rawRoute->segmentEndCoordinates[segmentIdx].targetPhantom.startNode);
                    appendCoordinateToString(descriptorState.currentCoordinate, descriptorState.routeGeometryString);
                    appendCoordinateToString(rawRoute->segmentEndCoordinates[segmentIdx].startPhantom.location, descriptorState.routeGeometryString);
                    if(config.instructions) {
                        double turnAngle = descriptorState.GetAngleBetweenCoordinates();
                        appendInstructionLengthToString(descriptorState.distanceOfInstruction, descriptorState.routeInstructionString);

                        getTurnDirectionOfInstruction(turnAngle, tmp);
                        tmp += " and reach via point";
                        appendInstructionNameToString(sEngine->GetEscapedNameForNameID(descriptorState.currentNameID), tmp, descriptorState.routeInstructionString);

                        //instruction to continue on the segment
                        appendInstructionLengthToString(ApproximateDistance(descriptorState.currentCoordinate, descriptorState.nextCoordinate), descriptorState.routeInstructionString);
                        appendInstructionNameToString(sEngine->GetEscapedNameForNameID(descriptorState.currentNameID), "Continue on", descriptorState.routeInstructionString);

                        //note the new segment starting coordinates
                        descriptorState.SetStartOfSegment();
                        descriptorState.previousCoordinate = descriptorState.currentCoordinate;
                        descriptorState.currentCoordinate = descriptorState.nextCoordinate;
                    } else {
                        assert(false);
                    }
                }

                for(vector< _PathData >::const_iterator it = path.begin(); it != path.end(); it++) {
                    sEngine->getCoordinatesForNodeID(it->node, descriptorState.nextCoordinate);
                    descriptorState.currentNameID = sEngine->GetNameIDForOriginDestinationNodeID(lastNodeID, it->node);

                    double area = fabs(0.5*( descriptorState.startOfSegmentCoordinate.lon*(descriptorState.nextCoordinate.lat - descriptorState.currentCoordinate.lat) + descriptorState.nextCoordinate.lon*(descriptorState.currentCoordinate.lat - descriptorState.startOfSegmentCoordinate.lat) + descriptorState.currentCoordinate.lon*(descriptorState.startOfSegmentCoordinate.lat - descriptorState.nextCoordinate.lat)  ) );
                    //if route is generalization does not skip this point, add it to description
                    if( it==path.end()-1 || config.z == 19 || area >= areaThresholds[config.z] || (false == descriptorState.CurrentAndPreviousNameIDsEqual()) ) {
                        //mark the beginning of the segment thats announced
                        appendCoordinateToString(descriptorState.currentCoordinate, descriptorState.routeGeometryString);
                        if( ( false == descriptorState.CurrentAndPreviousNameIDsEqual() ) && config.instructions) {
                            appendInstructionLengthToString(descriptorState.distanceOfInstruction, descriptorState.routeInstructionString);
                            getTurnDirectionOfInstruction(descriptorState.GetAngleBetweenCoordinates(), tmp);
                            appendInstructionNameToString(sEngine->GetEscapedNameForNameID(descriptorState.currentNameID), tmp, descriptorState.routeInstructionString);

                            //note the new segment starting coordinates
                            descriptorState.SetStartOfSegment();
                        }
                    }
                    descriptorState.distanceOfInstruction += ApproximateDistance(descriptorState.currentCoordinate, descriptorState.nextCoordinate);
                    lastNodeID = it->node;
                    if(it != path.begin()) {
                        descriptorState.previousCoordinate = descriptorState.currentCoordinate;
                        descriptorState.currentCoordinate = descriptorState.nextCoordinate;
                    }
                }
            }
            descriptorState.currentNameID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->startPhantom.targetNode, phantomNodes->targetPhantom.targetNode);
            descriptorState.nextCoordinate = phantomNodes->targetPhantom.location;
            appendCoordinateToString(descriptorState.currentCoordinate, descriptorState.routeGeometryString);

            if((false == descriptorState.CurrentAndPreviousNameIDsEqual()) && config.instructions) {
                appendInstructionLengthToString(descriptorState.distanceOfInstruction, descriptorState.routeInstructionString);
                getTurnDirectionOfInstruction(descriptorState.GetAngleBetweenCoordinates(), tmp);
                appendInstructionNameToString(sEngine->GetEscapedNameForNameID(descriptorState.currentNameID), tmp, descriptorState.routeInstructionString);
                descriptorState.distanceOfInstruction = 0;
            }
            summary.destName = sEngine->GetEscapedNameForNameID(descriptorState.currentNameID);
            descriptorState.distanceOfInstruction += ApproximateDistance(descriptorState.currentCoordinate, descriptorState.nextCoordinate);
            appendCoordinateToString(phantomNodes->targetPhantom.location, descriptorState.routeGeometryString);
            appendInstructionLengthToString(descriptorState.distanceOfInstruction, descriptorState.routeInstructionString);
            descriptorState.SetStartOfSegment();
            //compute distance/duration for route summary
            ostringstream s;
            s << 10*(round(descriptorState.entireDistance/10.));
            summary.lengthString = s.str();
            int travelTime = (distance/60) + 1;
            s.str("");
            s << travelTime;
            summary.durationString = s.str();

            //writing summary of route to reply
            reply.content += "<Placemark><name><![CDATA[Route from ";
            reply.content += summary.startName;
            reply.content += " to ";
            reply.content += summary.destName;
            reply.content += "]]></name><description><![CDATA[Distance: ";
            reply.content += summary.lengthString;
            reply.content += "&#160;m (approx. ";
            reply.content += summary.durationString;
            reply.content += " minutes)]]></description>\n";

            reply.content += "<GeometryCollection><LineString><coordinates>";
            if(config.geometry)
                reply.content += descriptorState.routeGeometryString;
            reply.content += "</coordinates></LineString></GeometryCollection>";
            reply.content += "</Placemark>";

            //list all viapoints so that the client may display it
            std::cout << "number of segment endpoints in route: " << rawRoute->segmentEndCoordinates.size() << std::endl;
            for(unsigned segmentIdx = 1; (true == config.geometry) && (segmentIdx < rawRoute->segmentEndCoordinates.size()); segmentIdx++) {
                reply.content += "<Placemark>";
                reply.content += "<name>Via Point 1</name>";
                reply.content += "<Point>";
                reply.content += "<coordinates>";
                appendCoordinateToString(rawRoute->segmentEndCoordinates[segmentIdx].startPhantom.location, reply.content);
                reply.content += "</coordinates>";
                reply.content += "</Point>";
                reply.content += "</Placemark>";
            }
        }
        reply.content += descriptorState.routeInstructionString;
        reply.content += "</Document>\n</kml>";
        std::cout << descriptorState.routeInstructionString << std::endl;
    }
private:
    void appendCoordinateToString(const _Coordinate coordinate, std::string & output) {
        if(config.geometry) {
            convertInternalCoordinateToString(coordinate, tmp);
            output += tmp;
        }
    }

    void appendInstructionNameToString(const std::string & nameOfStreet, const std::string & instructionOrDirection, std::string &output, bool firstAdvice = false) {
        if(config.instructions) {
            output += "<placemark><name><![CDATA[";
            if(firstAdvice) {
                output += "Head on ";
                output += nameOfStreet;
                output += " direction ";
                output += instructionOrDirection;
            } else {
                output += instructionOrDirection;
                output += " on ";
                output += nameOfStreet;
            }
            output += "]]></name>";
        }
    }

    void appendInstructionLengthToString(unsigned length, std::string &output) {
        if(config.instructions){
            output += "\n\t<description>drive for ";
            intToString(10*(round(length/10.)), tmp);
            output += tmp;
            output += "m</description></placemark>";
            output += "\n";
        }
    }

    void WriteHeaderToOutput(std::string & output) {
        output = ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n<Document>\n");
    }
};
#endif /* KML_DESCRIPTOR_H_ */
