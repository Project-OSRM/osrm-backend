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

#ifndef VIAROUTEPLUGIN_H_
#define VIAROUTEPLUGIN_H_

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "ObjectForPluginStruct.h"

#include "BasePlugin.h"
#include "RouteParameters.h"
#include "../Descriptors/BaseDescriptor.h"
#include "../Descriptors/GPXDescriptor.h"
#include "../Descriptors/JSONDescriptor.h"

#include "../DataStructures/HashTable.h"
#include "../DataStructures/StaticGraph.h"
#include "../DataStructures/SearchEngine.h"

#include "../Util/StringUtil.h"

class ViaRoutePlugin : public BasePlugin {
private:
    NodeInformationHelpDesk * nodeHelpDesk;
    std::vector<std::string> * names;
    StaticGraph<EdgeData> * graph;
    HashTable<std::string, unsigned> descriptorTable;
    std::string pluginDescriptorString;

    SearchEngine<EdgeData, StaticGraph<EdgeData> > * searchEngine;
public:

    ViaRoutePlugin(ObjectsForQueryStruct * objects, std::string psd = "viaroute") : pluginDescriptorString(psd) {
        nodeHelpDesk = objects->nodeHelpDesk;
        graph = objects->graph;
        names = objects->names;

        searchEngine = new SearchEngine<EdgeData, StaticGraph<EdgeData> >(graph, nodeHelpDesk, names);

        descriptorTable.Set("", 0); //default descriptor
        descriptorTable.Set("json", 0);
        descriptorTable.Set("gpx", 1);
    }

    virtual ~ViaRoutePlugin() {
        DELETE( searchEngine );
    }

