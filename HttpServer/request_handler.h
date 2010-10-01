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
                _Coordinate result;
                sEngine->findNearestNodeForLatLon(_Coordinate(lat, lon), result);

                rep.status = reply::ok;
                rep.content.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
                rep.content.append("<kml xmlns=\"http://www.opengis.net/kml/2.2\">");
                rep.content.append("<Placemark>");

                std::stringstream out1;
                out1 << setprecision(10);
                out1 << "<name>Nearest Place in map to " << lat/100000. << "," << lon/100000. << "</name>";
                rep.content.append(out1.str());
                rep.content.append("<Point>");

                std::stringstream out2;
                out2 << setprecision(10);
                out2 << "<coordinates>" << result.lon / 100000. << "," << result.lat / 100000.  << "</coordinates>";
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
                return;
            }
            if(command == "route")
            {
                double timestamp = get_timestamp();
                //http://localhost:5000/route&45.1427&12.14144&54.8733&8.59438
                std::size_t second_amp_pos = request.find_first_of("&", first_amp_pos+1);
                std::size_t third_amp_pos = request.find_first_of("&", second_amp_pos+1);
                std::size_t fourth_amp_pos = request.find_last_of("&");

                int lat1 = static_cast<int>(100000*atof(request.substr(first_amp_pos+1, request.length()-second_amp_pos-1).c_str()));
                int lon1 = static_cast<int>(100000*atof(request.substr(second_amp_pos+1, request.length()-third_amp_pos-1).c_str()));
                int lat2 = static_cast<int>(100000*atof(request.substr(third_amp_pos+1, request.length()-fourth_amp_pos-1).c_str()));
                int lon2 = static_cast<int>(100000*atof(request.substr(fourth_amp_pos+1).c_str()));

                _Coordinate startCoord(lat1, lon1);
                _Coordinate targetCoord(lat2, lon2);

                vector< _PathData > * path = new vector< _PathData >();
                PhantomNodes * phantomNodes = new PhantomNodes();
                sEngine->FindRoutingStarts(startCoord, targetCoord, phantomNodes);
                unsigned int distance = sEngine->ComputeRoute(phantomNodes, path, startCoord, targetCoord);
                rep.status = reply::ok;

                string tmp;

                rep.content += ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
                rep.content += ("<kml xmlns=\"http://www.opengis.net/kml/2.2\">");
                rep.content += ("<Document>");

                if(distance != std::numeric_limits<unsigned int>::max())
                    computeDescription(tmp, path, phantomNodes);
                cout << tmp << endl;

//                rep.content += tmp;
                rep.content += ("<Placemark>");
                rep.content += ("<name>OSM Routing Engine (c) Dennis Luxen and others </name>");

                rep.content += "<description>Route from ";
                convertLatLon(lat1, tmp);
                rep.content += tmp;
                rep.content += ",";
                convertLatLon(lon1, tmp);
                rep.content += tmp;
                rep.content += " to ";
                convertLatLon(lat2, tmp);
                rep.content += tmp;
                rep.content += ",";
                convertLatLon(lon2, tmp);
                rep.content += tmp;
                rep.content += "</description> ";
                rep.content += ("<LineString>");
                rep.content += ("<extrude>1</extrude>");
                rep.content += ("<tessellate>1</tessellate>");
                rep.content += ("<altitudeMode>absolute</altitudeMode>");
                rep.content += ("<coordinates>");


                if(distance != std::numeric_limits<unsigned int>::max())
                {   //A route has been found
                    convertLatLon(phantomNodes->startCoord.lon, tmp);
                    rep.content += tmp;
                    rep.content += (",");
                    doubleToString(phantomNodes->startCoord.lat/100000., tmp);
                    rep.content += tmp;
                    rep.content += (" ");
                    _Coordinate result;
                    for(vector< _PathData >::iterator it = path->begin(); it != path->end(); it++)
                    {
                        sEngine->getNodeInfo(it->node, result);
                        convertLatLon(result.lon, tmp);
                        rep.content += tmp;
                        rep.content += (",");
                        convertLatLon(result.lat, tmp);
                        rep.content += tmp;
                        rep.content += (" ");
                    }

                    convertLatLon(phantomNodes->targetCoord.lon, tmp);
                    rep.content += tmp;
                    rep.content += (",");
                    convertLatLon(phantomNodes->targetCoord.lat, tmp);
                    rep.content += tmp;
                }

                rep.content += ("</coordinates>");
                rep.content += ("</LineString>");
                rep.content += ("</Placemark>");
                rep.content += ("</Document>");
                rep.content += ("</kml>");

                rep.headers.resize(3);
                rep.headers[0].name = "Content-Length";
                intToString(rep.content.size(), tmp);
                rep.headers[0].value = tmp;
                rep.headers[1].name = "Content-Type";
                rep.headers[1].value = "application/vnd.google-earth.kml+xml";
                rep.headers[2].name = "Content-Disposition";
                rep.headers[2].value = "attachment; filename=\"route.kml\"";

                delete path;
                delete phantomNodes;
//                timestamp2 = get_timestamp();
                //cout << "description in " << timestamp2 - timestamp << "s" << endl;
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

    /* used to be boosts lexical cast, but this was too slow */
    inline void doubleToString(const double value, std::string & output)
    {
        // The largest 32-bit integer is 4294967295, that is 10 chars
        // On the safe side, add 1 for sign, and 1 for trailing zero
        char buffer[12] ;
        sprintf(buffer, "%f", value) ;
        output = buffer ;
    }

    inline void intToString(const int value, std::string & output)
    {
        // The largest 32-bit integer is 4294967295, that is 10 chars
        // On the safe side, add 1 for sign, and 1 for trailing zero
        char buffer[12] ;
        sprintf(buffer, "%i", value) ;
        output = buffer ;
    }

    inline void convertLatLon(const int value, std::string & output)
    {
        char buffer[100];
        buffer[10] = 0; // Nullterminierung
        char* string = printInt< 10, 5 >( buffer, value );
        output = string;
    }

    // precision: Nachkommastellen
    // length: Maximallänge inklusive Komma
    template< int length, int precision >
    inline char* printInt( char* buffer, int value )
    {
        bool minus = false;
        if ( value < 0 ) {
            minus = true;
            value = -value;
        }
        buffer += length - 1;
        for ( int i = 0; i < precision; i++ ) {
            *buffer = '0' + ( value % 10 );
            value /= 10;
            buffer--;
        }
        *buffer = '.';
        buffer--;
        for ( int i = precision + 1; i < length; i++ ) {
            *buffer = '0' + ( value % 10 );
            value /= 10;
            if ( value == 0 ) break;
            buffer--;
        }
        if ( minus ) {
            buffer--;
            *buffer = '-';
        }
        return buffer;
    }

    void computeDescription(string &tmp, vector< _PathData > * path, PhantomNodes * phantomNodes)
    {
        _Coordinate previous(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon);
        _Coordinate next, current, lastPlace;
        stringstream numberString;

        double tempDist = 0;
        NodeID nextID = UINT_MAX;
        NodeID nameID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->startNode1, phantomNodes->startNode2);
        short type = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->startNode1, phantomNodes->startNode2);
        lastPlace.lat = phantomNodes->startCoord.lat;
        lastPlace.lon = phantomNodes->startCoord.lon;
        short nextType = SHRT_MAX;
        short prevType = SHRT_MAX;
        tmp += "<Placemark>\n <Name>";
        for(vector< _PathData >::iterator it = path->begin(); it != path->end(); it++)
        {
            sEngine->getNodeInfo(it->node, current);
            if(it==path->end()-1){
                next = _Coordinate(phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
                nextID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
                nextType = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
            } else {
                sEngine->getNodeInfo((it+1)->node, next);
                nextID = sEngine->GetNameIDForOriginDestinationNodeID(it->node, (it+1)->node);
                nextType = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(it->node, (it+1)->node);
            }
            if(nextID == nameID) {
                tempDist += ApproximateDistance(previous.lat, previous.lon, current.lat, current.lon);
            } else {
                if(type == 0 && prevType != 0)
                    tmp += "enter motorway and ";
                if(type != 0 && prevType == 0 )
                    tmp += "leave motorway and ";

                double angle = GetAngleBetweenTwoEdges(previous, current, next);
//                if(it->turn)
//                    tmp += " turn! ";
                tmp += "follow road ";
                if(nameID != 0)
                    tmp += sEngine->GetNameForNameID(nameID);
                tmp += " (type: ";
                numberString << type;
                tmp += numberString.str();
                numberString.str("");
                tmp += ", id: ";
                numberString << nameID;
                tmp += numberString.str();
                numberString.str("");
                tmp += ")</Name>\n <Description>drive for ";
                numberString << ApproximateDistance(previous.lat, previous.lon, current.lat, current.lon)+tempDist;
                tmp += numberString.str();
                numberString.str("");
                tmp += "m </Description>";
                string lat; string lon;
                convertLatLon(lastPlace.lon, lon);
                convertLatLon(lastPlace.lat, lat);
                lastPlace = current;
                tmp += "\n <Point><Coordinates>";
                tmp += lon;
                tmp += ",";
                tmp += lat;
                tmp += "</Coordinates></Point>";
                tmp += "\n</Placemark>\n";
                tmp += "<Placemark>\n <Name> (";
                numberString << angle;
                tmp += numberString.str();
                numberString.str("");
                tmp +=") ";
                if(angle > 160 && angle < 200) {
                    tmp += /* " (" << angle << ")*/"drive ahead, ";
                } else if (angle > 290 && angle <= 360) {
                    tmp += /*" (" << angle << ")*/ "turn sharp left, ";
                } else if (angle > 245 && angle <= 290) {
                    tmp += /*" (" << angle << ")*/ "turn left, ";
                } else if (angle > 200 && angle <= 245) {
                    tmp += /*" (" << angle << ") */"bear left, ";
                } else if (angle > 115 && angle <= 160) {
                    tmp += /*" (" << angle << ") */"bear right, ";
                } else if (angle > 70 && angle <= 115) {
                    tmp += /*" (" << angle << ") */"turn right, ";
                } else {
                    tmp += /*" (" << angle << ") */"turn sharp right, ";
                }
                tempDist = 0;
                prevType = type;
            }
            nameID = nextID;
            previous = current;
            type = nextType;
        }
        nameID = sEngine->GetNameIDForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
        type = sEngine->GetTypeOfEdgeForOriginDestinationNodeID(phantomNodes->targetNode1, phantomNodes->targetNode2);
        tmp += "follow road ";
        tmp += sEngine->GetNameForNameID(nameID);
        tmp += " (type: ";
        numberString << type;
        tmp += numberString.str();
        numberString.str("");
        tmp += ")</name>\n <Description> drive for ";
        numberString << (previous.lat, previous.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon) + tempDist;
        tmp += numberString.str();
        numberString.str("");
        tmp += "m</Description>\n ";
        string lat; string lon;
        convertLatLon(lastPlace.lon, lon);
        convertLatLon(lastPlace.lat, lat);
        tmp += "<Point><Coordinates>";
        tmp += lon;
        tmp += ",";
        tmp += lat;
        tmp += "</Coordinates></Point>";
        tmp += "</Placemark>\n";
        tmp += "<Placemark>\n <Name>you have reached your destination</Name>\n ";
        tmp += "<Description>End of Route</Description>";
        convertLatLon(phantomNodes->targetCoord.lon, lon);
        convertLatLon(phantomNodes->targetCoord.lat, lat);
        tmp += "\n <Point><Coordinates>";
        tmp += lon;
        tmp += ",";
        tmp += lat;
        tmp +="</Coordinates></Point>\n";
        tmp += "</Placemark>";
    }

};
}

#endif // HTTP_ROUTER_REQUEST_HANDLER_HPP
