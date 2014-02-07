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

#ifndef TIMESTAMPPLUGIN_H_
#define TIMESTAMPPLUGIN_H_

#include "BasePlugin.h"

template<class DataFacadeT>
class TimestampPlugin : public BasePlugin {
public:
    TimestampPlugin(const DataFacadeT * facade)
     : facade(facade), descriptor_string("timestamp")
    { }
    const std::string & GetDescriptor() const { return descriptor_string; }
    void HandleRequest(const RouteParameters & routeParameters, http::Reply& reply) {
        std::string tmp;

        bool isJsonpRequest = !routeParameters.jsonpParameter.empty();

        //json
        reply = http::Reply::JsReply(http::Reply::ok, isJsonpRequest, "timestamp");

        if(isJsonpRequest) {
            reply.content.push_back(routeParameters.jsonpParameter);
            reply.content.push_back("(");
        }

        reply.content.push_back("{");
        reply.content.push_back("\"status\":");
            reply.content.push_back("0,");
        reply.content.push_back("\"timestamp\":\"");
        reply.content.push_back(facade->GetTimestamp());
        reply.content.push_back("\"");
        reply.content.push_back("}");
        
        if(isJsonpRequest) {
            reply.content.push_back(")");
        } 
        
        reply.setSize();
    }
private:
    const DataFacadeT * facade;
    std::string descriptor_string;
};

#endif /* TIMESTAMPPLUGIN_H_ */
