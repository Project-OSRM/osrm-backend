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

#include "OSRM.h"
#include <boost/foreach.hpp>


OSRM::OSRM(boost::unordered_map<const std::string,boost::filesystem::path>& paths) {
    objects = new QueryObjectsStorage( paths );
    RegisterPlugin(new HelloWorldPlugin());
    RegisterPlugin(new LocatePlugin(objects));
    RegisterPlugin(new NearestPlugin(objects));
    RegisterPlugin(new TimestampPlugin(objects));
    RegisterPlugin(new ViaRoutePlugin(objects));
}

OSRM::~OSRM() {
    BOOST_FOREACH(PluginMap::value_type & plugin_pointer, pluginMap) {
        delete plugin_pointer.second;
    }
    delete objects;
}

void OSRM::RegisterPlugin(BasePlugin * plugin) {
    SimpleLogger().Write()  << "loaded plugin: " << plugin->GetDescriptor();
    if( pluginMap.find(plugin->GetDescriptor()) != pluginMap.end() ) {
        delete pluginMap.find(plugin->GetDescriptor())->second;
    }
    pluginMap.emplace(plugin->GetDescriptor(), plugin);
}

void OSRM::RunQuery(RouteParameters & route_parameters, http::Reply & reply) {
    const PluginMap::const_iterator & iter = pluginMap.find(route_parameters.service);
    if(pluginMap.end() != iter) {
        reply.status = http::Reply::ok;
        iter->second->HandleRequest(route_parameters, reply );
    } else {
        reply = http::Reply::stockReply(http::Reply::badRequest);
    }
}
