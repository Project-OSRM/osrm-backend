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

#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>

#include "BasicDatastructures.h"

#include "../DataStructures/HashTable.h"
#include "../Plugins/BasePlugin.h"

namespace http {

class RequestHandler : private boost::noncopyable {
public:
	explicit RequestHandler() { }

	~RequestHandler() {
		pluginMap.EraseAll();
	}

	void handle_request(const Request& req, Reply& rep){
		//parse command
	    std::string request(req.uri);
		std::string command;
		std::size_t firstAmpPosition = request.find_first_of("&");
		command = request.substr(1,firstAmpPosition-1);
//		std::cout << "[debug] looking for handler for command: " << command << std::endl;
		try {
			if(pluginMap.Holds(command)) {

				std::vector<std::string> parameters;
				std::stringstream ss(( firstAmpPosition == std::string::npos ? "" : request.substr(firstAmpPosition+1) ));
				std::string item;
				while(std::getline(ss, item, '&')) {
					parameters.push_back(item);
				}
//				std::cout << "[debug] found handler for '" << command << "' at version: " << pluginMap.Find(command)->GetVersionString() << std::endl;
//				std::cout << "[debug] remaining parameters: " << parameters.size() << std::endl;
				rep.status = Reply::ok;
				pluginMap.Find(command)->HandleRequest(parameters, rep );
			} else {
				rep = Reply::stockReply(Reply::badRequest);
			}
			return;
		} catch(std::exception& e) {
			rep = Reply::stockReply(Reply::internalServerError);
			std::cerr << "[server error] code: " << e.what() << ", uri: " << req.uri << std::endl;
			return;
		}
	};

	void RegisterPlugin(BasePlugin * plugin) {
		std::cout << "[handler] registering plugin " << plugin->GetDescriptor() << std::endl;
		pluginMap.Add(plugin->GetDescriptor(), plugin);
	}

private:
	boost::mutex pluginMutex;
	HashTable<std::string, BasePlugin *> pluginMap;
};
} // namespace http

#endif // REQUEST_HANDLER_H
