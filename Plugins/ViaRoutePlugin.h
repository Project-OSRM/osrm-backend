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

#include "BasePlugin.h"
#include "RouteParameters.h"

#include "../Algorithms/ObjectToBase64.h"

#include "../Descriptors/BaseDescriptor.h"
#include "../Descriptors/GPXDescriptor.h"
#include "../Descriptors/JSONDescriptor.h"

#include "../DataStructures/HashTable.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/StaticGraph.h"
#include "../DataStructures/SearchEngine.h"

#include "../Util/StringUtil.h"

#include "../Server/DataStructures/QueryObjectsStorage.h"

class ViaRoutePlugin : public BasePlugin {
private:
    NodeInformationHelpDesk * nodeHelpDesk;
    std::vector<std::string> & names;
    StaticGraph<QueryEdge::EdgeData> * graph;
    HashTable<std::string, unsigned> descriptorTable;
    std::string pluginDescriptorString;
    SearchEngine<QueryEdge::EdgeData, StaticGraph<QueryEdge::EdgeData> > * searchEngine;
public:

    ViaRoutePlugin(QueryObjectsStorage * objects, std::string psd = "viaroute") : names(objects->names), pluginDescriptorString(psd) {
        nodeHelpDesk = objects->nodeHelpDesk;
        graph = objects->graph;

        searchEngine = new SearchEngine<QueryEdge::EdgeData, StaticGraph<QueryEdge::EdgeData> >(graph, nodeHelpDesk, names);

        descriptorTable.Set("", 0); //default descriptor
        descriptorTable.Set("json", 0);
        descriptorTable.Set("gpx", 1);
    }

    virtual ~ViaRoutePlugin() {
        delete searchEngine;
    }

    std::string GetDescriptor() const { return pluginDescriptorString; }
    std::string GetVersionString() const { return std::string("0.3 (DL)"); }
    void HandleRequest(const RouteParameters & routeParameters, http::Reply& reply) {
        //check number of parameters
        if( 2 > routeParameters.viaPoints.size() ) {
            reply = http::Reply::stockReply(http::Reply::badRequest);
            return;
        }

        RawRouteData rawRoute;
        rawRoute.checkSum = nodeHelpDesk->GetCheckSum();
        bool checksumOK = ((unsigned)atoi(routeParameters.options.Find("checksum").c_str()) == rawRoute.checkSum);
        std::vector<std::string> textCoord;
        for(unsigned i = 0; i < routeParameters.viaPoints.size(); ++i) {
            textCoord.resize(0);
            stringSplit (routeParameters.viaPoints[i], ',', textCoord);
            if(textCoord.size() != 2) {
                reply = http::Reply::stockReply(http::Reply::badRequest);
                return;
            }
            int vialat = static_cast<int>(100000.*atof(textCoord[0].c_str()));
            int vialon = static_cast<int>(100000.*atof(textCoord[1].c_str()));
            _Coordinate viaCoord(vialat, vialon);
            if(false == checkCoord(viaCoord)) {
                reply = http::Reply::stockReply(http::Reply::badRequest);
                return;
            }
            rawRoute.rawViaNodeCoordinates.push_back(viaCoord);
        }
        std::vector<PhantomNode> phantomNodeVector(rawRoute.rawViaNodeCoordinates.size());
        for(unsigned i = 0; i < rawRoute.rawViaNodeCoordinates.size(); ++i) {
            if(checksumOK && i < routeParameters.hints.size() && "" != routeParameters.hints[i]) {
//                INFO("Decoding hint: " << routeParameters.hints[i] << " for location index " << i);
                DecodeObjectFromBase64(phantomNodeVector[i], routeParameters.hints[i]);
                if(phantomNodeVector[i].isValid(nodeHelpDesk->getNumberOfNodes())) {
//                    INFO("Decoded hint " << i << " successfully");
                    continue;
                }
            }
//            INFO("Brute force lookup of coordinate " << i);
            searchEngine->FindPhantomNodeForCoordinate( rawRoute.rawViaNodeCoordinates[i], phantomNodeVector[i]);
        }
        //unsigned distance = 0;

        for(unsigned i = 0; i < phantomNodeVector.size()-1; ++i) {
            PhantomNodes segmentPhantomNodes;
            segmentPhantomNodes.startPhantom = phantomNodeVector[i];
            segmentPhantomNodes.targetPhantom = phantomNodeVector[i+1];
            rawRoute.segmentEndCoordinates.push_back(segmentPhantomNodes);
        }
        if(1 == rawRoute.segmentEndCoordinates.size()) {
//            INFO("Checking for alternative paths");
            searchEngine->alternativePaths(rawRoute.segmentEndCoordinates[0],  rawRoute);

        } else {
            searchEngine->shortestPath(rawRoute.segmentEndCoordinates, rawRoute);
        }
//        std::cout << "latitude,longitude" << std::endl;
//        for(unsigned i = 0; i < rawRoute.computedShortestPath.size(); ++i) {
//            _Coordinate current;
//            searchEngine->GetCoordinatesForNodeID(rawRoute.computedShortestPath[i].node, current);
//            std::cout << current.lat/100000. << "," << current.lon/100000. << std::endl;
//        }
//        std::cout << std::endl;
//
//        std::cout << "latitude,longitude" << std::endl;
//        for(unsigned i = 0; i < rawRoute.computedAlternativePath.size(); ++i) {
//            _Coordinate current;
//            searchEngine->GetCoordinatesForNodeID(rawRoute.computedAlternativePath[i].node, current);
//            std::cout << current.lat/100000. << "," << current.lon/100000. << std::endl;
//        }
//        std::cout << std::endl;


        if(INT_MAX == rawRoute.lengthOfShortestPath ) {
            DEBUG( "Error occurred, single path not found" );
        }
        reply.status = http::Reply::ok;

        //TODO: Move to member as smart pointer
        BaseDescriptor<SearchEngine<QueryEdge::EdgeData, StaticGraph<QueryEdge::EdgeData> > > * desc;
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
        if("cmp" == routeParameters.options.Find("no") || "cmp6" == routeParameters.options.Find("no")  ) {
            descriptorConfig.encodeGeometry = false;
        }
        switch(descriptorType){
        case 0:
            desc = new JSONDescriptor<SearchEngine<QueryEdge::EdgeData, StaticGraph<QueryEdge::EdgeData> > >();

            break;
        case 1:
            desc = new GPXDescriptor<SearchEngine<QueryEdge::EdgeData, StaticGraph<QueryEdge::EdgeData> > >();

            break;
        default:
            desc = new JSONDescriptor<SearchEngine<QueryEdge::EdgeData, StaticGraph<QueryEdge::EdgeData> > >();

            break;
        }

        PhantomNodes phantomNodes;
        phantomNodes.startPhantom = rawRoute.segmentEndCoordinates[0].startPhantom;
//        INFO("Start location: " << phantomNodes.startPhantom.location)
        phantomNodes.targetPhantom = rawRoute.segmentEndCoordinates[rawRoute.segmentEndCoordinates.size()-1].targetPhantom;
//        INFO("TargetLocation: " << phantomNodes.targetPhantom.location);
//        INFO("Number of segments: " << rawRoute.segmentEndCoordinates.size());
        desc->SetConfig(descriptorConfig);

        desc->Run(reply, rawRoute, phantomNodes, *searchEngine);
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

        delete desc;
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
