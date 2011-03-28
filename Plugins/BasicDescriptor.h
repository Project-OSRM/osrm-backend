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

#ifndef BASICDESCRIPTOR_H_
#define BASICDESCRIPTOR_H_

struct _PathData {
    _PathData(NodeID n) : node(n) { }
    NodeID node;
};

class BasicDescriptor {
public:
    template<class SearchEngineT>
    void Run(http::Reply& reply, std::vector< _PathData > * path, PhantomNodes * phantomNodes, SearchEngineT * sEngine, unsigned distance) {
        string tmp;
        string lineString;
        reply.content += ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        reply.content += ("<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n");
        reply.content += ("<Document>\n");
        //todo reply.content += ("<name><![CDATA[<start> to <target>]]><name>");
        reply.content += ("\t<Placemark>\n");
        reply.content += ("\t\t<name><![CDATA[Start from <start> direction <northeast>]]></name>\n");

        //put start coord to linestring;
        convertLatLon(phantomNodes->startCoord.lon, tmp);
        lineString += tmp;
        lineString += ",";
        convertLatLon(phantomNodes->startCoord.lat, tmp);
        lineString += tmp;
        lineString += " ";

        reply.content += ("\t</Placemark>\n");
        if(distance != UINT_MAX) {
            _Coordinate previous(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon);
            _Coordinate next, current, lastPlace;
            stringstream numberString;

            double tempDist = 0;
            double entireDistance = 0;
            double lengthOfInstruction = 0;
            NodeID nextID = UINT_MAX;
            NodeID nameID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->startNode1, phantomNodes->startNode2);
            short type = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->startNode1, phantomNodes->startNode2);
            lastPlace.lat = phantomNodes->startCoord.lat;
            lastPlace.lon = phantomNodes->startCoord.lon;
            short nextType = SHRT_MAX;
            short prevType = SHRT_MAX;
            reply.content += "\t<Placemark>\n\t\t<name><![CDATA[";
            for(vector< _PathData >::iterator it = path->begin(); it != path->end(); it++) {
                sEngine->getNodeInfo(it->node, current);
                convertLatLon(current.lon, tmp);
                lineString += tmp;
                lineString += ",";
                convertLatLon(current.lat, tmp);
                lineString += tmp;
                lineString += " ";

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
                        reply.content += "enter motorway and ";
                    if(type != 0 && prevType == 0 )
                        reply.content += "leave motorway and ";

                    double angle = GetAngleBetweenTwoEdges(previous, current, next);
                    reply.content += "follow road ";
                    if(nameID != 0)
                        reply.content += sEngine->GetNameForNameID(nameID);
                    /*
                reply.content += " (type: ";
                numberString << type;
                reply.content += numberString.str();
                numberString.str("");
                reply.content += ", id: ";
                numberString << nameID;
                reply.content += numberString.str();
                numberString.str("");
                reply.content += ")";
                     */
                    reply.content += "]]></name>\n\t\t<description>drive for ";
                    lengthOfInstruction = ApproximateDistance(previous.lat, previous.lon, current.lat, current.lon)+tempDist;
                    entireDistance += lengthOfInstruction;
                    numberString << 10*(round(lengthOfInstruction/10.));;
                    reply.content += numberString.str();
                    numberString.str("");
                    reply.content += "m</description>";
                    string lat; string lon;
                    convertLatLon(lastPlace.lon, lon);
                    convertLatLon(lastPlace.lat, lat);
                    lastPlace = current;
                    //                reply.content += "\n <Point>";
                    //                reply.content += "<Coordinates>";
                    //                reply.content += lon;
                    //                reply.content += ",";
                    //                reply.content += lat;
                    //                reply.content += "</Coordinates></Point>";
                    reply.content += "\n\t</Placemark>\n";
                    reply.content += "\t<Placemark>\n\t\t<name><![CDATA[";
                    if(angle > 160 && angle < 200) {
                        reply.content += /* " (" << angle << ")*/"drive ahead, ";
                    } else if (angle > 290 && angle <= 360) {
                        reply.content += /*" (" << angle << ")*/ "turn sharp left, ";
                    } else if (angle > 230 && angle <= 290) {
                        reply.content += /*" (" << angle << ")*/ "turn left, ";
                    } else if (angle > 200 && angle <= 230) {
                        reply.content += /*" (" << angle << ") */"bear left, ";
                    } else if (angle > 130 && angle <= 160) {
                        reply.content += /*" (" << angle << ") */"bear right, ";
                    } else if (angle > 70 && angle <= 130) {
                        reply.content += /*" (" << angle << ") */"turn right, ";
                    } else {
                        reply.content += /*" (" << angle << ") */"turn sharp right, ";
                    }
                    tempDist = 0;
                    prevType = type;
                }
                nameID = nextID;
                previous = current;
                type = nextType;
            }
            nameID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            type = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            reply.content += "follow road ";
            reply.content += sEngine->GetNameForNameID(nameID);
            reply.content += " (type: ";
            numberString << type;
            reply.content += numberString.str();
            numberString.str("");
            reply.content += ", id: ";
            numberString << nameID;
            reply.content += numberString.str();
            numberString.str("");
            reply.content += ")]]></name>\n\t\t<description>drive for ";
            lengthOfInstruction = ApproximateDistance(previous.lat, previous.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon) + tempDist;
            entireDistance += lengthOfInstruction;
            numberString << 10*(round(lengthOfInstruction/10.));
            reply.content += numberString.str();
            numberString.str("");
            reply.content += "m</description>\n ";
            string lat; string lon;
            //        convertLatLon(lastPlace.lon, lon);
            //        convertLatLon(lastPlace.lat, lat);
            //        reply.content += "<Point><Coordinates>";
            //        reply.content += lon;
            //        reply.content += ",";
            //        reply.content += lat;
            //        reply.content += "</Coordinates></Point>";
            reply.content += "\t</Placemark>\n";

            //put targetCoord to linestring
            convertLatLon(phantomNodes->targetCoord.lon, tmp);
            lineString += tmp;
            lineString += ",";
            convertLatLon(phantomNodes->targetCoord.lat, tmp);
            lineString += tmp;

            reply.content += "\t<Placemark>\n"
                    "\t\t<name>Route</name>\n"
                    "\t\t<description>"
                    "<![CDATA[Distance: ";

            //give complete distance in meters;
            ostringstream s;
            s << 10*(round(entireDistance/10.));
            reply.content += s.str();
            reply.content += "&#160;m (ca. ";

            //give travel time in minutes
            int travelTime = (distance/60) + 1;
            s.str("");
            s << travelTime;
            reply.content += s.str();

            reply.content += " minutes)]]>"
                    "</description>\n"
                    "\t\t<GeometryCollection>\n"
                    "\t\t\t<LineString>\n"
                    "\t\t\t\t<coordinates>";
            reply.content += lineString;
            reply.content += "</coordinates>\n"
                    "\t\t\t</LineString>\n"
                    "\t\t</GeometryCollection>\n"
                    "\t</Placemark>\n";
        }
        reply.content += "</Document>\n</kml>";

        reply.headers.resize(3);
        reply.headers[0].name = "Content-Length";
        intToString(reply.content.size(), tmp);
        reply.headers[0].value = tmp;
        reply.headers[1].name = "Content-Type";
        reply.headers[1].value = "application/vnd.google-earth.kml+xml";
        reply.headers[2].name = "Content-Disposition";
        reply.headers[2].value = "attachment; filename=\"route.kml\"";

    }
private:
};
#endif /* BASICDESCRIPTOR_H_ */
