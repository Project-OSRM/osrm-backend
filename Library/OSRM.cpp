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
    if( !testDataFile(server_ini_path) ){
        throw OSRMException("server.ini not found");
    }

    BaseConfiguration serverConfig(server_ini_path);

    boost::system::error_code ec;

    boost::filesystem::path base_path =
                boost::filesystem::absolute(server_ini_path).parent_path();

    objects = new QueryObjectsStorage(
        boost::filesystem::canonical(
            serverConfig.GetParameter("hsgrData"),
            base_path, ec ).string(),
        boost::filesystem::canonical(
            serverConfig.GetParameter("ramIndex"),
            base_path,
            ec
        ).string(),
        boost::filesystem::canonical(
            serverConfig.GetParameter("fileIndex"),
            base_path,
            ec
        ).string(),
        boost::filesystem::canonical(
            serverConfig.GetParameter("nodesData"),
            base_path,
        ec ).string(),
        boost::filesystem::canonical(
            serverConfig.GetParameter("edgesData"),
            base_path,
            ec
        ).string(),
        boost::filesystem::canonical(
            serverConfig.GetParameter("namesData"),
            base_path,
            ec
        ).string(),
        boost::filesystem::canonical(
            serverConfig.GetParameter("timestamp"),
            base_path,
            ec
        ).string()
    );

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
    std::cout << "[plugin] " << plugin->GetDescriptor() << std::endl;
    pluginMap[plugin->GetDescriptor()] = plugin;
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
