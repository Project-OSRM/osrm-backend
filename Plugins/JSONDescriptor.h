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

#include <cstdio>
#include <string>
#include <vector>

#include "../typedefs.h"
#include "../DataStructures/ExtractorStructs.h"
#include "../Util/StrIngUtil.h"

#ifndef JSONDESCRIPTOR_H_
#define JSONDESCRIPTOR_H_

class JSONDescriptor {
public:
    template<class SearchEngineT>
    void Run(http::Reply& reply, std::vector< _PathData > * path, PhantomNodes * phantomNodes, SearchEngineT * sEngine, unsigned distance) {
        string tmp;
        string routeGeometryString;
        string routeSummaryString;
        string routeInstructionString;
        string startPointName;
        string endPointName;
        string direction = "East";
        string shortDirection = "E";
        int lastPosition = 0;
        int position = 0;
        int travelTimeOnSegment = 0;
        double lastAngle = 0.;

        reply.content += ("{\n");
        reply.content += ("  \"version\":0.3,\n");

        if(distance != UINT_MAX) {
            reply.content += ("  \"status\":0,\n");
            reply.content += ("  \"status_message\":");
            reply.content += "\"Found route between points\",\n";
            unsigned streetID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->startNode1, phantomNodes->startNode2);
            startPointName = sEngine->GetNameForNameID(streetID);
            streetID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            endPointName = sEngine->GetNameForNameID(streetID);

            routeInstructionString += "      [\"Head ";
            _Coordinate tmpCoord;
            if(path->size() > 0)
                sEngine->getNodeInfo(path->begin()->node, tmpCoord);
            else
                tmpCoord = phantomNodes->targetCoord;

            double angle = GetAngleBetweenTwoEdges(_Coordinate(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon), tmpCoord, _Coordinate(tmpCoord.lat, tmpCoord.lon-1000));
            if(angle >= 23 && angle < 67) {
                direction = "southeast";
                shortDirection = "SE";
            }
            if(angle >= 67 && angle < 113) {
                direction = "south";
                shortDirection = "S";
            }
            if(angle >= 113 && angle < 158) {
                direction = "southwest";
                shortDirection = "SW";
            }
            if(angle >= 158 && angle < 202) {
                direction = "west";
                shortDirection = "W";
            }
            if(angle >= 202 && angle < 248) {
                direction = "northwest";
                shortDirection = "NW";
            }
            if(angle >= 248 && angle < 292) {
                direction = "north";
                shortDirection = "N";
            }
            if(angle >= 292 && angle < 336) {
                direction = "northeast";
                shortDirection = "NE";
            }

            routeInstructionString += direction;

            //put start coord to linestring;
            convertLatLon(phantomNodes->startCoord.lat, tmp);
            routeGeometryString += "      [";
            routeGeometryString += tmp;
            routeGeometryString += ", ";
            convertLatLon(phantomNodes->startCoord.lon, tmp);
            routeGeometryString += tmp;
            routeGeometryString += "],\n";
            position++;

            _Coordinate previous(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon);
            _Coordinate next, current, lastPlace;
            stringstream numberString;
            stringstream intNumberString;

            double tempDist = 0;
            double entireDistance = 0;
            double distanceOfInstruction = 0;
            NodeID nextID = UINT_MAX;
            NodeID nameID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->startNode1, phantomNodes->startNode2);
            short type = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->startNode1, phantomNodes->startNode2);
            lastPlace.lat = phantomNodes->startCoord.lat;
            lastPlace.lon = phantomNodes->startCoord.lon;
            short nextType = SHRT_MAX;
            short prevType = SHRT_MAX;
            for(vector< _PathData >::iterator it = path->begin(); it != path->end(); it++) {
                sEngine->getNodeInfo(it->node, current);
                convertLatLon(current.lat, tmp);
                routeGeometryString += "      [";
                routeGeometryString += tmp;
                routeGeometryString += ", ";
                convertLatLon(current.lon, tmp);
                routeGeometryString += tmp;
                routeGeometryString += "],\n";
                position++;

                if(it==path->end()-1){
                    next = _Coordinate(phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
                    nextID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
                    nextType = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
                } else {
                    sEngine->getNodeInfo((it+1)->node, next);
                    nextID = sEngine->GetNameIDForOriginDestinationNodeID(it->node, (it+1)->node);
                    nextType = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(it->node, (it+1)->node);
                }

                if(nextID == nameID) {
                    tempDist += ApproximateDistance(previous.lat, previous.lon, current.lat, current.lon);
                } else {
                    if(type == 0 && prevType != 0)
                        routeInstructionString += ",enter motorway and ";
                    if(type != 0 && prevType == 0 )
                        routeInstructionString += ",leave motorway and ";
                    routeInstructionString += " on ";
                    if(nameID != 0)
                        routeInstructionString += sEngine->GetNameForNameID(nameID);
                    routeInstructionString += "\",";
                    distanceOfInstruction = ApproximateDistance(previous.lat, previous.lon, current.lat, current.lon)+tempDist;
                    entireDistance += distanceOfInstruction;
                    intNumberString.str("");
                    intNumberString << 10*(round(distanceOfInstruction/10.));;
                    routeInstructionString += intNumberString.str();
                    routeInstructionString += ",";
                    intNumberString.str("");
                    intNumberString << lastPosition;
                    lastPosition = position;
                    routeInstructionString += intNumberString.str();
                    routeInstructionString += ",";
                    intNumberString.str("");
                    intNumberString << travelTimeOnSegment;
                    routeInstructionString += intNumberString.str();
                    routeInstructionString += ",\"";
                    intNumberString.str("");
                    intNumberString << 10*(round(distanceOfInstruction/10.));;
                    routeInstructionString += intNumberString.str();
                    routeInstructionString += "m\",\"";
                    routeInstructionString += shortDirection;
                    routeInstructionString += "\",";
                    numberString.str("");
                    numberString << fixed << setprecision(2) << lastAngle;
                    routeInstructionString += numberString.str();
                    routeInstructionString += "],\n";

                    string lat; string lon;
                    convertLatLon(lastPlace.lon, lon);
                    convertLatLon(lastPlace.lat, lat);
                    lastPlace = current;
                    routeInstructionString += "      [\"";

                    double angle = GetAngleBetweenTwoEdges(previous, current, next);
                    if(angle > 160 && angle < 200) {
                        routeInstructionString += /* " (" << angle << ")*/"Continue";
                    } else if (angle > 290 && angle <= 360) {
                        routeInstructionString += /*" (" << angle << ")*/ "Turn sharp left";
                    } else if (angle > 230 && angle <= 290) {
                        routeInstructionString += /*" (" << angle << ")*/ "Turn left";
                    } else if (angle > 200 && angle <= 230) {
                        routeInstructionString += /*" (" << angle << ") */"Bear left";
                    } else if (angle > 130 && angle <= 160) {
                        routeInstructionString += /*" (" << angle << ") */"Bear right";
                    } else if (angle > 70 && angle <= 130) {
                        routeInstructionString += /*" (" << angle << ") */"Turn right";
                    } else {
                        routeInstructionString += /*" (" << angle << ") */"Turn sharp right";
                    }
                    lastAngle = angle;
                    tempDist = 0;
                    prevType = type;
                }
                nameID = nextID;
                previous = current;
                type = nextType;
            }
            nameID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            type = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            routeInstructionString += " at ";
            routeInstructionString += sEngine->GetNameForNameID(nameID);
            routeInstructionString += "\",";
            distanceOfInstruction = ApproximateDistance(previous.lat, previous.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon) + tempDist;
            entireDistance += distanceOfInstruction;
            intNumberString.str("");
            intNumberString << 10*(round(distanceOfInstruction/10.));;
            routeInstructionString += intNumberString.str();
            routeInstructionString += ",";
            numberString.str("");
            numberString << lastPosition;
            routeInstructionString += numberString.str();
            routeInstructionString += ",";
            numberString.str("");
            numberString << travelTimeOnSegment;
            routeInstructionString += numberString.str();
            routeInstructionString += ",\"";
            intNumberString.str("");
            intNumberString << 10*(round(distanceOfInstruction/10.));;
            routeInstructionString += intNumberString.str();
            routeInstructionString += "m\",\"";
            routeInstructionString += shortDirection;
            routeInstructionString += "\",";
            numberString.str("");
            numberString << fixed << setprecision(2) << lastAngle;
            routeInstructionString += numberString.str();
            routeInstructionString += "]\n";

            string lat; string lon;

            //put targetCoord to linestring
            convertLatLon(phantomNodes->targetCoord.lat, tmp);
            routeGeometryString += "      [";
            routeGeometryString += tmp;
            routeGeometryString += ", ";
            convertLatLon(phantomNodes->targetCoord.lon, tmp);
            routeGeometryString += tmp;
            routeGeometryString += "]\n";
            position++;

            //give complete distance in meters;
            ostringstream s;
            s << 10*(round(entireDistance/10.));
            routeSummaryString += "      \"total_distance\":";
            routeSummaryString += s.str();
            routeSummaryString += ",\n      \"total_time\":";
            //give travel time in minutes
            int travelTime = (distance/60) + 1;
            s.str("");
            s << travelTime;
            routeSummaryString += s.str();
            routeSummaryString += ",\n      \"start_point\":\"";
            routeSummaryString += startPointName;
            routeSummaryString += "\",\n      \"end_point\":\"";
            routeSummaryString += endPointName;
            routeSummaryString += "\"\n";
        } else {
            reply.content += ("  \"status\":207,\n");
            reply.content += ("  \"status_message\":");
            reply.content += "\"Cannot find route between points\",\n";
            routeSummaryString += "      \"total_distance\":0";
            routeSummaryString += ",\n      \"total_time\":0";
            routeSummaryString += ",\n      \"start_point\":\"";
            routeSummaryString += "\",\n      \"end_point\":\"";
            routeSummaryString += "\"\n";
        }
        reply.content += "   \"route_summary\": {\n";
        reply.content += routeSummaryString;
        reply.content += "   },\n";
        reply.content += "   \"route_geometry\": [\n";
        reply.content += routeGeometryString;
        reply.content += "   ],\n";
        reply.content += "   \"route_instructions\": [\n";
        reply.content += routeInstructionString;
        reply.content += "   ],\n";
        reply.content += "   \"transactionId\": \"OSRM Routing Engine JSON Descriptor (beta)\"\n";
        reply.content += "}";

        reply.headers.resize(3);
        reply.headers[0].name = "Content-Length";
        intToString(reply.content.size(), tmp);
        reply.headers[0].value = tmp;
        reply.headers[1].name = "Content-Type";
        reply.headers[1].value = "application/vnd.google-earth.kml+xml; charset=UTF-8";
        reply.headers[2].name = "Content-Disposition";
        reply.headers[2].value = "attachment; filename=\"route.kml\"";

    }
private:
};
#endif /* JSONDESCRIPTOR_H_ */
