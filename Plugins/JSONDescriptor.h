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

#include <boost/foreach.hpp>

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

            for(unsigned segmentIdx = 0; segmentIdx < rawRoute.routeSegments.size(); segmentIdx++) {
                const std::vector< _PathData > & path = rawRoute.routeSegments[segmentIdx];
                BOOST_FOREACH(_PathData pathData, path) {
                    _Coordinate current;
                    sEngine.GetCoordinatesForNodeID(pathData.node, current);
                    polyline.push_back(current);
//                    INFO(pathData.node << " at " << current.lat << "," << current.lon);
                    //INFO("routed over node: " << pathData.node);
                }
            }
            polyline.push_back(phantomNodes.targetPhantom.location);
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
