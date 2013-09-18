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

#ifndef NearestPlugin_H_
#define NearestPlugin_H_

#include "BasePlugin.h"

#include "../DataStructures/NodeInformationHelpDesk.h"
#include "../Util/StringUtil.h"

/*
 * This Plugin locates the nearest point on a street in the road network for a given coordinate.
 */

//TODO: Rework data access to go through facade

template<class DataFacadeT>
class NearestPlugin : public BasePlugin {
public:
    NearestPlugin(DataFacadeT * objects )
     :
        m_query_objects(objects),
        descriptor_string("nearest")
    {
        descriptorTable.insert(std::make_pair(""    , 0)); //default descriptor
        descriptorTable.insert(std::make_pair("json", 1));
    }
    const std::string & GetDescriptor() const { return descriptor_string; }
    void HandleRequest(const RouteParameters & routeParameters, http::Reply& reply) {
        //check number of parameters
        if(!routeParameters.coordinates.size()) {
            reply = http::Reply::stockReply(http::Reply::badRequest);
            return;
        }
        if(false == checkCoord(routeParameters.coordinates[0])) {
            reply = http::Reply::stockReply(http::Reply::badRequest);
            return;
        }
        NodeInformationHelpDesk * nodeHelpDesk = m_query_objects->nodeHelpDesk;
        //query to helpdesk
        PhantomNode result;
        nodeHelpDesk->FindPhantomNodeForCoordinate(
            routeParameters.coordinates[0],
            result,
            routeParameters.zoomLevel
        );

        std::string temp_string;
        //json

        if("" != routeParameters.jsonpParameter) {
            reply.content += routeParameters.jsonpParameter;
            reply.content += "(";
        }

        reply.status = http::Reply::ok;
        reply.content += ("{");
        reply.content += ("\"version\":0.3,");
        reply.content += ("\"status\":");
        if(UINT_MAX != result.edgeBasedNode) {
            reply.content += "0,";
        } else {
            reply.content += "207,";
        }
        reply.content += ("\"mapped_coordinate\":");
        reply.content += "[";
        if(UINT_MAX != result.edgeBasedNode) {
            convertInternalLatLonToString(result.location.lat, temp_string);
            reply.content += temp_string;
            convertInternalLatLonToString(result.location.lon, temp_string);
            reply.content += ",";
            reply.content += temp_string;
        }
        reply.content += "],";
        reply.content += "\"name\":\"";
        if(UINT_MAX != result.edgeBasedNode) {
            m_query_objects->GetName(result.nodeBasedEdgeNameID, temp_string);
            reply.content += temp_string;
        }
        reply.content += "\"";
        reply.content += ",\"transactionId\":\"OSRM Routing Engine JSON Nearest (v0.3)\"";
        reply.content += ("}");
        reply.headers.resize(3);
        if("" != routeParameters.jsonpParameter) {
            reply.content += ")";
            reply.headers[1].name = "Content-Type";
            reply.headers[1].value = "text/javascript";
            reply.headers[2].name = "Content-Disposition";
            reply.headers[2].value = "attachment; filename=\"location.js\"";
        } else {
            reply.headers[1].name = "Content-Type";
            reply.headers[1].value = "application/x-javascript";
            reply.headers[2].name = "Content-Disposition";
            reply.headers[2].value = "attachment; filename=\"location.json\"";
        }
        reply.headers[0].name = "Content-Length";
        intToString(reply.content.size(), temp_string);
        reply.headers[0].value = temp_string;
    }

private:
    DataFacadeT * m_query_objects;
    HashTable<std::string, unsigned> descriptorTable;
    std::string descriptor_string;
};

#endif /* NearestPlugin_H_ */
