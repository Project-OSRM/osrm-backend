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

#include <algorithm>
#include <cctype> // std::tolower
#include <string>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>

#include "APIGrammar.h"
#include "BasicDatastructures.h"
#include "../DataStructures/HashTable.h"
#include "../Plugins/BasePlugin.h"
#include "../Plugins/RouteParameters.h"
#include "../Util/StringUtil.h"
#include "../typedefs.h"

namespace http {

class RequestHandler : private boost::noncopyable {
public:
    explicit RequestHandler() : _pluginCount(0) { }

    ~RequestHandler() {

        for(unsigned i = 0; i < _pluginVector.size(); i++) {
            BasePlugin * tempPointer = _pluginVector[i];
            delete tempPointer;
        }
    }

    void handle_request(const Request& req, Reply& rep){
        //parse command
        std::string request(req.uri);

        { //This block logs the current request to std out. should be moved to a logging component
            time_t ltime;
            struct tm *Tm;

            ltime=time(NULL);
            Tm=localtime(&ltime);

            INFO((Tm->tm_mday < 10 ? "0" : "" )  << Tm->tm_mday << "-" << (Tm->tm_mon+1 < 10 ? "0" : "" )  << (Tm->tm_mon+1) << "-" << 1900+Tm->tm_year << " " << (Tm->tm_hour < 10 ? "0" : "" ) << Tm->tm_hour << ":" << (Tm->tm_min < 10 ? "0" : "" ) << Tm->tm_min << ":" << (Tm->tm_sec < 10 ? "0" : "" ) << Tm->tm_sec << " " <<
                    req.endpoint.to_string() << " " << req.referrer << ( 0 == req.referrer.length() ? "- " :" ") << req.agent << ( 0 == req.agent.length() ? "- " :" ") << req.uri );
        }
        try {
            RouteParameters routeParameters;
            APIGrammar<std::string::iterator, RouteParameters> apiParser(&routeParameters);

            std::string::iterator it = request.begin();
            bool result = boost::spirit::qi::parse(it, request.end(), apiParser);    // returns true if successful
            if (!result || (it != request.end()) ) {
                rep = http::Reply::stockReply(http::Reply::badRequest);
                int position = std::distance(request.begin(), it);
                std::string tmp_position_string;
                intToString(position, tmp_position_string);
                rep.content += "Input seems to be malformed close to position ";
                rep.content += "<br><pre>";
                rep.content += request;
                rep.content += tmp_position_string;
                rep.content += "<br>";
                for(unsigned i = 0, end = std::distance(request.begin(), it); i < end; ++i)
                    rep.content += "&nbsp;";
                rep.content += "^<br></pre>";
            } else {
                //Finished parsing, lets call the right plugin to handle the request
                if(pluginMap.Holds(routeParameters.service)) {
                    rep.status = Reply::ok;
                    _pluginVector[pluginMap.Find(routeParameters.service)]->HandleRequest(routeParameters, rep );
                } else {
                    rep = Reply::stockReply(Reply::badRequest);
                }
                return;
            }
        } catch(std::exception& e) {
            rep = Reply::stockReply(Reply::internalServerError);
            std::cerr << "[server error] code: " << e.what() << ", uri: " << req.uri << std::endl;
            return;
        }
    };

    void RegisterPlugin(BasePlugin * plugin) {
        std::cout << "[handler] registering plugin " << plugin->GetDescriptor() << std::endl;
        pluginMap.Add(plugin->GetDescriptor(), _pluginCount);
        _pluginVector.push_back(plugin);
        ++_pluginCount;
    }

private:
    HashTable<std::string, unsigned> pluginMap;
    std::vector<BasePlugin *> _pluginVector;
    unsigned _pluginCount;
};
} // namespace http

#endif // REQUEST_HANDLER_H
