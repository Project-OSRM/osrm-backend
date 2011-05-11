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

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "BaseDescriptor.h"
#include "BasePlugin.h"
#include "RouteParameters.h"
#include "KMLDescriptor.h"
#include "JSONDescriptor.h"

#include "../DataStructures/HashTable.h"
#include "../DataStructures/StaticGraph.h"
#include "../DataStructures/SearchEngine.h"

#include "../Util/GraphLoader.h"
#include "../Util/StrIngUtil.h"

typedef ContractionCleanup::Edge::EdgeData EdgeData;
typedef StaticGraph<EdgeData>::InputEdge InputEdge;

class RoutePlugin : public BasePlugin {
public:
    RoutePlugin(std::string hsgrPath, std::string ramIndexPath, std::string fileIndexPath, std::string nodesPath, std::string namesPath) {
        //Init nearest neighbor data structure
        nodeHelpDesk = new NodeInformationHelpDesk(ramIndexPath.c_str(), fileIndexPath.c_str());
        ifstream nodesInStream(nodesPath.c_str(), ios::binary);
        ifstream hsgrInStream(hsgrPath.c_str(), ios::binary);
        nodeHelpDesk->initNNGrid(nodesInStream);

        //Deserialize road network graph
        std::vector< InputEdge> * edgeList = new std::vector< InputEdge>();
        readHSGRFromStream(hsgrInStream, edgeList);
        hsgrInStream.close();
        graph = new StaticGraph<EdgeData>(nodeHelpDesk->getNumberOfNodes()-1, *edgeList);
        delete edgeList;

        //deserialize street name list
        ifstream namesInStream(namesPath.c_str(), ios::binary);
        unsigned size = 0;
        namesInStream.read((char *)&size, sizeof(unsigned));
        names = new std::vector<std::string>();

        char buf[1024];
        for(unsigned i = 0; i < size; i++) {
            unsigned sizeOfString = 0;
            namesInStream.read((char *)&sizeOfString, sizeof(unsigned));
            memset(buf, 0, 1024*sizeof(char));
            namesInStream.read(buf, sizeOfString);
            std::string currentStreetName(buf);
            names->push_back(currentStreetName);
        }

        //init complete search engine
        sEngine = new SearchEngine<EdgeData, StaticGraph<EdgeData> >(graph, nodeHelpDesk, names);
        descriptorTable.Set("", 0); //default descriptor
        descriptorTable.Set("kml", 0);
        descriptorTable.Set("json", 1);
    }
    ~RoutePlugin() {
        delete names;
        delete sEngine;
        delete graph;
        delete nodeHelpDesk;
    }

