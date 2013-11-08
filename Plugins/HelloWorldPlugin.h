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
