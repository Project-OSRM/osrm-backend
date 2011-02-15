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

#include "BasePlugin.h"

#include "../DataStructures/NodeInformationHelpDesk.h"

/*
 * This Plugin locates the nearest point on a street in the road network for a given coordinate.
 */
class LocatePlugin : public BasePlugin {
public:
	LocatePlugin(std::string ramIndexPath, std::string fileIndexPath, std::string nodesPath) {
		nodeHelpDesk = new NodeInformationHelpDesk(ramIndexPath.c_str(), fileIndexPath.c_str());
		ifstream nodesInStream(nodesPath.c_str(), ios::binary);
		nodeHelpDesk->initNNGrid(nodesInStream);
	}
	~LocatePlugin() {
		delete nodeHelpDesk;
	}
	std::string GetDescriptor() { return std::string("locate"); }
	std::string GetVersionString() { return std::string("0.2a (DL)"); }
	void HandleRequest(std::vector<std::string> parameters, http::Reply& reply) {
		//check number of parameters
		if(parameters.size() != 2) {
			reply = http::Reply::stockReply(http::Reply::badRequest);
			return;
		}

		int lat = static_cast<int>(100000.*atof(parameters[0].c_str()));
		int lon = static_cast<int>(100000.*atof(parameters[1].c_str()));

		if(lat>90*100000 || lat <-90*100000 || lon>180*100000 || lon <-180*100000) {
		    reply = http::Reply::stockReply(http::Reply::badRequest);
		    return;
		}
		//query to helpdesk
		_Coordinate result;
		nodeHelpDesk->findNearestNodeCoordForLatLon(_Coordinate(lat, lon), result);

		//Write to stream
		reply.status = http::Reply::ok;
		reply.content.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
		reply.content.append("<kml xmlns=\"http://www.opengis.net/kml/2.2\">");
		reply.content.append("<Placemark>");

		std::stringstream out1;
		out1 << setprecision(10);
		out1 << "<name>Nearest Place in map to " << lat/100000. << "," << lon/100000. << "</name>";
		reply.content.append(out1.str());
		reply.content.append("<Point>");

		std::stringstream out2;
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
		return;
	}
private:
	NodeInformationHelpDesk * nodeHelpDesk;
};

#endif /* LOCATEPLUGIN_H_ */
