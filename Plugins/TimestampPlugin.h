/*
    open source routing machine
    Copyright (C) Dennis Luxen, 2010

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

#ifndef TIMESTAMPPLUGIN_H_
#define TIMESTAMPPLUGIN_H_

#include <cassert>

#include "BasePlugin.h"
#include "RouteParameters.h"

class TimestampPlugin : public BasePlugin {
public:
    TimestampPlugin(QueryObjectsStorage * o) : objects(o) {
    }
    std::string GetDescriptor() const { return std::string("timestamp"); }
    std::string GetVersionString() const { return std::string("0.3 (DL)"); }
    void HandleRequest(const RouteParameters & routeParameters, http::Reply& reply) {
        std::string tmp;
        std::string JSONParameter;

        //json
        JSONParameter = routeParameters.options.Find("jsonp");
        if("" != JSONParameter) {
            reply.content += JSONParameter;
            reply.content += "(";
        }

        reply.status = http::Reply::ok;
        reply.content += ("{");
        reply.content += ("\"version\":0.3,");
        reply.content += ("\"status\":");
            reply.content += "0,";
        reply.content += ("\"timestamp\":\"");
        reply.content += objects->timestamp;
        reply.content += "\"";
        reply.content += ",\"transactionId\":\"OSRM Routing Engine JSON timestamp (v0.3)\"";
        reply.content += ("}");
        reply.headers.resize(3);
        if("" != JSONParameter) {
            reply.content += ")";
            reply.headers[1].name = "Content-Type";
            reply.headers[1].value = "text/javascript";
            reply.headers[2].name = "Content-Disposition";
            reply.headers[2].value = "attachment; filename=\"timestamp.js\"";
        } else {
            reply.headers[1].name = "Content-Type";
            reply.headers[1].value = "application/x-javascript";
            reply.headers[2].name = "Content-Disposition";
            reply.headers[2].value = "attachment; filename=\"timestamp.json\"";
        }
        reply.headers[0].name = "Content-Length";
        intToString(reply.content.size(), tmp);
        reply.headers[0].value = tmp;
    }
private:
    QueryObjectsStorage * objects;
};

#endif /* TIMESTAMPPLUGIN_H_ */
