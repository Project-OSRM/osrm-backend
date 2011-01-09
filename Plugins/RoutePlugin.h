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

#ifndef ROUTEPLUGIN_H_
#define ROUTEPLUGIN_H_

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "BasePlugin.h"
#include "../DataStructures/StaticGraph.h"
#include "../DataStructures/SearchEngine.h"

#include "../Util/GraphLoader.h"
#include "../Util/StrIngUtil.h"

typedef ContractionCleanup::Edge::EdgeData EdgeData;
typedef StaticGraph<EdgeData>::InputEdge GridEdge;

class RoutePlugin : public BasePlugin {
public:
	RoutePlugin(std::string hsgrPath, std::string ramIndexPath, std::string fileIndexPath, std::string nodesPath, std::string namesPath) {
		//Init nearest neighbor data structure
		nodeHelpDesk = new NodeInformationHelpDesk(ramIndexPath.c_str(), fileIndexPath.c_str());
		ifstream nodesInStream(nodesPath.c_str(), ios::binary);
		ifstream hsgrInStream(hsgrPath.c_str(), ios::binary);
		nodeHelpDesk->initNNGrid(nodesInStream);

		//Deserialize road network graph
		std::vector< GridEdge> * edgeList = new std::vector< GridEdge>();
		readHSGRFromStream(hsgrInStream, edgeList);
		hsgrInStream.close();

		graph = new StaticGraph<EdgeData>(nodeHelpDesk->getNumberOfNodes()-1, *edgeList);
		delete edgeList;

		//deserialize street name list
		ifstream namesInStream(namesPath.c_str(), ios::binary);
		unsigned size = 0;
		namesInStream.read((char *)&size, sizeof(unsigned));
		names = new std::vector<std::string>();

		//init complete search engine
		sEngine = new SearchEngine<EdgeData, StaticGraph<EdgeData> >(graph, nodeHelpDesk, names);
	}
	~RoutePlugin() {
		delete names;
		delete sEngine;
		delete graph;
		delete nodeHelpDesk;
	}
	std::string GetDescriptor() { return std::string("route"); }
	std::string GetVersionString() { return std::string("0.2a (DL)"); }
	void HandleRequest(std::vector<std::string> parameters, http::Reply& reply) {
		//check number of parameters
		if(parameters.size() != 4) {
			reply = http::Reply::stockReply(http::Reply::badRequest);
			return;
		}

		int lat1 = static_cast<int>(100000.*atof(parameters[0].c_str()));
		int lon1 = static_cast<int>(100000.*atof(parameters[1].c_str()));
		int lat2 = static_cast<int>(100000.*atof(parameters[2].c_str()));
		int lon2 = static_cast<int>(100000.*atof(parameters[3].c_str()));

		_Coordinate startCoord(lat1, lon1);
		_Coordinate targetCoord(lat2, lon2);

		vector< _PathData > * path = new vector< _PathData >();
		PhantomNodes * phantomNodes = new PhantomNodes();
		sEngine->FindRoutingStarts(startCoord, targetCoord, phantomNodes);
		unsigned int distance = sEngine->ComputeRoute(phantomNodes, path, startCoord, targetCoord);
		reply.status = http::Reply::ok;

		string tmp;

		reply.content += ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
		reply.content += ("<kml xmlns=\"http://www.opengis.net/kml/2.2\">");
		reply.content += ("<Document>");

		/*              if(distance != std::numeric_limits<unsigned int>::max())
		                    computeDescription(tmp, path, phantomNodes);
		                cout << tmp << endl;
		 */
		//                reply.content += tmp;
		reply.content += ("<Placemark>");
		reply.content += ("<name>OSM Routing Engine (c) Dennis Luxen and others </name>");

		reply.content += "<description>Route from ";
		convertLatLon(lat1, tmp);
		reply.content += tmp;
		reply.content += ",";
		convertLatLon(lon1, tmp);
		reply.content += tmp;
		reply.content += " to ";
		convertLatLon(lat2, tmp);
		reply.content += tmp;
		reply.content += ",";
		convertLatLon(lon2, tmp);
		reply.content += tmp;
		reply.content += "</description> ";
		reply.content += ("<LineString>");
		reply.content += ("<extrude>1</extrude>");
		reply.content += ("<tessellate>1</tessellate>");
		reply.content += ("<altitudeMode>absolute</altitudeMode>");
		reply.content += ("<coordinates>");


		if(distance != std::numeric_limits<unsigned int>::max())
		{   //A route has been found
			convertLatLon(phantomNodes->startCoord.lon, tmp);
			reply.content += tmp;
			reply.content += (",");
			doubleToString(phantomNodes->startCoord.lat/100000., tmp);
			reply.content += tmp;
			reply.content += (" ");
			_Coordinate result;
			for(vector< _PathData >::iterator it = path->begin(); it != path->end(); it++)
			{
				sEngine->getNodeInfo(it->node, result);
				convertLatLon(result.lon, tmp);
				reply.content += tmp;
				reply.content += (",");
				convertLatLon(result.lat, tmp);
				reply.content += tmp;
				reply.content += (" ");
			}

			convertLatLon(phantomNodes->targetCoord.lon, tmp);
			reply.content += tmp;
			reply.content += (",");
			convertLatLon(phantomNodes->targetCoord.lat, tmp);
			reply.content += tmp;
		}

		reply.content += ("</coordinates>");
		reply.content += ("</LineString>");
		reply.content += ("</Placemark>");
		reply.content += ("</Document>");
		reply.content += ("</kml>");

		reply.headers.resize(3);
		reply.headers[0].name = "Content-Length";
		intToString(reply.content.size(), tmp);
		reply.headers[0].value = tmp;
		reply.headers[1].name = "Content-Type";
		reply.headers[1].value = "application/vnd.google-earth.kml+xml";
		reply.headers[2].name = "Content-Disposition";
		reply.headers[2].value = "attachment; filename=\"route.kml\"";

		delete path;
		delete phantomNodes;
		return;
	}
private:
//    void computeDescription(string &tmp, vector< _PathData > * path, PhantomNodes * phantomNodes)
//    {
//        _Coordinate previous(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon);
//        _Coordinate next, current, lastPlace;
//        stringstream numberString;
//
//        double tempDist = 0;
//        NodeID nextID = UINT_MAX;
//        NodeID nameID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->startNode1, phantomNodes->startNode2);
//        short type = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->startNode1, phantomNodes->startNode2);
//        lastPlace.lat = phantomNodes->startCoord.lat;
//        lastPlace.lon = phantomNodes->startCoord.lon;
//        short nextType = SHRT_MAX;
//        short prevType = SHRT_MAX;
//        tmp += "<Placemark>\n <Name>";
//        for(vector< _PathData >::iterator it = path->begin(); it != path->end(); it++)
//        {
//            sEngine->getNodeInfo(it->node, current);
//            if(it==path->end()-1){
//                next = _Coordinate(phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
//                nextID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
//                nextType = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
//            } else {
//                sEngine->getNodeInfo((it+1)->node, next);
//                nextID = sEngine->GetNameIDForOriginDestinationNodeID(it->node, (it+1)->node);
//                nextType = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(it->node, (it+1)->node);
//            }
//            if(nextID == nameID) {
//                tempDist += ApproximateDistance(previous.lat, previous.lon, current.lat, current.lon);
//            } else {
//                if(type == 0 && prevType != 0)
//                    tmp += "enter motorway and ";
//                if(type != 0 && prevType == 0 )
//                    tmp += "leave motorway and ";
//
//                double angle = GetAngleBetweenTwoEdges(previous, current, next);
////                if(it->turn)
////                    tmp += " turn! ";
//                tmp += "follow road ";
//                if(nameID != 0)
//                    tmp += sEngine->GetNameForNameID(nameID);
//                tmp += " (type: ";
//                numberString << type;
//                tmp += numberString.str();
//                numberString.str("");
//                tmp += ", id: ";
//                numberString << nameID;
//                tmp += numberString.str();
//                numberString.str("");
//                tmp += ")</Name>\n <Description>drive for ";
//                numberString << ApproximateDistance(previous.lat, previous.lon, current.lat, current.lon)+tempDist;
//                tmp += numberString.str();
//                numberString.str("");
//                tmp += "m </Description>";
//                string lat; string lon;
//                convertLatLon(lastPlace.lon, lon);
//                convertLatLon(lastPlace.lat, lat);
//                lastPlace = current;
//                tmp += "\n <Point><Coordinates>";
//                tmp += lon;
//                tmp += ",";
//                tmp += lat;
//                tmp += "</Coordinates></Point>";
//                tmp += "\n</Placemark>\n";
//                tmp += "<Placemark>\n <Name> (";
//                numberString << angle;
//                tmp += numberString.str();
//                numberString.str("");
//                tmp +=") ";
//                if(angle > 160 && angle < 200) {
//                    tmp += /* " (" << angle << ")*/"drive ahead, ";
//                } else if (angle > 290 && angle <= 360) {
//                    tmp += /*" (" << angle << ")*/ "turn sharp left, ";
//                } else if (angle > 230 && angle <= 290) {
//                    tmp += /*" (" << angle << ")*/ "turn left, ";
//                } else if (angle > 200 && angle <= 230) {
//                    tmp += /*" (" << angle << ") */"bear left, ";
//                } else if (angle > 130 && angle <= 160) {
//                    tmp += /*" (" << angle << ") */"bear right, ";
//                } else if (angle > 70 && angle <= 130) {
//                    tmp += /*" (" << angle << ") */"turn right, ";
//                } else {
//                    tmp += /*" (" << angle << ") */"turn sharp right, ";
//                }
//                tempDist = 0;
//                prevType = type;
//            }
//            nameID = nextID;
//            previous = current;
//            type = nextType;
//        }
//        nameID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
//        type = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
//        tmp += "follow road ";
//        tmp += sEngine->GetNameForNameID(nameID);
//        tmp += " (type: ";
//        numberString << type;
//        tmp += numberString.str();
//        numberString.str("");
//        tmp += ", id: ";
//        numberString << nameID;
//        tmp += numberString.str();
//        numberString.str("");
//        tmp += ")</name>\n <Description> drive for ";
//        numberString << ApproximateDistance(previous.lat, previous.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon) + tempDist;
//        tmp += numberString.str();
//        numberString.str("");
//        tmp += "m</Description>\n ";
//        string lat; string lon;
//        convertLatLon(lastPlace.lon, lon);
//        convertLatLon(lastPlace.lat, lat);
//        tmp += "<Point><Coordinates>";
//        tmp += lon;
//        tmp += ",";
//        tmp += lat;
//        tmp += "</Coordinates></Point>";
//        tmp += "</Placemark>\n";
//        tmp += "<Placemark>\n <Name>you have reached your destination</Name>\n ";
//        tmp += "<Description>End of Route</Description>";
//        convertLatLon(phantomNodes->targetCoord.lon, lon);
//        convertLatLon(phantomNodes->targetCoord.lat, lat);
//        tmp += "\n <Point><Coordinates>";
//        tmp += lon;
//        tmp += ",";
//        tmp += lat;
//        tmp +="</Coordinates></Point>\n";
//        tmp += "</Placemark>";
//    }

	NodeInformationHelpDesk * nodeHelpDesk;
	SearchEngine<EdgeData, StaticGraph<EdgeData> > * sEngine;
	std::vector<std::string> * names;
	StaticGraph<EdgeData> * graph;
};


#endif /* ROUTEPLUGIN_H_ */
