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

class HelloWorldPlugin : public BasePlugin
{
  private:
    std::string temp_string;

  public:
    HelloWorldPlugin() : descriptor_string("hello") {}
    virtual ~HelloWorldPlugin() {}
    const std::string GetDescriptor() const { return descriptor_string; }

    void HandleRequest(const RouteParameters &routeParameters, http::Reply &reply)
    {
        reply.status = http::Reply::ok;
        reply.content.emplace_back("<html><head><title>Hello World Demonstration "
                                   "Document</title></head><body><h1>Hello, World!</h1>");
        reply.content.emplace_back("<pre>");
        reply.content.emplace_back("zoom level: ");
        intToString(routeParameters.zoom_level, temp_string);
        reply.content.emplace_back(temp_string);
        reply.content.emplace_back("\nchecksum: ");
        intToString(routeParameters.check_sum, temp_string);
        reply.content.emplace_back(temp_string);
        reply.content.emplace_back("\ninstructions: ");
        reply.content.emplace_back((routeParameters.print_instructions ? "yes" : "no"));
        reply.content.emplace_back(temp_string);
        reply.content.emplace_back("\ngeometry: ");
        reply.content.emplace_back((routeParameters.geometry ? "yes" : "no"));
        reply.content.emplace_back("\ncompression: ");
        reply.content.emplace_back((routeParameters.compression ? "yes" : "no"));
        reply.content.emplace_back("\noutput format: ");
        reply.content.emplace_back(routeParameters.output_format);
        reply.content.emplace_back("\njson parameter: ");
        reply.content.emplace_back(routeParameters.jsonp_parameter);
        reply.content.emplace_back("\nlanguage: ");
        reply.content.emplace_back(routeParameters.language);
        reply.content.emplace_back("\nNumber of locations: ");
        intToString(routeParameters.coordinates.size(), temp_string);
        reply.content.emplace_back(temp_string);
        reply.content.emplace_back("\n");

        unsigned counter = 0;
        for (const FixedPointCoordinate &coordinate : routeParameters.coordinates)
        {
            reply.content.emplace_back("  [");
            intToString(counter, temp_string);
            reply.content.emplace_back(temp_string);
            reply.content.emplace_back("] ");
            doubleToString(coordinate.lat / COORDINATE_PRECISION, temp_string);
            reply.content.emplace_back(temp_string);
            reply.content.emplace_back(",");
            doubleToString(coordinate.lon / COORDINATE_PRECISION, temp_string);
            reply.content.emplace_back(temp_string);
            reply.content.emplace_back("\n");
            ++counter;
        }

        reply.content.emplace_back("Number of hints: ");
        intToString(routeParameters.hints.size(), temp_string);
        reply.content.emplace_back(temp_string);
        reply.content.emplace_back("\n");

        counter = 0;
        for (const std::string &current_string : routeParameters.hints)
        {
            reply.content.emplace_back("  [");
            intToString(counter, temp_string);
            reply.content.emplace_back(temp_string);
            reply.content.emplace_back("] ");
            reply.content.emplace_back(current_string);
            reply.content.emplace_back("\n");
            ++counter;
        }
        reply.content.emplace_back("</pre></body></html>");
    }

  private:
    std::string descriptor_string;
};

#endif /* HELLOWORLDPLUGIN_H_ */
