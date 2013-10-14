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

#ifndef LOCATEPLUGIN_H_
#define LOCATEPLUGIN_H_

#include "BasePlugin.h"
#include "../DataStructures/NodeInformationHelpDesk.h"
#include "../Server/DataStructures/QueryObjectsStorage.h"
#include "../Util/StringUtil.h"

/*
 * This Plugin locates the nearest node in the road network for a given coordinate.
 */
class LocatePlugin : public BasePlugin {
public:
    LocatePlugin(QueryObjectsStorage * objects) : descriptor_string("locate") {
        nodeHelpDesk = objects->nodeHelpDesk;
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

        //query to helpdesk
        FixedPointCoordinate result;
        std::string tmp;
        //json

//        JSONParameter = routeParameters.options.Find("jsonp");
        if("" != routeParameters.jsonpParameter) {
            reply.content += routeParameters.jsonpParameter;
            reply.content += "(";
        }
        reply.status = http::Reply::ok;
        reply.content += ("{");
        reply.content += ("\"version\":0.3,");
        if(!nodeHelpDesk->LocateClosestEndPointForCoordinate(routeParameters.coordinates[0], result)) {
            reply.content += ("\"status\":207,");
            reply.content += ("\"mapped_coordinate\":[]");
        } else {
            //Write coordinate to stream
            reply.status = http::Reply::ok;
            reply.content += ("\"status\":0,");
            reply.content += ("\"mapped_coordinate\":");
            convertInternalLatLonToString(result.lat, tmp);
            reply.content += "[";
            reply.content += tmp;
            convertInternalLatLonToString(result.lon, tmp);
            reply.content += ",";
            reply.content += tmp;
            reply.content += "]";
        }
        reply.content += ",\"transactionId\": \"OSRM Routing Engine JSON Locate (v0.3)\"";
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
        intToString(reply.content.size(), tmp);
        reply.headers[0].value = tmp;
        return;
    }

private:
    NodeInformationHelpDesk * nodeHelpDesk;
    std::string descriptor_string;
};

#endif /* LOCATEPLUGIN_H_ */
