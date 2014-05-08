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

#ifndef NEAREST_PLUGIN_H
#define NEAREST_PLUGIN_H

#include "BasePlugin.h"
#include "../DataStructures/PhantomNodes.h"
#include "../Util/StringUtil.h"

#include <unordered_map>

/*
 * This Plugin locates the nearest point on a street in the road network for a given coordinate.
 */

template <class DataFacadeT> class NearestPlugin : public BasePlugin
{
  public:
    explicit NearestPlugin(DataFacadeT *facade) : facade(facade), descriptor_string("nearest")
    {
        descriptor_table.emplace("", 0); // default descriptor
        descriptor_table.emplace("json", 1);
    }

    const std::string GetDescriptor() const { return descriptor_string; }

    void HandleRequest(const RouteParameters &route_parameters, http::Reply &reply)
    {
        // check number of parameters
        if (route_parameters.coordinates.empty())
        {
            reply = http::Reply::StockReply(http::Reply::badRequest);
            return;
        }
        if (!route_parameters.coordinates.front().isValid())
        {
            reply = http::Reply::StockReply(http::Reply::badRequest);
            return;
        }

        PhantomNode result;
        facade->FindPhantomNodeForCoordinate(
            route_parameters.coordinates.front(), result, route_parameters.zoom_level);

        // json

        if (!route_parameters.jsonp_parameter.empty())
        {
            reply.content.emplace_back(route_parameters.jsonp_parameter);
            reply.content.emplace_back("(");
        }

        reply.status = http::Reply::ok;
        reply.content.emplace_back("{\"status\":");
        if (SPECIAL_NODEID != result.forward_node_id)
        {
            reply.content.emplace_back("0,");
        }
        else
        {
            reply.content.emplace_back("207,");
        }
        reply.content.emplace_back("\"mapped_coordinate\":[");
        std::string temp_string;

        if (SPECIAL_NODEID != result.forward_node_id)
        {
            FixedPointCoordinate::convertInternalLatLonToString(result.location.lat, temp_string);
            reply.content.emplace_back(temp_string);
            FixedPointCoordinate::convertInternalLatLonToString(result.location.lon, temp_string);
            reply.content.emplace_back(",");
            reply.content.emplace_back(temp_string);
        }
        reply.content.emplace_back("],\"name\":\"");
        if (SPECIAL_NODEID != result.forward_node_id)
        {
            facade->GetName(result.name_id, temp_string);
            reply.content.emplace_back(temp_string);
        }
        reply.content.emplace_back("\"}");
        reply.headers.resize(3);
        if (!route_parameters.jsonp_parameter.empty())
        {
            reply.content.emplace_back(")");
            reply.headers[1].name = "Content-Type";
            reply.headers[1].value = "text/javascript";
            reply.headers[2].name = "Content-Disposition";
            reply.headers[2].value = "attachment; filename=\"location.js\"";
        }
        else
        {
            reply.headers[1].name = "Content-Type";
            reply.headers[1].value = "application/x-javascript";
            reply.headers[2].name = "Content-Disposition";
            reply.headers[2].value = "attachment; filename=\"location.json\"";
        }
        reply.headers[0].name = "Content-Length";
        unsigned content_length = 0;
        for (const std::string &snippet : reply.content)
        {
            content_length += snippet.length();
        }
        intToString(content_length, temp_string);
        reply.headers[0].value = temp_string;
    }

  private:
    DataFacadeT *facade;
    std::unordered_map<std::string, unsigned> descriptor_table;
    std::string descriptor_string;
};

#endif /* NEAREST_PLUGIN_H */
