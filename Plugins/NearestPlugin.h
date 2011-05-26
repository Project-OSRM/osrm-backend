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

#ifndef NearestPlugin_H_
#define NearestPlugin_H_

#include <fstream>

#include "BasePlugin.h"
#include "RouteParameters.h"

#include "ObjectForPluginStruct.h"

#include "../DataStructures/NodeInformationHelpDesk.h"
#include "../DataStructures/HashTable.h"
#include "../Util/StrIngUtil.h"

/*
 * This Plugin locates the nearest point on a street in the road network for a given coordinate.
 */
class NearestPlugin : public BasePlugin {
public:
    NearestPlugin(ObjectsForQueryStruct * objects) {
        nodeHelpDesk = objects->nodeHelpDesk;
        descriptorTable.Set("", 0); //default descriptor
        descriptorTable.Set("kml", 0);
        descriptorTable.Set("json", 1);
    }
    std::string GetDescriptor() { return std::string("nearest"); }
    std::string GetVersionString() { return std::string("0.3 (DL)"); }
    void HandleRequest(RouteParameters routeParameters, http::Reply& reply) {
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
        nodeHelpDesk->FindNearestPointOnEdge(_Coordinate(lat, lon), result);
        unsigned descriptorType = descriptorTable[routeParameters.options.Find("output")];
        std::stringstream out1;
        std::stringstream out2;
        std::string tmp;
        std::string JSONParameter;
        switch(descriptorType){
        case 1:
            //json

            JSONParameter = routeParameters.options.Find("jsonp");
            if("" != JSONParameter) {
                reply.content += JSONParameter;
                reply.content += "(\n";
            }

            reply.status = http::Reply::ok;
            reply.content += ("{");
            reply.content += ("\"version\":0.3,");
            reply.content += ("\"status\":0,");
            reply.content += ("\"status_message\":");
            out1 << setprecision(10);
            out1 << "\"Nearest Place in map to " << lat/100000. << "," << lon/100000. << "\",";
            reply.content.append(out1.str());
            reply.content += ("\"coordinate\": ");
            out2 << setprecision(10);
            out2 << "[" << result.lat / 100000. << "," << result.lon / 100000.  << "]";
            reply.content.append(out2.str());

            reply.content += ("}");
            reply.headers.resize(3);
            reply.headers[0].name = "Content-Length";
            intToString(reply.content.size(), tmp);
            reply.headers[0].value = tmp;if("" != JSONParameter) {
                reply.content += ")\n";
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

            break;
        default:
            reply.status = http::Reply::ok;

            //Write to stream
            reply.content.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
            reply.content.append("<kml xmlns=\"http://www.opengis.net/kml/2.2\">");
            reply.content.append("<Placemark>");


            out1 << setprecision(10);
            out1 << "<name>Nearest Place in map to " << lat/100000. << "," << lon/100000. << "</name>";
            reply.content.append(out1.str());
            reply.content.append("<Point>");

            out2 << setprecision(10);
            out2 << "<coordinates>" << result.lon / 100000. << "," << result.lat / 100000.  << "</coordinates>";
            reply.content.append(out2.str());
            reply.content.append("</Point>");
            reply.content.append("</Placemark>");
            reply.content.append("</kml>");

            reply.headers.resize(3);
            reply.headers[0].name = "Content-Length";
            reply.headers[0].value = boost::lexical_cast<std::string>(reply.content.size());
            reply.headers[1].name = "Content-Type";
            reply.headers[1].value = "application/vnd.google-earth.kml+xml";
            reply.headers[2].name = "Content-Disposition";
            reply.headers[2].value = "attachment; filename=\"placemark.kml\"";
            break;
        }
    }
private:
    NodeInformationHelpDesk * nodeHelpDesk;
    HashTable<std::string, unsigned> descriptorTable;
};

#endif /* NearestPlugin_H_ */
