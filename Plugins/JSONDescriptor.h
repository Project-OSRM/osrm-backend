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

#include "BaseDescriptor.h"
#include "../DataStructures/PolylineCompressor.h"

#ifndef JSONDESCRIPTOR_H_
#define JSONDESCRIPTOR_H_

static double areaThresholds[19] = { 5000, 5000, 5000, 5000, 5000, 2500, 2000, 1500, 800, 400, 250, 150, 100, 75, 25, 20, 10, 5, 0 };

template<class SearchEngineT>
class JSONDescriptor : public BaseDescriptor<SearchEngineT> {
private:
    DescriptorConfig config;
    vector<_Coordinate> polyline;
public:
    JSONDescriptor() {}
    void SetConfig(const DescriptorConfig c) {
        config = c;
    }

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
        unsigned durationOfInstruction = 0;
        double lastAngle = 0.;

        unsigned painted = 0;
        unsigned omitted = 0;

        reply.content += ("{");
        reply.content += ("\"version\":0.3,");

        if(distance != UINT_MAX) {
            reply.content += ("\"status\":0,");
            reply.content += ("\"status_message\":");
            reply.content += "\"Found route between points\",";
            unsigned streetID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->startNode1, phantomNodes->startNode2);
            startPointName = sEngine->GetEscapedNameForNameID(streetID);
            streetID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            endPointName = sEngine->GetEscapedNameForNameID(streetID);

            routeInstructionString += "[\"Head ";
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
            polyline.push_back(phantomNodes->startCoord);

            _Coordinate previous(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon);
            _Coordinate next, current, lastPlace, startOfSegment;
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
                startOfSegment = previous;

                if(it==path->end()-1){
                    next = _Coordinate(phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
                    nextID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
                    nextType = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
                    durationOfInstruction += sEngine->GetWeightForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
                } else {
                    sEngine->getNodeInfo((it+1)->node, next);
                    nextID = sEngine->GetNameIDForOriginDestinationNodeID(it->node, (it+1)->node);
                    nextType = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(it->node, (it+1)->node);
                    durationOfInstruction += sEngine->GetWeightForOriginDestinationNodeID(it->node, (it+1)->node);
                }
                double angle = GetAngleBetweenTwoEdges(startOfSegment, current, next);
                double area = fabs(0.5*( startOfSegment.lon*(current.lat - next.lat) + current.lon*(next.lat - startOfSegment.lat) + next.lon*(startOfSegment.lat - current.lat)  ) );
                //                std::cout << "Area for: " << area << std::endl;
                if(area > areaThresholds[config.z] || 19 == config.z) {
                    painted++;
                	polyline.push_back(current);

                    position++;
                    startOfSegment = current;
                } else {
                    omitted++;
                }
                if(nextID == nameID) {
                    double _dist = ApproximateDistance(previous.lat, previous.lon, current.lat, current.lon);
                    tempDist += _dist;
                    entireDistance += _dist;
                } else {
                    if(type == 0 && prevType != 0)
                        routeInstructionString += ",enter motorway, ";
                    if(type != 0 && prevType == 0 )
                        routeInstructionString += ",leave motorway, ";
                    routeInstructionString += "\", \"";
                    if(nameID != 0)
                        routeInstructionString += sEngine->GetEscapedNameForNameID(nameID);
                    routeInstructionString += "\",";
                    double _dist = ApproximateDistance(previous.lat, previous.lon, current.lat, current.lon);
                    distanceOfInstruction += _dist + tempDist;
                    entireDistance += _dist;
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
                    intNumberString << durationOfInstruction/10;
                    routeInstructionString += intNumberString.str();
                    routeInstructionString += ",\"";
                    intNumberString.str("");
                    intNumberString << 10*(round(distanceOfInstruction/10.));
                    routeInstructionString += intNumberString.str();
                    routeInstructionString += "m\",\"";
                    routeInstructionString += shortDirection;
                    routeInstructionString += "\",";
                    numberString.str("");
                    numberString << fixed << setprecision(2) << lastAngle;
                    routeInstructionString += numberString.str();
                    routeInstructionString += "],";

                    string lat; string lon;
                    lastPlace = current;
                    routeInstructionString += "[\"";

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
                    distanceOfInstruction = 0;
                    durationOfInstruction = 0;
                }
                nameID = nextID;
                previous = current;
                type = nextType;
            }
            nameID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            type = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            durationOfInstruction += sEngine->GetWeightForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            routeInstructionString += "\", \"";
            routeInstructionString += sEngine->GetEscapedNameForNameID(nameID);
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
            intNumberString.str("");
            intNumberString << durationOfInstruction/10;
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
            routeInstructionString += "]";

            string lat; string lon;

            //put targetCoord to linestring
            polyline.push_back(phantomNodes->targetCoord);
            position++;

            //give complete distance in meters;
            ostringstream s;
            s << 10*(round(entireDistance/10.));
            routeSummaryString += "\"total_distance\":";
            routeSummaryString += s.str();

            routeSummaryString += ",\"total_time\":";
            //give travel time in minutes
            int travelTime = distance;
            s.str("");
            s << travelTime;
            routeSummaryString += s.str();
            routeSummaryString += ",\"start_point\":\"";
            routeSummaryString += startPointName;
            routeSummaryString += "\",\"end_point\":\"";
            routeSummaryString += endPointName;
            routeSummaryString += "\"";
        } else {
            reply.content += ("\"status\":207,");
            reply.content += ("\"status_message\":");
            reply.content += "\"Cannot find route between points\",";
            routeSummaryString += "\"total_distance\":0";
            routeSummaryString += ",\"total_time\":0";
            routeSummaryString += ",\"start_point\":\"";
            routeSummaryString += "\",\"end_point\":\"";
            routeSummaryString += "\"";
        }
        reply.content += "\"route_summary\": {";
        reply.content += routeSummaryString;
        reply.content += "},";
        reply.content += "\"route_geometry\": [";
        if(config.geometry) {
        	if(config.encodeGeometry)
        		config.pc.printEncodedString(polyline, routeGeometryString);
        	else
        		config.pc.printUnencodedString(polyline, routeGeometryString);

            reply.content += routeGeometryString;
        }
        reply.content += "],";
        reply.content += "\"route_instructions\": [";
        if(config.instructions) {
            reply.content += routeInstructionString;
        }
        reply.content += "],";
        reply.content += "\"transactionId\": \"OSRM Routing Engine JSON Descriptor (beta)\"";
        reply.content += "}";
        //std::cout << "zoom: " << config.z << ", threshold: " << areaThresholds[config.z] << ", painted: " << painted << ", omitted: " << omitted << std::endl;
    }
};
#endif /* JSONDESCRIPTOR_H_ */
