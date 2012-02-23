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

#ifndef LOCATEPLUGIN_H_
#define LOCATEPLUGIN_H_

#include <fstream>

#include "ObjectForPluginStruct.h"
#include "BasePlugin.h"
#include "RouteParameters.h"
#include "../Util/StringUtil.h"
#include "../DataStructures/NodeInformationHelpDesk.h"

/*
 * This Plugin locates the nearest node in the road network for a given coordinate.
 */
class LocatePlugin : public BasePlugin {
public:
    LocatePlugin(ObjectsForQueryStruct * objects) {
        nodeHelpDesk = objects->nodeHelpDesk;
    }
	std::string GetDescriptor() const { return std::string("locate"); }
	std::string GetVersionString() const { return std::string("0.3 (DL)"); }
	void HandleRequest(const RouteParameters & routeParameters, http::Reply& reply) {
		//check number of parameters
		if(routeParameters.parameters.size() != 2) {
			reply = http::Reply::stockReply(http::Reply::badRequest);
			return;
		}

		int lat = static_cast<int>(100000.*atof(routeParameters.parameters[0].c_str()));
		int lon = static_cast<int>(100000.*atof(routeParameters.parameters[1].c_str()));

		if(lat>90*100000 || lat <-90*100000 || lon>180*100000 || lon <-180*100000) {
		    reply = http::Reply::stockReply(http::Reply::badRequest);
		    return;
		}
		//query to helpdesk
		_Coordinate result;
		nodeHelpDesk->FindNearestNodeCoordForLatLon(_Coordinate(lat, lon), result);

		std::string tmp;
        std::string JSONParameter;
        //json

        JSONParameter = routeParameters.options.Find("jsonp");
        if("" != JSONParameter) {
            reply.content += JSONParameter;
            reply.content += "(";
        }

		//Write to stream
        reply.status = http::Reply::ok;
        reply.content += ("{");
        reply.content += ("\"version\":0.3,");
        reply.content += ("\"status\":0,");
        reply.content += ("\"result\":");
        convertInternalLatLonToString(result.lat, tmp);
        reply.content += "[";
        reply.content += tmp;
        convertInternalLatLonToString(result.lon, tmp);
        reply.content += ", ";
        reply.content += tmp;
        reply.content += "]";
        reply.content += ",\"transactionId\": \"OSRM Routing Engine JSON Locate (v0.3)\"";
        reply.content += ("}");
        reply.headers.resize(3);
        if("" != JSONParameter) {
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
};

#endif /* LOCATEPLUGIN_H_ */
