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

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "APIGrammar.h"
#include "DataStructures/RouteParameters.h"
#include "Http/Request.h"
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
                rep = http::Reply::StockReply(http::Reply::badRequest);
                const int position = std::distance(request.begin(), it);
                std::string tmp_position_string;
                intToString(position, tmp_position_string);
                rep.content.push_back(
                    "Input seems to be malformed close to position "
                    "<br><pre>"
                );
                rep.content.push_back( request );
                rep.content.push_back(tmp_position_string);
                rep.content.push_back("<br>");
                const unsigned end = std::distance(request.begin(), it);
                for(unsigned i = 0; i < end; ++i) {
                    rep.content.push_back("&nbsp;");
                }
                rep.content.push_back("^<br></pre>");
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
            rep = http::Reply::StockReply(http::Reply::internalServerError);
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
