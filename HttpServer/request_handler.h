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

#ifndef HTTP_ROUTER_REQUEST_HANDLER_HPP
#define HTTP_ROUTER_REQUEST_HANDLER_HPP

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>

#include "reply.h"
#include "request.h"

#include "../typedefs.h"

namespace http {

struct reply;
struct Request;

/// The common handler for all incoming requests.
template<typename GraphT>
class request_handler : private boost::noncopyable
{
public:
    /// Construct with a directory containing files to be served.
    explicit request_handler(SearchEngine<EdgeData, GraphT> * s) : sEngine(s){}

    /// Handle a request and produce a reply.
    void handle_request(const Request& req, reply& rep){
        try {
            std::string request(req.uri);
            std::string command;
            std::size_t first_amp_pos = request.find_first_of("&");
            command = request.substr(1,first_amp_pos-1);
            if(command == "locate")
            {
                std::size_t last_amp_pos = request.find_last_of("&");
                int lat = static_cast<int>(100000*atof(request.substr(first_amp_pos+1, request.length()-last_amp_pos-1).c_str()));
                int lon = static_cast<int>(100000*atof(request.substr(last_amp_pos+1).c_str()));
                NodeCoords<NodeID> * data = new NodeCoords<NodeID>();
                NodeID start = sEngine->findNearestNodeForLatLon(lat, lon, data);

                rep.status = reply::ok;
                rep.content.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
                rep.content.append("<kml xmlns=\"http://www.opengis.net/kml/2.2\">");
                rep.content.append("<Placemark>");

                std::stringstream out1;
                out1 << setprecision(10);
                out1 << "<name>Nearest Place in map to " << lat/100000. << "," << lon/100000. << ": node with id " << start << "</name>";
                rep.content.append(out1.str());
                rep.content.append("<Point>");

                std::stringstream out2;
                out2 << setprecision(10);
                out2 << "<coordinates>" << data->lon / 100000. << "," << data->lat / 100000.  << "</coordinates>";
                rep.content.append(out2.str());
                rep.content.append("</Point>");
                rep.content.append("</Placemark>");
                rep.content.append("</kml>");

                rep.headers.resize(3);
                rep.headers[0].name = "Content-Length";
                rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
                rep.headers[1].name = "Content-Type";
                rep.headers[1].value = "application/vnd.google-earth.kml+xml";
                rep.headers[2].name = "Content-Disposition";
                rep.headers[2].value = "attachment; filename=\"placemark.kml\"";
                delete data;
                return;
            }
            if(command == "route")
            {
                //http://localhost:5000/route&45.1427&12.14144&54.8733&8.59438
                std::size_t second_amp_pos = request.find_first_of("&", first_amp_pos+1);
                std::size_t third_amp_pos = request.find_first_of("&", second_amp_pos+1);
                std::size_t fourth_amp_pos = request.find_last_of("&");

                int lat1 = static_cast<int>(100000*atof(request.substr(first_amp_pos+1, request.length()-second_amp_pos-1).c_str()));
                int lon1 = static_cast<int>(100000*atof(request.substr(second_amp_pos+1, request.length()-third_amp_pos-1).c_str()));
                int lat2 = static_cast<int>(100000*atof(request.substr(third_amp_pos+1, request.length()-fourth_amp_pos-1).c_str()));
                int lon2 = static_cast<int>(100000*atof(request.substr(fourth_amp_pos+1).c_str()));

                NodeCoords<NodeID> * startData = new NodeCoords<NodeID>();
                NodeCoords<NodeID> * targetData = new NodeCoords<NodeID>();
                vector<NodeID> * path = new vector<NodeID>();
                NodeID start = sEngine->findNearestNodeForLatLon(lat1, lon1, startData);
                NodeID target = sEngine->findNearestNodeForLatLon(lat2, lon2, targetData);
                unsigned int distance = sEngine->ComputeRoute(start, target, path);

                rep.status = reply::ok;
                rep.content.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
                rep.content.append("<kml xmlns=\"http://www.opengis.net/kml/2.2\">");
                rep.content.append("<Document>");
                rep.content.append("<Placemark>");
                rep.content.append("<name>OSM Routing Engine (c) Dennis Luxen and others </name>");
                std::stringstream out1;
                out1 << std::setprecision(10);
                out1 << "<description>Route from " << lat1/100000. << "," << lon1/100000. << " to " << lat2/100000. << "," << lon2/100000. << "</description>" << " ";
                rep.content.append(out1.str());
                rep.content.append("<LineString>");
                rep.content.append("<extrude>1</extrude>");
                rep.content.append("<tessellate>1</tessellate>");
                rep.content.append("<altitudeMode>absolute</altitudeMode>");
                rep.content.append("<coordinates>\n");
                if(distance != std::numeric_limits<unsigned int>::max())
                {   //A route has been found
                	NodeInfo * info = new NodeInfo();
                    for(vector<NodeID>::iterator it = path->begin(); it != path->end(); it++)
                    {
                        sEngine-> getNodeInfo(*it, info);
                        stringstream nodeout;
                        nodeout << std::setprecision(10);
                        nodeout << info->lon/100000. << "," << info->lat/100000. << " " << endl;
                        rep.content.append(nodeout.str());
                    }
                    delete info;
                }
                rep.content.append("</coordinates>");
                rep.content.append("</LineString>");
                rep.content.append("</Placemark>");
                rep.content.append("</Document>");
                rep.content.append("</kml>");

                rep.headers.resize(3);
                rep.headers[0].name = "Content-Length";
                rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
                rep.headers[1].name = "Content-Type";
                rep.headers[1].value = "application/vnd.google-earth.kml+xml";
                rep.headers[2].name = "Content-Disposition";
                rep.headers[2].value = "attachment; filename=\"route.kml\"";
                return;
            }
            rep = reply::stock_reply(reply::bad_request);
            return;
        } catch(std::exception& e)
        {
            //TODO: log exception somewhere
            rep = reply::stock_reply(reply::internal_server_error);
            return;
        }
    };

private:
    //SearchEngine object that is queried
    SearchEngine<EdgeData, GraphT> * sEngine;
};
}

#endif // HTTP_ROUTER_REQUEST_HANDLER_HPP