    std::string GetDescriptor() { return std::string("route"); }
    std::string GetVersionString() { return std::string("0.3 (DL)"); }
    void HandleRequest(RouteParameters routeParameters, http::Reply& reply) {
        //check number of parameters
        if(routeParameters.parameters.size() != 4) {
            reply = http::Reply::stockReply(http::Reply::badRequest);
            return;
        }

        int lat1 = static_cast<int>(100000.*atof(routeParameters.parameters[0].c_str()));
        int lon1 = static_cast<int>(100000.*atof(routeParameters.parameters[1].c_str()));
        int lat2 = static_cast<int>(100000.*atof(routeParameters.parameters[2].c_str()));
        int lon2 = static_cast<int>(100000.*atof(routeParameters.parameters[3].c_str()));

        bool geometry(true);

        if("false" == routeParameters.options["geometry"]) {
            geometry = false;
        }

        if(lat1>90*100000 || lat1 <-90*100000 || lon1>180*100000 || lon1 <-180*100000) {
            reply = http::Reply::stockReply(http::Reply::badRequest);
            return;
        }

        if(lat2>90*100000 || lat2 <-90*100000 || lon2>180*100000 || lon2 <-180*100000) {
            reply = http::Reply::stockReply(http::Reply::badRequest);
            return;
        }

        _Coordinate startCoord(lat1, lon1);
        _Coordinate targetCoord(lat2, lon2);

        vector< _PathData > * path = new vector< _PathData >();
        PhantomNodes * phantomNodes = new PhantomNodes();
        sEngine->FindRoutingStarts(startCoord, targetCoord, phantomNodes);
        unsigned int distance = sEngine->ComputeRoute(phantomNodes, path, startCoord, targetCoord);
        reply.status = http::Reply::ok;
        BaseDescriptor<SearchEngine<EdgeData, StaticGraph<EdgeData> > > * desc;
        std::string JSONParameter = routeParameters.options.Find("jsonp");
        if("" != JSONParameter) {
            reply.content += JSONParameter;
            reply.content += "(\n";
        }
        unsigned descriptorType = descriptorTable[routeParameters.options.Find("output")];
        unsigned short zoom = 18;
        if(routeParameters.options.Find("z") != ""){
            zoom = atoi(routeParameters.options.Find("z").c_str());
            if(18 < zoom)
                zoom = 18;
        }
        //todo: put options in a seperate struct and pass it to the descriptor
        switch(descriptorType){
        case 0:
            if(geometry)
                desc = new KMLDescriptor<SearchEngine<EdgeData, StaticGraph<EdgeData> >, true>();
            else
                desc = new KMLDescriptor<SearchEngine<EdgeData, StaticGraph<EdgeData> >, false>();

            break;
        case 1:
            if(geometry)
                desc = new JSONDescriptor<SearchEngine<EdgeData, StaticGraph<EdgeData> >, true>();
            else
                desc = new JSONDescriptor<SearchEngine<EdgeData, StaticGraph<EdgeData> >, false>();

            break;
        default:
            if(geometry)
                desc = new KMLDescriptor<SearchEngine<EdgeData, StaticGraph<EdgeData> >, true>();
            else
                desc = new KMLDescriptor<SearchEngine<EdgeData, StaticGraph<EdgeData> >, false>();

            break;
        }
        desc->SetZoom(zoom);
        desc->Run(reply, path, phantomNodes, sEngine, distance);
        if("" != JSONParameter) {
            reply.content += ")\n";
        }

        reply.headers.resize(3);
        reply.headers[0].name = "Content-Length";
        std::string tmp;
        intToString(reply.content.size(), tmp);
        reply.headers[0].value = tmp;
        switch(descriptorType){
        case 0:
            reply.headers[1].name = "Content-Type";
            reply.headers[1].value = "application/vnd.google-earth.kml+xml; charset=UTF-8";
            reply.headers[2].name = "Content-Disposition";
            reply.headers[2].value = "attachment; filename=\"route.kml\"";

            break;
        case 1:
            if("" != JSONParameter){
                reply.headers[1].name = "Content-Type";
                reply.headers[1].value = "text/javascript";
                reply.headers[2].name = "Content-Disposition";
                reply.headers[2].value = "attachment; filename=\"route.js\"";
            } else {
                reply.headers[1].name = "Content-Type";
                reply.headers[1].value = "application/x-javascript";
                reply.headers[2].name = "Content-Disposition";
                reply.headers[2].value = "attachment; filename=\"route.json\"";
            }

            break;
        default:
            reply.headers[1].name = "Content-Type";
            reply.headers[1].value = "application/vnd.google-earth.kml+xml; charset=UTF-8";
            reply.headers[2].name = "Content-Disposition";
            reply.headers[2].value = "attachment; filename=\"route.kml\"";

            break;
        }

        delete desc;
        delete path;
        delete phantomNodes;
        return;
    }
private:
    NodeInformationHelpDesk * nodeHelpDesk;
    SearchEngine<EdgeData, StaticGraph<EdgeData> > * sEngine;
    std::vector<std::string> * names;
    StaticGraph<EdgeData> * graph;
    HashTable<std::string, unsigned> descriptorTable;
};


#endif /* ROUTEPLUGIN_H_ */
