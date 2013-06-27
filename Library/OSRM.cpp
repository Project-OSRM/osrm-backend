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

#include "OSRM.h"

OSRM::OSRM(const char * server_ini_path) {
    BaseConfiguration serverConfig(server_ini_path);
    QueryObjectsStorage * objects = new QueryObjectsStorage(
        serverConfig.GetParameter("hsgrData"),
        serverConfig.GetParameter("ramIndex"),
        serverConfig.GetParameter("fileIndex"),
        serverConfig.GetParameter("nodesData"),
        serverConfig.GetParameter("edgesData"),
        serverConfig.GetParameter("namesData"),
        serverConfig.GetParameter("timestamp")
    );

    RegisterPlugin(new HelloWorldPlugin());
    RegisterPlugin(new LocatePlugin(objects));
    RegisterPlugin(new NearestPlugin(objects));
    RegisterPlugin(new TimestampPlugin(objects));
    RegisterPlugin(new ViaRoutePlugin(objects));
}

OSRM::~OSRM() {
    BOOST_FOREACH(PluginMap::value_type plugin_pointer, pluginMap) {
        delete plugin_pointer.second;
    }
}

void OSRM::RegisterPlugin(BasePlugin * plugin) {
    std::cout << "[plugin] " << plugin->GetDescriptor() << std::endl;
    pluginMap[plugin->GetDescriptor()] = plugin;
}

void OSRM::RunQuery(RouteParameters & route_parameters, http::Reply & reply) {
    PluginMap::const_iterator iter = pluginMap.find(route_parameters.service);
    if(pluginMap.end() != iter) {
        reply.status = http::Reply::ok;
        iter->second->HandleRequest(route_parameters, reply );
    } else {
        reply = http::Reply::stockReply(http::Reply::badRequest);
    }
}
