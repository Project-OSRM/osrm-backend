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

#ifndef HELLOWORLDPLUGIN_H_
#define HELLOWORLDPLUGIN_H_

#include "BasePlugin.h"

#include <sstream>

class HelloWorldPlugin : public BasePlugin {
public:
	HelloWorldPlugin() : descriptor_string("hello"){}
	virtual ~HelloWorldPlugin() { }
	const std::string & GetDescriptor()    const { return descriptor_string; }

	void HandleRequest(const RouteParameters & routeParameters, http::Reply& reply) {
		reply.status = http::Reply::ok;
		reply.content.append("<html><head><title>Hello World Demonstration Document</title></head><body><h1>Hello, World!</h1>");
		std::stringstream content;
		content << "<pre>";
        content << "zoom level: " << routeParameters.zoomLevel << "\n";
        content << "checksum: " << routeParameters.checkSum << "\n";
        content << "instructions: " << (routeParameters.printInstructions ? "yes" : "no") << "\n";
        content << "geometry: " << (routeParameters.geometry ? "yes" : "no") << "\n";
        content << "compression: " << (routeParameters.compression ? "yes" : "no") << "\n";
        content << "output format: " << routeParameters.outputFormat << "\n";
        content << "json parameter: " << routeParameters.jsonpParameter << "\n";
        content << "language: " << routeParameters.language << "<br>";
        content << "Number of locations: " << routeParameters.coordinates.size() << "\n";
        for(unsigned i = 0; i < routeParameters.coordinates.size(); ++i) {
            content << "  [" << i << "] " << routeParameters.coordinates[i].lat/COORDINATE_PRECISION << "," << routeParameters.coordinates[i].lon/COORDINATE_PRECISION << "\n";
        }
        content << "Number of hints: " << routeParameters.hints.size() << "\n";
        for(unsigned i = 0; i < routeParameters.hints.size(); ++i) {
            content << "  [" << i << "] " << routeParameters.hints[i] << "\n";
        }
        content << "</pre>";
		reply.content.append(content.str());
		reply.content.append("</body></html>");
	}
private:
    std::string descriptor_string;
};

#endif /* HELLOWORLDPLUGIN_H_ */
