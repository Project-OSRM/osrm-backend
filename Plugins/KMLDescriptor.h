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

template<class SearchEngineT, bool SimplifyRoute = false>
class KMLDescriptor : public BaseDescriptor<SearchEngineT>{
public:
    void SetZoom(const unsigned short z) { }
    void Run(http::Reply& reply, std::vector< _PathData > * path, PhantomNodes * phantomNodes, SearchEngineT * sEngine, unsigned distance) {
        string tmp;
        string lineString;
        string startName;
        string targetName;
        string direction = "East";
        reply.content += ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        reply.content += ("<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n");
        reply.content += ("<Document>\n");

        if(distance != UINT_MAX) {
            unsigned streetID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->startNode1, phantomNodes->startNode2);
            startName = sEngine->GetNameForNameID(streetID);
            streetID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            targetName = sEngine->GetNameForNameID(streetID);

            reply.content += ("\t<Placemark>\n");
            reply.content += ("\t\t<name><![CDATA[Start from ");
            reply.content += startName;
            reply.content += (" direction ");
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

            reply.content += direction;

            reply.content += ("]]></name>\n");

            //put start coord to linestring;
            convertLatLon(phantomNodes->startCoord.lon, tmp);
            lineString += tmp;
            lineString += ",";
            convertLatLon(phantomNodes->startCoord.lat, tmp);
            lineString += tmp;
            lineString += " ";

            reply.content += ("\t</Placemark>\n");
            _Coordinate previous(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon);
            _Coordinate next, current, lastPlace, startOfSegment;
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
                startOfSegment = previous;
                if(it==path->end()-1){
                    next = _Coordinate(phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
                    nextID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
                    nextType = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
                } else {
                    sEngine->getNodeInfo((it+1)->node, next);
                    nextID = sEngine->GetNameIDForOriginDestinationNodeID(it->node, (it+1)->node);
                    nextType = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(it->node, (it+1)->node);
                }

                double angle = GetAngleBetweenTwoEdges(startOfSegment, current, next);
                if(178 > angle || 182 < angle || false == SimplifyRoute) {
                    convertLatLon(current.lon, tmp);
                    lineString += tmp;
                    lineString += ",";
                    convertLatLon(current.lat, tmp);
                    lineString += tmp;
                    lineString += " ";
                    startOfSegment = current;
                }

                if(nextID == nameID) {
                    tempDist += ApproximateDistance(previous.lat, previous.lon, current.lat, current.lon);
                } else {
                    if(type == 0 && prevType != 0)
                        reply.content += "enter motorway and ";
                    if(type != 0 && prevType == 0 )
                        reply.content += "leave motorway and ";

                    //double angle = GetAngleBetweenTwoEdges(previous, current, next);
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
//            reply.content += " (type: ";
//            numberString << type;
//            reply.content += numberString.str();
//            numberString.str("");
//            reply.content += ", id: ";
//            numberString << nameID;
//            reply.content += numberString.str();
//            numberString.str(")");
            reply.content += "]]></name>\n\t\t<description>drive for ";
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
                    "\t\t<name><![CDATA[Route from ";
            reply.content += startName;
            reply.content += " to ";
            reply.content += targetName;
            reply.content += "]]></name>\n"
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
    }
private:
};
#endif /* KML_DESCRIPTOR_H_ */
