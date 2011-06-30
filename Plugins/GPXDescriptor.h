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

#ifndef GPX_DESCRIPTOR_H_
#define GPX_DESCRIPTOR_H_

#include "BaseDescriptor.h"

template<class SearchEngineT>
class GPXDescriptor : public BaseDescriptor<SearchEngineT>{
private:
    DescriptorConfig config;
public:
    void SetConfig(const DescriptorConfig c) { config = c; }
    void Run(http::Reply& reply, std::vector< _PathData > * path, PhantomNodes * phantomNodes, SearchEngineT * sEngine, unsigned distance) {
        string tmp;
        string lineString;
        string startName;
        string targetName;
        double entireDistance = 0;
        string startLoc, endLoc, bodyString;
        string direction = "East";
        reply.content += ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        reply.content += "<gpx xmlns=\"http://www.topografix.com/GPX/1/1\"\n";
        reply.content += "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n";
        reply.content +="xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 gpx.xsd\">\n";

        reply.content += ("\t<extensions>\n");

        if(distance != UINT_MAX) {
            unsigned streetID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->startNode1, phantomNodes->startNode2);
            startName = sEngine->GetEscapedNameForNameID(streetID);
            streetID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            targetName = sEngine->GetEscapedNameForNameID(streetID);
            convertLatLon(phantomNodes->startCoord.lat, tmp);
            bodyString += ("\t\t<rtept lat=\""+tmp+"\"");
            convertLatLon(phantomNodes->startCoord.lon, tmp);
            bodyString += (" lon=\""+tmp+"\">");
            bodyString += ("\n\t\t\t<desc>Start from ");
            bodyString += startName;
            bodyString += (" and head ");
            _Coordinate tmpCoord;
            if(path->size() > 0)
                sEngine->getNodeInfo(path->begin()->node, tmpCoord);
            else
                tmpCoord = phantomNodes->targetCoord;

            int angle = round(GetAngleBetweenTwoEdges(_Coordinate(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon), tmpCoord, _Coordinate(tmpCoord.lat, tmpCoord.lon-1000)));
            if(angle >= 23 && angle < 67)
                direction = "South-East";
            if(angle >= 67 && angle < 113)
                direction = "South";
            if(angle >= 113 && angle < 158)
                direction = "South-West";
            if(angle >= 158 && angle < 202)
                direction = "West";
            if(angle >= 202 && angle < 248)
                direction = "North-West";
            if(angle >= 248 && angle < 292)
                direction = "North";
            if(angle >= 292 && angle < 336)
                direction = "North-East";

            bodyString += direction;

            bodyString += ("</desc>\n\t\t\t<extensions>\n\t\t\t\t<direction>"+direction+"</direction>\n");

            _Coordinate previous(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon);
            _Coordinate next, current, lastPlace, startOfSegment;
            stringstream numberString;

            double tempDist = 0;
            double lengthOfInstruction = 0;
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
                if(it==path->begin()){

                    tempDist += ApproximateDistance(previous.lat, previous.lon, current.lat, current.lon);
                    numberString << 10*(round(tempDist/10.));;
                    bodyString+="\t\t\t\t<distance>"+numberString.str() +"m</distance>\n";
                    numberString.str("");
                    bodyString+="\t\t\t</extensions>\n\t\t</rtept>";
                }

                if(it==path->end()-1){
                    next = _Coordinate(phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
                    nextID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
                    nextType = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
                } else {
                    sEngine->getNodeInfo((it+1)->node, next);
                    nextID = sEngine->GetNameIDForOriginDestinationNodeID(it->node, (it+1)->node);
                    nextType = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(it->node, (it+1)->node);
                }

                convertLatLon(current.lat, tmp);
                bodyString += "\n\t\t<rtept lat=\"" + tmp + "\" ";
                convertLatLon(current.lon, tmp);
                bodyString += "lon=\"" + tmp + "\">\n";

                double angle = GetAngleBetweenTwoEdges(startOfSegment, current, next);
                if(178 > angle || 182 < angle) {
                    convertLatLon(current.lon, tmp);
                    lineString += tmp;
                    lineString += ",";
                    convertLatLon(current.lat, tmp);
                    lineString += tmp;
                    lineString += " ";
                    startOfSegment = current;
                }

                if(nextID == nameID) {
                    int temp = ApproximateDistance(previous.lat, previous.lon, current.lat, current.lon);
                    tempDist += temp;
                    numberString << 10*(round(temp/10.));
                    bodyString += "\t\t\t<extensions>\n\t\t\t\t<distance>"+numberString.str()+"m</distance>\n\t\t\t</extensions>";
                    numberString.str("");
                } else {
                    bodyString += "\t\t\t<desc>";
                    if(type == 0 && prevType != 0)
                        bodyString += "enter motorway and ";
                    if(type != 0 && prevType == 0 )
                        bodyString += "leave motorway and ";

                    bodyString += "follow road ";
                    if(nameID != 0)
                        bodyString += sEngine->GetEscapedNameForNameID(nameID);
                    string lat; string lon;
                    convertLatLon(lastPlace.lon, lon);
                    convertLatLon(lastPlace.lat, lat);
                    lastPlace = current;
                    if(angle > 160 && angle < 200) {
                        bodyString += /* " (" << angle << ")*/"drive ahead, ";
                    } else if (angle > 290 && angle <= 360) {
                        bodyString += /*" (" << angle << ")*/ "turn sharp left, ";
                    } else if (angle > 230 && angle <= 290) {
                        bodyString += /*" (" << angle << ")*/ "turn left, ";
                    } else if (angle > 200 && angle <= 230) {
                        bodyString += /*" (" << angle << ") */"bear left, ";
                    } else if (angle > 130 && angle <= 160) {
                        bodyString += /*" (" << angle << ") */"bear right, ";
                    } else if (angle > 70 && angle <= 130) {
                        bodyString += /*" (" << angle << ") */"turn right, ";
                    } else {
                        bodyString += /*" (" << angle << ") */"turn sharp right, ";
                    }
                    bodyString += "</desc>\n\t\t\t<extensions>\n\t\t\t\t<distance>";
                    lengthOfInstruction = ApproximateDistance(previous.lat, previous.lon, current.lat, current.lon)+tempDist;
                    entireDistance += lengthOfInstruction;
                    numberString << 10*(round(lengthOfInstruction/10.));;
                    bodyString += numberString.str();
                    numberString.str("");
                    bodyString += "m</distance>\n";
                    tempDist = 0;
                    prevType = type;
                    bodyString += "\t\t\t</extensions>";
                }
                bodyString +="\n\t\t</rtept>";
                nameID = nextID;
                previous = current;
                type = nextType;
            }
            convertLatLon(phantomNodes->targetCoord.lat, tmp);
            bodyString += "\n\t\t<rtept lat=\"" + tmp + "\" ";
            convertLatLon(phantomNodes->targetCoord.lon, tmp);
            bodyString += "lon=\"" + tmp + "\">\n";
            bodyString += "\t\t\t<desc>";

            nameID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            type = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            bodyString += "follow road ";

            bodyString += sEngine->GetEscapedNameForNameID(nameID);
            bodyString += "</desc>\n\t\t\t<extensions>\n\t\t\t\t<distance>";

            lengthOfInstruction = ApproximateDistance(previous.lat, previous.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon) + tempDist;
            entireDistance += lengthOfInstruction;
            numberString << 10*(round(lengthOfInstruction/10.));
            bodyString += numberString.str();
            numberString.str("");
            bodyString += "m</distance>\n ";
            string lat; string lon;

            bodyString += "\t\t\t</extensions>\n\t\t</rtept>";


            //put targetCoord to linestring
            convertLatLon(phantomNodes->targetCoord.lon, tmp);
            lineString += tmp;
            lineString += ",";
            convertLatLon(phantomNodes->targetCoord.lat, tmp);
            lineString += tmp;
            ostringstream s;
            s << 10*(round(entireDistance/10.));
            reply.content += ("\t\t<distance>"+s.str()+"m</distance>\n");
            int travelTime = (distance/60) + 1;
            s.str("");
            s << travelTime;
            reply.content += ("\t\t<time>"+s.str()+"</time>\n");
            reply.content += ("\t</extensions>\n\t<rte>\n");

            if(config.geometry){
                reply.content +=(bodyString+"\n");
            }
        }
        else {
            reply.content += "\t<extensions>\n\t<rte>\n";
        }
        reply.content += "\t</rte>\n</gpx>";
    }
private:
};
#endif /* GPX_DESCRIPTOR_H_ */
