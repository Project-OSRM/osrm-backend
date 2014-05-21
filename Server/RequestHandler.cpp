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

#include "APIGrammar.h"
#include "RequestHandler.h"
#include "Http/Request.h"

#include "../DataStructures/JSONContainer.h"
#include "../Library/OSRM.h"
#include "../Util/SimpleLogger.h"
#include "../Util/StringUtil.h"
#include "../typedefs.h"

#include <osrm/Reply.h>
#include <osrm/RouteParameters.h>

#include <ctime>

#include <algorithm>
#include <iostream>

RequestHandler::RequestHandler() : routing_machine(nullptr) {}

void RequestHandler::handle_request(const http::Request &req, http::Reply &reply)
{
    // parse command
    try
    {
        std::string request;
        URIDecode(req.uri, request);

        // deactivated as GCC apparently does not implement that, not even in 4.9
        // std::time_t t = std::time(nullptr);
        // SimpleLogger().Write() << std::put_time(std::localtime(&t), "%m-%d-%Y %H:%M:%S") <<
        //     " " << req.endpoint.to_string() << " " <<
        //     req.referrer << ( 0 == req.referrer.length() ? "- " :" ") <<
        //     req.agent << ( 0 == req.agent.length() ? "- " :" ") << request;

        time_t ltime;
        struct tm *Tm;

        ltime = time(nullptr);
        Tm = localtime(&ltime);

        SimpleLogger().Write() << (Tm->tm_mday < 10 ? "0" : "") << Tm->tm_mday << "-"
                               << (Tm->tm_mon + 1 < 10 ? "0" : "") << (Tm->tm_mon + 1) << "-"
                               << 1900 + Tm->tm_year << " " << (Tm->tm_hour < 10 ? "0" : "")
                               << Tm->tm_hour << ":" << (Tm->tm_min < 10 ? "0" : "") << Tm->tm_min
                               << ":" << (Tm->tm_sec < 10 ? "0" : "") << Tm->tm_sec << " "
                               << req.endpoint.to_string() << " " << req.referrer
                               << (0 == req.referrer.length() ? "- " : " ") << req.agent
                               << (0 == req.agent.length() ? "- " : " ") << request;

        RouteParameters route_parameters;
        APIGrammarParser api_parser(&route_parameters);

        auto it = request.begin();
        const bool result = boost::spirit::qi::parse(it, request.end(), api_parser);

        if (!result || (it != request.end()))
        {
            reply = http::Reply::StockReply(http::Reply::badRequest);
            reply.content.clear();
            const int position = std::distance(request.begin(), it);
            JSON::Object json_result;
            json_result.values["status"] = 400;
            std::string tmp_position_string = IntToString(position);
            std::string message = ("Query string malformed close to position " + IntToString(position));
            json_result.values["status_message"] = message;
            JSON::render(reply.content, json_result);
        }
        else
        {
            // parsing done, lets call the right plugin to handle the request
            BOOST_ASSERT_MSG(routing_machine != nullptr, "pointer not init'ed");

            if (!route_parameters.jsonp_parameter.empty())
            {
                const std::string json_p = (route_parameters.jsonp_parameter + "(");
                reply.content.insert(reply.content.end(), json_p.begin(), json_p.end());
            }
            routing_machine->RunQuery(route_parameters, reply);

            // set headers, still ugly and should be reworked
            reply.headers.resize(3);
            if ("gpx" == route_parameters.output_format)
            {
                reply.headers[1].name = "Content-Type";
                reply.headers[1].value = "application/gpx+xml; charset=UTF-8";
                reply.headers[2].name = "Content-Disposition";
                reply.headers[2].value = "attachment; filename=\"route.gpx\"";
            }
            else if (!route_parameters.jsonp_parameter.empty())
            {
                reply.content.push_back(')');
                reply.headers[1].name = "Content-Type";
                reply.headers[1].value = "text/javascript";
                reply.headers[2].name = "Content-Disposition";
                reply.headers[2].value = "attachment; filename=\"response.js\"";
            }
            else
            {
                reply.headers[1].name = "Content-Type";
                reply.headers[1].value = "application/x-javascript";
                reply.headers[2].name = "Content-Disposition";
                reply.headers[2].value = "attachment; filename=\"response.json\"";
            }
            reply.headers[0].name = "Content-Length";
            reply.headers[0].value = IntToString(reply.content.size());
            return;
        }
    }
    catch (const std::exception &e)
    {
        reply = http::Reply::StockReply(http::Reply::internalServerError);
        SimpleLogger().Write(logWARNING) << "[server error] code: " << e.what()
                                         << ", uri: " << req.uri;
        return;
    }
}

void RequestHandler::RegisterRoutingMachine(OSRM *osrm) { routing_machine = osrm; }
