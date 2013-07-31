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

#include "BasePlugin.h"
#include "RouteParameters.h"

#include "../Algorithms/ObjectToBase64.h"
#include "../DataStructures/HashTable.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/StaticGraph.h"
#include "../DataStructures/SearchEngine.h"
#include "../Descriptors/BaseDescriptor.h"
#include "../Descriptors/GPXDescriptor.h"
#include "../Descriptors/JSONDescriptor.h"
#include "../Server/DataStructures/QueryObjectsStorage.h"
#include "../Util/StringUtil.h"

#include <cstdlib>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class ViaRoutePlugin : public BasePlugin {
private:
    NodeInformationHelpDesk * nodeHelpDesk;
    std::vector<std::string> & names;
    StaticGraph<QueryEdge::EdgeData> * graph;
    HashTable<std::string, unsigned> descriptorTable;
    std::string pluginDescriptorString;
    SearchEngine * searchEnginePtr;
public:

    ViaRoutePlugin(QueryObjectsStorage * objects, std::string psd = "viaroute") : names(objects->names), pluginDescriptorString(psd) {
        nodeHelpDesk = objects->nodeHelpDesk;
        graph = objects->graph;

        searchEnginePtr = new SearchEngine(graph, nodeHelpDesk, names);

        descriptorTable.Set("", 0); //default descriptor
        descriptorTable.Set("json", 0);
        descriptorTable.Set("gpx", 1);
    }

    virtual ~ViaRoutePlugin() {
        delete searchEnginePtr;
    }

    std::string GetDescriptor() const { return pluginDescriptorString; }
    std::string GetVersionString() const { return std::string("0.3 (DL)"); }
    void HandleRequest(const RouteParameters & routeParameters, http::Reply& reply) {
        //check number of parameters
        if( 2 > routeParameters.coordinates.size() ) {
            reply = http::Reply::stockReply(http::Reply::badRequest);
            return;
        }

        RawRouteData rawRoute;
        rawRoute.checkSum = nodeHelpDesk->GetCheckSum();
        bool checksumOK = (routeParameters.checkSum == rawRoute.checkSum);
        std::vector<std::string> textCoord;
        for(unsigned i = 0; i < routeParameters.coordinates.size(); ++i) {
            if(false == checkCoord(routeParameters.coordinates[i])) {
                reply = http::Reply::stockReply(http::Reply::badRequest);
                return;
            }
            rawRoute.rawViaNodeCoordinates.push_back(routeParameters.coordinates[i]);
        }
        std::vector<PhantomNode> phantomNodeVector(rawRoute.rawViaNodeCoordinates.size());
        for(unsigned i = 0; i < rawRoute.rawViaNodeCoordinates.size(); ++i) {
            if(checksumOK && i < routeParameters.hints.size() && "" != routeParameters.hints[i]) {
//                INFO("Decoding hint: " << routeParameters.hints[i] << " for location index " << i);
                DecodeObjectFromBase64(routeParameters.hints[i], phantomNodeVector[i]);
                if(phantomNodeVector[i].isValid(nodeHelpDesk->getNumberOfNodes())) {
//                    INFO("Decoded hint " << i << " successfully");
                    continue;
                }
            }
//            INFO("Brute force lookup of coordinate " << i);
            searchEnginePtr->FindPhantomNodeForCoordinate( rawRoute.rawViaNodeCoordinates[i], phantomNodeVector[i], routeParameters.zoomLevel);
        }

        for(unsigned i = 0; i < phantomNodeVector.size()-1; ++i) {
            PhantomNodes segmentPhantomNodes;
            segmentPhantomNodes.startPhantom = phantomNodeVector[i];
            segmentPhantomNodes.targetPhantom = phantomNodeVector[i+1];
            rawRoute.segmentEndCoordinates.push_back(segmentPhantomNodes);
        }
        if( ( routeParameters.alternateRoute ) && (1 == rawRoute.segmentEndCoordinates.size()) ) {
//            INFO("Checking for alternative paths");
            searchEnginePtr->alternativePaths(rawRoute.segmentEndCoordinates[0],  rawRoute);

        } else {
            searchEnginePtr->shortestPath(rawRoute.segmentEndCoordinates, rawRoute);
        }


        if(INT_MAX == rawRoute.lengthOfShortestPath ) {
            DEBUG( "Error occurred, single path not found" );
        }
        reply.status = http::Reply::ok;

        //TODO: Move to member as smart pointer
        BaseDescriptor * desc;
        if("" != routeParameters.jsonpParameter) {
            reply.content += routeParameters.jsonpParameter;
            reply.content += "(";
        }

        _DescriptorConfig descriptorConfig;
        unsigned descriptorType = descriptorTable[routeParameters.outputFormat];
        descriptorConfig.z = routeParameters.zoomLevel;
        descriptorConfig.instructions = routeParameters.printInstructions;
        descriptorConfig.geometry = routeParameters.geometry;
        descriptorConfig.encodeGeometry = routeParameters.compression;

        switch(descriptorType){
        case 0:
            desc = new JSONDescriptor();

            break;
        case 1:
            desc = new GPXDescriptor();

            break;
        default:
            desc = new JSONDescriptor();

            break;
        }

        PhantomNodes phantomNodes;
        phantomNodes.startPhantom = rawRoute.segmentEndCoordinates[0].startPhantom;
//        INFO("Start location: " << phantomNodes.startPhantom.location)
        phantomNodes.targetPhantom = rawRoute.segmentEndCoordinates[rawRoute.segmentEndCoordinates.size()-1].targetPhantom;
//        INFO("TargetLocation: " << phantomNodes.targetPhantom.location);
//        INFO("Number of segments: " << rawRoute.segmentEndCoordinates.size());
        desc->SetConfig(descriptorConfig);

        desc->Run(reply, rawRoute, phantomNodes, *searchEnginePtr);
        if("" != routeParameters.jsonpParameter) {
            reply.content += ")\n";
        }
        reply.headers.resize(3);
        reply.headers[0].name = "Content-Length";
        std::string tmp;
        intToString(reply.content.size(), tmp);
        reply.headers[0].value = tmp;
        switch(descriptorType){
        case 0:
            if("" != routeParameters.jsonpParameter){
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
            if("" != routeParameters.jsonpParameter){
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
