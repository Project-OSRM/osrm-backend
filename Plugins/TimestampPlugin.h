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

#ifndef TIMESTAMP_PLUGIN_H
#define TIMESTAMP_PLUGIN_H

#include "BasePlugin.h"

template <class DataFacadeT> class TimestampPlugin : public BasePlugin
{
  public:
    explicit TimestampPlugin(const DataFacadeT *facade)
        : facade(facade), descriptor_string("timestamp")
    {
    }
    const std::string GetDescriptor() const { return descriptor_string; }
    void HandleRequest(const RouteParameters &route_parameters, http::Reply &reply)
    {
        std::string tmp;

        // json
        if (!route_parameters.jsonp_parameter.empty())
        {
            reply.content.emplace_back(route_parameters.jsonp_parameter);
            reply.content.emplace_back("(");
        }

        reply.status = http::Reply::ok;
        reply.content.emplace_back("{");
        reply.content.emplace_back("\"status\":");
        reply.content.emplace_back("0,");
        reply.content.emplace_back("\"timestamp\":\"");
        reply.content.emplace_back(facade->GetTimestamp());
        reply.content.emplace_back("\"");
        reply.content.emplace_back("}");
        reply.headers.resize(3);
        if (!route_parameters.jsonp_parameter.empty())
        {
            reply.content.emplace_back(")");
            reply.headers[1].name = "Content-Type";
            reply.headers[1].value = "text/javascript";
            reply.headers[2].name = "Content-Disposition";
            reply.headers[2].value = "attachment; filename=\"timestamp.js\"";
        }
        else
        {
            reply.headers[1].name = "Content-Type";
            reply.headers[1].value = "application/x-javascript";
            reply.headers[2].name = "Content-Disposition";
            reply.headers[2].value = "attachment; filename=\"timestamp.json\"";
        }
        unsigned content_length = 0;
        for (const std::string &snippet : reply.content)
        {
            content_length += snippet.length();
        }
        intToString(content_length, tmp);
        reply.headers[0].value = tmp;
    }

  private:
    const DataFacadeT *facade;
    std::string descriptor_string;
};

#endif /* TIMESTAMP_PLUGIN_H */
