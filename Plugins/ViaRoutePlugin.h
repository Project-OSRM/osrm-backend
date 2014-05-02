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

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

template <class DataFacadeT> class ViaRoutePlugin : public BasePlugin
{
  private:
    std::unordered_map<std::string, unsigned> descriptorTable;
    std::shared_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;

  public:
    explicit ViaRoutePlugin(DataFacadeT *facade) : descriptor_string("viaroute"), facade(facade)
    {
        search_engine_ptr = std::make_shared<SearchEngine<DataFacadeT>>(facade);

        descriptorTable.emplace("json", 0);
        descriptorTable.emplace("gpx", 1);
    }

    virtual ~ViaRoutePlugin() {}

    const std::string GetDescriptor() const { return descriptor_string; }

    void HandleRequest(const RouteParameters &routeParameters, http::Reply &reply)
    {
        // check number of parameters
        if (2 > routeParameters.coordinates.size())
        {
            reply = http::Reply::StockReply(http::Reply::badRequest);
            return;
        }

        RawRouteData raw_route;
        raw_route.checkSum = facade->GetCheckSum();

        if (std::any_of(begin(routeParameters.coordinates),
                        end(routeParameters.coordinates),
                        [&](FixedPointCoordinate coordinate)
                        { return !coordinate.isValid(); }))
        {
            reply = http::Reply::StockReply(http::Reply::badRequest);
            return;
        }

        for (const FixedPointCoordinate &coordinate : routeParameters.coordinates)
        {
            raw_route.rawViaNodeCoordinates.emplace_back(coordinate);
        }

        std::vector<PhantomNode> phantom_node_vector(raw_route.rawViaNodeCoordinates.size());
        const bool checksum_OK = (routeParameters.checkSum == raw_route.checkSum);

        for (unsigned i = 0; i < raw_route.rawViaNodeCoordinates.size(); ++i)
        {
            if (checksum_OK && i < routeParameters.hints.size() &&
                !routeParameters.hints[i].empty())
            {
                DecodeObjectFromBase64(routeParameters.hints[i], phantom_node_vector[i]);
                if (phantom_node_vector[i].isValid(facade->GetNumberOfNodes()))
                {
                    continue;
                }
            }
            facade->FindPhantomNodeForCoordinate(raw_route.rawViaNodeCoordinates[i],
                                                 phantom_node_vector[i],
                                                 routeParameters.zoomLevel);
        }

        PhantomNodes current_phantom_node_pair;
        for (unsigned i = 0; i < phantom_node_vector.size() - 1; ++i)
        {
            current_phantom_node_pair.source_phantom = phantom_node_vector[i];
            current_phantom_node_pair.target_phantom = phantom_node_vector[i + 1];
            raw_route.segmentEndCoordinates.emplace_back(current_phantom_node_pair);
        }

        if ((routeParameters.alternateRoute) && (1 == raw_route.segmentEndCoordinates.size()))
        {
            search_engine_ptr->alternative_path(raw_route.segmentEndCoordinates.front(), raw_route);
        }
        else
        {
            search_engine_ptr->shortest_path(raw_route.segmentEndCoordinates, raw_route);
        }

        if (INVALID_EDGE_WEIGHT == raw_route.lengthOfShortestPath)
        {
            SimpleLogger().Write(logDEBUG) << "Error occurred, single path not found";
        }
        reply.status = http::Reply::ok;

        if (!routeParameters.jsonpParameter.empty())
        {
            reply.content.push_back(routeParameters.jsonpParameter);
            reply.content.push_back("(");
        }

        DescriptorConfig descriptor_config;

        auto iter = descriptorTable.find(routeParameters.outputFormat);
        unsigned descriptor_type = (iter != descriptorTable.end() ? iter->second : 0);

        descriptor_config.zoom_level = routeParameters.zoomLevel;
        descriptor_config.instructions = routeParameters.printInstructions;
        descriptor_config.geometry = routeParameters.geometry;
        descriptor_config.encode_geometry = routeParameters.compression;

        std::shared_ptr<BaseDescriptor<DataFacadeT>> descriptor;
        switch (descriptor_type)
        {
        // case 0:
        //     descriptor = std::make_shared<JSONDescriptor<DataFacadeT>>();
        //     break;
        case 1:
            descriptor = std::make_shared<GPXDescriptor<DataFacadeT>>();
            break;
        default:
            descriptor = std::make_shared<JSONDescriptor<DataFacadeT>>();
            break;
        }

        PhantomNodes phantom_nodes;
        phantom_nodes.source_phantom = raw_route.segmentEndCoordinates.front().source_phantom;
        phantom_nodes.target_phantom = raw_route.segmentEndCoordinates.back().target_phantom;
        descriptor->SetConfig(descriptor_config);
        descriptor->Run(raw_route, phantom_nodes, facade, reply);

        if (!routeParameters.jsonpParameter.empty())
        {
            reply.content.push_back(")\n");
        }
        reply.headers.resize(3);
        reply.headers[0].name = "Content-Length";

        unsigned content_length = 0;
        for (const std::string &snippet : reply.content)
        {
            content_length += snippet.length();
        }
        std::string tmp_string;
        intToString(content_length, tmp_string);
        reply.headers[0].value = tmp_string;

        switch (descriptor_type)
        {
        case 0:
            if (!routeParameters.jsonpParameter.empty())
            {
                reply.headers[1].name = "Content-Type";
                reply.headers[1].value = "text/javascript";
                reply.headers[2].name = "Content-Disposition";
                reply.headers[2].value = "attachment; filename=\"route.js\"";
            }
            else
            {
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
            if (!routeParameters.jsonpParameter.empty())
            {
                reply.headers[1].name = "Content-Type";
                reply.headers[1].value = "text/javascript";
                reply.headers[2].name = "Content-Disposition";
                reply.headers[2].value = "attachment; filename=\"route.js\"";
            }
            else
            {
                reply.headers[1].name = "Content-Type";
                reply.headers[1].value = "application/x-javascript";
                reply.headers[2].name = "Content-Disposition";
                reply.headers[2].value = "attachment; filename=\"route.json\"";
            }
            break;
        }
        return;
    }

  private:
    std::string descriptor_string;
    DataFacadeT *facade;
};

#endif /* VIAROUTEPLUGIN_H_ */