    std::string GetDescriptor() const { return pluginDescriptorString; }
    std::string GetVersionString() const { return std::string("0.3 (DL)"); }
    void HandleRequest(const RouteParameters & routeParameters, http::Reply& reply) {
        //check number of parameters
        if(0 == routeParameters.options["start"].size() || 0 == routeParameters.options["dest"].size() ) {
            reply = http::Reply::stockReply(http::Reply::badRequest);
            return;
        }

        //Get start and end Coordinate
        std::string start = routeParameters.options["start"];
        std::string dest = routeParameters.options["dest"];

        std::vector<std::string> textCoord = split (start, ',');

        int lat1 = static_cast<int>(100000.*atof(textCoord[0].c_str()));
        int lon1 = static_cast<int>(100000.*atof(textCoord[1].c_str()));

        textCoord = split (dest, ',');

        int lat2 = static_cast<int>(100000.*atof(textCoord[0].c_str()));
        int lon2 = static_cast<int>(100000.*atof(textCoord[1].c_str()));

        _Coordinate startCoord(lat1, lon1);
        _Coordinate targetCoord(lat2, lon2);
        RawRouteData rawRoute;
        rawRoute.checkSum = nodeHelpDesk->GetCheckSum();
        if(false == checkCoord(startCoord) || false == checkCoord(targetCoord)) {
            reply = http::Reply::stockReply(http::Reply::badRequest);
            return;
        }
        rawRoute.rawViaNodeCoordinates.push_back(startCoord);

        for(unsigned i = 0; i < routeParameters.viaPoints.size(); ++i) {
            textCoord = split (routeParameters.viaPoints[i], ',');
            if(textCoord.size() != 2) {
                reply = http::Reply::stockReply(http::Reply::badRequest);
                return;
            }
            int vialat = static_cast<int>(100000.*atof(textCoord[0].c_str()));
            int vialon = static_cast<int>(100000.*atof(textCoord[1].c_str()));
//            INFO("[debug] via" << i << ": " << vialat << "," << vialon);
            _Coordinate viaCoord(vialat, vialon);
            if(false == checkCoord(viaCoord)) {
                reply = http::Reply::stockReply(http::Reply::badRequest);
                return;
            }
            rawRoute.rawViaNodeCoordinates.push_back(viaCoord);
        }
        rawRoute.rawViaNodeCoordinates.push_back(targetCoord);
        vector<PhantomNode> phantomNodeVector(rawRoute.rawViaNodeCoordinates.size());
        for(unsigned i = 0; i < rawRoute.rawViaNodeCoordinates.size(); ++i) {
            searchEngine->FindPhantomNodeForCoordinate( rawRoute.rawViaNodeCoordinates[i], phantomNodeVector[i]);
//            INFO("found coord [" << i << "|" << rawRoute.rawViaNodeCoordinates.size());
        }
        unsigned distance = 0;
        //single route or via point routing
        if(2 == rawRoute.rawViaNodeCoordinates.size()) {
            PhantomNodes segmentPhantomNodes;
            segmentPhantomNodes.startPhantom = phantomNodeVector[0];
            segmentPhantomNodes.targetPhantom = phantomNodeVector[1];
            distance = searchEngine->ComputeRoute(segmentPhantomNodes, rawRoute.computedRouted);
            rawRoute.segmentEndCoordinates.push_back(segmentPhantomNodes);
        } else {
            //Getting the shortest via path is a dynamic programming problem and is solved as such.
            for(unsigned i = 0; i < phantomNodeVector.size()-1; ++i) {
                 PhantomNodes segmentPhantomNodes;
                 segmentPhantomNodes.startPhantom = phantomNodeVector[i];
                 segmentPhantomNodes.targetPhantom = phantomNodeVector[i+1];
                 rawRoute.segmentEndCoordinates.push_back(segmentPhantomNodes);
            }
            distance = searchEngine->ComputeViaRoute(rawRoute.segmentEndCoordinates, rawRoute.computedRouted);
        }
        if(INT_MAX == distance ) {
            DEBUG( "Error occurred, single path not found" );
        }
        reply.status = http::Reply::ok;

        BaseDescriptor<SearchEngine<EdgeData, StaticGraph<EdgeData> > > * desc;
        std::string JSONParameter = routeParameters.options.Find("jsonp");
        if("" != JSONParameter) {
            reply.content += JSONParameter;
            reply.content += "(";
        }

        _DescriptorConfig descriptorConfig;
        unsigned descriptorType = descriptorTable[routeParameters.options.Find("output")];
        unsigned short zoom = 18;
        if(routeParameters.options.Find("z") != ""){
            zoom = atoi(routeParameters.options.Find("z").c_str());
            if(18 < zoom)
                zoom = 18;
        }
        descriptorConfig.z = zoom;
        if(routeParameters.options.Find("instructions") == "false") {
            descriptorConfig.instructions = false;
        }
        if(routeParameters.options.Find("geometry") == "false" ) {
            descriptorConfig.geometry = false;
        }
        if("cmp" == routeParameters.options.Find("geomformat") || "cmp6" == routeParameters.options.Find("geomformat")  ) {
            descriptorConfig.encodeGeometry = true;
        }
        switch(descriptorType){
        case 0:
            desc = new JSONDescriptor<SearchEngine<EdgeData, StaticGraph<EdgeData> > >();

            break;
        case 1:
            desc = new GPXDescriptor<SearchEngine<EdgeData, StaticGraph<EdgeData> > >();

            break;
        default:
            desc = new JSONDescriptor<SearchEngine<EdgeData, StaticGraph<EdgeData> > >();

            break;
        }

        PhantomNodes phantomNodes;
        phantomNodes.startPhantom = rawRoute.segmentEndCoordinates[0].startPhantom;
//        INFO("Start location: " << phantomNodes.startPhantom.location)
        phantomNodes.targetPhantom = rawRoute.segmentEndCoordinates[rawRoute.segmentEndCoordinates.size()-1].targetPhantom;
//        INFO("TargetLocation: " << phantomNodes.targetPhantom.location);
//        INFO("Number of segments: " << rawRoute.segmentEndCoordinates.size());
        desc->SetConfig(descriptorConfig);

        desc->Run(reply, rawRoute, phantomNodes, *searchEngine, distance);
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
        case 1:
            reply.headers[1].name = "Content-Type";
            reply.headers[1].value = "application/gpx+xml; charset=UTF-8";
            reply.headers[2].name = "Content-Disposition";
            reply.headers[2].value = "attachment; filename=\"route.gpx\"";

            break;
        default:
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
        }

        DELETE( desc );
        return;
    }
private:
    inline bool checkCoord(const _Coordinate & c) {
        if(c.lat > 90*100000 || c.lat < -90*100000 || c.lon > 180*100000 || c.lon <-180*100000) {
            return false;
        }
        return true;
    }
};


#endif /* VIAROUTEPLUGIN_H_ */
