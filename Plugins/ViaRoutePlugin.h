/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef VIAROUTEPLUGIN_H_
#define VIAROUTEPLUGIN_H_

#include "BasePlugin.h"

#include "../Algorithms/ObjectToBase64.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/SearchEngine.h"
#include "../Descriptors/BaseDescriptor.h"
#include "../Descriptors/GPXDescriptor.h"
#include "../Descriptors/JSONDescriptor.h"
#include "../Util/SimpleLogger.h"
#include "../Util/StringUtil.h"

#include <boost/unordered_map.hpp>

#include <cstdlib>

#include <string>
#include <vector>

template<class DataFacadeT>
class ViaRoutePlugin : public BasePlugin {
private:
    boost::unordered_map<std::string, unsigned> descriptorTable;
    SearchEngine<DataFacadeT> * search_engine_ptr;
public:

    ViaRoutePlugin(DataFacadeT * facade)
     :
        descriptor_string("viaroute"),
        facade(facade)
    {
        //TODO: set up an engine for each thread!!
        search_engine_ptr = new SearchEngine<DataFacadeT>(facade);

        descriptorTable.emplace("json", 0);
        descriptorTable.emplace("gpx" , 1);
    }

    virtual ~ViaRoutePlugin() {
        delete search_engine_ptr;
    }

    const std::string & GetDescriptor() const { return descriptor_string; }

    void HandleRequest(
        const RouteParameters & routeParameters,
        http::Reply& reply
    ) {
        //check number of parameters
        if( 2 > routeParameters.coordinates.size() ) {
            reply = http::Reply::StockReply(http::Reply::badRequest);
            return;
        }

        RawRouteData rawRoute;
        rawRoute.checkSum = facade->GetCheckSum();
        bool checksumOK = (routeParameters.checkSum == rawRoute.checkSum);
        std::vector<std::string> textCoord;
        for(unsigned i = 0; i < routeParameters.coordinates.size(); ++i) {
            if( !checkCoord(routeParameters.coordinates[i]) ) {
                reply = http::Reply::StockReply(http::Reply::badRequest);
                return;
            }
            rawRoute.rawViaNodeCoordinates.push_back(routeParameters.coordinates[i]);
        }
        std::vector<PhantomNode> phantomNodeVector(rawRoute.rawViaNodeCoordinates.size());
        for(unsigned i = 0; i < rawRoute.rawViaNodeCoordinates.size(); ++i) {
            if(checksumOK && i < routeParameters.hints.size() && "" != routeParameters.hints[i]) {
//                SimpleLogger().Write() <<"Decoding hint: " << routeParameters.hints[i] << " for location index " << i;
                DecodeObjectFromBase64(routeParameters.hints[i], phantomNodeVector[i]);
                if(phantomNodeVector[i].isValid(facade->GetNumberOfNodes())) {
//                    SimpleLogger().Write() << "Decoded hint " << i << " successfully";
                    continue;
                }
            }
//            SimpleLogger().Write() << "Brute force lookup of coordinate " << i;
            facade->FindPhantomNodeForCoordinate(
                rawRoute.rawViaNodeCoordinates[i],
                phantomNodeVector[i],
                routeParameters.zoomLevel
            );
        }

        for(unsigned i = 0; i < phantomNodeVector.size()-1; ++i) {
            PhantomNodes segmentPhantomNodes;
            segmentPhantomNodes.startPhantom = phantomNodeVector[i];
            segmentPhantomNodes.targetPhantom = phantomNodeVector[i+1];
            rawRoute.segmentEndCoordinates.push_back(segmentPhantomNodes);
        }
        if(
            ( routeParameters.alternateRoute ) &&
            (1 == rawRoute.segmentEndCoordinates.size())
        ) {
            search_engine_ptr->alternative_path(
                rawRoute.segmentEndCoordinates[0],
                rawRoute
            );
        } else {
            search_engine_ptr->shortest_path(
                rawRoute.segmentEndCoordinates,
                rawRoute
            );
        }

        if(INT_MAX == rawRoute.lengthOfShortestPath ) {
            SimpleLogger().Write(logDEBUG) <<
                "Error occurred, single path not found";
        }
        reply.status = http::Reply::ok;

        //TODO: Move to member as smart pointer
        BaseDescriptor<DataFacadeT> * desc;
        if("" != routeParameters.jsonpParameter) {
            reply.content.push_back(routeParameters.jsonpParameter);
            reply.content.push_back("(");
        }

        DescriptorConfig descriptorConfig;

        unsigned descriptorType = 0;
        if(descriptorTable.find(routeParameters.outputFormat) != descriptorTable.end() ) {
            descriptorType = descriptorTable.find(routeParameters.outputFormat)->second;
        }
        descriptorConfig.zoom_level = routeParameters.zoomLevel;
        descriptorConfig.instructions = routeParameters.printInstructions;
        descriptorConfig.geometry = routeParameters.geometry;
        descriptorConfig.encode_geometry = routeParameters.compression;

        switch(descriptorType){
        case 0:
            desc = new JSONDescriptor<DataFacadeT>();

            break;
        case 1:
            desc = new GPXDescriptor<DataFacadeT>();

            break;
        default:
            desc = new JSONDescriptor<DataFacadeT>();

            break;
        }

        PhantomNodes phantomNodes;
        phantomNodes.startPhantom = rawRoute.segmentEndCoordinates[0].startPhantom;
        phantomNodes.targetPhantom = rawRoute.segmentEndCoordinates[rawRoute.segmentEndCoordinates.size()-1].targetPhantom;
        desc->SetConfig(descriptorConfig);

        desc->Run(reply, rawRoute, phantomNodes, facade);
        if("" != routeParameters.jsonpParameter) {
            reply.content.push_back(")\n");
        }
        reply.headers.resize(3);
        reply.headers[0].name = "Content-Length";
        std::string tmp;
        unsigned content_length = 0;
        BOOST_FOREACH(const std::string & snippet, reply.content) {
            content_length += snippet.length();
        }
        intToString(content_length, tmp);
        reply.headers[0].value = tmp;
        switch(descriptorType){
        case 0:
            if( !routeParameters.jsonpParameter.empty() ){
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
            if( !routeParameters.jsonpParameter.empty() ){
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
    std::string descriptor_string;
    DataFacadeT * facade;
};


#endif /* VIAROUTEPLUGIN_H_ */
