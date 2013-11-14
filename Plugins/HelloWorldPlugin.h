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
#include "../Util/StringUtil.h"

#include <string>

class HelloWorldPlugin : public BasePlugin {
    private:
        std::string temp_string;
public:
	HelloWorldPlugin() : descriptor_string("hello"){}
	virtual ~HelloWorldPlugin() { }
	const std::string & GetDescriptor()    const { return descriptor_string; }

	void HandleRequest(const RouteParameters & routeParameters, http::Reply& reply) {
		reply.status = http::Reply::ok;
		reply.content.push_back("<html><head><title>Hello World Demonstration Document</title></head><body><h1>Hello, World!</h1>");
		reply.content.push_back("<pre>");
        reply.content.push_back("zoom level: ");
        intToString(routeParameters.zoomLevel, temp_string);
        reply.content.push_back(temp_string);
        reply.content.push_back("\nchecksum: ");
        intToString(routeParameters.checkSum, temp_string);
        reply.content.push_back(temp_string);
        reply.content.push_back("\ninstructions: ");
        reply.content.push_back((routeParameters.printInstructions ? "yes" : "no"));
        reply.content.push_back(temp_string);
        reply.content.push_back("\ngeometry: ");
        reply.content.push_back((routeParameters.geometry ? "yes" : "no"));
        reply.content.push_back("\ncompression: ");
        reply.content.push_back((routeParameters.compression ? "yes" : "no"));
        reply.content.push_back("\noutput format: ");
        reply.content.push_back(routeParameters.outputFormat);
        reply.content.push_back("\njson parameter: ");
        reply.content.push_back(routeParameters.jsonpParameter);
        reply.content.push_back("\nlanguage: ");
        reply.content.push_back(routeParameters.language);
        reply.content.push_back("\nNumber of locations: ");
        intToString(routeParameters.coordinates.size(), temp_string);
        reply.content.push_back(temp_string);
        reply.content.push_back("\n");
        for(unsigned i = 0; i < routeParameters.coordinates.size(); ++i) {
            reply.content.push_back( "  [");
            intToString(i, temp_string);
            reply.content.push_back(temp_string);
            reply.content.push_back("] ");
            doubleToString(routeParameters.coordinates[i].lat/COORDINATE_PRECISION, temp_string);
            reply.content.push_back(temp_string);
            reply.content.push_back(",");
            doubleToString(routeParameters.coordinates[i].lon/COORDINATE_PRECISION, temp_string);
            reply.content.push_back(temp_string);
            reply.content.push_back("\n");
        }
        reply.content.push_back( "Number of hints: ");
        intToString(routeParameters.hints.size(), temp_string);
        reply.content.push_back(temp_string);
        reply.content.push_back("\n");
        for(unsigned i = 0; i < routeParameters.hints.size(); ++i) {
            reply.content.push_back( "  [");
            intToString(i, temp_string);
            reply.content.push_back(temp_string);
            reply.content.push_back("] ");
            reply.content.push_back(routeParameters.hints[i]);
            reply.content.push_back("\n");
        }
        reply.content.push_back( "</pre></body></html>");
	}
private:
    std::string descriptor_string;
};

#endif /* HELLOWORLDPLUGIN_H_ */
