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

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "APIGrammar.h"
#include "BasicDatastructures.h"
#include "DataStructures/RouteParameters.h"
#include "../Library/OSRM.h"
#include "../Util/SimpleLogger.h"
#include "../Util/StringUtil.h"
#include "../typedefs.h"

#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>

class RequestHandler : private boost::noncopyable {
public:
    typedef APIGrammar<std::string::iterator, RouteParameters> APIGrammarParser;
    explicit RequestHandler() : routing_machine(NULL) { }

    void handle_request(const http::Request& req, http::Reply& rep){
        //parse command
        try {
            std::string request(req.uri);

            time_t ltime;
            struct tm *Tm;

            ltime=time(NULL);
            Tm=localtime(&ltime);

            SimpleLogger().Write() <<
                (Tm->tm_mday < 10 ? "0" : "" )  << Tm->tm_mday    << "-" <<
                (Tm->tm_mon+1 < 10 ? "0" : "" ) << (Tm->tm_mon+1) << "-" <<
                1900+Tm->tm_year << " " << (Tm->tm_hour < 10 ? "0" : "" ) <<
                Tm->tm_hour << ":" << (Tm->tm_min < 10 ? "0" : "" ) <<
                Tm->tm_min << ":" << (Tm->tm_sec < 10 ? "0" : "" ) <<
                Tm->tm_sec << " " << req.endpoint.to_string() << " " <<
                req.referrer << ( 0 == req.referrer.length() ? "- " :" ") <<
                req.agent << ( 0 == req.agent.length() ? "- " :" ") << req.uri;

            RouteParameters routeParameters;
            APIGrammarParser apiParser(&routeParameters);

            std::string::iterator it = request.begin();
            const bool result = boost::spirit::qi::parse(
                it,
                request.end(),
                apiParser
            );

            if ( !result || (it != request.end()) ) {
                rep = http::Reply::stockReply(http::Reply::badRequest);
                const int position = std::distance(request.begin(), it);
                std::string tmp_position_string;
                intToString(position, tmp_position_string);
                rep.content += "Input seems to be malformed close to position ";
                rep.content += "<br><pre>";
                rep.content += request;
                rep.content += tmp_position_string;
                rep.content += "<br>";
                const unsigned end = std::distance(request.begin(), it);
                for(unsigned i = 0; i < end; ++i) {
                    rep.content += "&nbsp;";
                }
                rep.content += "^<br></pre>";
            } else {
                //parsing done, lets call the right plugin to handle the request
                BOOST_ASSERT_MSG(
                    routing_machine != NULL,
                    "pointer not init'ed"
                );
                routing_machine->RunQuery(routeParameters, rep);
                return;
            }
        } catch(std::exception& e) {
            rep = http::Reply::stockReply(http::Reply::internalServerError);
            SimpleLogger().Write(logWARNING) <<
                "[server error] code: " << e.what() << ", uri: " << req.uri;
            return;
        }
    };

    void RegisterRoutingMachine(OSRM * osrm) {
        routing_machine = osrm;
    }

private:
    OSRM * routing_machine;
};

#endif // REQUEST_HANDLER_H
