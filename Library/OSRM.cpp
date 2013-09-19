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

OSRM::OSRM(const char * server_ini_path, const bool use_shared_memory)
 :  use_shared_memory(use_shared_memory)
{
    if( !use_shared_memory ) {
        if( !testDataFile(server_ini_path) ){
            std::string error_message(server_ini_path);
            error_message += " not found";
            throw OSRMException(error_message);
        }

        IniFile serverConfig(server_ini_path);

        boost::filesystem::path base_path =
                   boost::filesystem::absolute(server_ini_path).parent_path();

        if ( !serverConfig.Holds("hsgrData")) {
            throw OSRMException("no ram index file name in server ini");
        }
        if ( !serverConfig.Holds("ramIndex") ) {
            throw OSRMException("no mem index file name in server ini");
        }
        if ( !serverConfig.Holds("fileIndex") ) {
            throw OSRMException("no nodes file name in server ini");
        }
        if ( !serverConfig.Holds("nodesData") ) {
            throw OSRMException("no nodes file name in server ini");
        }
        if ( !serverConfig.Holds("edgesData") ) {
            throw OSRMException("no edges file name in server ini");
        }

        boost::filesystem::path hsgr_path = boost::filesystem::absolute(
                serverConfig.GetParameter("hsgrData"),
                base_path
        );

        boost::filesystem::path ram_index_path = boost::filesystem::absolute(
                serverConfig.GetParameter("ramIndex"),
                base_path
        );

        boost::filesystem::path file_index_path = boost::filesystem::absolute(
                serverConfig.GetParameter("fileIndex"),
                base_path
        );

        boost::filesystem::path node_data_path = boost::filesystem::absolute(
                serverConfig.GetParameter("nodesData"),
                base_path
        );
        boost::filesystem::path edge_data_path = boost::filesystem::absolute(
                serverConfig.GetParameter("edgesData"),
                base_path
        );
        boost::filesystem::path name_data_path = boost::filesystem::absolute(
                serverConfig.GetParameter("namesData"),
                base_path
        );
        boost::filesystem::path timestamp_path = boost::filesystem::absolute(
                serverConfig.GetParameter("timestamp"),
                base_path
        );

        query_data_facade = new InternalDataFacade<QueryEdge::EdgeData>();

    } else {
        //TODO: fetch pointers from shared memory
        query_data_facade = new SharedDataFacade<QueryEdge::EdgeData>();
    }

    //The following plugins handle all requests.
    RegisterPlugin(
        new HelloWorldPlugin()
    );
    RegisterPlugin(
        new LocatePlugin<BaseDataFacade<QueryEdge::EdgeData> >(
            query_data_facade
        )
    );
    RegisterPlugin(
        new NearestPlugin<BaseDataFacade<QueryEdge::EdgeData> >(
            query_data_facade
        )
    );
    RegisterPlugin(
        new TimestampPlugin<BaseDataFacade<QueryEdge::EdgeData> >(
            query_data_facade
        )
    );
    RegisterPlugin(
        new ViaRoutePlugin<BaseDataFacade<QueryEdge::EdgeData> >(
            query_data_facade
        )
    );
}

OSRM::~OSRM() {
    BOOST_FOREACH(PluginMap::value_type & plugin_pointer, plugin_map) {
        delete plugin_pointer.second;
    }
}

void OSRM::RegisterPlugin(BasePlugin * plugin) {
    SimpleLogger().Write()  << "loaded plugin: " << plugin->GetDescriptor();
    if( plugin_map.find(plugin->GetDescriptor()) != plugin_map.end() ) {
        delete plugin_map.find(plugin->GetDescriptor())->second;
    }
    plugin_map.emplace(plugin->GetDescriptor(), plugin);
}

void OSRM::RunQuery(RouteParameters & route_parameters, http::Reply & reply) {
    const PluginMap::const_iterator & iter = plugin_map.find(
        route_parameters.service
    );

    if(plugin_map.end() != iter) {
        reply.status = http::Reply::ok;
        iter->second->HandleRequest(route_parameters, reply );
    } else {
        reply = http::Reply::stockReply(http::Reply::badRequest);
    }
}
