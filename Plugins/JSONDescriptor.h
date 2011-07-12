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

#ifndef JSON_DESCRIPTOR_H_
#define JSON_DESCRIPTOR_H_

#include "BaseDescriptor.h"
#include "../DataStructures/PolylineCompressor.h"

template<class SearchEngineT>
class JSONDescriptor : public BaseDescriptor<SearchEngineT>{
private:
    _DescriptorConfig config;
    RouteSummary summary;
    DirectionOfInstruction directionOfInstruction;
    DescriptorState descriptorState;
    std::string tmp;
    vector<_Coordinate> polyline;

public:
    JSONDescriptor() {}
    void SetConfig(const _DescriptorConfig & c) { config = c; }

    void Run(http::Reply & reply, RawRouteData &rawRoute, PhantomNodes &phantomNodes, SearchEngineT &sEngine, unsigned distance) {
        WriteHeaderToOutput(reply.content);
        //We do not need to do much, if there is no route ;-)

        if(distance != UINT_MAX && rawRoute.routeSegments.size() > 0) {
            reply.content += "0,"
                    "\"status_message\": \"Found route between points\",";

            //Put first segment of route into geometry
            polyline.push_back(phantomNodes.startPhantom.location);
            descriptorState.geometryCounter++;
            descriptorState.startOfSegmentCoordinate = phantomNodes.startPhantom.location;
            //Generate initial instruction for start of route (order of NodeIDs does not matter, its the same name anyway)
            summary.startName = sEngine.GetEscapedNameForOriginDestinationNodeID(phantomNodes.startPhantom.startNode, phantomNodes.startPhantom.targetNode);
            descriptorState.lastNameID = sEngine.GetNameIDForOriginDestinationNodeID(phantomNodes.startPhantom.startNode, phantomNodes.startPhantom.targetNode);

            //If we have a route, i.e. start and dest not on same edge, than get it
            if(rawRoute.routeSegments[0].size() > 0)
                sEngine.getCoordinatesForNodeID(rawRoute.routeSegments[0].begin()->node, descriptorState.tmpCoord);
            else
                descriptorState.tmpCoord = phantomNodes.targetPhantom.location;

            descriptorState.previousCoordinate = phantomNodes.startPhantom.location;
            descriptorState.currentCoordinate = descriptorState.tmpCoord;
            descriptorState.distanceOfInstruction += ApproximateDistance(descriptorState.previousCoordinate, descriptorState.currentCoordinate);

            if(config.instructions) {
                //Get Heading
                double angle = GetAngleBetweenTwoEdges(_Coordinate(phantomNodes.startPhantom.location.lat, phantomNodes.startPhantom.location.lon), descriptorState.tmpCoord, _Coordinate(descriptorState.tmpCoord.lat, descriptorState.tmpCoord.lon-1000));
                getDirectionOfInstruction(angle, directionOfInstruction);
                appendInstructionNameToString(summary.startName, directionOfInstruction.direction, descriptorState.routeInstructionString, true);
            }
            NodeID lastNodeID = UINT_MAX;

            for(unsigned segmentIdx = 0; segmentIdx < rawRoute.routeSegments.size(); segmentIdx++) {
                const std::vector< _PathData > & path = rawRoute.routeSegments[segmentIdx];
                if(path.empty())
                    continue;
                if ( UINT_MAX == lastNodeID) {
                    lastNodeID = (phantomNodes.startPhantom.startNode == (*path.begin()).node ? phantomNodes.startPhantom.targetNode : phantomNodes.startPhantom.startNode);
                }
                //Check, if there is overlap between current and previous route segment
                //if not, than we are fine and can route over this edge without paying any special attention.
                if(lastNodeID == (*path.begin()).node) {
                    //                    appendCoordinateToString(descriptorState.currentCoordinate, descriptorState.routeGeometryString);
                    polyline.push_back(descriptorState.currentCoordinate);
                    descriptorState.geometryCounter++;
                    lastNodeID = (lastNodeID == rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.startNode ? rawRoute.segmentEndCoordinates[segmentIdx].targetPhantom.startNode : rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.startNode);

                    //output of the via nodes coordinates
                    polyline.push_back(rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.location);
                    descriptorState.geometryCounter++;
                    descriptorState.currentNameID = sEngine.GetNameIDForOriginDestinationNodeID(rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.startNode, rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.targetNode);
                    //Make a special announement to do a U-Turn.
                    appendInstructionLengthToString(descriptorState.distanceOfInstruction, descriptorState.routeInstructionString);
                    descriptorState.routeInstructionString += ",";
                    descriptorState.distanceOfInstruction = ApproximateDistance(descriptorState.currentCoordinate, rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.location);
                    getTurnDirectionOfInstruction(descriptorState.GetAngleBetweenCoordinates(), tmp);
                    appendInstructionNameToString(sEngine.GetEscapedNameForNameID(descriptorState.currentNameID), tmp, descriptorState.routeInstructionString);
                    appendInstructionLengthToString(descriptorState.distanceOfInstruction, descriptorState.routeInstructionString);
                    descriptorState.routeInstructionString += ",";
                    tmp = "U-turn at via point";
                    appendInstructionNameToString(sEngine.GetEscapedNameForNameID(descriptorState.currentNameID), tmp, descriptorState.routeInstructionString);
                    double tmpDistance = descriptorState.distanceOfInstruction;
                    descriptorState.SetStartOfSegment(); //Set start of segment but save distance information.
                    descriptorState.distanceOfInstruction = tmpDistance;
                } else if(segmentIdx > 0) { //We are going straight through an edge which is carrying the via point.
                    assert(segmentIdx != 0);
                    //routeInstructionString += "reaching via node: ";
                    descriptorState.nextCoordinate = rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.location;
                    descriptorState.currentNameID = sEngine.GetNameIDForOriginDestinationNodeID(rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.startNode, rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.targetNode);

                    polyline.push_back(descriptorState.currentCoordinate);
                    descriptorState.geometryCounter++;
                    polyline.push_back(rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.location);
                    descriptorState.geometryCounter++;
                    if(config.instructions) {
                        double turnAngle = descriptorState.GetAngleBetweenCoordinates();
                        appendInstructionLengthToString(descriptorState.distanceOfInstruction, descriptorState.routeInstructionString);
                        descriptorState.SetStartOfSegment();
                        descriptorState.routeInstructionString += ",";
                        getTurnDirectionOfInstruction(turnAngle, tmp);
                        tmp += " and reach via point";
                        appendInstructionNameToString(sEngine.GetEscapedNameForNameID(descriptorState.currentNameID), tmp, descriptorState.routeInstructionString);

                        //instruction to continue on the segment
                        appendInstructionLengthToString(ApproximateDistance(descriptorState.currentCoordinate, descriptorState.nextCoordinate), descriptorState.routeInstructionString);
                        descriptorState.routeInstructionString += ",";
                        appendInstructionNameToString(sEngine.GetEscapedNameForNameID(descriptorState.currentNameID), "Continue on", descriptorState.routeInstructionString);

                        //note the new segment starting coordinates
                        descriptorState.SetStartOfSegment();
                        descriptorState.previousCoordinate = descriptorState.currentCoordinate;
                        descriptorState.currentCoordinate = descriptorState.nextCoordinate;
                    }
                }
                for(vector< _PathData >::const_iterator it = path.begin(); it != path.end(); it++) {
                    sEngine.getCoordinatesForNodeID(it->node, descriptorState.nextCoordinate);
                    descriptorState.currentNameID = sEngine.GetNameIDForOriginDestinationNodeID(lastNodeID, it->node);

                    double area = fabs(0.5*( descriptorState.startOfSegmentCoordinate.lon*(descriptorState.nextCoordinate.lat - descriptorState.currentCoordinate.lat) + descriptorState.nextCoordinate.lon*(descriptorState.currentCoordinate.lat - descriptorState.startOfSegmentCoordinate.lat) + descriptorState.currentCoordinate.lon*(descriptorState.startOfSegmentCoordinate.lat - descriptorState.nextCoordinate.lat)  ) );
                    //if route is generalization does not skip this point, add it to description
                    if( it==path.end()-1 || config.z == 19 || area >= areaThresholds[config.z] || (false == descriptorState.CurrentAndPreviousNameIDsEqual()) ) {
                        //mark the beginning of the segment thats announced
                        //                        appendCoordinateToString(descriptorState.currentCoordinate, descriptorState.routeGeometryString);
                        polyline.push_back(descriptorState.currentCoordinate);
                        descriptorState.geometryCounter++;
                        if( ( false == descriptorState.CurrentAndPreviousNameIDsEqual() ) && config.instructions) {
                            appendInstructionLengthToString(descriptorState.distanceOfInstruction, descriptorState.routeInstructionString);
                            descriptorState.routeInstructionString += ",";
                            getTurnDirectionOfInstruction(descriptorState.GetAngleBetweenCoordinates(), tmp);
                            appendInstructionNameToString(sEngine.GetEscapedNameForNameID(descriptorState.currentNameID), tmp, descriptorState.routeInstructionString);

                            //note the new segment starting coordinates
                            descriptorState.SetStartOfSegment();
                        }
                    }
                    descriptorState.distanceOfInstruction += ApproximateDistance(descriptorState.currentCoordinate, descriptorState.nextCoordinate);
                    lastNodeID = it->node;
                    if(it != path.begin()-1) {
                        descriptorState.previousCoordinate = descriptorState.currentCoordinate;
                        descriptorState.currentCoordinate = descriptorState.nextCoordinate;
                    }
                }
            }

            descriptorState.currentNameID = sEngine.GetNameIDForOriginDestinationNodeID(phantomNodes.targetPhantom.startNode, phantomNodes.targetPhantom.targetNode);
            descriptorState.nextCoordinate = phantomNodes.targetPhantom.location;

            polyline.push_back(descriptorState.currentCoordinate);
            descriptorState.geometryCounter++;

            if((false == descriptorState.CurrentAndPreviousNameIDsEqual()) && config.instructions) {
                appendInstructionLengthToString(descriptorState.distanceOfInstruction, descriptorState.routeInstructionString);
                descriptorState.routeInstructionString += ",";
                getTurnDirectionOfInstruction(descriptorState.GetAngleBetweenCoordinates(), tmp);
                appendInstructionNameToString(sEngine.GetEscapedNameForNameID(descriptorState.currentNameID), tmp, descriptorState.routeInstructionString);
                descriptorState.distanceOfInstruction = 0;
                descriptorState.SetStartOfSegment();
            }
            summary.destName = sEngine.GetEscapedNameForNameID(descriptorState.currentNameID);
            descriptorState.distanceOfInstruction += ApproximateDistance(descriptorState.currentCoordinate, descriptorState.nextCoordinate);
            polyline.push_back(phantomNodes.targetPhantom.location);
            descriptorState.geometryCounter++;
            appendInstructionLengthToString(descriptorState.distanceOfInstruction, descriptorState.routeInstructionString);
            summary.BuildDurationAndLengthStrings(descriptorState.entireDistance, distance);

        } else {
            //no route found
            reply.content += "207,"
                    "\"status_message\": \"Cannot find route between points\",";
        }

        reply.content += "\"route_summary\": {"
                "\"total_distance\":";
        reply.content += summary.lengthString;
        reply.content += ","
                "\"total_time\":";
        reply.content += summary.durationString;
        reply.content += ","
                "\"start_point\":\"";
        reply.content += summary.startName;
        reply.content += "\","
                "\"end_point\":\"";
        reply.content += summary.destName;
        reply.content += "\"";
        reply.content += "},";
        reply.content += "\"route_geometry\": ";
        if(config.geometry) {
            if(config.encodeGeometry)
                config.pc.printEncodedString(polyline, descriptorState.routeGeometryString);
            else
                config.pc.printUnencodedString(polyline, descriptorState.routeGeometryString);

            reply.content += descriptorState.routeGeometryString;
        } else {
            reply.content += "[]";
        }

        reply.content += ","
                "\"route_instructions\": [";
        if(config.instructions)
            reply.content += descriptorState.routeInstructionString;
        reply.content += "],";
        //list all viapoints so that the client may display it
        reply.content += "\"via_points\":[";
        for(unsigned segmentIdx = 1; (true == config.geometry) && (segmentIdx < rawRoute.segmentEndCoordinates.size()); segmentIdx++) {
            if(segmentIdx > 1)
                reply.content += ",";
            reply.content += "[";
            if(rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.location.isSet())
                convertInternalReversedCoordinateToString(rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.location, tmp);
            else
                convertInternalReversedCoordinateToString(rawRoute.rawViaNodeCoordinates[segmentIdx], tmp);
            reply.content += tmp;
            reply.content += "]";
        }
        reply.content += "],"
                "\"transactionId\": \"OSRM Routing Engine JSON Descriptor (v0.2)\"";
        reply.content += "}";
    }
private:
    void appendInstructionNameToString(const std::string & nameOfStreet, const std::string & instructionOrDirection, std::string &output, bool firstAdvice = false) {
        output += "[";
        if(config.instructions) {
            output += "\"";
            if(firstAdvice) {
                output += "Head ";
            }
            output += instructionOrDirection;
            output += "\",\"";
            output += nameOfStreet;
            output += "\",";
        }
    }

    void appendInstructionLengthToString(unsigned length, std::string &output) {
        if(config.instructions){
            std::string tmpDistance;
            intToString(10*(round(length/10.)), tmpDistance);
            output += tmpDistance;
            output += ",";
            intToString(descriptorState.startIndexOfGeometry, tmp);
            output += tmp;
            output += ",";
            intToString(descriptorState.durationOfInstruction, tmp);
            output += tmp;
            output += ",";
            output += "\"";
            output += tmpDistance;
            output += "\",";
            double angle = descriptorState.GetAngleBetweenCoordinates();
            DirectionOfInstruction direction;
            getDirectionOfInstruction(angle, direction);
            output += "\"";
            output += direction.shortDirection;
            output += "\",";
            std::stringstream numberString;
            numberString << fixed << setprecision(2) << angle;
            output += numberString.str();
        }
        output += "]";
    }

    void WriteHeaderToOutput(std::string & output) {
        output += "{"
                "\"version\": 0.3,"
                "\"status\":";
    }
};
#endif /* JSON_DESCRIPTOR_H_ */
